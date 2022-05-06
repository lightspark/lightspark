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
#pragma comment (user,"$Id: r_tile_frequency.c,v 1.14 2008/03/18 21:34:04 steve Exp $")
#else
#ident "$Id: r_tile_frequency.c,v 1.14 2008/03/18 21:34:04 steve Exp $"
#endif

# include "jxr_priv.h"
# include <assert.h>

static void backup_dc_strip(jxr_image_t image, int tx, int ty, int my);
static void backup_dclp_strip(jxr_image_t image, int tx, int ty, int my);
static void backup_hp_strip(jxr_image_t image, int tx, int ty, int my);
static void recover_dc_strip(jxr_image_t image, int tx, int ty, int my);
static void recover_dclp_strip(jxr_image_t image, int tx, int ty, int my);
static void recover_dclphp_strip(jxr_image_t image, int tx, int ty, int my);

int _jxr_r_TILE_DC(jxr_image_t image, struct rbitstream*str,
                   unsigned tx, unsigned ty)
{
    DEBUG("START TILE_DC at tile=[%u %u] bitpos=%zu\n", tx, ty, _jxr_rbitstream_bitpos(str));

    /* TILE_STARTCODE == 1 */
    unsigned char s0, s1, s2, s3;
    s0 = _jxr_rbitstream_uint8(str); /* 0x00 */
    s1 = _jxr_rbitstream_uint8(str); /* 0x00 */
    s2 = _jxr_rbitstream_uint8(str); /* 0x01 */
    s3 = _jxr_rbitstream_uint8(str); /* reserved */
    DEBUG(" TILE_STARTCODE == %02x %02x %02x (reserved: %02x)\n", s0, s1, s2, s3);

    _jxr_r_TILE_HEADER_DC(image, str, 0 /* alpha */, tx, ty);
    if (ALPHACHANNEL_FLAG(image))
        _jxr_r_TILE_HEADER_DC(image->alpha, str, 1, tx, ty);


    /* Now form and write out all the compressed data for the
    tile. This involves scanning the macroblocks, and the
    blocks within the macroblocks, generating bits as we go. */

    unsigned mb_height = EXTENDED_HEIGHT_BLOCKS(image);
    unsigned mb_width = EXTENDED_WIDTH_BLOCKS(image);

    if (TILING_FLAG(image)) {
        mb_height = image->tile_row_height[ty];
        mb_width = image->tile_column_width[tx];
    }

    unsigned mx, my;
    for (my = 0 ; my < mb_height ; my += 1) {
        _jxr_r_rotate_mb_strip(image);
        image->cur_my = my;
        for (mx = 0 ; mx < mb_width ; mx += 1) {
            _jxr_r_MB_DC(image, str, 0, tx, ty, mx, my);
            if (image->bands_present == 3 /* DCONLY */)
                _jxr_complete_cur_dclp(image, tx, mx, my);
            if (ALPHACHANNEL_FLAG(image)) {
                _jxr_r_MB_DC(image->alpha, str, 1, tx, ty, mx, my);
                if (image->alpha->bands_present == 3 /* DCONLY */)
                    _jxr_complete_cur_dclp(image->alpha, tx, mx, my);
            }
        }
        
        if (ALPHACHANNEL_FLAG(image))
            backup_dc_strip(image->alpha, tx, ty, my);

        backup_dc_strip(image, tx, ty, my);
    }

    _jxr_rbitstream_syncbyte(str);
    DEBUG("END TILE_DC\n");

    return 0;
}

