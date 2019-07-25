/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbAppInputy4m.h"
#define YFM_HEADER_MAX 80
#define YUV4MPEG2_IND_SIZE 9
#define PRINT_HEADER 0
#define CHROMA_MAX 4

#include "EbAppConfig.h"
#include <ctype.h>

/* copy a string until a specified character or a new line is found */
char* copyUntilCharacterOrNewLine(char *src, char *dst, char chr) {
    rsize_t count = 0;
    char * src_init = src;

    while (*src != chr && *src != '\n') {
        src++;
        count++;
    }

    //EB_STRNCPY(dst, YFM_HEADER_MAX, src_init, count);
    EB_STRNCPY(dst, src_init, count);

    return src;
}

/* reads the y4m header and parses the input parameters */
int32_t read_y4m_header(EbConfig_t *cfg) {
    FILE *ptr_in;
    char buffer[YFM_HEADER_MAX];
    char *fresult, *tokstart, *tokend, format_str[YFM_HEADER_MAX];
    uint32_t bitdepth = 8, width = 0, height = 0, fr_n = 0,
        fr_d = 0, aspect_n, aspect_d;
    char chroma[CHROMA_MAX] = "420", scan_type = 'p';
    EB_BOOL interlaced = EB_TRUE;

    /* pointer to the input file */
    ptr_in = cfg->inputFile;

    /* get first line after YUV4MPEG2 */
    fresult = fgets(buffer, sizeof(buffer), ptr_in);
    if (fresult== NULL) { 
        return EB_ErrorBadParameter;
    }

    /* print header */
#ifdef PRINT_HEADER
    printf("y4m header:");
    fputs(buffer, stdout);
#endif

    /* read header parameters */
    for (tokstart = &(buffer[0]); *tokstart != '\0'; tokstart++) {
        if (*tokstart == 0x20)
            continue;
        switch (*tokstart++) {
        case 'W': /* width, required. */
            width = (uint32_t)strtol(tokstart, &tokend, 10);
#ifdef PRINT_HEADER
            printf("width = %d\n", width);
#endif
            tokstart = tokend;
            break;
        case 'H': /* height, required. */
            height = (uint32_t)strtol(tokstart, &tokend, 10);
#ifdef PRINT_HEADER
            printf("height = %d\n", height);
#endif
            tokstart = tokend;
            break;
        case 'I': /* scan type, not required, default: 'p' */
            switch (*tokstart++) {
            case 'p':
                interlaced = EB_FALSE;
                scan_type = 'p';
                break;
            case 't':
                interlaced = EB_TRUE;
                scan_type = 't';
                break;
            case 'b':
                interlaced = EB_TRUE;
                scan_type = 'b';
                break;
            case '?':
            default:
                fprintf(cfg->errorLogFile, "interlace type not supported\n");
                return EB_ErrorBadParameter;
            }
#ifdef PRINT_HEADER
            printf("scan_type = %c\n", scan_type);
#endif
            break;
        case 'C': /* color space, not required: default "420" */
            tokstart = copyUntilCharacterOrNewLine(tokstart, format_str, 0x20);
            if (EB_STRCMP("420mpeg2", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "420");
                // chroma left
                bitdepth = 8;
            }
            else if (EB_STRCMP("420paldv", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "420");
                // chroma top-left
                bitdepth = 8;
            }
            else if (EB_STRCMP("420jpeg", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "420");
                // chroma center
                bitdepth = 8;
            }
            else if (EB_STRCMP("420p16", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "420");
                bitdepth = 16;
            }
            else if (EB_STRCMP("422p16", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "422");
                bitdepth = 16;
            }
            else if (EB_STRCMP("444p16", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "444");
                bitdepth = 16;
            }
            else if (EB_STRCMP("420p14", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "420");
                bitdepth = 14;
            }
            else if (EB_STRCMP("422p14", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "422");
                bitdepth = 14;
            }
            else if (EB_STRCMP("444p14", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "444");
                bitdepth = 14;
            }
            else if (EB_STRCMP("420p12", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "420");
                bitdepth = 12;
            }
            else if (EB_STRCMP("422p12", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "422");
                bitdepth = 12;
            }
            else if (EB_STRCMP("444p12", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "444");
                bitdepth = 12;
            }
            else if (EB_STRCMP("420p10", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "420");
                bitdepth = 10;
            }
            else if (EB_STRCMP("422p10", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "422");
                bitdepth = 10;
            }
            else if (EB_STRCMP("444p10", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "444");
                bitdepth = 10;
            }
            else if (EB_STRCMP("420p9", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "420");
                bitdepth = 9;
            }
            else if (EB_STRCMP("422p9", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "422");
                bitdepth = 9;
            }
            else if (EB_STRCMP("444p9", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "444");
                bitdepth = 9;
            }
            else if (EB_STRCMP("420", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "420");
                bitdepth = 8;
            }
            else if (EB_STRCMP("411", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "411");
                bitdepth = 8;
            }
            else if (EB_STRCMP("422", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "422");
                bitdepth = 8;
            }
            else if (EB_STRCMP("444", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "444");
                bitdepth = 8;
            }
            else if (EB_STRCMP("mono16", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "400");
                bitdepth = 16;
            }
            else if (EB_STRCMP("mono12", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "400");
                bitdepth = 12;
            }
            else if (EB_STRCMP("mono10", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "400");
                bitdepth = 10;
            }
            else if (EB_STRCMP("mono9", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "400");
                bitdepth = 9;
            }
            else if (EB_STRCMP("mono", format_str) == 0) {
                EB_STRCPY(chroma, CHROMA_MAX, "400");
                bitdepth = 8;
            }
            else {
                fprintf(cfg->errorLogFile, "chroma format not supported\n");
                return EB_ErrorBadParameter;
            }
#ifdef PRINT_HEADER
            printf("chroma = %s, bitdepth = %d\n", chroma, bitdepth);
#endif
            break;
        case 'F': /* frame rate, required */
            tokstart = copyUntilCharacterOrNewLine(tokstart, format_str, ':');
            fr_n = (uint32_t)strtol(format_str, (char **)NULL, 10);
            tokstart++;
            tokstart = copyUntilCharacterOrNewLine(tokstart, format_str, 0x20);
            fr_d = (uint32_t)strtol(format_str, (char **)NULL, 10);
#ifdef PRINT_HEADER
            printf("framerate_n = %d\n", fr_n);
            printf("framerate_d = %d\n", fr_d);
#endif
            break;
        case 'A': /* aspect ratio, not required */
            tokstart = copyUntilCharacterOrNewLine(tokstart, format_str, ':');
            aspect_n = (uint32_t)strtol(format_str, (char **)NULL, 10);
            tokstart++;
            tokstart = copyUntilCharacterOrNewLine(tokstart, format_str, 0x20);
            aspect_d = (uint32_t)strtol(format_str, (char **)NULL, 10);
#ifdef PRINT_HEADER
            printf("aspect_n = %d\n", aspect_n);
            printf("aspect_d = %d\n", aspect_d);
#endif
            break;
        default:
            break;
        }
    }

    /*check if required parameters were read*/
    if (width == 0) {
        fprintf(cfg->errorLogFile, "width not found in y4m header\n");
        return EB_ErrorBadParameter;
    }
    if (height == 0) {
        fprintf(cfg->errorLogFile, "height not found in y4m header\n");
        return EB_ErrorBadParameter;
    }
    if (fr_n == 0 || fr_d == 0) {
        fprintf(cfg->errorLogFile, "frame rate not found in y4m header\n");
        return EB_ErrorBadParameter;
    }

    /* Assign parameters to cfg */
    cfg->sourceWidth = width;
    cfg->sourceHeight = height;
    cfg->frameRateNumerator = fr_n;
    cfg->frameRateDenominator = fr_d;
    cfg->frameRate = fr_n / fr_d;
    cfg->encoderBitDepth = bitdepth;
    cfg->interlacedVideo = interlaced;
    if (EB_STRCMP("420", chroma) == 0) {
        cfg->encoderColorFormat = EB_YUV420;
    }
    else if (EB_STRCMP("422", chroma) == 0) {
        cfg->encoderColorFormat = EB_YUV422;
    }
    else if (EB_STRCMP("444", chroma) == 0) {
        cfg->encoderColorFormat = EB_YUV444;
    }
    else if (EB_STRCMP("400", chroma) == 0) {
        cfg->encoderColorFormat = EB_YUV400;
    }
    else {
        fprintf(cfg->errorLogFile, "Unsupported color format: %s\n", chroma);
        return EB_ErrorBadParameter;
    }

    return EB_ErrorNone;
}

