
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
**********************************************************************/

#ifdef _MSC_VER
#pragma comment (user,"$Id: w_tile_spatial.c,v 1.47 2008/03/20 18:11:25 steve Exp $")
#else
#ident "$Id: w_tile_spatial.c,v 1.47 2008/03/20 18:11:25 steve Exp $"
#endif

# include "jxr_priv.h"
# include <stdlib.h>
# include <assert.h>


void _jxr_w_TILE_SPATIAL(jxr_image_t image, struct wbitstream*str,
                         unsigned tx, unsigned ty)
{
    DEBUG("START TILE_SPATIAL at tile=[%u %u] bitpos=%zu\n", tx, ty, _jxr_wbitstream_bitpos(str));

    /* TILE_STARTCODE == 1 */
    DEBUG(" TILE_STARTCODE at bitpos=%zu\n", _jxr_wbitstream_bitpos(str));
    _jxr_wbitstream_uint8(str, 0x00);
    _jxr_wbitstream_uint8(str, 0x00);
    _jxr_wbitstream_uint8(str, 0x01);
    _jxr_wbitstream_uint8(str, 0x00); /* ARBITRARY_BYTE */

    if (TRIM_FLEXBITS_FLAG(image)) {
        _jxr_wbitstream_uint4(str, image->trim_flexbits);
    }

    /* Write out the tile header (which includes sub-headers for
    all the major passes). */

    _jxr_w_TILE_HEADER_DC(image, str, 0, tx, ty);
    if (image->bands_present != 3 /* DCONLY */) {
        _jxr_w_TILE_HEADER_LOWPASS(image, str, 0, tx, ty);

        if (image->bands_present != 2 /* NO_HIGHPASS */) {
            _jxr_w_TILE_HEADER_HIGHPASS(image, str, 0, tx, ty);
        }
    }

    if (ALPHACHANNEL_FLAG(image)) {
        _jxr_w_TILE_HEADER_DC(image->alpha, str, 1, tx, ty);
        if (image->bands_present != 3 /* DCONLY */) {
            _jxr_w_TILE_HEADER_LOWPASS(image->alpha, str, 1, tx, ty);

            if (image->bands_present != 2 /* NO_HIGHPASS */) {
                _jxr_w_TILE_HEADER_HIGHPASS(image->alpha, str, 1, tx, ty);
            }
        }
    }


    /* Now form and write out all the compressed data for the
    tile. This involves scanning the macroblocks, and the
    blocks within the macroblocks, generating bits as we go. */

    unsigned mb_height = EXTENDED_HEIGHT_BLOCKS(image);
    unsigned mb_width = EXTENDED_WIDTH_BLOCKS(image);

    if (TILING_FLAG(image)) {
        mb_height = image->tile_row_height[ty];
        mb_width = image->tile_column_width[tx];
    }

    DEBUG(" TILE_SPATIAL at [%d %d] is %u x %u MBs\n", tx, ty, mb_width, mb_height);
    unsigned mx, my;
    unsigned plane_idx, num_planes = ((ALPHACHANNEL_FLAG(image)) ? 2 : 1);

    for (my = 0 ; my < mb_height ; my += 1) {

        _jxr_wflush_mb_strip(image, tx, ty, my, 1);

        for (mx = 0 ; mx < mb_width ; mx += 1)
        for (plane_idx = 0; plane_idx < num_planes; plane_idx ++) {

            jxr_image_t plane = (plane_idx == 0 ? image : image->alpha);
            if (plane->bands_present!=3) {
                if (plane->num_lp_qps>1 && !plane->lp_use_dc_qp) {
                    unsigned qp_index = _jxr_select_lp_index(plane, tx,ty,mx,my);
                    DEBUG(" DECODE_QP_INDEX(%d) --> %u\n", plane->num_lp_qps, qp_index);
                    _jxr_w_ENCODE_QP_INDEX(plane, str, tx, ty, mx, my, plane->num_lp_qps, qp_index);
                }
                if (plane->bands_present!=2 && plane->num_hp_qps>1 && !plane->hp_use_lp_qp) {
                    unsigned qp_index = _jxr_select_hp_index(plane, tx,ty,mx,my);
                    DEBUG(" DECODE_QP_INDEX(%d) --> %u\n", plane->num_hp_qps, qp_index);
                    _jxr_w_ENCODE_QP_INDEX(plane, str, tx, ty, mx, my, plane->num_hp_qps, qp_index);
                }
            }

            _jxr_w_MB_DC(plane, str, 0, tx, ty, mx, my);
            if (plane->bands_present != 3 /* DCONLY */) {
                _jxr_w_MB_LP(plane, str, 0, tx, ty, mx, my);
                if (plane->bands_present != 2 /* NOHIGHPASS */) {
                    _jxr_w_MB_CBP(plane, str, 0, tx, ty, mx, my);
                    _jxr_w_MB_HP(plane, str, 0, tx, ty, mx, my, 0);

                    /* In SPATIAL mode, the MB_HP block
                    will include the FLEXBITS. DO NOT
                    include the MB_FLEXBITS separately. */
                }
            }
        }
    }

    _jxr_wbitstream_syncbyte(str);
    _jxr_wbitstream_flush(str);
    DEBUG("END TILE_SPATIAL\n");
}


