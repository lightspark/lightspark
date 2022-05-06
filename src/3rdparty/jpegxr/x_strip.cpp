
/*************************************************************************
*
* This software module was originally contributed by Microsoft
* Corporation in the course of development of the
* ITU-T T.832 | ISO/IEC 29199-2 ("JPEG XR") format standard for
* reference purposes and its performance may not have been optimized.
*
* This software module is an implementation of one or more
* tools as specified by the JPEG XR standard.
*
* ITU/ISO/IEC give You a royalty-free, worldwide, non-exclusive
* copyright license to copy, distribute, and make derivative works
* of this software module or modifications thereof for use in
* products claiming conformance to the JPEG XR standard as
* specified by ITU-T T.832 | ISO/IEC 29199-2.
*
* ITU/ISO/IEC give users the same free license to this software
* module or modifications thereof for research purposes and further
* ITU/ISO/IEC standardization.
*
* Those intending to use this software module in products are advised
* that its use may infringe existing patents. ITU/ISO/IEC have no
* liability for use of this software module or modifications thereof.
*
* Copyright is not released for products that do not conform to
* to the JPEG XR standard as specified by ITU-T T.832 |
* ISO/IEC 29199-2.
*
* Microsoft Corporation retains full right to modify and use the code
* for its own purpose, to assign or donate the code to a third party,
* and to inhibit third parties from using the code for products that
* do not conform to the JPEG XR standard as specified by ITU-T T.832 |
* ISO/IEC 29199-2.
*
* This copyright notice must be included in all copies or derivative
* works.
*
* Copyright (c) ITU-T/ISO/IEC 2008, 2009.
***********************************************************************/

#ifdef _MSC_VER
#pragma comment (user,"$Id: x_strip.c,v 1.4 2008/03/05 06:58:10 gus Exp $")
#else
#ident "$Id: x_strip.c,v 1.4 2008/03/05 06:58:10 gus Exp $"
#endif

# include "jxr_priv.h"
# include <assert.h>

void _jxr_clear_strip_cur(jxr_image_t image)
{
    assert(image->num_channels > 0);
    int ch;
    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        unsigned idx;
        for (idx = 0 ; idx < EXTENDED_WIDTH_BLOCKS(image) ; idx += 1) {
            int jdx;
            for (jdx = 0 ; jdx < 256 ; jdx += 1)
                image->strip[ch].cur[idx].data[jdx] = 0;
        }
    }
}

/*
* $Log: x_strip.c,v $
* Revision 1.6 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.5 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.4 2008/03/05 06:58:10 gus
* *** empty log message ***
*
* Revision 1.3 2008/02/26 23:52:45 steve
* Remove ident for MS compilers.
*
* Revision 1.2 2007/11/26 01:47:16 steve
* Add copyright notices per MS request.
*
* Revision 1.1 2007/11/12 23:21:55 steve
* Infrastructure for frequency mode ordering.
*
*/

