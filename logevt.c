/*-
 * xnumon - monitor macOS for malicious activity
 * https://www.roe.ch/xnumon
 *
 * Copyright (c) 2017-2018, Daniel Roethlisberger <daniel@roe.ch>.
 * All rights reserved.
 *
 * Licensed under the Open Software License version 3.0.
 */

#include "logevt.h"

#include "build.h"
#include "os.h"
#include "evtloop.h"
#include "procmon.h"
#include "filemon.h"
#include "hackmon.h"

#include <assert.h>

config_t *config;

void
logevt_init(config_t *cfg) {
	config = cfg;
}

static void
logevt_header(logfmt_t *fmt, FILE *f, logevt_header_t *hdr) {
	assert(hdr);
	fmt->record_begin(f);
	fmt->dict_begin(f);
	fmt->dict_item(f, "version");
	fmt->value_uint(f, LOGEVT_VERSION);
	fmt->dict_item(f, "time");
	fmt->value_timespec(f, &hdr->tv);
	fmt->dict_item(f, "eventcode");
	fmt->value_uint(f, hdr->type);
}

static void
logevt_footer(logfmt_t *fmt, FILE *f) {
	fmt->dict_end(f);
	fmt->record_end(f);
}

int
logevt_xnumon_ops(logfmt_t *fmt, FILE *f, void *arg0) {
	xnumon_ops_t *ops = (xnumon_ops_t *)arg0;

	logevt_header(fmt, f, (logevt_header_t *)arg0);

	fmt->dict_item(f, "op");
	fmt->value_string(f, ops->subtype);

	fmt->dict_item(f, "build");
	fmt->dict_begin(f);
	fmt->dict_item(f, "version");
	fmt->value_string(f, build_version);
	fmt->dict_item(f, "date");
	fmt->value_string(f, build_date);
	fmt->dict_item(f, "info");
	fmt->value_string(f, build_info);
	fmt->dict_end(f); /* build */

	fmt->dict_item(f, "config");
	fmt->dict_begin(f);
	fmt->dict_item(f, "path");
	fmt->value_string(f, config->path);
	fmt->dict_item(f, "id");
	if (config->id)
		fmt->value_string(f, config->id);
	else
		fmt->value_null(f);
	fmt->dict_item(f, "launchd_mode");
	fmt->value_bool(f, config->launchd_mode);
	fmt->dict_item(f, "limit_nofile");
	fmt->value_uint(f, config->limit_nofile);
	fmt->dict_item(f, "kextlevel");
	fmt->value_string(f, config_kextlevel_s(config));
	fmt->dict_item(f, "hashes");
	fmt->value_string(f, hashes_flags_s(config->hflags));
	fmt->dict_item(f, "codesign");
	fmt->value_bool(f, config->codesign);
	fmt->dict_item(f, "ancestors");
	fmt->value_uint(f, config->ancestors);
	fmt->dict_item(f, "logdst");
	fmt->value_string(f, logdst_s(config));
	fmt->dict_item(f, "logfmt");
	fmt->value_string(f, logfmt_s(config));
	fmt->dict_item(f, "logoneline");
	if (config->logoneline == -1)
		fmt->value_null(f);
	else
		fmt->value_bool(f, config->logoneline);
	fmt->dict_item(f, "logfile");
	if (config->logfile)
		fmt->value_string(f, config->logfile);
	else
		fmt->value_null(f);
	fmt->dict_item(f, "suppress_image_exec_by_ident");
	fmt->value_uint(f, strset_size(&config->suppress_image_exec_by_ident));
	fmt->dict_item(f, "suppress_image_exec_by_path");
	fmt->value_uint(f, strset_size(&config->suppress_image_exec_by_path));
	fmt->dict_item(f, "suppress_process_access_by_ident");
	fmt->value_uint(f,
	                strset_size(&config->suppress_process_access_by_ident));
	fmt->dict_item(f, "suppress_process_access_by_path");
	fmt->value_uint(f,
	                strset_size(&config->suppress_process_access_by_path));
	fmt->dict_end(f); /* config */

	fmt->dict_item(f, "system");
	fmt->dict_begin(f);
	fmt->dict_item(f, "name");
	fmt->value_string(f, os_name());
	fmt->dict_item(f, "version");
	fmt->value_string(f, os_version());
	fmt->dict_item(f, "build");
	fmt->value_string(f, os_build());
	fmt->dict_end(f); /* system */

	logevt_footer(fmt, f);
	return 0;
}