int _jxr_r_TILE_LP(jxr_image_t image, struct rbitstream*str,
                   unsigned tx, unsigned ty)
{
    DEBUG("START TILE_LOWPASS at tile=[%u %u] bitpos=%zu\n", tx, ty, _jxr_rbitstream_bitpos(str));

    /* TILE_STARTCODE == 1 */
    unsigned char s0, s1, s2, s3;
    s0 = _jxr_rbitstream_uint8(str); /* 0x00 */
    s1 = _jxr_rbitstream_uint8(str); /* 0x00 */
    s2 = _jxr_rbitstream_uint8(str); /* 0x01 */
    s3 = _jxr_rbitstream_uint8(str); /* reserved */
    DEBUG(" TILE_STARTCODE == %02x %02x %02x (reserved: %02x)\n", s0, s1, s2, s3);
    if (s0 != 0x00 || s1 != 0x00 || s2 != 0x01) {
        DEBUG(" TILE_LOWPASS ERROR: Invalid marker.\n");
        return JXR_EC_ERROR;
    }

    _jxr_r_TILE_HEADER_LOWPASS(image, str, 0 /* alpha */, tx, ty);
    if (ALPHACHANNEL_FLAG(image))
        _jxr_r_TILE_HEADER_LOWPASS(image->alpha, str, 1, tx, ty);

    /* Now form and write out all the compressed data for the
    tile. This involves scanning the macroblocks, and the
    blocks within the macroblocks, generating bits as we go. */

    unsigned mb_height = EXTENDED_HEIGHT_BLOCKS(image);
    unsigned mb_width = EXTENDED_WIDTH_BLOCKS(image);

    if (TILING_FLAG(image)) {
        mb_height = image->tile_row_height[ty];
        mb_width = image->tile_column_width[tx];
    }

    unsigned mx, my;
    unsigned plane_idx, num_planes = ((ALPHACHANNEL_FLAG(image)) ? 2 : 1);
    for (my = 0 ; my < mb_height ; my += 1) {
        _jxr_r_rotate_mb_strip(image);
        if (ALPHACHANNEL_FLAG(image)) {
            image->alpha->cur_my = my;
            recover_dc_strip(image->alpha, tx, ty, my);
        }
        image->cur_my = my;
        recover_dc_strip(image, tx, ty, my);

        for (mx = 0 ; mx < mb_width ; mx += 1)
        for (plane_idx = 0; plane_idx < num_planes; plane_idx ++) {
            /* The qp_index_lp table goes only into channel 0 */
            int qp_index_lp = 0;
            jxr_image_t plane = (plane_idx == 0 ? image : image->alpha);

            if (!plane->lp_use_dc_qp && plane->num_lp_qps>1) {
                qp_index_lp = _jxr_DECODE_QP_INDEX(str, plane->num_lp_qps);
                DEBUG(" DECODE_QP_INDEX(%d) --> %u\n", plane->num_lp_qps, qp_index_lp);
            }
            int ch;
            for (ch = 0 ; ch < plane->num_channels ; ch += 1) {
                MACROBLK_CUR_LP_QUANT(plane,ch,tx,mx) = qp_index_lp;
                DEBUG(" LP_QUANT for MBx=%d ch=%d is %d\n", mx, ch, MACROBLK_CUR_LP_QUANT(plane,ch,tx,mx));
            }
            _jxr_r_MB_LP(plane, str, 0, tx, ty, mx, my);
            if (plane->bands_present != 3 /* !DCONLY */)
                _jxr_complete_cur_dclp(plane, tx, mx, my);

        }
        if (ALPHACHANNEL_FLAG(image))
            backup_dclp_strip(image->alpha, tx, ty, my);
        backup_dclp_strip(image, tx, ty, my);
    }

    _jxr_rbitstream_syncbyte(str);
    DEBUG("END TILE_LOWPASS\n");
    return 0;
}

