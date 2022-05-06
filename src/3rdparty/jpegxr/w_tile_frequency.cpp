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
#pragma comment (user,"$Id: w_tile_frequency.c,v 1.1 2009/03/09 12:00:00 dan Exp $")
#else
#ident "$Id: w_tile_frequency.c,v 1.1 2009/03/09 21:00:00 dan Exp $"
#endif

# include "jxr_priv.h"
# include <assert.h>

void _jxr_w_TILE_DC(jxr_image_t image, struct wbitstream*str,
                          unsigned tx, unsigned ty)
{
    DEBUG("START TILE_DC at tile=[%u %u] bitpos=%zu\n", tx, ty, _jxr_wbitstream_bitpos(str));

#ifndef JPEGXR_ADOBE_EXT
    uint8_t bands_present = image->bands_present_of_primary;
#endif //#ifndef JPEGXR_ADOBE_EXT

    /* TILE_STARTCODE == 1 */
    DEBUG(" DC_TILE_STARTCODE at bitpos=%zu\n", _jxr_wbitstream_bitpos(str));
    _jxr_wbitstream_uint8(str, 0x00);
    _jxr_wbitstream_uint8(str, 0x00);
    _jxr_wbitstream_uint8(str, 0x01);
    _jxr_wbitstream_uint8(str, 0x00); /* ARBITRARY_BYTE */

    /* Write out the tile header (which includes sub-headers for
    all the major passes). */

    _jxr_w_TILE_HEADER_DC(image, str, 0, tx, ty);
    if (ALPHACHANNEL_FLAG(image)) {
        _jxr_w_TILE_HEADER_DC(image->alpha, str, 1, tx, ty);
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

    DEBUG(" TILE_DC at [%d %d] is %u x %u MBs\n", tx, ty, mb_width, mb_height);
    unsigned mx, my;
    for (my = 0 ; my < mb_height ; my += 1) {

        _jxr_wflush_mb_strip(image, tx, ty, my, 1);

        for (mx = 0 ; mx < mb_width ; mx += 1) {

            _jxr_w_MB_DC(image, str, 0 /* IsCurrPlaneAlphaFlag = FALSE */, tx, ty, mx, my);
            if (ALPHACHANNEL_FLAG(image)) {
                _jxr_w_MB_DC(image->alpha, str, 1 /* IsCurrPlaneAlphaFlag = TRUE */, tx, ty, mx, my);
            }
        }
    }

#ifndef JPEGXR_ADOBE_EXT
    unsigned tile_idx = ty * image->tile_columns + tx;
#endif //#ifndef JPEGXR_ADOBE_EXT

    _jxr_wbitstream_syncbyte(str);
    _jxr_wbitstream_flush(str);
    DEBUG("END TILE_DC\n");
}

void _jxr_w_TILE_LP(jxr_image_t image, struct wbitstream*str,
                         unsigned tx, unsigned ty)
{
    DEBUG("START TILE_LP at tile=[%u %u] bitpos=%zu\n", tx, ty, _jxr_wbitstream_bitpos(str));

    uint8_t bands_present = image->bands_present_of_primary;

    if (bands_present < 3 /* LOWPASS */) {
        /* TILE_STARTCODE == 1 */
        DEBUG(" LP_TILE_STARTCODE at bitpos=%zu\n", _jxr_wbitstream_bitpos(str));
        _jxr_wbitstream_uint8(str, 0x00);
        _jxr_wbitstream_uint8(str, 0x00);
        _jxr_wbitstream_uint8(str, 0x01);
        _jxr_wbitstream_uint8(str, 0x00); /* ARBITRARY_BYTE */

        _jxr_w_TILE_HEADER_LOWPASS(image, str, 0, tx, ty);
        if (ALPHACHANNEL_FLAG(image)) {
            _jxr_w_TILE_HEADER_LOWPASS(image->alpha, str, 1, tx, ty);
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

    DEBUG(" TILE_LP at [%d %d] is %u x %u MBs\n", tx, ty, mb_width, mb_height);
    unsigned mx, my;
    for (my = 0 ; my < mb_height ; my += 1) {

        _jxr_wflush_mb_strip(image, tx, ty, my, 0);

        for (mx = 0 ; mx < mb_width ; mx += 1) {

            if (bands_present<3 /* LOWPASS */) {
                if (image->num_lp_qps>1 && !image->lp_use_dc_qp) {
                    unsigned qp_index = _jxr_select_lp_index(image, tx,ty,mx,my);
                    DEBUG(" DECODE_QP_INDEX(%d) --> %u\n", image->num_lp_qps, qp_index);
                    _jxr_w_ENCODE_QP_INDEX(image, str, tx, ty, mx, my, image->num_lp_qps, qp_index);
                }

                _jxr_w_MB_LP(image, str, 0 /* IsCurrPlaneAlphaFlag = FALSE */, tx, ty, mx, my);
                if (ALPHACHANNEL_FLAG(image)) {
                    _jxr_w_MB_LP(image->alpha, str, 1 /* IsCurrPlaneAlphaFlag = TRUE */, tx, ty, mx, my);
                }
            }
        }
    }

#ifndef JPEGXR_ADOBE_EXT
    unsigned tile_idx = ty * image->tile_columns + tx;
#endif //#ifndef JPEGXR_ADOBE_EXT

    _jxr_wbitstream_syncbyte(str);
    _jxr_wbitstream_flush(str);
    DEBUG("END TILE_LP\n");
}

void _jxr_w_TILE_HP_FLEX(jxr_image_t image, struct wbitstream*str,
                         unsigned tx, unsigned ty)
{
    DEBUG("START TILE_HP_FLEX at tile=[%u %u] bitpos=%zu\n", tx, ty, _jxr_wbitstream_bitpos(str));

    uint8_t bands_present = image->bands_present_of_primary;

    struct wbitstream strFP;
#ifndef JPEGXR_ADOBE_EXT
    FILE*fdFP = fopen("fp.tmp", "wb");
#endif //#ifndef JPEGXR_ADOBE_EXT
    _jxr_wbitstream_initialize(&strFP
#ifndef JPEGXR_ADOBE_EXT
		, fdFP
#endif //#ifndef JPEGXR_ADOBE_EXT
	);

    if (bands_present < 2 /* HIGHPASS */) {
        /* TILE_STARTCODE == 1 */
        DEBUG(" HP_TILE_STARTCODE at bitpos=%zu\n", _jxr_wbitstream_bitpos(str));
        _jxr_wbitstream_uint8(str, 0x00);
        _jxr_wbitstream_uint8(str, 0x00);
        _jxr_wbitstream_uint8(str, 0x01);
        _jxr_wbitstream_uint8(str, 0x00); /* ARBITRARY_BYTE */

        _jxr_w_TILE_HEADER_HIGHPASS(image, str, 0, tx, ty);
        if (ALPHACHANNEL_FLAG(image)) {
                _jxr_w_TILE_HEADER_HIGHPASS(image->alpha, str, 1, tx, ty);
        }
    }

    if (bands_present == 0 /* ALL */) {
        /* TILE_STARTCODE == 1 */
        DEBUG(" FLEX_TILE_STARTCODE at bitpos=%zu\n", _jxr_wbitstream_bitpos(&strFP));
        _jxr_wbitstream_uint8(&strFP, 0x00);
        _jxr_wbitstream_uint8(&strFP, 0x00);
        _jxr_wbitstream_uint8(&strFP, 0x01);
        _jxr_wbitstream_uint8(&strFP, 0x00); /* ARBITRARY_BYTE */

        if (TRIM_FLEXBITS_FLAG(image)) {
            _jxr_wbitstream_uint4(&strFP, image->trim_flexbits);
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

    DEBUG(" TILE_HP_FLEX at [%d %d] is %u x %u MBs\n", tx, ty, mb_width, mb_height);
    unsigned mx, my;
    for (my = 0 ; my < mb_height ; my += 1) {

        _jxr_wflush_mb_strip(image, tx, ty, my, 0);

        for (mx = 0 ; mx < mb_width ; mx += 1) {

            if (bands_present<2 /* HIGHPASS */) {
                if (image->num_hp_qps>1 && !image->hp_use_lp_qp) {
                    unsigned qp_index = _jxr_select_hp_index(image, tx,ty,mx,my);
                    DEBUG(" DECODE_QP_INDEX(%d) --> %u\n", image->num_hp_qps, qp_index);
                    _jxr_w_ENCODE_QP_INDEX(image, str, tx, ty, mx, my, image->num_hp_qps, qp_index);
                }

                _jxr_w_MB_CBP(image, str, 0 /* IsCurrPlaneAlphaFlag = FALSE */, tx, ty, mx, my);
                if (bands_present == 0 /* ALL */) {
                    _jxr_w_MB_HP(image, str, 0 /* IsCurrPlaneAlphaFlag = FALSE */, tx, ty, mx, my, &strFP);
                }
                else {
                    _jxr_w_MB_HP(image, str, 0 /* IsCurrPlaneAlphaFlag = FALSE */, tx, ty, mx, my, 0);
                }

                if (ALPHACHANNEL_FLAG(image)) {
                    _jxr_w_MB_CBP(image->alpha, str, 1 /* IsCurrPlaneAlphaFlag = TRUE */, tx, ty, mx, my);
                    if (bands_present == 0 /* ALL */) {
                        _jxr_w_MB_HP(image->alpha, str, 1 /* IsCurrPlaneAlphaFlag = TRUE */, tx, ty, mx, my, &strFP);
                    }
                    else {
                        _jxr_w_MB_HP(image->alpha, str, 1 /* IsCurrPlaneAlphaFlag = TRUE */, tx, ty, mx, my, 0);
                    }
                }
            }
        }
    }

    unsigned tile_idx = ty * image->tile_columns + tx;

    _jxr_wbitstream_syncbyte(str);
    _jxr_wbitstream_flush(str);
    image->tile_index_table[tile_idx * (4 - bands_present) + 2] = str->write_count;

    _jxr_wbitstream_syncbyte(&strFP);
    _jxr_wbitstream_flush(&strFP);
#ifndef JPEGXR_ADOBE_EXT
    fclose(fdFP);
#endif //#ifndef JPEGXR_ADOBE_EXT
    if (bands_present == 0 /* ALL */) {
        struct rbitstream strFPRead
#ifdef JPEGXR_ADOBE_EXT
			(strFP.buffer(), strFP.len())
#endif //#ifdef JPEGXR_ADOBE_EXT
        ;
#ifndef JPEGXR_ADOBE_EXT
        FILE*fdFPRead = fopen("fp.tmp", "rb");
#endif //#ifndef JPEGXR_ADOBE_EXT
        _jxr_rbitstream_initialize(&strFPRead
#ifndef JPEGXR_ADOBE_EXT
			, fdFPRead
#endif //#ifndef JPEGXR_ADOBE_EXT
		);

        size_t idx;
        for (idx = 0; idx < strFP.write_count; idx++) {
            _jxr_wbitstream_uint8(str, _jxr_rbitstream_uint8(&strFPRead));
        }
#ifndef JPEGXR_ADOBE_EXT
        fclose(fdFPRead);
#endif //#ifndef JPEGXR_ADOBE_EXT
        _jxr_wbitstream_flush(str);
        image->tile_index_table[tile_idx * (4 - bands_present) + 3] = str->write_count;
    }
    /* delete file associated with CodedTiles */
#ifndef JPEGXR_ADOBE_EXT
    remove("fp.tmp");
#endif //#ifndef JPEGXR_ADOBE_EXT

    _jxr_wbitstream_flush(str);
    DEBUG("END TILE_HP_FLEX\n");
}

/*
* $Log: w_tile_frequency.c,v $
* Revision 1.2 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.1 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
*/

