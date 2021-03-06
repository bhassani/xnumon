/*-
 * xnumon - monitor macOS for malicious activity
 * https://www.roe.ch/xnumon
 *
 * Copyright (c) 2017-2018, Daniel Roethlisberger <daniel@roe.ch>.
 * All rights reserved.
 *
 * Licensed under the Open Software License version 3.0.
 */

#ifndef AEV_H
#define AEV_H

#include "attrib.h"

#include <stddef.h>

char ** aev_new(size_t, char **) MALLOC NONNULL(2);

#endif