int _jxr_r_TILE_HP(jxr_image_t image, struct rbitstream*str,
                   unsigned tx, unsigned ty)
{
    DEBUG("START TILE_HIGHPASS at tile=[%u %u] bitpos=%zu\n", tx, ty, _jxr_rbitstream_bitpos(str));

    /* TILE_STARTCODE == 1 */
    unsigned char s0, s1, s2, s3;
    s0 = _jxr_rbitstream_uint8(str); /* 0x00 */
    s1 = _jxr_rbitstream_uint8(str); /* 0x00 */
    s2 = _jxr_rbitstream_uint8(str); /* 0x01 */
    s3 = _jxr_rbitstream_uint8(str); /* reserved */
    DEBUG(" TILE_STARTCODE == %02x %02x %02x (reserved: %02x)\n", s0, s1, s2, s3);
    if (s0 != 0x00 || s1 != 0x00 || s2 != 0x01) {
        DEBUG(" TILE_HIGHPASS ERROR: Invalid marker.\n");
        return JXR_EC_ERROR;
    }

    _jxr_r_TILE_HEADER_HIGHPASS(image, str, 0 /* alpha */, tx, ty);
    if (ALPHACHANNEL_FLAG(image))
        _jxr_r_TILE_HEADER_HIGHPASS(image->alpha, str, 1, tx, ty);

    /* Now form and write out all the compressed data for the
    tile. This involves scanning the macroblocks, and the
    blocks within the macroblocks, generating bits as we go. */

    unsigned mb_height = EXTENDED_HEIGHT_BLOCKS(image);
    unsigned mb_width = EXTENDED_WIDTH_BLOCKS(image);

    if (TILING_FLAG(image)) {
        mb_height = image->tile_row_height[ty];
        mb_width = image->tile_column_width[tx];
    }

    unsigned mx, my;
    unsigned plane_idx, num_planes = ((ALPHACHANNEL_FLAG(image)) ? 2 : 1);
    for (my = 0 ; my < mb_height ; my += 1) {
        _jxr_r_rotate_mb_strip(image);

        if (ALPHACHANNEL_FLAG(image)) {
            image->alpha->cur_my = my;
            recover_dclp_strip(image->alpha, tx, ty, my);
        }
        image->cur_my = my;
        recover_dclp_strip(image, tx, ty, my);

        for (mx = 0 ; mx < mb_width ; mx += 1) 
        for (plane_idx = 0; plane_idx < num_planes; plane_idx ++) {
            /* The qp_index_hp table goes only into channel 0 */
            int qp_index_hp = 0;
            jxr_image_t plane = (plane_idx == 0 ? image : image->alpha);
            if (plane->num_hp_qps>1) {
                if (!plane->hp_use_lp_qp)
                    qp_index_hp = _jxr_DECODE_QP_INDEX(str, plane->num_hp_qps);
                else
                    qp_index_hp = MACROBLK_CUR_LP_QUANT(plane,0,tx,mx);
            }
            DEBUG(" HP_QP_INDEX for MBx=%d is %d\n", mx, qp_index_hp);
            int ch;
            for (ch = 0 ; ch < plane->num_channels ; ch += 1) {
                MACROBLK_CUR_HP_QUANT(plane,ch,tx,mx) = plane->hp_quant_ch[ch][qp_index_hp];
                DEBUG(" HP_QUANT for MBx=%d ch=%d is %d\n", mx, ch, MACROBLK_CUR_HP_QUANT(plane,ch,tx,mx));
            }

            int rc = _jxr_r_MB_CBP(plane, str, 0, tx, ty, mx, my);
            if (rc < 0) {
                DEBUG("r_MB_CBP returned ERROR rc=%d\n", rc);
                return rc;
            }
            rc = _jxr_r_MB_HP(plane, str, 0, tx, ty, mx, my);
            if (rc < 0) {
                DEBUG("r_MB_HP returned ERROR rc=%d\n", rc);
                return rc;
            }
        }
        if (ALPHACHANNEL_FLAG(image))
            backup_hp_strip(image->alpha, tx, ty, my);
        backup_hp_strip(image, tx, ty, my);
    }

    _jxr_rbitstream_syncbyte(str);
    DEBUG("END TILE_HIGHPASS\n");
    return 0;
}