int
logevt_xnumon_stats(logfmt_t *fmt, FILE *f, void *arg0) {
	evtloop_stat_t *st = (evtloop_stat_t *)arg0;

	logevt_header(fmt, f, (logevt_header_t *)arg0);

	fmt->dict_item(f, "evtloop");
	fmt->dict_begin(f);
	fmt->dict_item(f, "failedsyscall");
	fmt->value_uint(f, st->el_failedsyscalls);
	fmt->dict_item(f, "pathbug");
	fmt->value_uint(f, st->el_pathbugs);
	fmt->dict_item(f, "aueunknown");
	fmt->value_uint(f, st->el_aueunknowns);
	fmt->dict_item(f, "oom");
	fmt->value_uint(f, st->el_ooms);
	fmt->dict_end(f); /* evtloop */

	fmt->dict_item(f, "procmon");
	fmt->dict_begin(f);
	fmt->dict_item(f, "actprocs");
	fmt->value_uint(f, st->pm.procs);
	fmt->dict_item(f, "actexecimages");
	fmt->value_uint(f, st->pm.images);
	fmt->dict_item(f, "cwdmiss");
	fmt->value_uint(f, st->pm.cwdmisseds);
	fmt->dict_item(f, "eimiss");
	fmt->value_uint(f, st->pm.eimisseds);
	fmt->dict_item(f, "oom");
	fmt->value_uint(f, st->pm.ooms);
	fmt->dict_end(f); /* procmon */

	fmt->dict_item(f, "hackmon");
	fmt->dict_begin(f);
	fmt->dict_item(f, "recvd");
	fmt->value_uint(f, st->fm.receiveds);
	fmt->dict_item(f, "procd");
	fmt->value_uint(f, st->fm.processeds);
	fmt->dict_item(f, "oom");
	fmt->value_uint(f, st->fm.ooms);
	fmt->dict_end(f); /* hackmon */

	fmt->dict_item(f, "filemon");
	fmt->dict_begin(f);
	fmt->dict_item(f, "recvd");
	fmt->value_uint(f, st->fm.receiveds);
	fmt->dict_item(f, "procd");
	fmt->value_uint(f, st->fm.processeds);
	fmt->dict_item(f, "lpmiss");
	fmt->value_uint(f, st->fm.lpmisseds);
	fmt->dict_item(f, "oom");
	fmt->value_uint(f, st->fm.ooms);
	fmt->dict_end(f); /* filemon */

	fmt->dict_item(f, "kext_cdevq");
	fmt->dict_begin(f);
	fmt->dict_item(f, "buckets");
	fmt->value_uint(f, st->ke.cdev_qsize);
	fmt->dict_item(f, "visitors");
	fmt->value_uint(f, st->ke.kauth_visitors);
	fmt->dict_item(f, "timeout");
	fmt->value_uint(f, st->ke.kauth_timeouts);
	fmt->dict_item(f, "error");
	fmt->value_uint(f, st->ke.kauth_errors);
	fmt->dict_item(f, "defer");
	fmt->value_uint(f, st->ke.kauth_defers);
	fmt->dict_item(f, "deny");
	fmt->value_uint(f, st->ke.kauth_denies);
	fmt->dict_end(f); /* kext-cdevq */

	fmt->dict_item(f, "prep_queue");
	fmt->dict_begin(f);
	fmt->dict_item(f, "buckets");
	fmt->value_uint(f, st->pm.kqsize);
	fmt->dict_item(f, "lookup");
	fmt->value_uint(f, st->pm.kqlookups);
	fmt->dict_item(f, "miss");
	fmt->value_uint(f, st->pm.kqnotfounds);
	fmt->dict_item(f, "drop");
	fmt->value_uint(f, st->pm.kqtimeouts);
	fmt->dict_item(f, "bktskip");
	fmt->value_uint(f, st->pm.kqskips);
	fmt->dict_end(f); /* prep-queue */

	fmt->dict_item(f, "aupi_cdevq");
	fmt->dict_begin(f);
	fmt->dict_item(f, "buckets");
	fmt->value_uint(f, st->ap.qlen);
	fmt->dict_item(f, "bucketmax");
	fmt->value_uint(f, st->ap.qlimit);
	fmt->dict_item(f, "insert");
	fmt->value_uint(f, st->ap.inserts);
	fmt->dict_item(f, "read");
	fmt->value_uint(f, st->ap.reads);
	fmt->dict_item(f, "drop");
	fmt->value_uint(f, st->ap.drops);
	fmt->dict_end(f); /* aupi-cdevq */

	fmt->dict_item(f, "work_queue");
	fmt->dict_begin(f);
	fmt->dict_item(f, "buckets");
	fmt->value_uint(f, st->wq.qsize);
	fmt->dict_end(f); /* work-queue */

	fmt->dict_item(f, "log_queue");
	fmt->dict_begin(f);
	fmt->dict_item(f, "buckets");
	fmt->value_uint(f, st->lq.qsize);
	fmt->dict_item(f, "events");
	fmt->list_begin(f);
	for (int i = 0; i < LOGEVT_SIZE; i++) {
		fmt->list_item(f);
		fmt->value_uint(f, st->lq.counts[i]);
	}
	fmt->list_end(f);
	fmt->dict_item(f, "errors");
	fmt->value_uint(f, st->lq.errors);
	fmt->dict_end(f); /* log-queue */

	fmt->dict_item(f, "hash_cache");
	fmt->dict_begin(f);
	fmt->dict_item(f, "buckets");
	fmt->value_uint(f, st->ch.used);
	fmt->dict_item(f, "bucketmax");
	fmt->value_uint(f, st->ch.size);
	fmt->dict_item(f, "put");
	fmt->value_uint(f, st->ch.puts);
	fmt->dict_item(f, "get");
	fmt->value_uint(f, st->ch.gets);
	fmt->dict_item(f, "hit");
	fmt->value_uint(f, st->ch.hits);
	fmt->dict_item(f, "miss");
	fmt->value_uint(f, st->ch.misses);
	fmt->dict_item(f, "inv");
	fmt->value_uint(f, st->ch.invalids);
	fmt->dict_end(f); /* hash-cache */

	fmt->dict_item(f, "csig_cache");
	fmt->dict_begin(f);
	fmt->dict_item(f, "buckets");
	fmt->value_uint(f, st->cc.used);
	fmt->dict_item(f, "bucketmax");
	fmt->value_uint(f, st->cc.size);
	fmt->dict_item(f, "put");
	fmt->value_uint(f, st->cc.puts);
	fmt->dict_item(f, "get");
	fmt->value_uint(f, st->cc.gets);
	fmt->dict_item(f, "hit");
	fmt->value_uint(f, st->cc.hits);
	fmt->dict_item(f, "miss");
	fmt->value_uint(f, st->cc.misses);
	fmt->dict_item(f, "inv");
	fmt->value_uint(f, st->cc.invalids);
	fmt->dict_end(f); /* csig-cache */

	fmt->dict_item(f, "ldpl_cache");
	fmt->dict_begin(f);
	fmt->dict_item(f, "buckets");
	fmt->value_uint(f, st->cl.used);
	fmt->dict_item(f, "bucketmax");
	fmt->value_uint(f, st->cl.size);
	fmt->dict_item(f, "put");
	fmt->value_uint(f, st->cl.puts);
	fmt->dict_item(f, "get");
	fmt->value_uint(f, st->cl.gets);
	fmt->dict_item(f, "hit");
	fmt->value_uint(f, st->cl.hits);
	fmt->dict_item(f, "miss");
	fmt->value_uint(f, st->cl.misses);
	fmt->dict_item(f, "inv");
	fmt->value_uint(f, st->cl.invalids);
	fmt->dict_end(f); /* ldpl-cache */

	logevt_footer(fmt, f);
	return 0;
}

