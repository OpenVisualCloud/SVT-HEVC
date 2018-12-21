/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

// main.cpp
//  -Contructs the following resources needed during the encoding process
//      -memory
//      -threads
//  -Configures the encoder
//  -Calls the encoder via the API
//  -Destructs the resources

/***************************************
 * Includes
 ***************************************/
#include "EbSimpleAppContext.h"
#include "EbApi.h"
#if !__linux
#include <Windows.h>
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#define FOPEN(f,s,m) fopen_s(&f,s,m)
#else
#define fseeko64 fseek
#define ftello64 ftell
#define FOPEN(f,s,m) f=fopen(s,m)
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#endif

/** The APPEXITCONDITIONTYPE type is used to define the App main loop exit
conditions.
*/
typedef enum APPEXITCONDITIONTYPE {
    APP_ExitConditionNone = 0,
    APP_ExitConditionFinished,
    APP_ExitConditionError
} APPEXITCONDITIONTYPE;

/****************************************
* Padding
****************************************/
#define LEFT_INPUT_PADDING 0
#define RIGHT_INPUT_PADDING 0
#define TOP_INPUT_PADDING 0
#define BOTTOM_INPUT_PADDING 0
   
 /**********************************
 * Constructor
 **********************************/
static void EbConfigCtor(EbConfig_t *configPtr)
{
    configPtr->inputFile = NULL;
    configPtr->bitstreamFile = NULL;
    configPtr->reconFile = NULL;
    configPtr->encoderBitDepth = 8;
    configPtr->compressedTenBitFormat = 0;
    configPtr->sourceWidth = 0;
    configPtr->sourceHeight = 0;
    configPtr->framesToBeEncoded = 0;
    configPtr->channelId = 0;
    configPtr->stopEncoder = 0;

    return;
}

/**********************************
* Destructor
**********************************/
static void EbConfigDtor(EbConfig_t *configPtr)
{

    if (configPtr->inputFile) {
        fclose(configPtr->inputFile);
        configPtr->inputFile = (FILE *)NULL;
    }

    if (configPtr->reconFile) {
        fclose(configPtr->reconFile);
        configPtr->reconFile = (FILE *)NULL;
    }

    if (configPtr->bitstreamFile) {
        fclose(configPtr->bitstreamFile);
        configPtr->bitstreamFile = (FILE *)NULL;
    }

    return;
}

