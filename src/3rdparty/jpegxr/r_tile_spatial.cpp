
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
#pragma comment (user,"$Id: r_tile_spatial.c,v 1.53 2008/03/20 22:39:41 steve Exp $")
#else
#ident "$Id: r_tile_spatial.c,v 1.53 2008/03/20 22:39:41 steve Exp $"
#endif

# include "jxr_priv.h"
# include <assert.h>



/*
* Process a single spatial time. The tx/ty is the coordintes of the
* tile in units of tiles. tx=0 for the first time, tx=1 for the
* second, and so forth.
*/
int _jxr_r_TILE_SPATIAL(jxr_image_t image, struct rbitstream*str,
                        unsigned tx, unsigned ty)
{
    int rc = 0;
    DEBUG("START TILE_SPATIAL at tile=[%u %u] bitpos=%zu\n", tx, ty, _jxr_rbitstream_bitpos(str));

    /* TILE_STARTCODE == 1 */
    unsigned char s0, s1, s2, s3;
    s0 = _jxr_rbitstream_uint8(str); /* 0x00 */
    s1 = _jxr_rbitstream_uint8(str); /* 0x00 */
    s2 = _jxr_rbitstream_uint8(str); /* 0x01 */
    s3 = _jxr_rbitstream_uint8(str); /* reserved */
    DEBUG(" TILE_STARTCODE == %02x %02x %02x (reserved: %02x)\n", s0, s1, s2, s3);

    image->trim_flexbits = 0;
    if (TRIM_FLEXBITS_FLAG(image)) {
        image->trim_flexbits =_jxr_rbitstream_uint4(str);
        DEBUG(" TRIM_FLEXBITS = %u\n", image->trim_flexbits);
    }

    /* Read the tile header (which includes sub-headers for
    all the major passes). */

    _jxr_r_TILE_HEADER_DC(image, str, 0, tx, ty);
    if (image->bands_present != 3 /* DCONLY */) {
        _jxr_r_TILE_HEADER_LOWPASS(image, str, 0, tx, ty);

        if (image->bands_present != 2 /* NO_HIGHPASS */) {
            _jxr_r_TILE_HEADER_HIGHPASS(image, str, 0, tx, ty);
        }
    }

    /* If the alpha channel is present, then run another set of
    headers for the alpha channel. */
    if (ALPHACHANNEL_FLAG(image)) {
        _jxr_r_TILE_HEADER_DC(image->alpha, str, 1, tx, ty);
        if (image->bands_present != 3 /* DCONLY */) {
            _jxr_r_TILE_HEADER_LOWPASS(image->alpha, str, 1, tx, ty);

            if (image->bands_present != 2 /* NO_HIGHPASS */) {
                _jxr_r_TILE_HEADER_HIGHPASS(image->alpha, str, 1, tx, ty);
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

    unsigned mx, my, plane_idx;
    for (my = 0 ; my < mb_height ; my += 1) {
        if (ALPHACHANNEL_FLAG(image))
            _jxr_rflush_mb_strip(image->alpha, tx, ty, my);
        _jxr_rflush_mb_strip(image, tx, ty, my);

        for (mx = 0 ; mx < mb_width ; mx += 1) {
        for(plane_idx = 0U; plane_idx < (ALPHACHANNEL_FLAG(image) ? 2U : 1U); plane_idx ++){
            int ch;

            /* There is one LP_QP_INDEX per macroblock (if any)
            and that value applies to all the channels.
            Same for HP_QP_INDEX. There is no DC_QP_INDEX
            because DC QP values are per-tile, not per MB. */
            int qp_index_lp = 0;
            int qp_index_hp = 0;
            jxr_image_t plane = (plane_idx == 0 ? image : image->alpha);

            if (plane->bands_present!=3) {
                if (plane->num_lp_qps>1 && !plane->lp_use_dc_qp) {
                    qp_index_lp = _jxr_DECODE_QP_INDEX(str, plane->num_lp_qps);
                    DEBUG(" DECODE_QP_INDEX(%d) --> %u (LP)\n", plane->num_lp_qps, qp_index_lp);
                }
                qp_index_hp = 0;
                if (plane->bands_present!=2 && plane->num_hp_qps>1) {
                    if (!plane->hp_use_lp_qp) {
                        qp_index_hp = _jxr_DECODE_QP_INDEX(str, plane->num_hp_qps);
                        DEBUG(" DECODE_QP_INDEX(%d) --> %u (HP)\n", plane->num_hp_qps, qp_index_hp);
                    }
                    else {
                        qp_index_hp = qp_index_lp;
                    }
                }
            }
            for (ch = 0 ; ch < plane->num_channels ; ch += 1) {
                /* Save the LP Quant *INDEX* here. Prediction needs it. */
                MACROBLK_CUR_LP_QUANT(plane,ch,tx,mx) = qp_index_lp;
                DEBUG(" LP_QUANT INDEX for tx=%u ty=%u ch=%u MBx=%d is %d\n", tx, ty, ch, mx,
                    MACROBLK_CUR_LP_QUANT(plane,ch,tx,mx));
                MACROBLK_CUR_HP_QUANT(plane,ch,tx,mx) = plane->hp_quant_ch[ch][qp_index_hp];
                DEBUG(" HP_QUANT VALUE for tx=%u ty=%u ch=%u MBx=%d is %d\n", tx, ty, ch, mx,
                    MACROBLK_CUR_HP_QUANT(plane,ch,tx,mx));
            }

            _jxr_r_MB_DC(plane, str, plane_idx, tx, ty, mx, my);
            if (plane->bands_present != 3 /* DCONLY */) {
                _jxr_r_MB_LP(plane, str, plane_idx, tx, ty, mx, my);
                _jxr_complete_cur_dclp(plane, tx, mx, my);
                if (plane->bands_present != 2 /* NOHIGHPASS */) {
                    rc = _jxr_r_MB_CBP(plane, str, plane_idx, tx, ty, mx, my);
                    if (rc < 0) {
                        DEBUG("r_MB_CBP returned ERROR rc=%d\n", rc);
                        return rc;
                    }
                    rc = _jxr_r_MB_HP(plane, str, plane_idx, tx, ty, mx, my);
                    if (rc < 0) {
                        DEBUG("r_MB_HP returned ERROR rc=%d\n", rc);
                        return rc;
                    }
                }
            } else {
                _jxr_complete_cur_dclp(plane, tx, mx, my);
            }
        }
    }
    }

    /* Flush the remaining strips to output. */
    if (tx+1 == image->tile_columns && ty+1 == image->tile_rows) {
        DEBUG(" Cleanup flush after last tile (tx=%d, ty=%d)\n", tx, ty);
        if (ALPHACHANNEL_FLAG(image))
            _jxr_rflush_mb_strip(image->alpha, tx, ty, mb_height);
        _jxr_rflush_mb_strip(image, tx, ty, mb_height);

        if (ALPHACHANNEL_FLAG(image))
            _jxr_rflush_mb_strip(image->alpha, tx, ty, mb_height+1);
        _jxr_rflush_mb_strip(image, tx, ty, mb_height+1);

        if (ALPHACHANNEL_FLAG(image))
            _jxr_rflush_mb_strip(image->alpha, tx, ty, mb_height+2);
        _jxr_rflush_mb_strip(image, tx, ty, mb_height+2);

        if (ALPHACHANNEL_FLAG(image))
            _jxr_rflush_mb_strip(image->alpha, tx, ty, mb_height+3);
        _jxr_rflush_mb_strip(image, tx, ty, mb_height+3);
    }
    _jxr_rbitstream_syncbyte(str);
    DEBUG("END TILE_SPATIAL\n");
    return 0;
}

/*
* $Log: r_tile_spatial.c,v $
* Revision 1.55 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.54 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.53 2008/03/20 22:39:41 steve
* Fix various debug prints of QP data.
*
* Revision 1.52 2008/03/20 18:11:25 steve
* Handle case of NumLPQP==1 and NumHPQPS>1
*
* Revision 1.51 2008/03/18 21:09:12 steve
* Fix distributed color prediction.
*
* Revision 1.50 2008/03/07 19:00:52 steve
* Improved comments.
*
* Revision 1.49 2008/03/06 22:47:39 steve
* Clean up parsing/encoding of QP counts
*
* Revision 1.48 2008/03/06 02:05:48 steve
* Distributed quantization
*
* Revision 1.47 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.46 2007/11/26 01:47:15 steve
* Add copyright notices per MS request.
*
* Revision 1.45 2007/11/21 23:26:14 steve
* make all strip buffers store MB data.
*
* Revision 1.44 2007/11/20 17:08:02 steve
* Fix SPATIAL processing of QUANT values for color.
*
* Revision 1.43 2007/11/16 21:33:48 steve
* Store MB Quant, not qp_index.
*
* Revision 1.42 2007/11/16 20:03:57 steve
* Store MB Quant, not qp_index.
*
* Revision 1.41 2007/11/16 00:29:06 steve
* Support FREQUENCY mode HP and FLEXBITS
*
* Revision 1.40 2007/11/14 23:56:17 steve
* Fix TILE ordering, using seeks, for FREQUENCY mode.
*
* Revision 1.39 2007/11/14 00:17:27 steve
* Fix parsing of QP indices.
*
* Revision 1.38 2007/11/13 03:27:24 steve
* Add Frequency mode LP support.
*
* Revision 1.37 2007/11/12 23:21:55 steve
* Infrastructure for frequency mode ordering.
*
* Revision 1.36 2007/11/08 19:38:38 steve
* Get stub DCONLY compression to work.
*
* Revision 1.35 2007/11/01 21:09:40 steve
* Multiple rows of tiles.
*
* Revision 1.34 2007/10/31 21:20:54 steve
* Init, not Adapt, on tile boundaries.
*
* Revision 1.33 2007/10/30 21:32:46 steve
* Support for multiple tile columns.
*
* Revision 1.32 2007/10/19 16:20:21 steve
* Parse YUV420 HP
*
* Revision 1.31 2007/10/04 23:03:26 steve
* HP blocks uf YUV42X chroma are not shuffled.
*
* Revision 1.30 2007/10/04 00:30:47 steve
* Fix prediction of HP CBP for YUV422 data.
*
* Revision 1.29 2007/10/02 20:36:29 steve
* Fix YUV42X DC prediction, add YUV42X HP parsing.
*
* Revision 1.28 2007/10/01 20:39:34 steve
* Add support for YUV422 LP bands.
*
* Revision 1.27 2007/09/18 17:00:50 steve
* Fix bad calculation of lap_mean for chroma.
*
* Revision 1.26 2007/09/13 23:12:34 steve
* Support color HP bands.
*
* Revision 1.25 2007/09/12 01:09:24 steve
* Dump the TRIM_FLEXBITS value.
*
* Revision 1.24 2007/09/11 01:06:12 steve
* Forgot to properly save LP data.
*
* Revision 1.23 2007/09/11 00:40:06 steve
* Fix rendering of chroma to add the missing *2.
* Fix handling of the chroma LP samples
* Parse some of the HP CBP data in chroma.
*
* Revision 1.22 2007/09/10 23:42:00 steve
* Fix LP processing steps when color involved.
*
* Revision 1.21 2007/09/08 01:01:44 steve
* YUV444 color parses properly.
*
* Revision 1.20 2007/09/04 22:48:09 steve
* Fix calculation of flex bits on 0 coefficients.
*
* Revision 1.19 2007/09/04 19:10:46 steve
* Finish level1 overlap filtering.
*
* Revision 1.18 2007/08/31 23:31:49 steve
* Initialize CBP VLC tables at the right time.
*
* Revision 1.17 2007/08/31 23:20:57 steve
* Dump MB_CBP details.
*
* Revision 1.16 2007/08/15 01:54:11 steve
* Add level2 filter to decoder.
*
* Revision 1.15 2007/08/13 22:24:43 steve
* Fix Reset Context of absLevelInd.
*
* Revision 1.14 2007/07/31 15:27:19 steve
* Get transpose of FLEXBITS right.
*
* Revision 1.13 2007/07/30 23:09:57 steve
* Interleave FLEXBITS within HP block.
*
* Revision 1.12 2007/07/24 20:56:28 steve
* Fix HP prediction and model bits calculations.
*
* Revision 1.11 2007/07/21 00:25:48 steve
* snapshot 2007 07 20
*
* Revision 1.10 2007/07/12 22:48:17 steve
* Decode FLEXBITS
*
* Revision 1.9 2007/07/11 00:53:36 steve
* HP adaptation and precition corrections.
*
* Revision 1.8 2007/07/06 23:18:41 steve
* calculate and propagate HP band predictions.
*
* Revision 1.7 2007/07/05 20:19:13 steve
* Fix accumulation of HP CBP, and add HP predictions.
*
* Revision 1.6 2007/07/03 20:45:11 steve
* Parse and place HP data.
*
* Revision 1.5 2007/06/28 20:03:11 steve
* LP processing seems to be OK now.
*
* Revision 1.4 2007/06/21 17:31:22 steve
* Successfully parse LP components.
*
* Revision 1.3 2007/06/11 20:00:09 steve
* Parse FLEXBITS
*
* Revision 1.2 2007/06/07 18:53:06 steve
* Parse HP coeffs that are all 0.
*
* Revision 1.1 2007/06/06 17:19:12 steve
* Introduce to CVS.
*
*/