static void
logevt_image_exec_image(logfmt_t *fmt, FILE *f, image_exec_t *ei) {
	fmt->dict_begin(f);
	fmt->dict_item(f, "path");
	fmt->value_string(f, ei->path);
	if (ei->flags & (EIFLAG_STAT|EIFLAG_ATTR)) {
		fmt->dict_item(f, "mode");
		fmt->value_uint_oct(f, ei->stat.mode);
		fmt->dict_item(f, "uid");
		fmt->value_uint(f, ei->stat.uid);
		fmt->dict_item(f, "gid");
		fmt->value_uint(f, ei->stat.gid);
	}
	if (ei->flags & EIFLAG_STAT) {
		fmt->dict_item(f, "size");
		fmt->value_uint(f, ei->stat.size);
		fmt->dict_item(f, "mtime");
		fmt->value_timespec(f, &ei->stat.mtime);
		fmt->dict_item(f, "ctime");
		fmt->value_timespec(f, &ei->stat.ctime);
		fmt->dict_item(f, "btime");
		fmt->value_timespec(f, &ei->stat.btime);
	}
	if (ei->flags & EIFLAG_HASHES) {
		if (config->hflags & HASH_MD5) {
			fmt->dict_item(f, "md5");
			fmt->value_buf_hex(f, ei->hashes.md5, MD5SZ);
		}
		if (config->hflags & HASH_SHA1) {
			fmt->dict_item(f, "sha1");
			fmt->value_buf_hex(f, ei->hashes.sha1, SHA1SZ);
		}
		if (config->hflags & HASH_SHA256) {
			fmt->dict_item(f, "sha256");
			fmt->value_buf_hex(f, ei->hashes.sha256, SHA256SZ);
		}
	}

	if (ei->codesign) {
		fmt->dict_item(f, "codesign");
		fmt->dict_begin(f);
		fmt->dict_item(f, "result");
		fmt->value_string(f, ei->codesign->result);
		if (ei->codesign->error) {
			fmt->dict_item(f, "error");
			fmt->value_int(f, ei->codesign->error);
		}
		if (ei->codesign->ident) {
			fmt->dict_item(f, "ident");
			fmt->value_string(f, ei->codesign->ident);
		}
		if (ei->codesign->crtc > 0) {
			fmt->dict_item(f, "cert");
			fmt->value_string(f, ei->codesign->crtv[0]);
			fmt->dict_item(f, "chain");
			fmt->list_begin(f);
			for (int i = 1; i < ei->codesign->crtc; i++) {
				fmt->list_item(f);
				if (ei->codesign->crtv[i])
					fmt->value_string(f,
					        ei->codesign->crtv[i]);
				else
					fmt->value_null(f);
			}
			fmt->list_end(f); /* certchain */
		}
		fmt->dict_end(f); /* codesign */
	}
	fmt->dict_end(f); /* image */
}