int _jxr_r_TILE_FLEXBITS(jxr_image_t image, struct rbitstream*str,
                         unsigned tx, unsigned ty)
{
    DEBUG("START TILE_FLEXBITS at tile=[%u %u] bitpos=%zu\n", tx, ty, _jxr_rbitstream_bitpos(str));

    /* TILE_STARTCODE == 1 */
    unsigned char s0, s1, s2, s3;
    s0 = _jxr_rbitstream_uint8(str); /* 0x00 */
    s1 = _jxr_rbitstream_uint8(str); /* 0x00 */
    s2 = _jxr_rbitstream_uint8(str); /* 0x01 */
    s3 = _jxr_rbitstream_uint8(str); /* reserved */
    DEBUG(" TILE_STARTCODE == %02x %02x %02x (reserved: %02x)\n", s0, s1, s2, s3);
    if (s0 != 0x00 || s1 != 0x00 || s2 != 0x01) {
        DEBUG(" TILE_FLEXBITS ERROR: Invalid marker.\n");
        return JXR_EC_ERROR;
    }

    image->trim_flexbits = 0;
    if (TRIM_FLEXBITS_FLAG(image)) {
        image->trim_flexbits =_jxr_rbitstream_uint4(str);
        DEBUG(" TRIM_FLEXBITS = %u\n", image->trim_flexbits);
    }

    int use_num_channels = image->num_channels;
    if (image->use_clr_fmt == 1/*YUV420*/ || image->use_clr_fmt == 2/*YUV422*/)
        use_num_channels = 1;

    /* Now form and write out all the compressed data for the
    tile. This involves scanning the macroblocks, and the
    blocks within the macroblocks, generating bits as we go. */

    unsigned mb_height = EXTENDED_HEIGHT_BLOCKS(image);
    unsigned mb_width = EXTENDED_WIDTH_BLOCKS(image);

    if (TILING_FLAG(image)) {
        mb_height = image->tile_row_height[ty];
        mb_width = image->tile_column_width[tx];
    }

    int mx, my;
    int plane_idx, num_planes = ((ALPHACHANNEL_FLAG(image)) ? 2 : 1);
    for (my = 0 ; my < (int) mb_height ; my += 1) {
        _jxr_r_rotate_mb_strip(image);
        if (ALPHACHANNEL_FLAG(image)) {
            image->alpha->cur_my = my;
            recover_dclphp_strip(image->alpha, tx, ty, my);
        }
        image->cur_my = my;
        recover_dclphp_strip(image, tx, ty, my);

        for (mx = 0 ; mx < (int) mb_width ; mx += 1) 
        for (plane_idx = 0; plane_idx < num_planes; plane_idx ++) {
            jxr_image_t plane = (plane_idx == 0 ? image : image->alpha);
            int channels = (plane_idx == 0 ? use_num_channels : 1);
            int rc = _jxr_r_MB_FLEXBITS(plane, str, 0, tx, ty, mx, my);
            if (rc < 0) {
                DEBUG("r_MB_FLEXBITS returned ERROR rc=%d\n", rc);
                return rc;
            }

            /* Now the HP values are complete, so run the propagation
            process. This involves recovering some bits of data saved
            by the HP tile. */
            int mbhp_pred_mode = MACROBLK_CUR(plane,0,tx,mx).mbhp_pred_mode;
            int idx;
            for (idx = 0 ; idx < channels ; idx += 1) {
                DEBUG(" MB_FLEXBITS: propagate HP predictions in MB_FLEXBITS\n");
                _jxr_propagate_hp_predictions(plane, idx, tx, mx, mbhp_pred_mode);
            }
        }
        if (ALPHACHANNEL_FLAG(image))
            backup_hp_strip(image->alpha, tx, ty, my);
        backup_hp_strip(image, tx, ty, my);
    }

    _jxr_rbitstream_syncbyte(str);
    DEBUG("END TILE_FLEXBITS bitpos=%zu\n", _jxr_rbitstream_bitpos(str));
    return 0;
}

/*
* This function handles the special case that the FLEXBITS tile is
* escaped away. Do all the soft processing that is otherwise needed.
*/
int _jxr_r_TILE_FLEXBITS_ESCAPE(jxr_image_t image, unsigned tx, unsigned ty)
{
    DEBUG("START TILE_FLEXBITS_ESCAPE at tile=[%u %u]\n", tx, ty);

    int use_num_channels = image->num_channels;
    if (image->use_clr_fmt == 1/*YUV420*/ || image->use_clr_fmt == 2/*YUV422*/)
        use_num_channels = 1;

    unsigned mb_height = EXTENDED_HEIGHT_BLOCKS(image);
    unsigned mb_width = EXTENDED_WIDTH_BLOCKS(image);

    if (TILING_FLAG(image)) {
        mb_height = image->tile_row_height[ty];
        mb_width = image->tile_column_width[tx];
    }

    int mx, my;
    for (my = 0 ; my < (int) mb_height ; my += 1) {
        _jxr_r_rotate_mb_strip(image);
        image->cur_my = my;
        recover_dclphp_strip(image, tx, ty, my);

        for (mx = 0 ; mx < (int) mb_width ; mx += 1) {
            /* */
            int mbhp_pred_mode = MACROBLK_CUR(image,0,tx,mx).mbhp_pred_mode;
            int idx;
            for (idx = 0 ; idx < use_num_channels ; idx += 1) {
                DEBUG(" MB_FLEXBITS_ESCAPE: propagate HP predictions in MB_FLEXBITS\n");
                _jxr_propagate_hp_predictions(image, idx, tx, mx, mbhp_pred_mode);
            }
        }
        backup_hp_strip(image, tx, ty, my);
    }

    DEBUG("END TILE_FLEXBIT_ESCAPE\n");
    return 0;
}

