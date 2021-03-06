/*-
 * xnumon - monitor macOS for malicious activity
 * https://www.roe.ch/xnumon
 *
 * Copyright (c) 2017-2018, Daniel Roethlisberger <daniel@roe.ch>.
 * All rights reserved.
 *
 * Licensed under the Open Software License version 3.0.
 */

#ifndef AUEVENT_H
#define AUEVENT_H

#include "ipaddr.h"
#include "attrib.h"

#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>

#include <bsm/audit_kevents.h> /* auevent_* take lists of event types */

typedef struct {
	pid_t pid;
	uid_t auid;
	uint32_t sid;
	uid_t euid;
	gid_t egid;
	uid_t ruid;
	gid_t rgid;
	dev_t dev;
	ipaddr_t addr;
} audit_proc_t;

typedef struct {
	mode_t mode;
	uid_t uid;
	gid_t gid;
	dev_t dev;
	ino_t ino;
	/* dev_t rdev; */
} audit_attr_t;

typedef struct {
	int             present;
	uint64_t        value;
#ifdef DEBUG_AUDITPIPE
	char *          text;                   /* strdup/free */
#endif
} audit_arg_t;

typedef struct {
	int             flags;
#define AEFLAG_ENOMEM 1                         /* ENOMEM encountered */

	uint16_t        type;
	uint16_t        mod;
	struct timespec tv;

	int             args_count;
	audit_arg_t     args[UCHAR_MAX+1];

	int             return_present;
	unsigned char   return_error;
	uint32_t        return_value;

	int             subject_present;
	audit_proc_t    subject;

	int             process_present;
	audit_proc_t    process;

	const char *    path[4];

	int             attr_present;
	audit_attr_t    attr;

	int             exit_present;
	uint32_t        exit_status;
	uint32_t        exit_return;

	char **         execarg;                /* malloc/free */
	char **         execenv;                /* malloc/free */

	unsigned char   unk_tokids[UCHAR_MAX+1]; /* zero-terminated list */
} audit_event_t;

void auevent_init(audit_event_t *) NONNULL(1);
ssize_t auevent_fread(audit_event_t *ev, const uint16_t[], FILE *) NONNULL(1,3);
void auevent_destroy(audit_event_t *) NONNULL(1);
#ifdef DEBUG_AUDITPIPE
void auevent_fprint(FILE *, audit_event_t *) NONNULL(1,2);
#endif

#endif

