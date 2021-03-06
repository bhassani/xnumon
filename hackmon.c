/*-
 * xnumon - monitor macOS for malicious activity
 * https://www.roe.ch/xnumon
 *
 * Copyright (c) 2017-2018, Daniel Roethlisberger <daniel@roe.ch>.
 * All rights reserved.
 *
 * Licensed under the Open Software License version 3.0.
 */

/*
 * Monitoring core for specific techniques.
 */

#include "hackmon.h"

#include "work.h"
#include "atomic.h"

#include <strings.h>
#include <assert.h>

static config_t *config;

static uint64_t events_received;    /* number of events received */
static uint64_t events_processed;   /* number of events processed */
static atomic64_t ooms;             /* counts events impaired due to OOM */

strset_t *suppress_process_access_by_ident;
strset_t *suppress_process_access_by_path;

static void process_access_free(process_access_t *);
static int process_access_work(process_access_t *);

static process_access_t *
process_access_new() {
	process_access_t *pa;

	pa = malloc(sizeof(*pa));
	if (!pa)
		return NULL;
	bzero(pa, sizeof(*pa));
	pa->hdr.type = LOGEVT_PROCESS_ACCESS;
	pa->hdr.le_work = (__typeof__(pa->hdr.le_work))process_access_work;
	pa->hdr.le_free = (__typeof__(pa->hdr.le_free))process_access_free;
	return pa;
}

static void
process_access_free(process_access_t *pa) {
	if (pa->subject_image_exec)
		image_exec_free(pa->subject_image_exec);
	if (pa->object_image_exec)
		image_exec_free(pa->object_image_exec);
	free(pa);
}

/*
 * Return true iff the event should not be logged (i.e. filtered).
 *
 * Executed by worker thread.
 */
static bool
process_access_filter(process_access_t *pa) {
	image_exec_t *ie;

	ie = pa->subject_image_exec;
	if (ie->codesign && ie->codesign->ident) {
		/* presence of ident implies that signature is good */
		if (strset_contains(suppress_process_access_by_ident,
		                    ie->codesign->ident))
			return true;
	}
	if (ie->path) {
		if (strset_contains(suppress_process_access_by_path,
		                    ie->path))
			return true;
	}
	return false;
}

/*
 * Executed by worker thread.
 */
static int
process_access_work(process_access_t *pa) {
	if (process_access_filter(pa))
		return -1;
	return 0;
}

static void
log_event_process_access(struct timespec *tv,
                         audit_proc_t *subject,
                         audit_proc_t *object,
                         const char *method) {
	process_access_t *pa;

	pa = process_access_new();
	if (!pa) {
		atomic64_inc(&ooms);
		return;
	}
	pa->subject_image_exec = image_exec_by_pid(subject->pid);
	pa->object_image_exec = image_exec_by_pid(object->pid);
	pa->subject = *subject;
	pa->object = *object;
	pa->method = method;
	pa->hdr.tv = *tv;
	work_submit(pa);
}

/*
 * Called for task_for_pid invocations.
 */
void
hackmon_taskforpid(struct timespec *tv,
                   audit_proc_t *subject,
                   audit_proc_t *object) {
	events_received++;
	if (subject->pid != object->pid) {
		events_processed++;
		log_event_process_access(tv, subject, object, "task_for_pid");
		return;
	}
}

/*
 * Called for ptrace invocations.
 */
void
hackmon_ptrace(struct timespec *tv,
               audit_proc_t *subject,
               audit_proc_t *object) {
	events_received++;
	if (subject->pid != object->pid) {
		events_processed++;
		log_event_process_access(tv, subject, object, "ptrace");
		return;
	}
}

void
hackmon_init(config_t *cfg) {
	config = cfg;
	ooms = 0;
	events_received = 0;
	events_processed = 0;
	suppress_process_access_by_ident =
		&cfg->suppress_process_access_by_ident;
	suppress_process_access_by_path =
		&cfg->suppress_process_access_by_path;
}

void
hackmon_fini(void) {
	if (!config)
		return;
	config = NULL;
}

void
hackmon_stats(hackmon_stat_t *st) {
	assert(st);

	st->receiveds = events_received;
	st->processeds = events_processed;
	st->ooms = (uint64_t)ooms;
}