static void backup_dc_strip(jxr_image_t image, int tx, int ty, int my)
{
    int mx;
    int use_my = my + image->tile_row_position[ty];
    int use_mx = image->tile_column_position[tx];
    int ptr = use_my*EXTENDED_WIDTH_BLOCKS(image) + use_mx;

    int ch;
    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        struct macroblock_s*mb = image->mb_row_buffer[ch] + ptr;

        for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
            mb[mx].data[0] = MACROBLK_CUR_DC(image,ch,tx,mx);
            DEBUG(" backup_dc_strip: tx=%d, ty=%d, mx=%d, my=%d, ch=%d, DC=0x%0x8\n",
                tx, ty, mx, my, ch, mb[mx].data[0]);
        }
    }
}

static void backup_dclp_strip(jxr_image_t image, int tx, int ty, int my)
{
    int mx;
    int use_my = my + image->tile_row_position[ty];
    int use_mx = image->tile_column_position[tx];
    int ptr = use_my*EXTENDED_WIDTH_BLOCKS(image) + use_mx;


    int format_scale = 15;
    if (image->use_clr_fmt == 2 /* YUV422 */) {
        format_scale = 7;
    } else if (image->use_clr_fmt == 1 /* YUV420 */) {
        format_scale = 3;
    }

    int ch;
    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        struct macroblock_s*mb = image->mb_row_buffer[ch] + ptr;
        int count = ch==0? 15 : format_scale;

        for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
            mb[mx].data[0] = MACROBLK_CUR_DC(image,ch,tx,mx);
            DEBUG(" backup_dclp_strip: tx=%d, ty=%d, mx=%d, my=%d, ch=%d, DC=0x%x, LP=",
                tx, ty, mx, my, ch, mb[mx].data[0]);
            int idx;
            for (idx = 0 ; idx < count ; idx += 1) {
                mb[mx].data[idx+1] = MACROBLK_CUR_LP(image,ch,tx,mx,idx);
                DEBUG(" 0x%x", mb[mx].data[idx+1]);
            }
            DEBUG("\n");
            mb[mx].lp_quant = MACROBLK_CUR_LP_QUANT(image,ch,tx,mx);
        }
    }
}

static void backup_hp_strip(jxr_image_t image, int tx, int ty, int my)
{
    int mx;
    int use_my = my + image->tile_row_position[ty];
    int use_mx = image->tile_column_position[tx];
    int ptr = use_my*EXTENDED_WIDTH_BLOCKS(image) + use_mx;


    int format_scale = 16;
    if (image->use_clr_fmt == 2 /* YUV422 */) {
        format_scale = 8;
    } else if (image->use_clr_fmt == 1 /* YUV420 */) {
        format_scale = 4;
    }

    int ch;
    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        struct macroblock_s*mb = image->mb_row_buffer[ch] + ptr;
        int count = ch==0? 16 : format_scale;

        if (ch == 0) {
            /* Backup also the hp_model_bits, which are
            stored only in the channel-0 blocks. */
            for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
                mb[mx].hp_model_bits[0] = MACROBLK_CUR(image,0,tx,mx).hp_model_bits[0];
                mb[mx].hp_model_bits[1] = MACROBLK_CUR(image,0,tx,mx).hp_model_bits[1];
                mb[mx].mbhp_pred_mode = MACROBLK_CUR(image,0,tx,mx).mbhp_pred_mode;
            }
        }
        for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
            mb[mx].data[0] = MACROBLK_CUR_DC(image,ch,tx,mx);
            DEBUG(" backup_hp_strip: tx=%d, ty=%d, mx=%d, my=%d, ch=%d\n",
                tx, ty, mx, my, ch);
            int blk;
            for (blk = 0 ; blk < count ; blk += 1) {
                int idx;
                for (idx = 0 ; idx < 15 ; idx += 1)
                    mb[mx].data[count+15*blk+idx] = MACROBLK_CUR_HP(image,ch,tx,mx,blk,idx);
            }
            mb[mx].hp_quant = MACROBLK_CUR_HP_QUANT(image,ch,tx,mx);
        }
    }
}