EB_BOOL validateAlphanumeric(unsigned char* buffer)
{
    /* validate input is alphanumeric */
    unsigned char *cp = buffer;
    const unsigned char *end = buffer + strlen((char*)buffer);
    for (cp = buffer; cp != end; cp++)
    {
        if (!isalnum(*cp) && *cp!='\n')
            return EB_FALSE;
    }
    return EB_TRUE;
}

/* read next line which contains the "FRAME" delimiter */
int32_t read_y4m_frame_delimiter(EbConfig_t *cfg) {
    unsigned char bufferY4Mheader[10];
    char *fresult;

    fresult = fgets((char *)bufferY4Mheader, sizeof(bufferY4Mheader), cfg->inputFile);

    if (fresult == NULL) {
        assert(feof(cfg->inputFile));
        return EB_ErrorNone;
    }
    if (!validateAlphanumeric(bufferY4Mheader)){
        return EB_ErrorBadParameter;
    }
    if (EB_STRCMP((const char*)bufferY4Mheader, "FRAME\n") != 0) {
        fprintf(cfg->errorLogFile, "Failed to read proper y4m frame delimeter. Read broken.\n");
        return EB_ErrorBadParameter;
    }

    return EB_ErrorNone;
}

/* check if the input file is in YUV4MPEG2 (y4m) format */
EB_BOOL check_if_y4m(EbConfig_t *cfg) {
    unsigned char buffer[YUV4MPEG2_IND_SIZE + 1];
    size_t headerReadLength;

    /* Parse the header for the "YUV4MPEG2" string */
    headerReadLength = fread(buffer, YUV4MPEG2_IND_SIZE, 1, cfg->inputFile);
    if (headerReadLength != 1) {
        assert(feof(cfg->inputFile));
        return EB_FALSE;
    }

    buffer[YUV4MPEG2_IND_SIZE] = 0;
    if (validateAlphanumeric(buffer) && EB_STRCMP((char*)buffer, "YUV4MPEG2") == 0) {
        return EB_TRUE; /* YUV4MPEG2 file */
    }
    else {
        if (cfg->inputFile != stdin) {
            fseek(cfg->inputFile, 0, SEEK_SET);
        }
        else {
            EB_STRNCPY((char*)cfg->y4m_buf, (char*)buffer, YUV4MPEG2_IND_SIZE);
        }
        return EB_FALSE; /* Not a YUV4MPEG2 file */
    }
}
