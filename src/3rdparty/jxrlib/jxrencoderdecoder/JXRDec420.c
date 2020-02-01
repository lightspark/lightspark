//*@@@+++@@@@******************************************************************
//
// Copyright © Microsoft Corp.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// • Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// • Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//*@@@---@@@@******************************************************************
#define _CRT_SECURE_NO_WARNINGS
#include <JXRTest.h>
#include <errno.h>

//================================================================
// main function
//================================================================
int 
#ifndef __ANSI__
__cdecl 
#endif // __ANSI__
main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Required arguments:\n");
        fprintf(stderr, "1. Path to JXR input file\n");
        fprintf(stderr, "2. Path to YUV output file\n");
        return 1;
    }

    errno = 0;

    const char *jxr_path = argv[1];
    const char *yuv_path = argv[2];

    /* the planar YUV buffer */
    unsigned char *image_buffer = NULL;
    int yuv_size = 0;

    {
        ERR err = WMP_errSuccess;

        int width, height;
        PKFactory*      pFactory      = NULL;
        PKCodecFactory* pCodecFactory = NULL;
        PKImageDecode*  pDecoder      = NULL;
        const PKIID*    pIID          = NULL;
        PKImageEncode*  pEncoder      = NULL;
        struct WMPStream* pEncodeStream = NULL;

        Call( PKCreateFactory(&pFactory, PK_SDK_VERSION) );
        Call( pFactory->CreateStreamFromFilename(&pEncodeStream, yuv_path, "wb") );

        Call( PKCreateCodecFactory(&pCodecFactory, WMP_SDK_VERSION) );
        Call( pCodecFactory->CreateDecoderFromFile(jxr_path, &pDecoder) );

        Call( GetTestEncodeIID(".iyuv", &pIID) );
        Call( PKTestFactory_CreateCodec(pIID, (void **) &pEncoder) );
        
        // check that pixel format is YCC 4:2:0
        PKPixelFormatGUID pix_frmt;
        Call( pDecoder->GetPixelFormat(pDecoder, &pix_frmt) );
        if( memcmp(&pix_frmt, &GUID_PKPixelFormat12bppYCC420, 
                   sizeof(PKPixelFormatGUID)) != 0 ) {
            err = WMP_errFail;
            goto Cleanup;
        }
        
        // get the dimensions
        Call( pDecoder->GetSize(pDecoder, &width, &height) );
		yuv_size = width*height+2*(width>>1)*(height>>1);

        // decode
        PKRect rc;
        rc.X = 0;
        rc.Y = 0;
        rc.Width  = width;
        rc.Height = height;
    
        image_buffer = (U8*)malloc(yuv_size);
        Call( pDecoder->Copy(pDecoder, &rc, (U8*)image_buffer, width*3) );

        // write the decoded result
        Call( pEncoder->Initialize(pEncoder, pEncodeStream, NULL, 0) );
        Call( pEncoder->SetPixelFormat(pEncoder, pix_frmt));
        Call( pEncoder->SetSize(pEncoder, width, height));
        Call( pEncoder->WritePixels(pEncoder, height, image_buffer, width) );
    
    Cleanup:
        if( pDecoder )      pDecoder->Release(&pDecoder);
        if( pEncoder )      pEncoder->Release(&pEncoder);
        if( pCodecFactory ) pCodecFactory->Release(&pCodecFactory);
        if( pFactory )      pFactory->Release(&pFactory);
        if( err != WMP_errSuccess ) {
            fprintf(stderr, "Failed to decode\n");
            return 1;
        }
    }

    free(image_buffer);

    return 0;
}
