/*-
 * xnumon - monitor macOS for malicious activity
 * https://www.roe.ch/xnumon
 *
 * Copyright (c) 2017-2018, Daniel Roethlisberger <daniel@roe.ch>.
 * All rights reserved.
 *
 * Licensed under the Open Software License version 3.0.
 */

#ifndef CF_H
#define CF_H

#include "attrib.h"

#include <CoreFoundation/CoreFoundation.h>

char * cf_cstr(CFStringRef) MALLOC;
char ** cf_cstrv(CFArrayRef) MALLOC;
char ** cf_aev(CFArrayRef) MALLOC;
CFURLRef cf_url(const char *) MALLOC NONNULL(1);
CFPropertyListRef cf_plist_load(const char *) MALLOC;

#endif