static void recover_dc_strip(jxr_image_t image, int tx, int ty, int my)
{
    int mx;
    int use_my = my + image->tile_row_position[ty];
    int use_mx = image->tile_column_position[tx];
    int ptr = use_my*EXTENDED_WIDTH_BLOCKS(image) + use_mx;

    int ch;
    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        struct macroblock_s*mb = image->mb_row_buffer[ch] + ptr;

        for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
            MACROBLK_CUR_DC(image,ch,tx,mx) = mb[mx].data[0];
            DEBUG(" recover_dc_strip: tx=%d, ty=%d, mx=%d, my=%d, ch=%d, DC=0x%0x8\n",
                tx, ty, mx, my, ch, mb[mx].data[0]);
        }
    }
}

static void recover_dclp_strip(jxr_image_t image, int tx, int ty, int my)
{
    int mx;
    int use_my = my + image->tile_row_position[ty];
    int use_mx = image->tile_column_position[tx];
    int ptr = use_my*EXTENDED_WIDTH_BLOCKS(image) + use_mx;

    int format_scale = 15;
    if (image->use_clr_fmt == 2 /* YUV422 */) {
        format_scale = 7;
    } else if (image->use_clr_fmt == 1 /* YUV420 */) {
        format_scale = 3;
    }

    int ch;
    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        struct macroblock_s*mb = image->mb_row_buffer[ch] + ptr;
        int count = ch==0? 15 : format_scale;

        for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
            MACROBLK_CUR_DC(image,ch,tx,mx) = mb[mx].data[0];
            DEBUG(" recover_dclp_strip: tx=%d, ty=%d, mx=%d, my=%d, ch=%d, DC=0x%0x8, LP=\n",
                tx, ty, mx, my, ch, mb[mx].data[0]);
            int idx;
            for (idx = 0 ; idx < count ; idx += 1) {
                MACROBLK_CUR_LP(image,ch,tx,mx,idx) = mb[mx].data[idx+1];
                DEBUG(" 0x%x", mb[mx].data[idx+1]);
            }
            DEBUG("\n");
            MACROBLK_CUR_LP_QUANT(image,ch,tx,mx) = mb[mx].lp_quant;
        }
    }
}

static void recover_dclphp_strip(jxr_image_t image, int tx, int ty, int my)
{
    int mx;
    int use_my = my + image->tile_row_position[ty];
    int use_mx = image->tile_column_position[tx];
    int ptr = use_my*EXTENDED_WIDTH_BLOCKS(image) + use_mx;

    int format_scale = 16;
    if (image->use_clr_fmt == 2 /* YUV422 */) {
        format_scale = 8;
    } else if (image->use_clr_fmt == 1 /* YUV420 */) {
        format_scale = 4;
    }

    int ch;
    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        struct macroblock_s*mb = image->mb_row_buffer[ch] + ptr;
        int count = ch==0? 16 : format_scale;

        if (ch == 0) {
            /* Recover also the hp_model_bits, which are
            stored only in the channel-0 blocks. */
            for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
                MACROBLK_CUR(image,0,tx,mx).hp_model_bits[0] = mb[mx].hp_model_bits[0];
                MACROBLK_CUR(image,0,tx,mx).hp_model_bits[1] = mb[mx].hp_model_bits[1];
                MACROBLK_CUR(image,0,tx,mx).mbhp_pred_mode = mb[mx].mbhp_pred_mode;
            }
        }
        for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
            MACROBLK_CUR_DC(image,ch,tx,mx) = mb[mx].data[0];
            DEBUG(" recover_dclphp_strip: tx=%d, ty=%d, mx=%d, my=%d, ch=%d, DC=0x%0x8, LP=\n",
                tx, ty, mx, my, ch, mb[mx].data[0]);
            int blk;
            for (blk = 1 ; blk < count ; blk += 1) {
                MACROBLK_CUR_LP(image,ch,tx,mx,blk-1) = mb[mx].data[blk];
                DEBUG(" 0x%x", mb[mx].data[blk]);
            }

            for (blk = 0 ; blk < count ; blk += 1) {
                int idx;
                for (idx = 0 ; idx < 15 ; idx += 1) {
                    int data_ptr = count+15*blk+idx;
                    MACROBLK_CUR_HP(image,ch,tx,mx,blk,idx) = mb[mx].data[data_ptr];
                }
            }
            DEBUG("\n");
            MACROBLK_CUR_LP_QUANT(image,ch,tx,mx) = mb[mx].lp_quant;
            MACROBLK_CUR_HP_QUANT(image,ch,tx,mx) = mb[mx].hp_quant;
        }
    }
}