static void
logevt_process_image_exec(logfmt_t *fmt, FILE *f, image_exec_t *ie) {
	fmt->dict_begin(f);
	if (ie->hdr.tv.tv_sec) {
		fmt->dict_item(f, "exec_time");
		fmt->value_timespec(f, &ie->hdr.tv);
	}
	fmt->dict_item(f, "exec_pid");
	fmt->value_int(f, ie->pid);
	fmt->dict_item(f, "path");
	fmt->value_string(f, ie->path);
	if (ie->flags & EIFLAG_HASHES) {
		if (config->hflags & HASH_MD5) {
			fmt->dict_item(f, "md5");
			fmt->value_buf_hex(f, ie->hashes.md5, MD5SZ);
		}
		if (config->hflags & HASH_SHA1) {
			fmt->dict_item(f, "sha1");
			fmt->value_buf_hex(f, ie->hashes.sha1, SHA1SZ);
		}
		if (config->hflags & HASH_SHA256) {
			fmt->dict_item(f, "sha256");
			fmt->value_buf_hex(f, ie->hashes.sha256, SHA256SZ);
		}
	}
	if (ie->codesign && ie->codesign->ident) {
		fmt->dict_item(f, "ident");
		fmt->value_string(f, ie->codesign->ident);
	}
	if (ie->script) {
		fmt->dict_item(f, "script");
		fmt->dict_begin(f);
		fmt->dict_item(f, "path");
		fmt->value_string(f, ie->script->path);
		if (ie->script->flags & EIFLAG_HASHES) {
			if (config->hflags & HASH_MD5) {
				fmt->dict_item(f, "md5");
				fmt->value_buf_hex(f,
				        ie->script->hashes.md5, MD5SZ);
			}
			if (config->hflags & HASH_SHA1) {
				fmt->dict_item(f, "sha1");
				fmt->value_buf_hex(f,
				        ie->script->hashes.sha1, SHA1SZ);
			}
			if (config->hflags & HASH_SHA256) {
				fmt->dict_item(f, "sha256");
				fmt->value_buf_hex(f,
				        ie->script->hashes.sha256, SHA256SZ);
			}
		}
		fmt->dict_end(f); /* script */
	}
	fmt->dict_end(f); /* exec */
}

static void
logevt_process_image_exec_ancestors(logfmt_t *fmt, FILE *f, image_exec_t *ie) {
	size_t depth = 0;

	fmt->list_begin(f);
	for (image_exec_t *pie = ie; pie && pie->pid > 0; pie = pie->prev) {
		fmt->list_item(f);
		logevt_process_image_exec(fmt, f, pie);
		if (++depth == config->ancestors)
			break;
	}
	fmt->list_end(f); /* process image exec ancestors */
}

