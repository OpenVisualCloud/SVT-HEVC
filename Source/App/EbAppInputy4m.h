/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "EbAppConfig.h"

#define YUV4MPEG2_IND_SIZE 9 // EB_STRLEN("YUV4MPEG2", MAX_STRING_LENGTH)

int32_t read_y4m_header(EbConfig_t *cfg);

int32_t read_y4m_frame_delimiter(EbConfig_t *cfg);

EB_BOOL check_if_y4m(EbConfig_t *cfg);