/*
* $Log: w_tile_spatial.c,v $
* Revision 1.49 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.48 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.47 2008/03/20 18:11:25 steve
* Handle case of NumLPQP==1 and NumHPQPS>1
*
* Revision 1.46 2008/03/18 18:36:56 steve
* Support compress of CMYK images.
*
* Revision 1.45 2008/03/13 17:49:31 steve
* Fix problem with YUV422 CBP prediction for UV planes
*
* Add support for YUV420 encoding.
*
* Revision 1.44 2008/03/13 00:07:23 steve
* Encode HP of YUV422
*
* Revision 1.43 2008/03/12 21:10:27 steve
* Encode LP of YUV422
*
* Revision 1.42 2008/03/06 22:47:40 steve
* Clean up parsing/encoding of QP counts
*
* Revision 1.41 2008/03/06 19:21:59 gus
* *** empty log message ***
*
* Revision 1.40 2008/03/06 02:05:49 steve
* Distributed quantization
*
* Revision 1.39 2008/03/05 00:31:18 steve
* Handle UNIFORM/IMAGEPLANE_UNIFORM compression.
*
* Revision 1.38 2008/02/28 18:50:31 steve
* Portability fixes.
*
* Revision 1.37 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.36 2008/02/24 17:42:33 steve
* Remove dead code.
*
* Revision 1.35 2008/02/23 01:55:52 steve
* CBP REFINE is more complex when CHR is involved.
*
* Revision 1.34 2008/02/22 23:54:15 steve
* Encode large NUM_BLKCBP values.
*
* Revision 1.33 2008/02/22 23:08:15 steve
* Remove dead code.
*
* Revision 1.32 2008/02/22 23:01:33 steve
* Compress macroblock HP CBP packets.
*
* Revision 1.31 2008/02/05 23:18:26 steve
* Fix cbplp encoding is backwards.
*
* Revision 1.30 2008/02/05 19:14:42 steve
* Encode LP values of YUV444 data.
*
* Revision 1.29 2008/02/01 22:49:53 steve
* Handle compress of YUV444 color DCONLY
*
* Revision 1.28 2008/01/07 16:18:54 steve
* Shift out trimmed flexbits.
*
* Revision 1.27 2008/01/06 01:29:28 steve
* Add support for TRIM_FLEXBITS in compression.
*
* Revision 1.26 2008/01/05 02:11:54 steve
* Add FLEXBITS support.
*
* Revision 1.25 2008/01/04 19:34:05 steve
* Fix DecNumBlkCBP table adaptation.
*
* Revision 1.24 2008/01/03 18:07:31 steve
* Detach prediction mode calculations from prediction.
*
* Revision 1.23 2008/01/02 21:13:58 steve
* Extract HP model bits accounts for sign.
*
* Revision 1.22 2008/01/02 21:02:55 steve
* Account for model_bits while encoding HP values.
*
* Revision 1.21 2008/01/01 23:13:00 steve
* Fix AdaptiveHPPermute totals counting.
*
* Revision 1.20 2008/01/01 01:07:26 steve
* Add missing HP prediction.
*
* Revision 1.19 2007/12/30 00:16:00 steve
* Add encoding of HP values.
*
* Revision 1.18 2007/12/17 23:02:57 steve
* Implement MB_CBP encoding.
*
* Revision 1.17 2007/12/14 21:33:39 steve
* Process LPInput values in sign-magnitude form.
*
* Revision 1.16 2007/12/14 17:10:39 steve
* HP CBP Prediction
*
* Revision 1.15 2007/12/13 19:27:36 steve
* Careful of small negative LP values that are all refinement.
*
* Revision 1.14 2007/12/13 18:01:09 steve
* Stubs for HP encoding.
*
* Revision 1.13 2007/12/12 00:38:07 steve
* Encode LP data.
*
* Revision 1.12 2007/12/07 01:20:34 steve
* Fix adapt not adapting on line ends.
*
* Revision 1.11 2007/12/06 23:12:41 steve
* Stubs for LP encode operations.
*
* Revision 1.10 2007/12/06 17:55:30 steve
* Fix AbsLevelInd tabel select and adaptation.
*
* Revision 1.9 2007/12/05 02:18:01 steve
* Formatting of nil LP values.
*
* Revision 1.8 2007/12/04 23:44:02 steve
* Calculation of encoded IS_DC_CH accounts for sign.
*
* Revision 1.7 2007/12/04 22:06:10 steve
* Infrastructure for encoding LP.
*
* Revision 1.6 2007/11/30 01:50:58 steve
* Compression of DCONLY GRAY.
*
* Revision 1.5 2007/11/26 01:47:16 steve
* Add copyright notices per MS request.
*
* Revision 1.4 2007/11/09 01:18:59 steve
* Stub strip input processing.
*
* Revision 1.3 2007/11/08 19:38:38 steve
* Get stub DCONLY compression to work.
*
* Revision 1.2 2007/11/08 02:52:33 steve
* Some progress in some encoding infrastructure.
*
* Revision 1.1 2007/06/06 17:19:13 steve
* Introduce to CVS.
*
*/