static void
logevt_process(logfmt_t *fmt, FILE *f,
               audit_proc_t *process,
               struct timespec *fork_tv,
               image_exec_t *ie) {
	fmt->dict_begin(f);
	if (process) {
		fmt->dict_item(f, "pid");
		fmt->value_int(f, process->pid);
		fmt->dict_item(f, "auid");
		fmt->value_uint(f, process->auid);
		fmt->dict_item(f, "sid");
		fmt->value_uint(f, process->sid);
		fmt->dict_item(f, "euid");
		fmt->value_uint(f, process->euid);
		fmt->dict_item(f, "egid");
		fmt->value_uint(f, process->egid);
		fmt->dict_item(f, "ruid");
		fmt->value_uint(f, process->ruid);
		fmt->dict_item(f, "rgid");
		fmt->value_uint(f, process->rgid);
		fmt->dict_item(f, "dev");
		fmt->value_ttydev(f, process->dev);
		fmt->dict_item(f, "addr");
		if (process->addr.family) {
			fmt->value_string(f, ipaddrtoa(&process->addr, NULL));
		}
	}
	if (fork_tv) {
		fmt->dict_item(f, "fork_time");
		fmt->value_timespec(f, fork_tv);
	}
	if (ie) {
		fmt->dict_item(f, "image");
		logevt_process_image_exec(fmt, f, ie);
		fmt->dict_item(f, "ancestors");
		logevt_process_image_exec_ancestors(fmt, f, ie->prev);
	}
	fmt->dict_end(f); /* process */
}

int
logevt_image_exec(logfmt_t *fmt, FILE *f, void *arg0) {
	image_exec_t *ei = (image_exec_t *)arg0;

	logevt_header(fmt, f, (logevt_header_t *)arg0);

	if (ei->argv) {
		fmt->dict_item(f, "argv");
		fmt->list_begin(f);
		for (int i = 0; ei->argv[i]; i++) {
			fmt->list_item(f);
			fmt->value_string(f, ei->argv[i]);
		}
		fmt->list_end(f); /* argv */
	}

	if (ei->cwd) {
		fmt->dict_item(f, "cwd");
		fmt->value_string(f, ei->cwd);
	}

	fmt->dict_item(f, "image");
	logevt_image_exec_image(fmt, f, ei);

	if (ei->script) {
		fmt->dict_item(f, "script");
		logevt_image_exec_image(fmt, f, ei->script);
	}

	fmt->dict_item(f, "subject");
	logevt_process(fmt, f,
	               (ei->flags & EIFLAG_PIDLOOKUP) ? NULL : &ei->subject,
	               &ei->fork_tv,
	               ei->prev);

	logevt_footer(fmt, f);
	return 0;
}

int
logevt_process_access(logfmt_t *fmt, FILE *f, void *arg0) {
	process_access_t *pa = (process_access_t *)arg0;

	logevt_header(fmt, f, (logevt_header_t *)arg0);

	fmt->dict_item(f, "method");
	fmt->value_string(f, pa->method);

	fmt->dict_item(f, "object");
	logevt_process(fmt, f,
	               &pa->object,
	               pa->object_image_exec ?
	                   &pa->object_image_exec->fork_tv : NULL,
	               pa->object_image_exec);

	fmt->dict_item(f, "subject");
	logevt_process(fmt, f,
	               &pa->subject,
	               pa->subject_image_exec ?
	                   &pa->subject_image_exec->fork_tv : NULL,
	               pa->subject_image_exec);

	logevt_footer(fmt, f);
	return 0;
}

int
logevt_launchd_add(logfmt_t *fmt, FILE *f, void *arg0) {
	launchd_add_t *ldadd = (launchd_add_t *)arg0;

	logevt_header(fmt, f, (logevt_header_t *)arg0);

	fmt->dict_item(f, "plist");
	fmt->dict_begin(f);
	fmt->dict_item(f, "path");
	fmt->value_string(f, ldadd->plist_path);
	fmt->dict_end(f); /* plist */

	fmt->dict_item(f, "program");
	fmt->dict_begin(f);
	fmt->dict_item(f, "path");
	fmt->value_string(f, ldadd->program_path);
	if (ldadd->program_argv) {
		fmt->dict_item(f, "argv");
		fmt->list_begin(f);
		for (size_t i = 0; ldadd->program_argv[i]; i++) {
			fmt->list_item(f);
			fmt->value_string(f, ldadd->program_argv[i]);
		}
		fmt->list_end(f); /* argv */
	}
	fmt->dict_end(f); /* program */

	fmt->dict_item(f, "subject");
	logevt_process(fmt, f,
	               &ldadd->subject,
	               ldadd->subject_image_exec ?
	                   &ldadd->subject_image_exec->fork_tv : NULL,
	               ldadd->subject_image_exec);

	logevt_footer(fmt, f);
	return 0;
}