APPEXITCONDITIONTYPE ProcessOutputReconBuffer(
    EbConfig_t             *config,
    EbAppContext_t         *appCallBack)
{
    EB_BUFFERHEADERTYPE    *headerPtr = appCallBack->reconBuffer; // needs to change for buffered input
    EB_COMPONENTTYPE       *componentHandle = (EB_COMPONENTTYPE*)appCallBack->svtEncoderHandle;
    APPEXITCONDITIONTYPE    return_value = APP_ExitConditionNone;
    EB_ERRORTYPE            recon_status = EB_ErrorNone;
    int fseekReturnVal;
    // non-blocking call until all input frames are sent
    recon_status = EbH265GetRecon(componentHandle, headerPtr);

    if (recon_status == EB_ErrorMax) {
        printf("\nError while outputing recon, code 0x%x\n", headerPtr->nFlags);
        return APP_ExitConditionError;
    }
    else if (recon_status != EB_NoErrorEmptyQueue) {
        //Sets the File position to the beginning of the file.
        rewind(config->reconFile);
        unsigned long long frameNum = headerPtr->pts;
        while (frameNum>0) {
            fseekReturnVal = fseeko64(config->reconFile, headerPtr->nFilledLen, SEEK_CUR);

            if (fseekReturnVal != 0) {
                printf("Error in fseeko64  returnVal %i\n", fseekReturnVal);
                return APP_ExitConditionError;
            }
            frameNum = frameNum - 1;
        }

        fwrite(headerPtr->pBuffer, 1, headerPtr->nFilledLen, config->reconFile);

        // Update Output Port Activity State
        return_value = (headerPtr->nFlags & EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : APP_ExitConditionNone;
    }
    return return_value;
}
APPEXITCONDITIONTYPE ProcessOutputStreamBuffer(
    EbConfig_t             *config,
    EbAppContext_t         *appCallback,
    unsigned char           picSendDone
)
{
    EB_BUFFERHEADERTYPE    *headerPtr = appCallback->outputStreamBuffer;
    EB_COMPONENTTYPE        *componentHandle = (EB_COMPONENTTYPE*)appCallback->svtEncoderHandle;
    APPEXITCONDITIONTYPE    return_value = APP_ExitConditionNone;
    EB_ERRORTYPE            stream_status = EB_ErrorNone;
    // System performance variables
    static long long int              frameCount = 0;

    // non-blocking call
    stream_status = EbH265GetPacket(componentHandle, headerPtr, picSendDone);

    if (stream_status == EB_ErrorMax) {
        printf("\nError while encoding, code 0x%x\n", headerPtr->nFlags);
        return APP_ExitConditionError;
    }else if (stream_status != EB_NoErrorEmptyQueue) {
        fwrite(headerPtr->pBuffer, 1, headerPtr->nFilledLen, config->bitstreamFile);

        // Update Output Port Activity State
        return_value = (headerPtr->nFlags & EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : APP_ExitConditionNone;
        //printf("\b\b\b\b\b\b\b\b\b%9d", ++frameCount);
        printf("\nDecode Order:\t%lld\tdts:\t%lld\tpts:\t%lld\tSliceType:\t%d", frameCount++, headerPtr->dts , headerPtr->pts, headerPtr->sliceType);

        fflush(stdout);
    }
    return return_value;
}

#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height,is16bit) ( ( ((width)*(height)*3)>>1 )<<is16bit)
void ReadInputFrames(
    EbConfig_t                  *config,
    unsigned char                is16bit,
    EB_BUFFERHEADERTYPE         *headerPtr)
{

    unsigned long long  readSize;
    unsigned int  inputPaddedWidth = config->inputPaddedWidth;
    unsigned int  inputPaddedHeight = config->inputPaddedHeight;
    FILE   *inputFile = config->inputFile;
    unsigned char  *ebInputPtr;
    EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)headerPtr->pBuffer;
    inputPtr->yStride  = inputPaddedWidth;
    inputPtr->cbStride = inputPaddedWidth >> 1;
    inputPtr->crStride = inputPaddedWidth >> 1;
    {
        if (is16bit == 0 || (is16bit == 1 && config->compressedTenBitFormat == 0)) {

            readSize = (unsigned long long)SIZE_OF_ONE_FRAME_IN_BYTES(inputPaddedWidth, inputPaddedHeight, is16bit);

            headerPtr->nFilledLen = 0;

            {
                unsigned long long lumaReadSize = (unsigned long long)inputPaddedWidth*inputPaddedHeight << is16bit;
                ebInputPtr = inputPtr->luma;
                headerPtr->nFilledLen += (unsigned int)fread(ebInputPtr, 1, lumaReadSize, inputFile);
                ebInputPtr = inputPtr->cb;
                headerPtr->nFilledLen += (unsigned int)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
                ebInputPtr = inputPtr->cr;
                headerPtr->nFilledLen += (unsigned int)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
                inputPtr->luma = inputPtr->luma + ((config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING) << is16bit);
                inputPtr->cb = inputPtr->cb + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)) << is16bit);
                inputPtr->cr = inputPtr->cr + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)) << is16bit);

                 if (readSize != headerPtr->nFilledLen) {
                    config->stopEncoder = 1;
                 }
            }
        }
        // 10-bit Compressed Unpacked Mode
        else if (is16bit == 1 && config->compressedTenBitFormat == 1) {

            // Fill the buffer with a complete frame
            headerPtr->nFilledLen = 0;

            unsigned long long lumaReadSize = (unsigned long long)inputPaddedWidth*inputPaddedHeight;
            unsigned long long nbitlumaReadSize = (unsigned long long)(inputPaddedWidth / 4)*inputPaddedHeight;

            ebInputPtr = inputPtr->luma;
            headerPtr->nFilledLen += (unsigned int)fread(ebInputPtr, 1, lumaReadSize, inputFile);
            ebInputPtr = inputPtr->cb;
            headerPtr->nFilledLen += (unsigned int)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
            ebInputPtr = inputPtr->cr;
            headerPtr->nFilledLen += (unsigned int)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);

            inputPtr->luma = inputPtr->luma + config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING;
            inputPtr->cb = inputPtr->cb + (config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1);
            inputPtr->cr = inputPtr->cr + (config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1);


            ebInputPtr = inputPtr->lumaExt;
            headerPtr->nFilledLen += (unsigned int)fread(ebInputPtr, 1, nbitlumaReadSize, inputFile);
            ebInputPtr = inputPtr->cbExt;
            headerPtr->nFilledLen += (unsigned int)fread(ebInputPtr, 1, nbitlumaReadSize >> 2, inputFile);
            ebInputPtr = inputPtr->crExt;
            headerPtr->nFilledLen += (unsigned int)fread(ebInputPtr, 1, nbitlumaReadSize >> 2, inputFile);

            inputPtr->lumaExt = inputPtr->lumaExt + ((config->inputPaddedWidth >> 2)*TOP_INPUT_PADDING + (LEFT_INPUT_PADDING >> 2));
            inputPtr->cbExt = inputPtr->cbExt + (((config->inputPaddedWidth >> 1) >> 2)*(TOP_INPUT_PADDING >> 1) + ((LEFT_INPUT_PADDING >> 1) >> 2));
            inputPtr->crExt = inputPtr->crExt + (((config->inputPaddedWidth >> 1) >> 2)*(TOP_INPUT_PADDING >> 1) + ((LEFT_INPUT_PADDING >> 1) >> 2));

            readSize = ((lumaReadSize * 3) >> 1) + ((nbitlumaReadSize * 3) >> 1);

            if (readSize != headerPtr->nFilledLen) {
                config->stopEncoder = 1;
            }

        }
    }
    // If we reached the end of file, loop over again
    if (feof(inputFile) != 0) {
        //fseek(inputFile, 0, SEEK_SET);
        config->stopEncoder = 1;
    }

    return;
}
#define  TEST_IDR 0
APPEXITCONDITIONTYPE ProcessInputBuffer(
    EbConfig_t                  *config,
    EbAppContext_t              *appCallBack)
{
    unsigned char            is16bit = (unsigned char)(config->encoderBitDepth > 8);
    EB_BUFFERHEADERTYPE     *headerPtr = appCallBack->inputPictureBuffer; // needs to change for buffered input
    EB_COMPONENTTYPE        *componentHandle = (EB_COMPONENTTYPE*)appCallBack->svtEncoderHandle;
    APPEXITCONDITIONTYPE     return_value = APP_ExitConditionNone;
    static int               frameCount = 0;

    if (config->stopEncoder == 0) {
        ReadInputFrames(
            config,
            is16bit,
            headerPtr);

        if (config->stopEncoder == 0) {
            // Fill in Buffers Header control data
            headerPtr->nFlags = 0;
            headerPtr->pAppPrivate = NULL;
            headerPtr->pts         = frameCount++;
            headerPtr->sliceType   = EB_INVALID_SLICE;
#if TEST_IDR
            if (frameCount == 200)
                headerPtr->sliceType = IDR_SLICE;
            if (frameCount == 150)
                headerPtr->sliceType = I_SLICE;
#endif
            // Send the picture
            EbH265EncSendPicture(componentHandle, headerPtr);
        }
        else {
            EB_BUFFERHEADERTYPE headerPtrLast;
            headerPtrLast.nAllocLen = 0;
            headerPtrLast.nFilledLen = 0;
            headerPtrLast.nTickCount = 0;
            headerPtrLast.pAppPrivate = NULL;
            headerPtrLast.nFlags = EB_BUFFERFLAG_EOS;
            headerPtrLast.pBuffer = NULL;

            EbH265EncSendPicture(componentHandle, &headerPtrLast);
        }
        return_value = (headerPtr->nFlags == EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : return_value;
    }
    return return_value;
}

/***************************************
 * Encoder App Main
 ***************************************/
int main(int argc, char* argv[])
{
    EB_ERRORTYPE            return_error = EB_ErrorNone;            // Error Handling
    APPEXITCONDITIONTYPE    exitConditionOutput = APP_ExitConditionNone , exitConditionInput = APP_ExitConditionNone , exitConditionRecon = APP_ExitConditionNone;    // Processing loop exit condition
    EbConfig_t             *config;        // Encoder Configuration
    EbAppContext_t         *appCallback;   // Instances App callback data
    
    // Print Encoder Info
    printf("-------------------------------------\n");
    printf("SVT-HEVC Encoder Simple Sample Application v1.2.0\n");
    printf("Platform:   %u bit\n", (unsigned) sizeof(void*)*8);
#if ( defined( _MSC_VER ) && (_MSC_VER < 1910) ) 
	printf("Compiler: VS13\n");
#elif ( defined( _MSC_VER ) && (_MSC_VER >= 1910) ) 
	printf("Compiler: VS17\n");
#elif defined(__INTEL_COMPILER)
	printf("Compiler: Intel\n");
#elif defined(__GNUC__)
	printf("Compiler: GCC\n");
#else
	printf("Compiler: unknown\n");
#endif

    printf("APP Build date: %s %s\n",__DATE__,__TIME__);
    fflush(stdout);
    {
        // Initialize config
        config = (EbConfig_t*)malloc(sizeof(EbConfig_t));
        EbConfigCtor(config);
        if (argc != 6 && argc != 7) {
            printf("Usage: ./HevcEncoderSimpleApp in.yuv out.265 width height bitdepth recon.yuv(optional)\n");
            return_error = EB_ErrorBadParameter;
        }
        else {
            // Get info for config
            FILE * fin;
            FOPEN(fin,argv[1], "rb");
            if (!fin) {
                printf("Invalid input file \n");
                return_error = EB_ErrorBadParameter;
            }
            else
                config->inputFile = fin;

            FILE * fout;
            FOPEN(fout,argv[2], "wb");
            if (!fout) {
                printf("Invalid input file \n");
                return_error = EB_ErrorBadParameter;
            }
            else
                config->bitstreamFile = fout;

            unsigned int width = 0, height = 0;
            
            width = strtoul(argv[3], NULL, 0);
            height = strtoul(argv[4], NULL, 0);
            if ((width&&height) == 0) { printf("Invalid video dimensions\n"); return_error = EB_ErrorBadParameter; }

            config->inputPaddedWidth  = config->sourceWidth = width;
            config->inputPaddedHeight = config->sourceHeight = height;

            unsigned int bdepth = width = strtoul(argv[5], NULL, 0);
            if ((bdepth != 8) && (bdepth != 10)) {printf("Invalid bit depth\n"); return_error = EB_ErrorBadParameter; }
            config->encoderBitDepth = bdepth;

            if (argc == 7) {
                FILE * frec;
                FOPEN(frec, argv[6], "wb");
                if (!frec) {
                    printf("Invalid recon file \n");
                    return_error = EB_ErrorBadParameter;
                }
                else
                    config->reconFile = frec;
            }

        }
        if (return_error == EB_ErrorNone) {

            // Initialize appCallback
            appCallback = (EbAppContext_t*)malloc(sizeof(EbAppContext_t));
            EbAppContextCtor(appCallback,config);
            
            return_error = InitEncoder(config, appCallback, 0);

            printf("Encoding          ");
            fflush(stdout);

            // Input Loop Thread
            exitConditionOutput = APP_ExitConditionNone;
            exitConditionRecon = APP_ExitConditionNone;
            while (exitConditionOutput == APP_ExitConditionNone) {
                exitConditionInput = ProcessInputBuffer(config, appCallback);
                if (config->reconFile) {
                    exitConditionRecon = ProcessOutputReconBuffer(config, appCallback);
                }                
                exitConditionOutput = ProcessOutputStreamBuffer(config, appCallback, (exitConditionInput == APP_ExitConditionNone || (exitConditionRecon == APP_ExitConditionNone && config->reconFile) ? 0 : 1));
            }
            
            printf("\n");
            fflush(stdout);

            // DeInit Encoder
            return_error = DeInitEncoder(appCallback, 0);

            // Destruct the App memory variables
            EbAppContextDtor(appCallback);
            EbConfigDtor(config);
            free(config);
            free(appCallback);
        }
        else {
            printf("Error in configuration, could not begin encoding! ... \n");
        }
    }
    printf("Encoder finished\n");

    return (return_error == 0) ? 0 : 1;
}