void _jxr_frequency_mode_render(jxr_image_t image)
{

    int ty;
    for (ty = 0 ; ty < (int) image->tile_rows ; ty += 1) {
        int my;
        for (my = 0 ; my < (int) image->tile_row_height[ty] ; my += 1) {
            if (ALPHACHANNEL_FLAG(image))
                _jxr_rflush_mb_strip(image->alpha, -1, -1, my + image->alpha->tile_row_position[ty]);
            _jxr_rflush_mb_strip(image, -1, -1, my + image->tile_row_position[ty]);
            int tx;
            for (tx = 0 ; tx < (int) image->tile_columns ; tx += 1) {
                if (ALPHACHANNEL_FLAG(image))
                    recover_dclphp_strip(image->alpha, tx, ty, my);
                recover_dclphp_strip(image, tx, ty, my);
            }
        }
    }

    if (ALPHACHANNEL_FLAG(image))
        _jxr_rflush_mb_strip(image->alpha, -1, -1, EXTENDED_HEIGHT_BLOCKS(image->alpha)+0);
    _jxr_rflush_mb_strip(image, -1, -1, EXTENDED_HEIGHT_BLOCKS(image)+0);
    
    if (ALPHACHANNEL_FLAG(image))
        _jxr_rflush_mb_strip(image->alpha, -1, -1, EXTENDED_HEIGHT_BLOCKS(image->alpha)+1);
    _jxr_rflush_mb_strip(image, -1, -1, EXTENDED_HEIGHT_BLOCKS(image)+1);
    
    if (ALPHACHANNEL_FLAG(image))
        _jxr_rflush_mb_strip(image->alpha, -1, -1, EXTENDED_HEIGHT_BLOCKS(image->alpha)+2);
    _jxr_rflush_mb_strip(image, -1, -1, EXTENDED_HEIGHT_BLOCKS(image)+2);

    if (ALPHACHANNEL_FLAG(image))
        _jxr_rflush_mb_strip(image->alpha, -1, -1, EXTENDED_HEIGHT_BLOCKS(image->alpha)+3);
    _jxr_rflush_mb_strip(image, -1, -1, EXTENDED_HEIGHT_BLOCKS(image)+3);
}

/*
* $Log: r_tile_frequency.c,v $
* Revision 1.16 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.15 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.14 2008/03/18 21:34:04 steve
* Fix distributed color prediction.
*
* Revision 1.13 2008/03/05 06:58:10 gus
* *** empty log message ***
*
* Revision 1.12 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.11 2007/11/26 01:47:15 steve
* Add copyright notices per MS request.
*
* Revision 1.10 2007/11/21 00:34:30 steve
* Rework spatial mode tile macroblock shuffling.
*
* Revision 1.9 2007/11/20 00:05:47 steve
* Complex handling of mbhp_pred_mode in frequency dmoe.
*
* Revision 1.8 2007/11/16 21:33:48 steve
* Store MB Quant, not qp_index.
*
* Revision 1.7 2007/11/16 20:03:57 steve
* Store MB Quant, not qp_index.
*
* Revision 1.6 2007/11/16 17:33:24 steve
* Do HP prediction after FLEXBITS frequency tiles.
*
* Revision 1.5 2007/11/16 00:29:06 steve
* Support FREQUENCY mode HP and FLEXBITS
*
* Revision 1.4 2007/11/15 17:44:13 steve
* Frequency mode color support.
*
* Revision 1.3 2007/11/14 23:56:17 steve
* Fix TILE ordering, using seeks, for FREQUENCY mode.
*
* Revision 1.2 2007/11/13 03:27:23 steve
* Add Frequency mode LP support.
*
* Revision 1.1 2007/11/12 23:21:55 steve
* Infrastructure for frequency mode ordering.
*
*/

