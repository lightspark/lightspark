
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
#pragma comment (user,"$Id: w_strip.c,v 1.47 2008/03/24 18:06:56 steve Exp $")
#else
#ident "$Id: w_strip.c,v 1.47 2008/03/24 18:06:56 steve Exp $"
#endif

# include "jxr_priv.h"
# include <stdlib.h>
# include <limits.h>
# include <assert.h>

# define FILTERED_YUV_SAMPLE 1

static void w_rotate_mb_strip(jxr_image_t image);
static void collect_and_scale_up4(jxr_image_t image, int ty);
static void scale_and_shuffle_up3(jxr_image_t image);
static void first_prefilter_up2(jxr_image_t image, int ty);
static void PCT_stage2_up1(jxr_image_t image, int ch, int ty);
static void second_prefilter_up1(jxr_image_t image, int ty);
static void PCT_stage1_up2(jxr_image_t image, int ch, int ty);
static void w_predict_up1_dclp(jxr_image_t image, int tx, int ty, int mx);
static int w_calculate_mbhp_mode_up1(jxr_image_t image, int tx, int mx);
static void w_predict_lp444(jxr_image_t image, int tx, int mx, int my, int ch, int mblp_mode);
static void w_predict_lp422(jxr_image_t image, int tx, int mx, int my, int ch, int mblp_mode, int mbdc_mode);
static void w_predict_lp420(jxr_image_t image, int tx, int mx, int my, int ch, int mblp_mode);
static void calculate_hpcbp_up1(jxr_image_t image, int tx, int ty, int mx);
static void w_predict_up1_hp(jxr_image_t image, int ch, int tx, int mx, int mbhp_pred_mode);
static void w_PredCBP(jxr_image_t image, unsigned tx, unsigned ty, unsigned mx);
static void wflush_to_tile_buffer(jxr_image_t image, int my);
static void wflush_collect_mb_strip_data(jxr_image_t image, int my);
static void _jxr_w_store_hpcbp_state(jxr_image_t image, int tx);
static void _jxr_w_load_hpcbp_state(jxr_image_t image, int tx);

static unsigned char w_guess_dc_quant(jxr_image_t image, int ch,
                                      unsigned tx, unsigned ty)
{
    if (image->dc_frame_uniform)
        return image->dc_quant_ch[ch];

    struct jxr_tile_qp*cur = GET_TILE_QUANT(image,tx,ty);

    if (ch == 0)
        return cur->channel[0].dc_qp;

    switch (cur->component_mode) {
        case JXR_CM_UNIFORM:
            return cur->channel[0].dc_qp;
        case JXR_CM_SEPARATE:
            return cur->channel[1].dc_qp;
        case JXR_CM_INDEPENDENT:
            return cur->channel[ch].dc_qp;
        default:
            assert(0);
    return 0;
    }
}

unsigned char _jxr_select_lp_index(jxr_image_t image, unsigned tx, unsigned ty,
                                   unsigned mx, unsigned my)
{
    /* If this is frame uniform LP QP, then implicitly the QP
    index is always zero. */
    if (image->lp_frame_uniform)
        return 0;

    /* Otherwise we have the more complicated case that the LP QP
    index may vary per MB. */
    struct jxr_tile_qp*cur = GET_TILE_QUANT(image,tx,ty);
    if (cur->lp_map == 0)
        return 0;

    unsigned mxy = my*jxr_get_TILE_WIDTH(image, tx) + mx;

    unsigned lp_index = cur->lp_map[mxy];
    assert(lp_index < cur->channel[0].num_lp || lp_index==0);

    return lp_index;
}

static unsigned char w_guess_lp_quant(jxr_image_t image, int ch,
                                      unsigned tx, unsigned ty, unsigned mx, unsigned my)
{
    if (image->lp_frame_uniform)
        return image->lp_quant_ch[ch][0];

    unsigned idx = _jxr_select_lp_index(image, tx, ty, mx, my);
    struct jxr_tile_qp*cur = GET_TILE_QUANT(image,tx,ty);

    if (ch == 0)
        return cur->channel[0].lp_qp[idx];

    switch (cur->component_mode) {
        case JXR_CM_UNIFORM:
            return cur->channel[0].lp_qp[idx];
        case JXR_CM_SEPARATE:
            return cur->channel[1].lp_qp[idx];
        case JXR_CM_INDEPENDENT:
            return cur->channel[ch].lp_qp[idx];
        default:
            assert(0);
    return 0;
    }
}

unsigned char _jxr_select_hp_index(jxr_image_t image, unsigned tx, unsigned ty,
                                   unsigned mx, unsigned my)
{
    if (image->hp_frame_uniform)
        return 0;

    struct jxr_tile_qp*cur = GET_TILE_QUANT(image,tx,ty);
    /* If there is no actual map, then assume all indices in the
    tile are zero. */
    if (cur->hp_map == 0)
        return 0;

    unsigned mxy = my*jxr_get_TILE_WIDTH(image, tx) + mx;

    unsigned hp_index = cur->hp_map[mxy];
    assert(hp_index < cur->channel[0].num_hp || hp_index == 0);

    return hp_index;
}

static unsigned char w_guess_hp_quant(jxr_image_t image, int ch,
                                      unsigned tx, unsigned ty,
                                      int mx, int my)
{
    if (image->hp_frame_uniform)
        return image->hp_quant_ch[ch][0];

    unsigned idx = _jxr_select_hp_index(image, tx, ty, mx, my);
    struct jxr_tile_qp*cur = GET_TILE_QUANT(image,tx,ty);

    if (ch == 0)
        return cur->channel[0].hp_qp[idx];

    switch (cur->component_mode) {
        case JXR_CM_UNIFORM:
            return cur->channel[0].hp_qp[idx];
        case JXR_CM_SEPARATE:
            return cur->channel[1].hp_qp[idx];
        case JXR_CM_INDEPENDENT:
            return cur->channel[ch].hp_qp[idx];
        default:
            assert(0);
    return 0;
    }
}

static int quantize_dc(int value, int quant)
{
    int sign = 1;
    if (value < 0) {
        sign = -1;
        value = -value;
    }

    int offset = quant >> 1;

    return sign * (value + offset)/quant;
}

static int quantize_lphp(int value, int quant)
{
    int sign = 1;
    if (value < 0) {
        sign = -1;
        value = -value;
    }

    int offset = (quant*3 + 1) >> 3;

    return sign * (value + offset)/quant;
}

static void dump_all_strips(jxr_image_t /*image*/)
{
#if defined(DETAILED_DEBUG) && 0
    int mx;
    int ch;
#if 1
    for (mx = 0 ; mx < EXTENDED_WIDTH_BLOCKS(image) ; mx += 1) {
        for (ch = 0 ; ch < image->num_channels ; ch += 1) {
            int jdx;
            DEBUG("up3: mx=%3d ch=%d:", mx, ch);
            for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                if (jdx%8 == 0 && jdx != 0)
                    DEBUG("\n%*s", 17, "");
                DEBUG(" %08x", image->strip[ch].up3[mx].data[jdx]);
            }
            DEBUG("\n");
        }
    }
#endif
#if 0
    for (mx = 0 ; mx < EXTENDED_WIDTH_BLOCKS(image) ; mx += 1) {
        for (ch = 0 ; ch < image->num_channels ; ch += 1) {
            int jdx;
            DEBUG("up2: mx=%3d ch=%d:", mx, ch);
            for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                if (jdx%8 == 0 && jdx != 0)
                    DEBUG("\n%*s", 17, "");
                DEBUG(" %08x", image->strip[ch].up2[mx].data[jdx]);
            }
            DEBUG("\n");
        }
    }
#endif
#if 0
    for (mx = 0 ; mx < EXTENDED_WIDTH_BLOCKS(image) ; mx += 1) {
        for (ch = 0 ; ch < image->num_channels ; ch += 1) {
            int jdx;
            DEBUG("up1: mx=%3d ch=%d:", mx, ch);
            for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                if (jdx%8 == 0 && jdx != 0)
                    DEBUG("\n%*s", 17, "");
                DEBUG(" %08x", image->strip[ch].up1[mx].data[jdx]);
            }
            DEBUG("\n");
        }
    }
#endif
#endif
}

/*
* This function is use to prepare a strip of MB data for use by the
* encoder. After this call is complete, the "my" strip with tx/ty is
* ready in the CUR strip. The data is filled into the pipeline
* starting with UP3, where it is processed and worked down to
* CUR. The CUR strip is then processed and formatted.
*
* On entry to this function, the CUR strip is no longer needed, so
* may immediately be tossed.
*/
static void wflush_process_strip(jxr_image_t image, int ty)
{
    int ch;
    const int height = EXTENDED_HEIGHT_BLOCKS(image);
    int cur_row;

    DEBUG("wflush_process_strip: image->cur_my = %d\n", image->cur_my);
    dump_all_strips(image);

    cur_row = image->tile_row_position[ty] + image->cur_my;

    /* Finish up scaling of the image data, and shuffle it to the
    internal sub-block format. */
    if (cur_row >= -3 && cur_row < (height-3)) {
        scale_and_shuffle_up3(image);
    }

    /* Transform on up2 data. At this point, the up2 and up3
    strips are lines N (up2) and N+1 (up3) of scaled image
    data. After this section is done, the image data in up2
    becomes DC-HP data. */
    if (cur_row >= -2 && cur_row < (height-2)) {


        /* If overlap filtering is enabled, then do it. The
        filter assumes that this strip (up2) and the next
        (up3) are collected and scaled. The filter looks
        ahead to up3 for bottom context. */
        if (OVERLAP_INFO(image) != 0)
            first_prefilter_up2(image,ty);

        /* Transform up2 data to DC-HP coefficients. */
        for (ch = 0; ch < image->num_channels ; ch += 1)
            PCT_stage1_up2(image, ch, ty);

        /* read lwf test flag into image container */
        if (image->lwf_test == 0)
            image->lwf_test = _jxr_read_lwf_test_flag();
    }

    /* Second tranform on up1 data. The DC-HP data becomes DC-LP-HP. */
    if (cur_row >= -1 && cur_row < (height-1)) {

        /* If intermediate overlap filtering is enabled, then do
        it. The filter assumes that this strip (up1) and the
        next (up2) have already been PCT processed. */
        if (OVERLAP_INFO(image) == 2)
            second_prefilter_up1(image,ty);

        /* PCT_level1_cur */
        for (ch = 0; ch < image->num_channels ; ch += 1)
            PCT_stage2_up1(image, ch, ty);

        /* read lwf test flag into image container */
        if (image->lwf_test == 0)
            image->lwf_test = _jxr_read_lwf_test_flag();
    }

    if (cur_row >= -1 && cur_row < (height-1)) {
        /* DC-LP prediction on the CURUP1strip. At this point CUR an UP1
        are PCT transformed, and CUR is predicted. After this, UP1
        will also be predicted with pred_dclp members filled in
        with saved PCT coefficients. */
        int tx;
        for (tx = 0; tx < (int) image->tile_columns ; tx += 1) {
            int mx;
            for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
                /* Calculate the MB HP prediction mode. This uses
                only local information, namely the LP values. */
                int mbhp_pred_mode = w_calculate_mbhp_mode_up1(image, tx, mx);
                assert(mbhp_pred_mode < 4);

                MACROBLK_UP1(image,0,tx,mx).mbhp_pred_mode = mbhp_pred_mode;

                /* Run DCLP prediction. This assumes that the
                previous (CUR) strip has already been
                predicted, and uses some predict values from
                that strip. */
                w_predict_up1_dclp(image, tx, ty, mx);
            }
        }
    }

    if (cur_row >= -1 && cur_row < (height-1)) {
        DEBUG("wflush_process_string: Calculate HP CBP for my=%d\n", cur_row+1);

        /* Perform HP prediction. */
        int use_num_channels = image->num_channels;
        if (image->use_clr_fmt == 1/*YUV420*/ || image->use_clr_fmt == 2/*YUV422*/)
            use_num_channels = 1;

        int tx;
        for (tx = 0; tx < (int) image->tile_columns ; tx += 1) {
            if (image->tile_columns > 1)
                _jxr_w_load_hpcbp_state(image, tx);
            int mx;
            for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
                int mbhp_pred_mode = MACROBLK_UP1(image,0,tx,mx).mbhp_pred_mode;
                int idx;
                for (idx = 0 ; idx < use_num_channels ; idx += 1)
                    w_predict_up1_hp(image, idx, tx, mx, mbhp_pred_mode);

                /* Also calculate and predict HPCBP in up1. */
                calculate_hpcbp_up1(image, tx, ty, mx);
                w_PredCBP(image, tx, ty, mx);
            }
            if (image->tile_columns > 1)
                _jxr_w_store_hpcbp_state(image, tx);
        }
    }

    DEBUG("wflush_process_strip done: cur_row = %d\n", cur_row);
}

/* since hpcbp is processed at each row, need to save context for each tile column */
void _jxr_w_load_hpcbp_state(jxr_image_t image, int tx)
{
    unsigned idx;
    for (idx = 0 ; idx < 2 ; idx += 1) {
        image->model_hp.bits[idx] = image->model_hp_buffer[tx].bits[idx];
        image->model_hp.state[idx] = image->model_hp_buffer[tx].state[idx];

        image->hp_cbp_model.state[idx] = image->hp_cbp_model_buffer[tx].state[idx];
        image->hp_cbp_model.count0[idx] = image->hp_cbp_model_buffer[tx].count0[idx];
        image->hp_cbp_model.count1[idx] = image->hp_cbp_model_buffer[tx].count1[idx];
    }
}

void _jxr_w_store_hpcbp_state(jxr_image_t image, int tx)
{
    unsigned idx;
    for (idx = 0 ; idx < 2 ; idx += 1) {
        image->model_hp_buffer[tx].bits[idx] = image->model_hp.bits[idx];
        image->model_hp_buffer[tx].state[idx] = image->model_hp.state[idx];

        image->hp_cbp_model_buffer[tx].state[idx] = image->hp_cbp_model.state[idx];
        image->hp_cbp_model_buffer[tx].count0[idx] = image->hp_cbp_model.count0[idx];
        image->hp_cbp_model_buffer[tx].count1[idx] = image->hp_cbp_model.count1[idx];
    }
}

void _jxr_wflush_mb_strip(jxr_image_t image, int tx, int ty, int my, int read_new)
{
    DEBUG("wflush_mb_strip: cur_my=%d, tile-x/y=%d/%d, my=%d\n", image->cur_my, tx, ty, my);

    unsigned ty_offset = 0;
    if (FREQUENCY_MODE_CODESTREAM_FLAG(image))
        ty_offset = image->tile_row_position[ty];

    if (my == 0 && (image->cur_my >= 0)) {
        /* reset the current row value for a new tile */
        image->cur_my = my - 1;
        
        if (ALPHACHANNEL_FLAG(image)) 
            image->alpha->cur_my = my - 1;
    }
    
    if ((tx == 0) && (read_new == 1)) {
        /* Process entire strip, then store it */
        while (image->cur_my < my) {
            const int height = EXTENDED_HEIGHT_BLOCKS(image);
            int cur_row;

            w_rotate_mb_strip(image);
            image->cur_my += 1;
            cur_row = image->tile_row_position[ty] + image->cur_my;
            if (ALPHACHANNEL_FLAG(image)) {
                w_rotate_mb_strip(image->alpha);
                image->alpha->cur_my += 1;
            }

            /* Load up4 with new image data. */
            if (cur_row >= -4 && cur_row < (height-4)) {
                collect_and_scale_up4(image, ty);
            }
                       
            wflush_process_strip(image, ty);
            if ((INDEXTABLE_PRESENT_FLAG(image)) && (image->cur_my >= 0)) {
                /* save processed row */
                wflush_to_tile_buffer(image, image->cur_my + ty_offset);
            }
            if (ALPHACHANNEL_FLAG(image)) {
                wflush_process_strip(image->alpha, ty);
                if ((INDEXTABLE_PRESENT_FLAG(image->alpha)) && (image->alpha->cur_my >= 0)) {
                    /* save processed row */
                    wflush_to_tile_buffer(image->alpha, image->alpha->cur_my + ty_offset);
                }
            }
        }
    }
    else {
        /* load a row of mb data as appropriate  */
        image->cur_my += 1;
        wflush_collect_mb_strip_data(image, image->cur_my + ty_offset);
        
        if (ALPHACHANNEL_FLAG(image)) {
            image->alpha->cur_my += 1;
            wflush_collect_mb_strip_data(image->alpha, image->alpha->cur_my + ty_offset);
        }
    }
}

/*
* The tile_row_buffer holds flushed mb data in image raster order,
* along with other per-mb data. This is in support of tiled SPATIAL processing.
* This function saves mb row information for all rows of the the current tile row
*/
static void wflush_to_tile_buffer(jxr_image_t image, int my)
{
    DEBUG("wflush_mb_strip: wflush_to_tile_buffer my=%d\n", my);

    int format_scale = 256;
    if (image->use_clr_fmt == 2 /* YUV422 */) {
        format_scale = 16 + 8*15;
    } else if (image->use_clr_fmt == 1 /* YUV420 */) {
        format_scale = 16 + 4*15;
    }


    int tx;
    for (tx = 0; tx < (int) image->tile_columns ; tx += 1) {
        int mx;
        for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
            DEBUG("wflush_mb_strip: wflush_to_tile_buffer tx=%d, mx=%d, CUR=0x%08x UP1=0x%08x, UP2=0x%08x, UP3=0x%08x, LP_QUANT=%d\n",
                tx, mx, MACROBLK_CUR(image,0,tx,mx).data[0],
                MACROBLK_UP1(image,0,tx,mx).data[0],
                MACROBLK_UP2(image,0,tx,mx).data[0],
                MACROBLK_UP3(image,0,tx,mx).data[0],
                MACROBLK_CUR_LP_QUANT(image,0,tx,mx));

            int off = my * EXTENDED_WIDTH_BLOCKS(image) + image->tile_column_position[tx] + mx;
            int ch;
            for (ch = 0; ch < image->num_channels; ch += 1) {
                struct macroblock_s*mb = image->mb_row_buffer[ch] + off;
                mb->lp_quant = MACROBLK_CUR_LP_QUANT(image,ch,tx,mx);
                mb->hp_quant = MACROBLK_CUR(image,ch,tx,mx).hp_quant;
                mb->hp_cbp = MACROBLK_CUR(image,ch,tx,mx).hp_cbp;
                mb->hp_diff_cbp = MACROBLK_CUR(image,ch,tx,mx).hp_diff_cbp;
                mb->mbhp_pred_mode = MACROBLK_CUR(image,ch,tx,mx).mbhp_pred_mode;
                mb->hp_model_bits[0] = MACROBLK_CUR(image,ch,tx,mx).hp_model_bits[0];
                mb->hp_model_bits[1] = MACROBLK_CUR(image,ch,tx,mx).hp_model_bits[1];
                int count = (ch==0)? 256 : format_scale;
                int idx;
                for (idx = 0 ; idx < count ; idx += 1)
                    mb->data[idx] = MACROBLK_CUR(image,ch,tx,mx).data[idx];
                for (idx = 0 ; idx < 7 ; idx += 1)
                    mb->pred_dclp[idx] = MACROBLK_CUR(image,ch,tx,mx).pred_dclp[idx];
            }
        }
    }
}

/*
* Recover a strip of data from all but the last column of data. Skip
* the last column because this function is called while the last
* column is being processed.
* This function loads mb row information for all rows of the the current tile row
*/
static void wflush_collect_mb_strip_data(jxr_image_t image, int my)
{
    DEBUG("wflush_mb_strip: wflush_collect_mb_strip_data my=%d\n", my);

    int format_scale = 256;
    if (image->use_clr_fmt == 2 /* YUV422 */) {
        format_scale = 16 + 8*15;
    } else if (image->use_clr_fmt == 1 /* YUV420 */) {
        format_scale = 16 + 4*15;
    }

    int tx;
    for (tx = 0; tx < (int) image->tile_columns ; tx += 1) {
        int mx;
        for (mx = 0; mx < (int) image->tile_column_width[tx]; mx += 1) {
            int off = my * EXTENDED_WIDTH_BLOCKS(image) + image->tile_column_position[tx] + mx;
            int ch;
            for (ch = 0; ch < image->num_channels; ch += 1) {
                struct macroblock_s*mb = image->mb_row_buffer[ch] + off;
                MACROBLK_CUR_LP_QUANT(image,ch,tx,mx) = mb->lp_quant;
                MACROBLK_CUR(image,ch,tx,mx).hp_quant = mb->hp_quant;
                MACROBLK_CUR(image,ch,tx,mx).hp_cbp = mb->hp_cbp;
                MACROBLK_CUR(image,ch,tx,mx).hp_diff_cbp = mb->hp_diff_cbp;
                MACROBLK_CUR(image,ch,tx,mx).mbhp_pred_mode = mb->mbhp_pred_mode;
                MACROBLK_CUR(image,ch,tx,mx).hp_model_bits[0] = mb->hp_model_bits[0];
                MACROBLK_CUR(image,ch,tx,mx).hp_model_bits[1] = mb->hp_model_bits[1];
                int count = (ch==0)? 256 : format_scale;
                int idx;
                for (idx = 0 ; idx < count; idx += 1)
                    MACROBLK_CUR(image,ch,tx,mx).data[idx] = mb->data[idx];
                for (idx = 0 ; idx < 7 ; idx += 1)
                     MACROBLK_CUR(image,ch,tx,mx).pred_dclp[idx] = mb->pred_dclp[idx];
            }
            DEBUG("wflush_mb_strip: wflush_collect_mb_strip_data tx=%d, mx=%d, CUR=0x%08x UP1=0x%08x, UP2=0x%08x, UP3=0x%08x lp_quant=%d\n",
                tx, mx, MACROBLK_CUR(image,0,tx,mx).data[0],
                MACROBLK_UP1(image,0,tx,mx).data[0],
                MACROBLK_UP2(image,0,tx,mx).data[0],
                MACROBLK_UP3(image,0,tx,mx).data[0],
                MACROBLK_CUR_LP_QUANT(image,0,tx,mx));
        }
    }
}

static void w_rotate_mb_strip(jxr_image_t image)
{
    int ch;

    _jxr_clear_strip_cur(image);

    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        struct macroblock_s*tmp = image->strip[ch].cur;
        image->strip[ch].cur = image->strip[ch].up1;
        image->strip[ch].up1 = image->strip[ch].up2;
        image->strip[ch].up2 = image->strip[ch].up3;
        image->strip[ch].up3 = image->strip[ch].up4;
        image->strip[ch].up4 = tmp;
    }
}

/*
* Transform RGB input data to YUV444 data. Do the transform in place,
* assuming the input channels are R, G and B in order.
*/
static void rgb_to_yuv444_up4(jxr_image_t image)
{
    unsigned mx;
    for (mx = 0 ; mx < EXTENDED_WIDTH_BLOCKS(image) ; mx += 1) {
        int*dataR = MACROBLK_UP4(image,0,0,mx).data;
        int*dataG = MACROBLK_UP4(image,1,0,mx).data;
        int*dataB = MACROBLK_UP4(image,2,0,mx).data;
        int idx;
        for (idx = 0 ; idx < 16*16 ; idx += 1) {
            const int R = dataR[idx];
            const int G = dataG[idx];
            const int B = dataB[idx];
            const int V = B - R;
            const int tmp = R - G + _jxr_ceil_div2(V);
            const int Y = G + _jxr_floor_div2(tmp);
            const int U = -tmp;
            dataR[idx] = Y;
            dataG[idx] = U;
            dataB[idx] = V;
        }
    }
}

static void cmyk_to_yuvk_up4(jxr_image_t image)
{
    unsigned mx;
    for (mx = 0 ; mx < EXTENDED_WIDTH_BLOCKS(image) ; mx += 1) {
        int*datac = MACROBLK_UP4(image,0,0,mx).data;
        int*datam = MACROBLK_UP4(image,1,0,mx).data;
        int*datay = MACROBLK_UP4(image,2,0,mx).data;
        int*datak = MACROBLK_UP4(image,3,0,mx).data;
        int idx;
        for (idx = 0 ; idx < 16*16 ; idx += 1) {
            const int c = datac[idx];
            const int m = datam[idx];
            const int y = datay[idx];
            const int k = datak[idx];
            const int V = c - y;
            const int U = c - m - _jxr_floor_div2(V);
            const int Y = k - m - _jxr_floor_div2(U);
            const int K = k - _jxr_floor_div2(Y);
            datac[idx] = Y;
            datam[idx] = U;
            datay[idx] = V;
            datak[idx] = K;
        }
    }
}

/*
* Transform YUV444 input data to YUV422 by subsampling the UV planes
* horizontally.
*/
#if defined(FILTERED_YUV_SAMPLE)
static void yuv444_to_yuv422_up4(jxr_image_t image)
{
    int ch;
    int px, py;

    int*buf[16];
    for (py = 0 ; py < 16 ; py += 1)
        buf[py] = (int*)jpegxr_calloc(8*EXTENDED_WIDTH_BLOCKS(image), sizeof(int));

    for (ch = 1 ; ch < 3 ; ch += 1) {
        unsigned mx;
        for (mx = 0 ; mx < EXTENDED_WIDTH_BLOCKS(image) ; mx += 1) {
            int*prev = mx>0? MACROBLK_UP4(image,ch,0,mx-1).data : 0;
            int*data = MACROBLK_UP4(image,ch,0,mx).data;
            int*next = (mx+1)<EXTENDED_WIDTH_BLOCKS(image)? MACROBLK_UP4(image,ch,0,mx+1).data : 0;

            for (py = 0 ; py < 16 ; py += 1) {
                int*bp = buf[py]+8*mx;
                int*src = data + 16*py;
                if (prev == 0) {
                    bp[0] = 2*src[2] + 8*src[1] + 6*src[0] + 8;
                } else {
                    int*tmp = prev + 16*py;
                    bp[0] = 1*tmp[14] + 4*tmp[15] + 6*src[0] + 4*src[1] + 1*src[2] + 8;
                }

                for (px = 2 ; px < 14 ; px += 2) {
                    bp[px/2] = 1*src[px-2] + 4*src[px-1] + 6*src[px] + 4*src[px+1] + 1*src[px+2] + 8;
                }

                if (next==0) {
                    bp[7] = 1*src[12] + 4*src[13] + 6*src[14] + 4*src[15] + 1*src[14] + 8;
                } else {
                    int*tmp = next + 16*py;
                    bp[7] = 1*src[12] + 4*src[13] + 6*src[14] + 4*src[15] + 1*tmp[0] + 8;
                }
            }
        }

        for (mx = 0 ; mx < EXTENDED_WIDTH_BLOCKS(image) ; mx += 1) {
            int*data = MACROBLK_UP4(image,ch,0,mx).data;
            for (py = 0 ; py < 16 ; py += 1) {
                int*bp = buf[py] + 8*mx;
                int*dst = data+8*py;
                for (px = 0 ; px < 8 ; px += 1)
                    dst[px] = bp[px] >> 4;
            }
        }
    }

    for (py = 0 ; py < 16 ; py += 1)
        jpegxr_free(buf[py]);
}

static void yuv422_to_yuv420_up3(jxr_image_t image)
{
    int my = image->cur_my + 3;
    assert(my >= 0);

    int ch;
    for (ch = 1 ; ch < 3 ; ch += 1) {
        unsigned mx;
        for (mx = 0 ; mx < EXTENDED_WIDTH_BLOCKS(image); mx += 1) {
            int*data = MACROBLK_UP3(image,ch,0,mx).data;
            /* Save the unreduced data to allow for overlapping */
            int*dataX = data + 128;
            int idx;
            for (idx = 0 ; idx < 128 ; idx += 1)
                dataX[idx] = data[idx];

            int px, py;

            /* First, handle py==0 */
            if (my == 0) {
                int*next1 = data+1*8;
                int*next2 = data+2*8;
                for (px = 0 ; px < 8 ; px += 1) {
                    int val = 2*next2[px] + 8*next1[px] + 6*data[px] + 8;
                    data[px] = val >> 4;
                }
            } else {
                int*prev2 = MACROBLK_UP2(image,ch,0,mx).data + 128 + 14*8;
                int*prev1 = MACROBLK_UP2(image,ch,0,mx).data + 128 + 15*8;
                int*cur = dataX+0*8;
                int*next1 = dataX+1*8;
                int*next2 = dataX+2*8;
                for (px = 0 ; px < 8 ; px += 1) {
                    int val = 1*next2[px] + 4*next1[px] + 6*cur[px] + 4*prev1[px] + 1*prev2[px] +8;
                    data[px] = val >> 4;
                }
            }

            /* py = 1-6 */
            for (py = 2 ; py < 14 ; py += 2) {
                int*prev2 = dataX + 8*(py-2);
                int*prev1 = dataX + 8*(py-1);
                int*cur = dataX + 8*(py+0);
                int*next1 = dataX + 8*(py+1);
                int*next2 = dataX + 8*(py+2);
                for (px = 0 ; px < 8 ; px += 1) {
                    int val = 1*next2[px] + 4*next1[px] + 6*cur[px] + 4*prev1[px] + 1*prev2[px] +8;
                    data[px + 8*(py/2)] = val >> 4;
                }
            }

            /* py == 7 */
            if ((my+1) < (int) EXTENDED_HEIGHT_BLOCKS(image)) {
                int*prev2 = dataX + 8*(14-2);
                int*prev1 = dataX + 8*(14-1);
                int*cur = dataX + 8*(14+0);
                int*next1 = dataX + 8*(14+1);
                int*next2 = MACROBLK_UP4(image,ch,0,mx).data + 0*8;
                for (px = 0 ; px < 8 ; px += 1) {
                    int val = 1*next2[px] + 4*next1[px] + 6*cur[px] + 4*prev1[px] + 1*prev2[px] +8;
                    data[px + 8*(py/2)] = val >> 4;
                }
            } else {
                int*prev2 = dataX + 8*(14-2);
                int*prev1 = dataX + 8*(14-1);
                int*cur = dataX + 8*(14+0);
                int*next1 = dataX + 8*(14+1);
                int*next2 = dataX + 8*(14+0);
                for (px = 0 ; px < 8 ; px += 1) {
                    int val = 1*next2[px] + 4*next1[px] + 6*cur[px] + 4*prev1[px] + 1*prev2[px] +8;
                    data[px + 8*(py/2)] = val >> 4;
                }
            }
        }
    }
}

#else

/*
* These are simple-minded subsamples for convertion YUV444 to YUV42x.
*/
static void yuv444_to_yuv422_up4(jxr_image_t image)
{
    int mx;
    for (mx = 0 ; mx < EXTENDED_WIDTH_BLOCKS(image); mx += 1) {
        int ch;
        for (ch = 1 ; ch < 3 ; ch += 1) {
            int*data = MACROBLK_UP4(image,ch,0,mx).data;
            int px, py;
            for (py = 0 ; py < 16 ; py += 1) {
                int*src = data + 16*py;
                int*dst = data + 8*py;
                for (px = 0 ; px < 16 ; px += 2) {
                    dst[px/2] = src[px+0];
                }
            }
        }
    }
}

static void yuv422_to_yuv420_up3(jxr_image_t image)
{
    int mx;
    for (mx = 0 ; mx < EXTENDED_WIDTH_BLOCKS(image); mx += 1) {
        int ch;
        for (ch = 1 ; ch < 3 ; ch += 1) {
            int*data = MACROBLK_UP3(image,ch,0,mx).data;
            int px, py;
            for (py = 0 ; py < 16 ; py += 2) {
                int*src = data + 8*py;
                int*dst = data + 8*py/2;
                for (px = 0 ; px < 8 ; px += 1) {
                    dst[px] = src[px];
                }
            }
        }
    }
}
#endif

/*
* Shuffle a set of values that are in a single 16x16 raster block to
* 16 4x4 blocks. The 4x4 blocks are the units that the PCT works on.
*/
static void block_shuffle444(int*data)
{
    int tmp[256];

    int idx;
    for (idx = 0 ; idx < 256 ; idx += 4) {
        int blk = idx/16;
        int mbx = blk%4;
        int mby = blk/4;
        int pix = idx%16;
        int py = pix/4;

        int ptr = 16*4*mby + 4*mbx + 16*py;
        tmp[idx+0] = data[ptr+0];
        tmp[idx+1] = data[ptr+1];
        tmp[idx+2] = data[ptr+2];
        tmp[idx+3] = data[ptr+3];
    }

    for (idx = 0 ; idx < 256 ; idx += 1)
        data[idx] = tmp[idx];
}

static void block_shuffle422(int*data)
{
    int tmp[128];

    int idx;
    for (idx = 0 ; idx < 128 ; idx += 4) {
        int blk = idx/16;
        int mbx = blk%2;
        int mby = blk/2;
        int pix = idx%16;
        int py = pix/4;

        int ptr = 16*2*mby + 4*mbx + 8*py;
        tmp[idx+0] = data[ptr+0];
        tmp[idx+1] = data[ptr+1];
        tmp[idx+2] = data[ptr+2];
        tmp[idx+3] = data[ptr+3];
    }

    for (idx = 0 ; idx < 128 ; idx += 1)
        data[idx] = tmp[idx];
}

static void block_shuffle420(int*data)
{
    int tmp[64];

    int idx;
    for (idx = 0 ; idx < 64 ; idx += 4) {
        int blk = idx/16;
        int mbx = blk%2;
        int mby = blk/2;
        int pix = idx%16;
        int py = pix/4;

        int ptr = 16*2*mby + 4*mbx + 8*py;
        tmp[idx+0] = data[ptr+0];
        tmp[idx+1] = data[ptr+1];
        tmp[idx+2] = data[ptr+2];
        tmp[idx+3] = data[ptr+3];
    }

    for (idx = 0 ; idx < 64 ; idx += 1)
        data[idx] = tmp[idx];
}

static void collect_and_scale_up4(jxr_image_t image, int ty)
{
    int scale = image->scaled_flag? 3 : 0;
    int bias;
    int round;
	(void)round;
#ifndef JPEGXR_ADOBE_EXT
    int shift_bits = image->shift_bits;
#endif //#ifndef JPEGXR_ADOBE_EXT

    if (image->output_clr_fmt == JXR_OCF_RGBE) {
        bias = 0;
        round = image->scaled_flag? 3 : 0;
    }
    else {
        switch (SOURCE_BITDEPTH(image)) {
            case 0: /* BD1WHITE1 */
            case 15: /* BD1BLACK1 */
                bias = 0;
                round = image->scaled_flag? 4 : 0;
                break;
            case 1: /* BD8 */
                bias = 1 << 7;
                round = image->scaled_flag? 3 : 0;
                break;
            case 2: /* BD16 */
                bias = 1 << 15;
                round = image->scaled_flag? 4 : 0;
                break;
            case 3: /* BD16S */
                bias = 0;
                round = image->scaled_flag? 3 : 0;
                break;
            case 4: /* BD16F */
                bias = 0;
                round = image->scaled_flag? 3 : 0;
                break;
            case 6: /* BD32S */
                bias = 0;
                round = image->scaled_flag? 3 : 0;
                break;
            case 7: /* BD32F */
                bias = 0;
                round = image->scaled_flag? 3 : 0;
                break;
            case 8: /* BD5 */
                bias = 1 << 4;
                round = image->scaled_flag? 3 : 0;
                break;
            case 9: /* BD10 */
                bias = 1 << 9;
                round = image->scaled_flag? 3 : 0;
                break;
            case 10: /* BD565 */
                bias = 1 << 5;
                round = image->scaled_flag? 3 : 0;
                break;
            default: /* RESERVED */
                fprintf(stderr, "XXXX Don't know how to scale bit depth %d?\n", SOURCE_BITDEPTH(image));
                bias = 0;
                round = image->scaled_flag? 3 : 0;
                break;
        }
    }

    DEBUG("scale_and_emit_top: scale=%d, bias=%d, round=%d, shift_bits=%d\n",
        scale, bias, round, shift_bits);

    assert(image->num_channels > 0);

    /* This function operates on a whole image (not tile) basis */
    int my = image->cur_my + image->tile_row_position[ty] + 4;
    DEBUG("collect_and_scale_up4: Collect strip %d\n", my);

    int mx;
    int ch;
    int num_channels = image->num_channels;
    
    if (ALPHACHANNEL_FLAG(image)) {
        num_channels ++;
        image->strip[image->num_channels].up4 = image->alpha->strip[0].up4;
    }

    for (mx = 0 ; mx < (int) EXTENDED_WIDTH_BLOCKS(image) ; mx += 1) {

        /* Collect the data from the application. */
        assert(image->num_channels <= 16);
        int buffer[17*256];
        image->inp_fun(image, mx, my, buffer); 

        /* Pad to the bottom by repeating the last pixel */
        if ((my+1) == (int) EXTENDED_HEIGHT_BLOCKS(image) && ((image->height1+image->window_extra_top+1) % 16 != 0)) {
            int last_y = (image->height1 + image->window_extra_top) % 16;
            int ydx;
            for (ydx = last_y+1 ; ydx < 16 ; ydx += 1) {
                int xdx;
                for (xdx = 0 ; xdx < 16 ; xdx += 1) {
                    for (ch = 0 ; ch < num_channels ; ch += 1) {
                        int pad = buffer[(16*last_y + xdx)*num_channels + ch];
                        if ((16*mx + xdx) > (int) image->width1) {
                            int use_x = (image->width1 + image->window_extra_left) % 16;
                            pad = buffer[(16*last_y + use_x)*num_channels + ch];
                        }

                        buffer[(16*ydx + xdx)*num_channels + ch] = pad;
                    }
                }
            }
        }

        /* Pad to the right by repeating the last pixel */
        if ((mx+1) == (int) EXTENDED_WIDTH_BLOCKS(image) && ((image->width1+image->window_extra_left+1) % 16 != 0)) {
            int last_x = (image->width1 + image->window_extra_left) % 16;
            int ydx ;
            for (ydx = 0; ydx < 16 ; ydx += 1) {
                int xdx;
                for (xdx = last_x+1 ; xdx < 16 ; xdx += 1) {
                    for (ch = 0 ; ch < num_channels ; ch += 1) {
                        int pad = buffer[(16*ydx + last_x)*num_channels + ch];
                        buffer[(16*ydx + xdx)*num_channels + ch] = pad;
                    }
                }
            }
        }

        /* And finally collect the strip data. */
        /* image->alpha->strip[0].up4 == image->strip[image->num_channels].up4 !!! */
        int jdx;
        for (jdx = 0 ; jdx < 256 ; jdx += 1) {
            int ch;
            for (ch = 0 ; ch < num_channels ; ch += 1)
                image->strip[ch].up4[mx].data[jdx] = buffer[jdx*num_channels + ch];
        }

#if defined(DETAILED_DEBUG) && 0
        for (ch = 0 ; ch < num_channels ; ch += 1) {
            DEBUG("image data mx=%3d my=%3d ch=%d:", mx, my, ch);
            for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                if (jdx%16 == 0 && jdx != 0)
                    DEBUG("\n%*s", 30, "");
                DEBUG(" %02x", image->strip[ch].up4[mx].data[jdx]);
            }
            DEBUG("\n");
        }
#endif

        /* AddBias and ComputeScaling */
        if (image->use_clr_fmt == 4 /*YUVK*/ && image->output_clr_fmt == JXR_OCF_CMYK) {
            int*dp;
            assert(image->num_channels == 4);
            for (ch = 0 ; ch < 3 ; ch += 1) {
                dp = image->strip[ch].up4[mx].data;
                int jdx;
                for (jdx = 0 ; jdx < 256 ; jdx += 1)
                    dp[jdx] = (dp[jdx] - (bias>>1)) << scale;
            }
            dp = image->strip[3].up4[mx].data;
            for (jdx = 0 ; jdx < 256 ; jdx += 1)
                dp[jdx] = (dp[jdx] + (bias>>1)) << scale;

            if (num_channels == 5) { // for interleaved CMYKA, num_channels = 5 and image->num_channels = 4
                dp = image->strip[4].up4[mx].data;
                for (jdx = 0 ; jdx < 256 ; jdx += 1)
                    dp[jdx] = (dp[jdx] - bias) << scale;
            }
        } else {
            for (ch = 0 ; ch < num_channels ; ch += 1) {
                int*dp = image->strip[ch].up4[mx].data;
                int jdx;
                for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                    dp[jdx] = (dp[jdx] - bias) << scale;
                }
            }
        }
    }

    /* Transform the color space. */
    switch (image->use_clr_fmt) {
        case 0: /* YONLY */
            break;
        case 1: /* YUV420 */
            if (image->output_clr_fmt == JXR_OCF_RGB) {
                rgb_to_yuv444_up4(image);
                yuv444_to_yuv422_up4(image);
                /* Do yuv422_to_yuv420_up3 after further processing */
            }
            break;
        case 2: /* YUV422 */
            if (image->output_clr_fmt == JXR_OCF_RGB) {
                rgb_to_yuv444_up4(image);
                yuv444_to_yuv422_up4(image);
            }
            break;
        case 3: /* YUV444 */
            if (image->output_clr_fmt == JXR_OCF_RGB)
                rgb_to_yuv444_up4(image);
            break;
        case 4: /* YUVK */
            if (image->output_clr_fmt == JXR_OCF_CMYK)
                cmyk_to_yuvk_up4(image);
            break;
        case 6: /* NCOMPONENT */
            break;
    }

}

static void scale_and_shuffle_up3(jxr_image_t image)
{
    int mx;
#ifndef JPEGXR_ADOBE_EXT
    int my = image->cur_my + 3;
#endif //#ifndef JPEGXR_ADOBE_EXT
    int ch;

    /* Finish transform of the color space. */
    switch (image->use_clr_fmt) {
        case 0: /* YONLY */
            break;
        case 1: /* YUV420 */
            if (image->output_clr_fmt == JXR_OCF_RGB)
                yuv422_to_yuv420_up3(image);
            break;
        case 2: /* YUV422 */
            break;
        case 3: /* YUV444 */
            break;
        case 6: /* NCOMPONENT */
            break;
    }
    for (mx = 0 ; mx < (int) EXTENDED_WIDTH_BLOCKS(image) ; mx += 1) {

#if defined(DETAILED_DEBUG) && 1
        int jdx;
        for (ch = 0 ; ch < image->num_channels ; ch += 1) {
            int count = 256;
            if (ch > 0 && image->use_clr_fmt==2/*YUV422*/)
                count = 128;
            if (ch > 0 && image->use_clr_fmt==1/*YUV420*/)
                count = 64;

            DEBUG("image yuv mx=%3d my=%3d ch=%d:", mx, my, ch);
            for (jdx = 0 ; jdx < count ; jdx += 1) {
                if (jdx%8 == 0 && jdx != 0)
                    DEBUG("\n%*s", 29, "");
                DEBUG(" %08x", image->strip[ch].up3[mx].data[jdx]);
            }
            DEBUG("\n");
        }
#endif

        /* shuffle the data into the internal format. */
        block_shuffle444(image->strip[0].up3[mx].data);
        for (ch = 1 ; ch < image->num_channels ; ch += 1) {
            switch (image->use_clr_fmt) {
                case 1: /* YUV420 */
                    block_shuffle420(image->strip[ch].up3[mx].data);
                    break;
                case 2: /* YUV422 */
                    block_shuffle422(image->strip[ch].up3[mx].data);
                    break;
                default:
                    block_shuffle444(image->strip[ch].up3[mx].data);
                    break;
            }
        }
    }
}

static int*R2B(int*data, int x, int y)
{
    int bx = x/4;
    int by = y/4;
    int bl = by*4 + bx;
    return data + bl*16 + 4*(y%4) + x%4;
}

static int*R2B42(int*data, int x, int y)
{
    int bx = x/4;
    int by = y/4;
    int bl = by*2 + bx;
    return data + bl*16 + 4*(y%4) + x%4;
}
#define TOP_Y(y) ( y == image->tile_row_position[ty])
#define BOTTOM_Y(y) ( y == image->tile_row_position[ty] + image->tile_row_height[ty] - 1)
#define LEFT_X(idx) ( idx == 0)
#define RIGHT_X(idx) ( idx == image->tile_column_width[tx] -1 )


static void first_prefilter444_up2(jxr_image_t image, int ch, int ty)
{
    assert(ch == 0 || (image->use_clr_fmt != 2/*YUV422*/ && image->use_clr_fmt !=1/* YUV420*/));
    uint32_t tx = 0; /* XXXX */
    uint32_t top_my = image->cur_my + 2;
    uint32_t idx;

    if (top_my >= image->tile_row_height[ty])
        top_my -= image->tile_row_height[ty++];
    if (top_my >= image->tile_row_height[ty])
        top_my -= image->tile_row_height[ty++];
    top_my += image->tile_row_position[ty];

    DEBUG("Pre Level2 for row %d\n", top_my);

    for(tx = 0; tx < image->tile_columns; tx++)
    {
        int jdx;
        /* Left edge */
        if (tx == 0 || image->disableTileOverlapFlag)
        {
            int*dp = MACROBLK_UP2(image,ch,tx,0).data;
            for (jdx = 2 ; jdx < 14 ; jdx += 4) {
                _jxr_4PreFilter(R2B(dp,0,jdx+0),R2B(dp,0,jdx+1),R2B(dp,0,jdx+2),R2B(dp,0,jdx+3));
                _jxr_4PreFilter(R2B(dp,1,jdx+0),R2B(dp,1,jdx+1),R2B(dp,1,jdx+2),R2B(dp,1,jdx+3));
            }
        }

        /* Right edge */
        if(tx == image->tile_columns -1 || image->disableTileOverlapFlag){
            int*dp = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
            for (jdx = 2 ; jdx < 14 ; jdx += 4) {
                _jxr_4PreFilter(R2B(dp,14,jdx+0),R2B(dp,14,jdx+1),R2B(dp,14,jdx+2),R2B(dp,14,jdx+3));
                _jxr_4PreFilter(R2B(dp,15,jdx+0),R2B(dp,15,jdx+1),R2B(dp,15,jdx+2),R2B(dp,15,jdx+3));
            }
        }

        /* Top edge */
        if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my) ))
        {
            /* If this is the very first strip of blocks, then process the
            first two scan lines with the smaller 4Pre filter. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*dp = MACROBLK_UP2(image,ch,tx,idx).data;
                _jxr_4PreFilter(R2B(dp, 2,0),R2B(dp, 3,0),R2B(dp, 4,0),R2B(dp, 5,0));
                _jxr_4PreFilter(R2B(dp, 6,0),R2B(dp, 7,0),R2B(dp, 8,0),R2B(dp, 9,0));
                _jxr_4PreFilter(R2B(dp,10,0),R2B(dp,11,0),R2B(dp,12,0),R2B(dp,13,0));

                _jxr_4PreFilter(R2B(dp, 2,1),R2B(dp, 3,1),R2B(dp, 4,1),R2B(dp, 5,1));
                _jxr_4PreFilter(R2B(dp, 6,1),R2B(dp, 7,1),R2B(dp, 8,1),R2B(dp, 9,1));
                _jxr_4PreFilter(R2B(dp,10,1),R2B(dp,11,1),R2B(dp,12,1),R2B(dp,13,1));

                /* Top edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                    int*pp = MACROBLK_UP2(image,ch,tx,idx-1).data;
                    _jxr_4PreFilter(R2B(pp,14,0),R2B(pp,15,0),R2B(dp,0,0),R2B(dp,1,0));
                    _jxr_4PreFilter(R2B(pp,14,1),R2B(pp,15,1),R2B(dp,0,1),R2B(dp,1,1));
                }
            }

            /* Top left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch, tx, 0).data;
                _jxr_4PreFilter(R2B(dp, 0,0),R2B(dp, 1,0),R2B(dp, 0,1),R2B(dp, 1,1));
            }
            /* Top right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4PreFilter(R2B(dp, 14,0),R2B(dp, 15,0),R2B(dp, 14,1),R2B(dp, 15,1));
            }

        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. finish up with 4Pre filters. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*tp = MACROBLK_UP2(image,ch,tx,idx).data;

                _jxr_4PreFilter(R2B(tp, 2,14),R2B(tp, 3,14),R2B(tp, 4,14),R2B(tp, 5,14));
                _jxr_4PreFilter(R2B(tp, 6,14),R2B(tp, 7,14),R2B(tp, 8,14),R2B(tp, 9,14));
                _jxr_4PreFilter(R2B(tp,10,14),R2B(tp,11,14),R2B(tp,12,14),R2B(tp,13,14));

                _jxr_4PreFilter(R2B(tp, 2,15),R2B(tp, 3,15),R2B(tp, 4,15),R2B(tp, 5,15));
                _jxr_4PreFilter(R2B(tp, 6,15),R2B(tp, 7,15),R2B(tp, 8,15),R2B(tp, 9,15));
                _jxr_4PreFilter(R2B(tp,10,15),R2B(tp,11,15),R2B(tp,12,15),R2B(tp,13,15));

                /* Bottom edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                        int*tn = MACROBLK_UP2(image,ch,tx,idx-1).data;
                        _jxr_4PreFilter(R2B(tn,14,14),R2B(tn,15,14),R2B(tp, 0,14),R2B(tp, 1,14));
                        _jxr_4PreFilter(R2B(tn,14,15),R2B(tn,15,15),R2B(tp, 0,15),R2B(tp, 1,15));
                }
            }

            /* Bottom left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch,tx,0).data;
                _jxr_4PreFilter(R2B(dp, 0,14),R2B(dp, 1, 14),R2B(dp, 0,15),R2B(dp, 1, 15));
            }
            /* Bottom right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4PreFilter(R2B(dp, 14, 14),R2B(dp, 15, 14),R2B(dp, 14,15),R2B(dp, 15, 15));
            }

        }

        for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {
            int jdx;

            for (jdx = 2 ; jdx < 14 ; jdx += 4) {

                int*dp = MACROBLK_UP2(image,ch,tx,idx).data;
                /* Fully interior 4x4 filter blocks... */
                _jxr_4x4PreFilter(R2B(dp, 2,jdx+0),R2B(dp, 3,jdx+0),R2B(dp, 4,jdx+0),R2B(dp, 5,jdx+0),
                    R2B(dp, 2,jdx+1),R2B(dp, 3,jdx+1),R2B(dp, 4,jdx+1),R2B(dp, 5,jdx+1),
                    R2B(dp, 2,jdx+2),R2B(dp, 3,jdx+2),R2B(dp, 4,jdx+2),R2B(dp, 5,jdx+2),
                    R2B(dp, 2,jdx+3),R2B(dp, 3,jdx+3),R2B(dp, 4,jdx+3),R2B(dp, 5,jdx+3));
                _jxr_4x4PreFilter(R2B(dp, 6,jdx+0),R2B(dp, 7,jdx+0),R2B(dp, 8,jdx+0),R2B(dp, 9,jdx+0),
                    R2B(dp, 6,jdx+1),R2B(dp, 7,jdx+1),R2B(dp, 8,jdx+1),R2B(dp, 9,jdx+1),
                    R2B(dp, 6,jdx+2),R2B(dp, 7,jdx+2),R2B(dp, 8,jdx+2),R2B(dp, 9,jdx+2),
                    R2B(dp, 6,jdx+3),R2B(dp, 7,jdx+3),R2B(dp, 8,jdx+3),R2B(dp, 9,jdx+3));
                _jxr_4x4PreFilter(R2B(dp,10,jdx+0),R2B(dp,11,jdx+0),R2B(dp,12,jdx+0),R2B(dp,13,jdx+0),
                    R2B(dp,10,jdx+1),R2B(dp,11,jdx+1),R2B(dp,12,jdx+1),R2B(dp,13,jdx+1),
                    R2B(dp,10,jdx+2),R2B(dp,11,jdx+2),R2B(dp,12,jdx+2),R2B(dp,13,jdx+2),
                    R2B(dp,10,jdx+3),R2B(dp,11,jdx+3),R2B(dp,12,jdx+3),R2B(dp,13,jdx+3));

                if ( (image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && !RIGHT_X(idx))) {
                        /* 4x4 at the right */
                        int*np = MACROBLK_UP2(image,ch,tx,idx+1).data;

                        _jxr_4x4PreFilter(R2B(dp,14,jdx+0),R2B(dp,15,jdx+0),R2B(np, 0,jdx+0),R2B(np, 1,jdx+0),
                            R2B(dp,14,jdx+1),R2B(dp,15,jdx+1),R2B(np, 0,jdx+1),R2B(np, 1,jdx+1),
                            R2B(dp,14,jdx+2),R2B(dp,15,jdx+2),R2B(np, 0,jdx+2),R2B(np, 1,jdx+2),
                            R2B(dp,14,jdx+3),R2B(dp,15,jdx+3),R2B(np, 0,jdx+3),R2B(np, 1,jdx+3));
                }
            }

            if ((top_my+1) < EXTENDED_HEIGHT_BLOCKS(image)) {

                int*dp = MACROBLK_UP2(image,ch,tx,idx).data;
                int*up = MACROBLK_UP3(image,ch,tx,idx).data;

                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        /* Across vertical blocks, left edge */
                        _jxr_4PreFilter(R2B(dp,0,14),R2B(dp,0,15),R2B(up,0,0),R2B(up,0,1));
                        _jxr_4PreFilter(R2B(dp,1,14),R2B(dp,1,15),R2B(up,1,0),R2B(up,1,1));
                }
                if((!image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !BOTTOM_Y(top_my)))
                {
                    /* 4x4 bottom */
                    _jxr_4x4PreFilter(R2B(dp, 2,14),R2B(dp, 3,14),R2B(dp, 4,14),R2B(dp, 5,14),
                        R2B(dp, 2,15),R2B(dp, 3,15),R2B(dp, 4,15),R2B(dp, 5,15),
                        R2B(up, 2, 0),R2B(up, 3, 0),R2B(up, 4, 0),R2B(up, 5, 0),
                        R2B(up, 2, 1),R2B(up, 3, 1),R2B(up, 4, 1),R2B(up, 5, 1));
                    _jxr_4x4PreFilter(R2B(dp, 6,14),R2B(dp, 7,14),R2B(dp, 8,14),R2B(dp, 9,14),
                        R2B(dp, 6,15),R2B(dp, 7,15),R2B(dp, 8,15),R2B(dp, 9,15),
                        R2B(up, 6, 0),R2B(up, 7, 0),R2B(up, 8, 0),R2B(up, 9, 0),
                        R2B(up, 6, 1),R2B(up, 7, 1),R2B(up, 8, 1),R2B(up, 9, 1));
                    _jxr_4x4PreFilter(R2B(dp,10,14),R2B(dp,11,14),R2B(dp,12,14),R2B(dp,13,14),
                        R2B(dp,10,15),R2B(dp,11,15),R2B(dp,12,15),R2B(dp,13,15),
                        R2B(up,10, 0),R2B(up,11, 0),R2B(up,12, 0),R2B(up,13, 0),
                        R2B(up,10, 1),R2B(up,11, 1),R2B(up,12, 1),R2B(up,13, 1));
                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    ) {
                        /* Blocks that span the MB to the right */
                        int*dn = MACROBLK_UP2(image,ch,tx,idx+1).data;
                        int*un = MACROBLK_UP3(image,ch,tx,idx+1).data;

                        /* 4x4 on right, below, below-right */
                        _jxr_4x4PreFilter(R2B(dp,14,14),R2B(dp,15,14),R2B(dn, 0,14),R2B(dn, 1,14),
                            R2B(dp,14,15),R2B(dp,15,15),R2B(dn, 0,15),R2B(dn, 1,15),
                            R2B(up,14, 0),R2B(up,15, 0),R2B(un, 0, 0),R2B(un, 1, 0),
                            R2B(up,14, 1),R2B(up,15, 1),R2B(un, 0, 1),R2B(un, 1, 1));
                }
                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    /* Across vertical blocks, right edge */
                    _jxr_4PreFilter(R2B(dp,14,14),R2B(dp,14,15),R2B(up,14,0),R2B(up,14,1));
                    _jxr_4PreFilter(R2B(dp,15,14),R2B(dp,15,15),R2B(up,15,0),R2B(up,15,1));
                }
            }
        }
    }
}

static void first_prefilter422_up2(jxr_image_t image, int ch, int ty)
{
    assert(ch > 0 && image->use_clr_fmt == 2/*YUV422*/);
    uint32_t tx = 0; /* XXXX */

    uint32_t top_my = image->cur_my + 2;
    uint32_t idx;

    if (top_my >= image->tile_row_height[ty])
        top_my -= image->tile_row_height[ty++];
    if (top_my >= image->tile_row_height[ty])
        top_my -= image->tile_row_height[ty++];
    top_my += image->tile_row_position[ty];

    DEBUG("Pre Level2 for row %d\n", top_my);

    for(tx = 0; tx < image->tile_columns; tx++)
    {
        /* Left edge */
        if (tx == 0 || image->disableTileOverlapFlag)
        {
            int*dp = MACROBLK_UP2(image,ch,tx,0).data;
            _jxr_4PreFilter(R2B42(dp,0, 2),R2B42(dp,0, 3),R2B42(dp,0, 4),R2B42(dp,0, 5));
            _jxr_4PreFilter(R2B42(dp,0, 6),R2B42(dp,0, 7),R2B42(dp,0, 8),R2B42(dp,0, 9));
            _jxr_4PreFilter(R2B42(dp,0,10),R2B42(dp,0,11),R2B42(dp,0,12),R2B42(dp,0,13));

            _jxr_4PreFilter(R2B42(dp,1, 2),R2B42(dp,1, 3),R2B42(dp,1, 4),R2B42(dp,1, 5));
            _jxr_4PreFilter(R2B42(dp,1, 6),R2B42(dp,1, 7),R2B42(dp,1, 8),R2B42(dp,1, 9));
            _jxr_4PreFilter(R2B42(dp,1,10),R2B42(dp,1,11),R2B42(dp,1,12),R2B42(dp,1,13));
        }

        /* Right edge */
        if(tx == image->tile_columns -1 || image->disableTileOverlapFlag){

            int*dp = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
            _jxr_4PreFilter(R2B42(dp,6,2),R2B42(dp,6,3),R2B42(dp,6,4),R2B42(dp,6,5));
            _jxr_4PreFilter(R2B42(dp,7,2),R2B42(dp,7,3),R2B42(dp,7,4),R2B42(dp,7,5));

            _jxr_4PreFilter(R2B42(dp,6,6),R2B42(dp,6,7),R2B42(dp,6,8),R2B42(dp,6,9));
            _jxr_4PreFilter(R2B42(dp,7,6),R2B42(dp,7,7),R2B42(dp,7,8),R2B42(dp,7,9));

            _jxr_4PreFilter(R2B42(dp,6,10),R2B42(dp,6,11),R2B42(dp,6,12),R2B42(dp,6,13));
            _jxr_4PreFilter(R2B42(dp,7,10),R2B42(dp,7,11),R2B42(dp,7,12),R2B42(dp,7,13));
        }

        /* Top edge */
        if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my) ))
        {
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*dp = MACROBLK_UP2(image,ch,tx,idx).data;

                _jxr_4PreFilter(R2B42(dp, 2,0),R2B42(dp, 3,0),R2B42(dp, 4,0),R2B42(dp, 5,0));
                _jxr_4PreFilter(R2B42(dp, 2,1),R2B42(dp, 3,1),R2B42(dp, 4,1),R2B42(dp, 5,1));

                /* Top across for soft tiles */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                    int*pp = MACROBLK_UP2(image,ch,tx,idx-1).data;
                    _jxr_4PreFilter(R2B42(pp,6,0),R2B42(pp,7,0),R2B(dp,0,0),R2B42(dp,1,0));
                    _jxr_4PreFilter(R2B42(pp,6,1),R2B42(pp,7,1),R2B(dp,0,1),R2B42(dp,1,1));
                }
            }

            /* Top left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch, tx, 0).data;
                _jxr_4PreFilter(R2B42(dp,0,0),R2B42(dp,1,0),R2B42(dp,0,1),R2B42(dp,1,1));
            }
            /* Top right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4PreFilter(R2B42(dp,6,0),R2B42(dp,7,0),R2B42(dp,6,1),R2B42(dp,7,1));
            }
        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. finish up with 4Pre filters. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*tp = MACROBLK_UP2(image,ch,tx,idx).data;

                _jxr_4PreFilter(R2B42(tp,2,14),R2B42(tp,3,14),R2B42(tp,4,14),R2B42(tp,5,14));
                _jxr_4PreFilter(R2B42(tp,2,15),R2B42(tp,3,15),R2B42(tp,4,15),R2B42(tp,5,15));

                /* Bottom across for soft tiles */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                        /* Blocks that span the MB to the right */
                        int*tn = MACROBLK_UP2(image,ch,tx,idx-1).data;
                        _jxr_4PreFilter(R2B42(tn,6,14),R2B42(tn,7,14),R2B42(tp,0,14),R2B42(tp,1,14));
                        _jxr_4PreFilter(R2B42(tn,6,15),R2B42(tn,7,15),R2B42(tp,0,15),R2B42(tp,1,15));
                }
            }

            /* Bottom left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch,tx,0).data;
                _jxr_4PreFilter(R2B42(dp,0,14),R2B42(dp,1,14),R2B42(dp,0,15),R2B42(dp,1,15));
            }
            /* Bottom right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4PreFilter(R2B42(dp,6,14),R2B42(dp,7,14),R2B42(dp,6,15),R2B42(dp,7,15));
            }
        }

        for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {

            int*dp = MACROBLK_UP2(image,ch,tx,idx).data;

            /* Fully interior 4x4 filter blocks... */
            _jxr_4x4PreFilter(R2B42(dp,2,2),R2B42(dp,3,2),R2B42(dp,4,2),R2B42(dp,5,2),
                R2B42(dp,2,3),R2B42(dp,3,3),R2B42(dp,4,3),R2B42(dp,5,3),
                R2B42(dp,2,4),R2B42(dp,3,4),R2B42(dp,4,4),R2B42(dp,5,4),
                R2B42(dp,2,5),R2B42(dp,3,5),R2B42(dp,4,5),R2B42(dp,5,5));

            _jxr_4x4PreFilter(R2B42(dp,2,6),R2B42(dp,3,6),R2B42(dp,4,6),R2B42(dp,5,6),
                R2B42(dp,2,7),R2B42(dp,3,7),R2B42(dp,4,7),R2B42(dp,5,7),
                R2B42(dp,2,8),R2B42(dp,3,8),R2B42(dp,4,8),R2B42(dp,5,8),
                R2B42(dp,2,9),R2B42(dp,3,9),R2B42(dp,4,9),R2B42(dp,5,9));

            _jxr_4x4PreFilter(R2B42(dp,2,10),R2B42(dp,3,10),R2B42(dp,4,10),R2B42(dp,5,10),
                R2B42(dp,2,11),R2B42(dp,3,11),R2B42(dp,4,11),R2B42(dp,5,11),
                R2B42(dp,2,12),R2B42(dp,3,12),R2B42(dp,4,12),R2B42(dp,5,12),
                R2B42(dp,2,13),R2B42(dp,3,13),R2B42(dp,4,13),R2B42(dp,5,13));

            if ( (image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                (image->disableTileOverlapFlag && !RIGHT_X(idx))) {
                    /* Blocks that span the MB to the right */
                    int*np = MACROBLK_UP2(image,ch,tx,idx+1).data;
                    _jxr_4x4PreFilter(R2B42(dp,6,2),R2B42(dp,7,2),R2B42(np,0,2),R2B42(np,1,2),
                        R2B42(dp,6,3),R2B42(dp,7,3),R2B42(np,0,3),R2B42(np,1,3),
                        R2B42(dp,6,4),R2B42(dp,7,4),R2B42(np,0,4),R2B42(np,1,4),
                        R2B42(dp,6,5),R2B42(dp,7,5),R2B42(np,0,5),R2B42(np,1,5));

                    _jxr_4x4PreFilter(R2B42(dp,6,6),R2B42(dp,7,6),R2B42(np,0,6),R2B42(np,1,6),
                        R2B42(dp,6,7),R2B42(dp,7,7),R2B42(np,0,7),R2B42(np,1,7),
                        R2B42(dp,6,8),R2B42(dp,7,8),R2B42(np,0,8),R2B42(np,1,8),
                        R2B42(dp,6,9),R2B42(dp,7,9),R2B42(np,0,9),R2B42(np,1,9));

                    _jxr_4x4PreFilter(R2B42(dp,6,10),R2B42(dp,7,10),R2B42(np,0,10),R2B42(np,1,10),
                        R2B42(dp,6,11),R2B42(dp,7,11),R2B42(np,0,11),R2B42(np,1,11),
                        R2B42(dp,6,12),R2B42(dp,7,12),R2B42(np,0,12),R2B42(np,1,12),
                        R2B42(dp,6,13),R2B42(dp,7,13),R2B42(np,0,13),R2B42(np,1,13));
            }

            if ((top_my+1) < EXTENDED_HEIGHT_BLOCKS(image)) {

                /* Blocks that MB below */
                int*dp = MACROBLK_UP2(image,ch,tx,idx).data;
                int*up = MACROBLK_UP3(image,ch,tx,idx).data;

                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        _jxr_4PreFilter(R2B42(dp,0,14),R2B42(dp,0,15),R2B42(up,0,0),R2B42(up,0,1));
                        _jxr_4PreFilter(R2B42(dp,1,14),R2B42(dp,1,15),R2B42(up,1,0),R2B42(up,1,1));
                }
                if((!image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !BOTTOM_Y(top_my)))
                {
                    _jxr_4x4PreFilter(R2B42(dp,2,14),R2B42(dp,3,14),R2B42(dp,4,14),R2B42(dp,5,14),
                        R2B42(dp,2,15),R2B42(dp,3,15),R2B42(dp,4,15),R2B42(dp,5,15),
                        R2B42(up,2, 0),R2B42(up,3, 0),R2B42(up,4, 0),R2B42(up,5, 0),
                        R2B42(up,2, 1),R2B42(up,3, 1),R2B42(up,4, 1),R2B42(up,5, 1));

                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    ) {
                        /* Blocks that span the MB to the right, below, below-right */
                        int*dn = MACROBLK_UP2(image,ch,tx,idx+1).data;
                        int*un = MACROBLK_UP3(image,ch,tx,idx+1).data;

                        _jxr_4x4PreFilter(R2B42(dp,6,14),R2B42(dp,7,14),R2B42(dn,0,14),R2B42(dn,1,14),
                            R2B42(dp,6,15),R2B42(dp,7,15),R2B42(dn,0,15),R2B42(dn,1,15),
                            R2B42(up,6, 0),R2B42(up,7, 0),R2B42(un,0, 0),R2B42(un,1, 0),
                            R2B42(up,6, 1),R2B42(up,7, 1),R2B42(un,0, 1),R2B42(un,1, 1));
                }
                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    _jxr_4PreFilter(R2B42(dp,6,14),R2B42(dp,6,15),R2B42(up,6,0),R2B42(up,6,1));
                    _jxr_4PreFilter(R2B42(dp,7,14),R2B42(dp,7,15),R2B42(up,7,0),R2B42(up,7,1));
                }
            }
        }
    }
}

static void first_prefilter420_up2(jxr_image_t image, int ch, int ty)
{
    assert(ch > 0 && image->use_clr_fmt == 1/*YUV420*/);
    uint32_t tx = 0; /* XXXX */
    uint32_t top_my = image->cur_my + 2;
    uint32_t idx;

    if (top_my >= image->tile_row_height[ty])
        top_my -= image->tile_row_height[ty++];
    if (top_my >= image->tile_row_height[ty])
        top_my -= image->tile_row_height[ty++];
    top_my += image->tile_row_position[ty];

    DEBUG("Pre Level2 (YUV420) for row %d\n", top_my);

    for(tx = 0; tx < image->tile_columns; tx++)
    {

        /* Left edge */
        if (tx == 0 || image->disableTileOverlapFlag)
        {
            int*dp = MACROBLK_UP2(image,ch,tx,0).data;
            _jxr_4PreFilter(R2B42(dp,0,2),R2B42(dp,0,3),R2B42(dp,0,4),R2B42(dp,0,5));
            _jxr_4PreFilter(R2B42(dp,1,2),R2B42(dp,1,3),R2B42(dp,1,4),R2B42(dp,1,5));
        }

        /* Right edge */
        if(tx == image->tile_columns -1 || image->disableTileOverlapFlag){
            int*dp = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
            _jxr_4PreFilter(R2B42(dp,6,2),R2B42(dp,6,3),R2B42(dp,6,4),R2B42(dp,6,5));
            _jxr_4PreFilter(R2B42(dp,7,2),R2B42(dp,7,3),R2B42(dp,7,4),R2B42(dp,7,5));
        }

        /* Top edge */
        if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my) ))
        {
            /* If this is the very first strip of blocks, then process the
            first two scan lines with the smaller 4Pre filter. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*dp = MACROBLK_UP2(image,ch,tx,idx).data;
                _jxr_4PreFilter(R2B42(dp, 2,0),R2B42(dp, 3,0),R2B42(dp, 4,0),R2B42(dp, 5,0));
                _jxr_4PreFilter(R2B42(dp, 2,1),R2B42(dp, 3,1),R2B42(dp, 4,1),R2B42(dp, 5,1));
                /* Top edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                    int*pp = MACROBLK_UP2(image,ch,tx,idx-1).data;
                    _jxr_4PreFilter(R2B42(pp,6,0),R2B42(pp,7,0),R2B(dp,0,0),R2B42(dp,1,0));
                    _jxr_4PreFilter(R2B42(pp,6,1),R2B42(pp,7,1),R2B(dp,0,1),R2B42(dp,1,1));
                }
            }

            /* Top left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch,tx,0).data;
                _jxr_4PreFilter(R2B42(dp, 0,0),R2B42(dp, 1, 0),R2B42(dp, 0 ,1),R2B42(dp, 1,1));
            }
            /* Top right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4PreFilter(R2B42(dp, 6,0),R2B42(dp, 7,0),R2B42(dp, 6,1),R2B42(dp, 7,1));;
            }

        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. finish up with 4Pre filters. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*tp = MACROBLK_UP2(image,ch,tx,idx).data;

                _jxr_4PreFilter(R2B42(tp,2,6),R2B42(tp,3,6),R2B42(tp,4,6),R2B42(tp,5,6));
                _jxr_4PreFilter(R2B42(tp,2,7),R2B42(tp,3,7),R2B42(tp,4,7),R2B42(tp,5,7));


                /* Bottom edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                        int*tn = MACROBLK_UP2(image,ch,tx,idx-1).data;
                        _jxr_4PreFilter(R2B42(tn,6,6),R2B42(tn,7,6),R2B42(tp,0,6),R2B42(tp,1,6));
                        _jxr_4PreFilter(R2B42(tn,6,7),R2B42(tn,7,7),R2B42(tp,0,7),R2B42(tp,1,7));
                }
            }

            /* Bottom left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch,tx,0).data;
                _jxr_4PreFilter(R2B42(dp, 0,6),R2B42(dp, 1, 6),R2B42(dp, 0,7),R2B42(dp, 1, 7));
            }

            /* Bottom right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP2(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4PreFilter(R2B42(dp, 6, 6),R2B42(dp, 7, 6),R2B42(dp, 6, 7),R2B42(dp, 7, 7));
            }
        }

        for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {

            int*dp = MACROBLK_UP2(image,ch,tx,idx).data;
#ifndef JPEGXR_ADOBE_EXT
            int*up = MACROBLK_UP3(image,ch,tx,idx).data;
#endif //#ifndef JPEGXR_ADOBE_EXT

            /* Fully interior 4x4 filter blocks... */
            _jxr_4x4PreFilter(R2B42(dp,2,2),R2B42(dp,3,2),R2B42(dp,4,2),R2B42(dp,5,2),
                R2B42(dp,2,3),R2B42(dp,3,3),R2B42(dp,4,3),R2B42(dp,5,3),
                R2B42(dp,2,4),R2B42(dp,3,4),R2B42(dp,4,4),R2B42(dp,5,4),
                R2B42(dp,2,5),R2B42(dp,3,5),R2B42(dp,4,5),R2B42(dp,5,5));

            if ( (image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                (image->disableTileOverlapFlag && !RIGHT_X(idx)))
            {
                /* 4x4 at the right */
                int*np = MACROBLK_UP2(image,ch,tx,idx+1).data;

                _jxr_4x4PreFilter(R2B42(dp,6,2),R2B42(dp,7,2),R2B42(np,0,2),R2B42(np,1,2),
                    R2B42(dp,6,3),R2B42(dp,7,3),R2B42(np,0,3),R2B42(np,1,3),
                    R2B42(dp,6,4),R2B42(dp,7,4),R2B42(np,0,4),R2B42(np,1,4),
                    R2B42(dp,6,5),R2B42(dp,7,5),R2B42(np,0,5),R2B42(np,1,5));
            }

            if ((top_my+1) < EXTENDED_HEIGHT_BLOCKS(image)) {

                int*dp = MACROBLK_UP2(image,ch,tx,idx).data;
                int*up = MACROBLK_UP3(image,ch,tx,idx).data;

                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        /* Across vertical blocks, left edge */
                        _jxr_4PreFilter(R2B42(dp,0,6),R2B42(dp,0,7),R2B42(up,0,0),R2B42(up,0,1));
                        _jxr_4PreFilter(R2B42(dp,1,6),R2B42(dp,1,7),R2B42(up,1,0),R2B42(up,1,1));
                }
                if((!image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !BOTTOM_Y(top_my)))
                {
                    /* 4x4 straddling lower MB */
                    _jxr_4x4PreFilter(R2B42(dp,2,6),R2B42(dp,3,6),R2B42(dp,4,6),R2B42(dp,5,6),
                        R2B42(dp,2,7),R2B42(dp,3,7),R2B42(dp,4,7),R2B42(dp,5,7),
                        R2B42(up,2,0),R2B42(up,3,0),R2B42(up,4,0),R2B42(up,5,0),
                        R2B42(up,2,1),R2B42(up,3,1),R2B42(up,4,1),R2B42(up,5,1));
                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    ) {
                        /* Blocks that span the MB to the right */
                        int*dn = MACROBLK_UP2(image,ch,tx,idx+1).data;
                        int*un = MACROBLK_UP3(image,ch,tx,idx+1).data;

                        /* 4x4 right, below, below-right */
                        _jxr_4x4PreFilter(R2B42(dp,6,6),R2B42(dp,7,6),R2B42(dn,0,6),R2B42(dn,1,6),
                            R2B42(dp,6,7),R2B42(dp,7,7),R2B42(dn,0,7),R2B42(dn,1,7),
                            R2B42(up,6,0),R2B42(up,7,0),R2B42(un,0,0),R2B42(un,1,0),
                            R2B42(up,6,1),R2B42(up,7,1),R2B42(un,0,1),R2B42(un,1,1));
                }
                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    /* Across vertical blocks, right edge */
                    _jxr_4PreFilter(R2B42(dp,6,6),R2B42(dp,6,7),R2B42(up,6,0),R2B42(up,6,1));
                    _jxr_4PreFilter(R2B42(dp,7,6),R2B42(dp,7,7),R2B42(up,7,0),R2B42(up,7,1));
                }
            }
        }
    }
}

static void first_prefilter_up2(jxr_image_t image, int ty)
{
    int ch;
    first_prefilter444_up2(image, 0, ty);
    switch (image->use_clr_fmt) {
        case 0:
            assert(image->num_channels == 1);
            break;
        case 1:/*YUV420*/
            first_prefilter420_up2(image, 1, ty);
            first_prefilter420_up2(image, 2, ty);
            break;
        case 2:/*YUV422*/
            first_prefilter422_up2(image, 1, ty);
            first_prefilter422_up2(image, 2, ty);
            break;
        default:
            for (ch = 1 ; ch < image->num_channels ; ch += 1)
                first_prefilter444_up2(image, ch, ty);
            break;
    }
}

static void PCT_stage2_up1(jxr_image_t image, int ch, int ty)
{
    uint32_t tx = 0;
    uint32_t use_my = image->cur_my + 1;

    /* make adjustments based on tiling */
    if (use_my == image->tile_row_height[ty]) {
        ty += 1;
        use_my = 0;
    }

    int dclp_count = 16;
    if (ch>0 && image->use_clr_fmt == 2/*YUV422*/)
        dclp_count = 8;
    else if (ch>0 && image->use_clr_fmt == 1/*YUV420*/)
        dclp_count = 4;

    int dc_quant = w_guess_dc_quant(image, ch, tx, ty);
    dc_quant = _jxr_quant_map(image, dc_quant, ch==0? 1 : 0/* iShift for YONLY */);
    assert(dc_quant > 0);

    int mx;
    for (mx = 0 ; mx < (int) EXTENDED_WIDTH_BLOCKS(image) ; mx += 1) {

        if (ch > 0 && image->use_clr_fmt == 1/*YUV420*/) {

            /* Scale up the chroma channel */
            if (image->scaled_flag) {
                int jdx;
                for (jdx = 0 ; jdx < 4 ; jdx += 1) {
                    int val = image->strip[ch].up1[mx].data[jdx];
                    val = _jxr_floor_div2(val);
                    image->strip[ch].up1[mx].data[jdx] = val;
                }
            }

            _jxr_InvPermute2pt(image->strip[ch].up1[mx].data+1,
                image->strip[ch].up1[mx].data+2);
            _jxr_2x2IPCT(image->strip[ch].up1[mx].data+0);

        } else if (ch > 0 && image->use_clr_fmt == 2/*YUV422*/) {

#if defined(DETAILED_DEBUG) && 1
            DEBUG(" DC/LP scaled_flag=%d\n", image->scaled_flag);
            { int jdx;
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) Pre-PCT:", use_my, mx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,tx,mx));
            for (jdx = 0; jdx < 7 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,tx,mx,jdx));
                if ((jdx+1)%4 == 3 && jdx != 6)
                    DEBUG("\n%*s:", 43, "");
            }
            DEBUG("\n");
            }
#endif
            /* Scale up the chroma channel */
            if (image->scaled_flag) {
                int jdx;
                for (jdx = 0 ; jdx < 8 ; jdx += 1) {
                    int val = image->strip[ch].up1[mx].data[jdx];
                    val = _jxr_floor_div2(val);
                    image->strip[ch].up1[mx].data[jdx] = val;
                }
            }
#if defined(DETAILED_DEBUG) && 1
            { int jdx;
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) scaled :", use_my, mx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,tx,mx));
            for (jdx = 0; jdx < 7 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,tx,mx,jdx));
                if ((jdx+1)%4 == 3 && jdx != 6)
                    DEBUG("\n%*s:", 43, "");
            }
            DEBUG("\n");
            }
#endif

            /* The InvPermute2pt and FwdPermute2pt are identical */
            _jxr_InvPermute2pt(image->strip[ch].up1[mx].data+1,
                image->strip[ch].up1[mx].data+2);
            _jxr_InvPermute2pt(image->strip[ch].up1[mx].data+5,
                image->strip[ch].up1[mx].data+6);
            /* The 2x2PCT and 2x2IPCT are identical */
            _jxr_2x2IPCT(image->strip[ch].up1[mx].data+0);
            _jxr_2x2IPCT(image->strip[ch].up1[mx].data+4);

            _jxr_2ptFwdT(image->strip[ch].up1[mx].data+0,
                image->strip[ch].up1[mx].data+4);

#if defined(DETAILED_DEBUG) && 1
            DEBUG(" DC/LP scaled_flag=%d\n", image->scaled_flag);
            { int jdx;
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) Post-PCT:", use_my, mx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,tx,mx));
            for (jdx = 0; jdx < 7 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,tx,mx,jdx));
                if ((jdx+1)%4 == 3 && jdx != 6)
                    DEBUG("\n%*s:", 44, "");
            }
            DEBUG("\n");
            }
#endif

        } else {
#if defined(DETAILED_DEBUG) && 1
            { int jdx;
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) Pre-PCT:", use_my, mx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,tx,mx));
            for (jdx = 0; jdx < 15 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,tx,mx,jdx));
                if ((jdx+1)%4 == 3 && jdx != 14)
                    DEBUG("\n%*s:", 43, "");
            }
            DEBUG("\n");
            }
#endif
            /* Scale up the chroma channel */
            if (ch > 0 && image->scaled_flag) {
                int jdx;
                for (jdx = 0 ; jdx < 16 ; jdx += 1) {
                    int val = image->strip[ch].up1[mx].data[jdx];
                    val = _jxr_floor_div2(val);
                    image->strip[ch].up1[mx].data[jdx] = val;
                }
            }

            _jxr_4x4PCT(image->strip[ch].up1[mx].data);

#if defined(DETAILED_DEBUG)
            { int jdx;
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) post-PCT:", use_my, mx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,0,mx));
            for (jdx = 0; jdx < 15 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,tx,mx,jdx));
                if ((jdx+1)%4 == 3 && jdx != 14)
                    DEBUG("\n%*s:", 44, "");
            }
            DEBUG("\n");
            }
#endif
        }

        /* Quantize */
        int lp_quant = w_guess_lp_quant(image, ch, tx, ty, mx, use_my);
        MACROBLK_UP1_LP_QUANT(image,ch,tx,mx) = lp_quant;
        lp_quant = _jxr_quant_map(image, lp_quant, ch==0? 1 : 0/* iShift for YONLY */);
        assert(lp_quant > 0);

        DEBUG(" DC-LP (mx=%d ch=%d) use_dc_quant=%d, use_lp_quant=%d\n",
            mx, ch, dc_quant, lp_quant);

        CHECK1(image->lwf_test, MACROBLK_CUR_DC(image,ch,tx,mx));
        MACROBLK_UP_DC(image,ch,tx,mx) = quantize_dc(MACROBLK_UP_DC(image,ch,tx,mx), dc_quant);
        int jdx;
        for (jdx = 1 ; jdx < dclp_count ; jdx += 1) {
            CHECK1(image->lwf_test, MACROBLK_UP_LP(image,ch,tx,mx,jdx-1));
            int value = MACROBLK_UP_LP(image,ch,tx,mx,jdx-1);
            MACROBLK_UP_LP(image,ch,tx,mx,jdx-1) = quantize_lphp(value, lp_quant);
        }
#if defined(DETAILED_DEBUG)
        { int jdx;
        DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) post-QP:", use_my, mx, ch);
        DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,tx,mx));
        for (jdx = 0; jdx < dclp_count-1 ; jdx += 1) {
            DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,tx,mx,jdx));
            if ((jdx+1)%4 == 3 && jdx != 14)
                DEBUG("\n%*s:", 43, "");
        }
        DEBUG("\n");
        }
#endif
    }
}

static void second_prefilter444_up1(jxr_image_t image, int ch, int ty)
{
    assert(ch == 0 || (image->use_clr_fmt != 2/*YUV422*/ && image->use_clr_fmt !=1/* YUV420*/));
    uint32_t tx = 0; /* XXXX */
    uint32_t top_my = image->cur_my + 1;
    uint32_t idx;

    if (top_my >= image->tile_row_height[ty])
        top_my -= image->tile_row_height[ty++];
    top_my += image->tile_row_position[ty];

    DEBUG("Pre Level1 (YUV444) for row %d\n", top_my);

    for(tx = 0; tx < image->tile_columns; tx++)
    {
        /* Top edge */
        if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my) ))
        {
            /* If this is the very first strip of blocks, then process the
            first two scan lines with the smaller 4Pre filter. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                /* Top edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                    int*tp0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                    int*tp1 = MACROBLK_UP1(image,ch,tx,idx-1).data; /* Macroblock to the right */

                    _jxr_4PreFilter(tp1+2, tp1+3, tp0+0, tp0+1);
                    _jxr_4PreFilter(tp1+6, tp1+7, tp0+4, tp0+5);
                }
            }
            /* Top left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                _jxr_4PreFilter(tp0+0, tp0+1, tp0+4, tp0+5);
            }
            /* Top right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                _jxr_4PreFilter(tp0+2, tp0+3, tp0+6, tp0+7);
            }
        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. finish up with 4Pre filters. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                /* Bottom edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {

                        int*tp0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                        int*tp1 = MACROBLK_UP1(image,ch,tx,idx-1).data;
                        _jxr_4PreFilter(tp1+10, tp1+11, tp0+8, tp0+9);
                        _jxr_4PreFilter(tp1+14, tp1+15, tp0+12, tp0+13);
                }
            }

            /* Bottom left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                _jxr_4PreFilter(tp0+8, tp0+9, tp0+12, tp0+13);
            }
            /* Bottom right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                _jxr_4PreFilter(tp0+10, tp0+11, tp0+14, tp0+15);
            }
        }

        for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {
            if ((top_my+1) < EXTENDED_HEIGHT_BLOCKS(image)) {

                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                        int*up0 = MACROBLK_UP2(image,ch,tx,0).data;

                        /* Left edge Across Vertical MBs */
                        _jxr_4PreFilter(tp0+8, tp0+12, up0+0, up0+4);
                        _jxr_4PreFilter(tp0+9, tp0+13, up0+1, up0+5);
                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    ) {
                        /* This assumes that the DCLP coefficients are the first
                        16 values in the array, and ordered properly. */
                        int*tp0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                        int*tp1 = MACROBLK_UP1(image,ch,tx,idx+1).data;
                        int*up0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                        int*up1 = MACROBLK_UP2(image,ch,tx,idx+1).data;

                        /* MB below, right, right-below */
                        _jxr_4x4PreFilter(tp0+10, tp0+11, tp1+ 8, tp1+ 9,
                            tp0+14, tp0+15, tp1+12, tp1+13,
                            up0+ 2, up0+ 3, up1+ 0, up1+ 1,
                            up0+ 6, up0+ 7, up1+ 4, up1+ 5);
                }
                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    int*tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                    int*up0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;

                    /* Right edge Across Vertical MBs */
                    _jxr_4PreFilter(tp0+10, tp0+14, up0+2, up0+6);
                    _jxr_4PreFilter(tp0+11, tp0+15, up0+3, up0+7);
                }
            }
        }
    }
}


static void second_prefilter422_up1(jxr_image_t image, int ch, int ty)
{
    assert(ch > 0);
    uint32_t tx = 0; /* XXXX */
    uint32_t top_my = image->cur_my + 1;
    uint32_t idx;

    if (top_my >= image->tile_row_height[ty])
        top_my -= image->tile_row_height[ty++];
    top_my += image->tile_row_position[ty];

    DEBUG("Pre Level2 (YUV422) for row %d\n", top_my);

    /* Top edge */
    if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my)))
    {
        for(tx = 0; tx < image->tile_columns; tx++)
        {
            /* Top Left Corner Difference */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                tp0[0] = tp0[0] - tp0[1];
                CHECK1(image->lwf_test, tp0[0]);
            }
            /* Top Right Corner Difference */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                tp0[1] = tp0[1] - tp0[0];
                CHECK1(image->lwf_test, tp0[1]);
            }
        }
    }


    /* Bottom edge */
    if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my)))
    {
        for(tx = 0; tx < image->tile_columns; tx++)
        {
            /* Bottom Left Corner Difference */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                tp0[6] = tp0[6] - tp0[7];
                CHECK1(image->lwf_test, tp0[6]);
            }
            /* Bottom Right Corner Difference */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                tp0[7] = tp0[7] - tp0[6];
                CHECK1(image->lwf_test, tp0[7]);
            }
        }
    }

    for(tx = 0; tx < image->tile_columns; tx++)
    {
        /* Left edge */
        if (tx == 0 || image->disableTileOverlapFlag)
        {
            /* Interior left edge */
            int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
            _jxr_2PreFilter(tp0+2, tp0+4);
        }

        /* Right edge */
        if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
        {
            int*tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
            /* Interior Right edge */
            _jxr_2PreFilter(tp0+3, tp0+5);
        }


        /* Top edge */
        if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my) ))
        {
            /* If this is the very first strip of blocks, then process the
            first two scan lines with the smaller 4Pre filter. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                /* Top edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                    int*tp0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                    int*tp1 = MACROBLK_UP1(image,ch,tx,idx-1).data; /* The macroblock to the right */

                    _jxr_2PreFilter(tp1+1, tp0+0);
                }
            }
        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. finish up with 4Pre filters. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                /* Bottom edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                        int*tp0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                        int*tp1 = MACROBLK_UP1(image,ch,tx,idx - 1).data;
                        _jxr_2PreFilter(tp1+7, tp0+6);
                }
            }
        }

        for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {
            if(top_my< EXTENDED_HEIGHT_BLOCKS(image) -1)
            {
                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        /* Across vertical blocks, left edge */
                        int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                        int*up0 = MACROBLK_UP2(image,ch,tx,0).data;

                        /* Left edge across vertical MBs */
                        _jxr_2PreFilter(tp0+6, up0+0);
                }

                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    int*tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                    int*up0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;

                    /* Right edge across MBs */
                    _jxr_2PreFilter(tp0+7, up0+1);
                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    )
                {
                    /* This assumes that the DCLP coefficients are the first
                    16 values in the array, and ordered properly. */
                    int*tp0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                    int*tp1 = MACROBLK_UP1(image,ch,tx,idx+1).data;
                    int*up0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                    int*up2 = MACROBLK_UP2(image,ch,tx,idx+1).data;

                    /* MB below, right, right-below */
                    _jxr_2x2PreFilter(tp0+7, tp1+6, up0+1, up2+0);
                }
            }

            if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                ( image->disableTileOverlapFlag && !RIGHT_X(idx) )
                )
            {
                /* This assumes that the DCLP coefficients are the first
                16 values in the array, and ordered properly. */
                int*tp0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                int*tp1 = MACROBLK_UP1(image,ch,tx,idx+1).data;

                /* MB to the right */
                _jxr_2x2PreFilter(tp0+3, tp1+2, tp0+5, tp1+4);
            }
        }
    }

    /* Top edge */
    if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my)))
    {
        for(tx = 0; tx < image->tile_columns; tx++)
        {
            /* Top Left Corner Addition */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                tp0[0] = tp0[0] + tp0[1];
                CHECK1(image->lwf_test, tp0[0]);
            }
            /* Top Right Corner Addition */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                tp0[1] = tp0[1] + tp0[0];
                CHECK1(image->lwf_test, tp0[1]);
            }
        }
    }

    /* Bottom edge */
    if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my)))
    {
        for(tx = 0; tx < image->tile_columns; tx++)
        {
            /* Bottom Left Corner Addition */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                tp0[6] = tp0[6] + tp0[7];
                CHECK1(image->lwf_test, tp0[6]);
            }
            /* Bottom Right Corner Addition */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                tp0[7] = tp0[7] + tp0[6];
                CHECK1(image->lwf_test, tp0[7]);
            }
        }
    }
}

static void second_prefilter420_up1(jxr_image_t image, int ch, int ty)
{
    assert(ch > 0);
    uint32_t tx = 0; /* XXXX */
    uint32_t top_my = image->cur_my + 1;
    uint32_t idx;

    if (top_my >= image->tile_row_height[ty])
        top_my -= image->tile_row_height[ty++];
    top_my += image->tile_row_position[ty];

    DEBUG("Pre Level2 (YUV420) for row %d\n", top_my);

    if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my)))
    {
        for(tx = 0; tx < image->tile_columns; tx++)
        {
            /* Top Left Corner Difference*/
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                tp0[0] = tp0[0] - tp0[1];
                CHECK1(image->lwf_test, tp0[0]);
            }
            /* Top Right Corner Difference*/
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                tp0[1] = tp0[1] - tp0[0];
                CHECK1(image->lwf_test, tp0[1]);
            }
        }
    }

    /* Bottom edge */
    if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my)))
    {
        for(tx = 0; tx < image->tile_columns; tx++)
        {
            /* Bottom Left Corner Difference*/
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                tp0[2] = tp0[2] - tp0[3];
                CHECK1(image->lwf_test, tp0[2]);
            }
            /* Bottom Right Corner Difference*/
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                tp0[3] = tp0[3] - tp0[2];
                CHECK1(image->lwf_test, tp0[3]);
            }
        }
    }

    for(tx = 0; tx < image->tile_columns; tx++)
    {
        /* Top edge */
        if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my)))
        {
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                /* Top edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                    int*tp0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                    int*tp1 = MACROBLK_UP1(image,ch,tx,idx-1).data;
                    _jxr_2PreFilter(tp1+1, tp0+0);
                }
            }
        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                /* Bottom edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                        int*tp0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                        int*tp1 = MACROBLK_UP1(image,ch,tx,idx-1).data;
                        _jxr_2PreFilter(tp1+3, tp0+2);
                }
            }
        }
        else
        {
            for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {

                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        /* Left edge across vertical MBs */
                        int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                        int*up0 = MACROBLK_UP2(image,ch,tx,0).data;

                        _jxr_2PreFilter(tp0+2, up0+0);
                }

                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    /* Right edge across vertical MBs */
                    int*tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                    int*up0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;

                    _jxr_2PreFilter(tp0+3, up0+1);
                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    )
                {
                    /* This assumes that the DCLP coefficients are the first
                    16 values in the array, and ordered properly. */
                    /* MB below, right, right-below */
                    int*tp0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                    int*tp1 = MACROBLK_UP1(image,ch,tx,idx+1).data;
                    int*up0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                    int*up2 = MACROBLK_UP2(image,ch,tx,idx+1).data;

                    _jxr_2x2PreFilter(tp0+3, tp1+2,
                        up0+1, up2+0);
                }
            }
        }
    }

    /* Top edge */
    if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my)))
    {
        for(tx = 0; tx < image->tile_columns; tx++)
        {
            /* Top Left Corner Addition */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                tp0[0] = tp0[0] + tp0[1];
                CHECK1(image->lwf_test, tp0[0]);
            }
            /* Top Right Corner Addition */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                tp0[1] = tp0[1] + tp0[0];
                CHECK1(image->lwf_test, tp0[1]);
            }
        }
    }


    /* Bottom edge */
    if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my)))
    {
        for(tx = 0; tx < image->tile_columns; tx++)
        {
            /* Bottom Left Corner Addition*/
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP1(image,ch,tx,0).data;
                tp0[2] = tp0[2] + tp0[3];
                CHECK1(image->lwf_test, tp0[2]);
            }
            /* Bottom Right Corner Addition*/
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;
                tp0[3] = tp0[3] + tp0[2];
                CHECK1(image->lwf_test, tp0[3]);
            }
        }
    }
}

static void second_prefilter_up1(jxr_image_t image, int ty)
{
    int ch;
    second_prefilter444_up1(image, 0, ty);
    switch (image->use_clr_fmt) {
        case 0:
            assert(image->num_channels == 1);
            break;
        case 1:/*YUV420*/
            second_prefilter420_up1(image, 1, ty);
            second_prefilter420_up1(image, 2, ty);
            break;
        case 2:/*YUV422*/
            second_prefilter422_up1(image, 1, ty);
            second_prefilter422_up1(image, 2, ty);
            break;
        default:
            for (ch = 1 ; ch < image->num_channels ; ch += 1)
                second_prefilter444_up1(image, ch, ty);
            break;
    }
}

static void dclphp_unshuffle(int*data, int dclp_count)
{
    int buf[256];
    int idx, jdx;

    for (idx = 0 ; idx < dclp_count ; idx += 1) {
        buf[idx] = data[idx*16];

        for (jdx = 1 ; jdx < 16 ; jdx += 1) {
            buf[16 + 15*idx + jdx-1] = data[16*idx+jdx];
        }
    }

    for (idx = 0 ; idx < 16+15*dclp_count ; idx += 1)
        data[idx] = buf[idx];
}

static void PCT_stage1_up2(jxr_image_t image, int ch, int ty)
{
    uint32_t tx = 0;
    uint32_t use_my = image->cur_my + 2;

    /* make adjustments based on tiling */
    if (use_my >= image->tile_row_height[ty]) {
        use_my -= image->tile_row_height[ty];
        ty += 1;
        if (use_my >= image->tile_row_height[ty]) {
            use_my -= image->tile_row_height[ty];
            ty += 1;
        }
    }

    int dclp_count = 16;
    if (ch>0 && image->use_clr_fmt == 2/*YUV422*/)
        dclp_count = 8;
    else if (ch>0 && image->use_clr_fmt == 1/*YUV420*/)
        dclp_count = 4;

    int mx;
    for (mx = 0 ; mx < (int) EXTENDED_WIDTH_BLOCKS(image) ; mx += 1) {
        int jdx;
        for (jdx = 0 ; jdx < 16*dclp_count ; jdx += 16) {
#if defined(DETAILED_DEBUG)
            {
                int pix;
                DEBUG(" DC-LP-HP (strip=%3d, mbx=%4d ch=%d, block=%2d) pre-PCT:",
                    use_my, mx, ch, jdx/16);
                for (pix = 0; pix < 16 ; pix += 1) {
                    DEBUG(" 0x%08x", image->strip[ch].up2[mx].data[jdx+pix]);
                    if (pix%4 == 3 && pix != 15)
                        DEBUG("\n%*s:", 55, "");
                }
                DEBUG("\n");
            }
#endif
            _jxr_4x4PCT(image->strip[ch].up2[mx].data+jdx);

#if defined(DETAILED_DEBUG)
            {
                int pix;
                DEBUG(" DC-LP-HP (strip=%3d, mbx=%4d ch=%d, block=%2d) PCT:",
                    use_my, mx, ch, jdx/16);
                for (pix = 0; pix < 16 ; pix += 1) {
                    DEBUG(" 0x%08x", image->strip[ch].up2[mx].data[jdx+pix]);
                    if (pix%4 == 3 && pix != 15)
                        DEBUG("\n%*s:", 51, "");
                }
                DEBUG("\n");
            }
#endif
        }

        dclphp_unshuffle(image->strip[ch].up2[mx].data, dclp_count);

        int hp_quant_raw = w_guess_hp_quant(image, ch, tx, ty, mx, use_my);
        int hp_quant = _jxr_quant_map(image, hp_quant_raw, 1);
        assert(hp_quant > 0);

        DEBUG(" DC-LP-HP (mx=%d ch=%d) use_hp_quant=%d (hp quant index=%u raw=%d)\n", mx, ch,
            hp_quant, _jxr_select_hp_index(image, tx, ty, mx, use_my), hp_quant_raw);

        for (jdx = 0 ; jdx < dclp_count ; jdx += 1) {
            int k;
            for (k = 0 ; k < 15 ; k += 1) {
                CHECK1(image->lwf_test, MACROBLK_UP2_HP(image,ch,tx,mx,jdx,k));
                int value = MACROBLK_UP2_HP(image,ch,tx,mx,jdx,k);
                MACROBLK_UP2_HP(image,ch,tx,mx,jdx,k) = quantize_lphp(value, hp_quant);
            }
#if defined(DETAILED_DEBUG)
            {
                int pix;
                DEBUG(" DC-LP-HP (strip=%3d, mbx=%4d ch=%d, block=%2d) Quantized: 0x%08x",
                    use_my, mx, ch, jdx, MACROBLK_UP2_LP(image,ch,tx,mx,jdx));
                for (pix = 1; pix < 16 ; pix += 1) {
                    DEBUG(" 0x%08x", MACROBLK_UP2_HP(image,ch,tx,mx,jdx,pix-1));
                    if (pix%4 == 3 && pix != 15)
                        DEBUG("\n%*s:", 57, "");
                }
                DEBUG("\n");
            }
#endif
        }
    }
}

/*
* Calcualte the value of MBDCMode, which defines the prediction for
* the macroblock. There is one MBDCMode value for all the components,
* and will take into account some of the chroma planes if it makes
* sense.
*
* Note that CUR is before UP1 in the stream, and UP1 is the strip
* that is being worked on.
*/
static int w_calculate_mbdc_mode(jxr_image_t image, int tx, int mx, int my)
{
    if (mx == 0 && my == 0)
        return 3; /* No prediction. */

    if (mx == 0)
        return 1; /* Predictions from top only. */

    if (my == 0)
        return 0; /* prediction from left only */

    /* The context. */
    long left = MACROBLK_UP1(image, 0, tx, mx-1).pred_dclp[0];
    long top = MACROBLK_CUR(image, 0, tx, mx).pred_dclp[0];
    long topleft = MACROBLK_CUR(image, 0, tx, mx-1).pred_dclp[0];

    /* Calculate horizontal and vertical "strengths". We will use
    those strengths to decide which direction prediction should
    be. */
    long strhor = 0;
    long strvert = 0;
    if (image->use_clr_fmt==0 || image->use_clr_fmt==6) {/* YONLY or NCOMPONENT */

        /* YONLY and NCOMPONENT use only the context of channel-0
        to calculate the strengths. */
        strhor = labs(topleft - left);
        strvert = labs(topleft - top);

    } else {
        /* YUV4XX and YUVK also include the U and V channels to
        calculate the strengths. Note that YUVK (CMYK)
        ignores the K channel so that all color images are
        processed the same, generally. */
        long left_u = MACROBLK_UP1(image, 1, tx, mx-1).pred_dclp[0];
        long top_u = MACROBLK_CUR(image, 1, tx, mx).pred_dclp[0];
        long topleft_u = MACROBLK_CUR(image, 1, tx, mx-1).pred_dclp[0];
        long left_v = MACROBLK_UP1(image, 2, tx, mx-1).pred_dclp[0];
        long top_v = MACROBLK_CUR(image, 2, tx, mx).pred_dclp[0];
        long topleft_v = MACROBLK_CUR(image, 2, tx, mx-1).pred_dclp[0];

        long scale = 2;
        if (image->use_clr_fmt == 2 /*YUV422*/)
            scale = 4;
        if (image->use_clr_fmt == 1 /*YUV420*/)
            scale = 8;

        strhor = labs(topleft - left)*scale + labs(topleft_u - left_u) + labs(topleft_v - left_v);
        strvert = labs(topleft - top)*scale + labs(topleft_u - top_u) + labs(topleft_v - top_v);
    }

    if ((strhor*4) < strvert)
        return 1;

    if ((strvert*4) < strhor)
        return 0;

    return 2;
}

/*
* Do DC-LP prediction on the UP1 strip. The CUR strip has already
* been processed (and (rotated down from UP1) so CUR is earlier in
* the image stream and is the top context for the prediction.
*/
static void w_predict_up1_dclp(jxr_image_t image, int tx, int ty, int mx)
{
    uint32_t my = image->cur_my + 1;

    /* make adjustments based on tiling */
    if (my == image->tile_row_height[ty]) {
        my = 0;
        ty += 1;
    }

    /* Calculate the mbcd prediction mode. This mode is used for
    all the planes of DC data. */
    int mbdc_mode = w_calculate_mbdc_mode(image, tx, mx, my);
    /* Now process all the planes of DC data. */
    int ch;
    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        long left = mx>0? MACROBLK_UP1(image,ch,tx,mx-1).pred_dclp[0] : 0;
        long top = MACROBLK_CUR(image,ch,tx,mx).pred_dclp[0];

        DEBUG(" MBDC_MODE=%d for TX=%d, MBx=%d, MBy=%d, ch=%d, left=0x%lx, top=0x%lx, cur=0x%x\n",
            mbdc_mode, tx, mx, my, ch, left, top, MACROBLK_UP_DC(image,ch,tx,mx));

        /* Save this unpredicted DC value for later prediction steps. */
        MACROBLK_UP1(image,ch,tx,mx).pred_dclp[0] = MACROBLK_UP_DC(image,ch,tx,mx);

        /* Perform the actual DC prediction. */
        switch (mbdc_mode) {
            case 0: /* left */
                MACROBLK_UP_DC(image,ch,tx,mx) -= left;
                break;
            case 1: /* top */
                MACROBLK_UP_DC(image,ch,tx,mx) -= top;
                break;
            case 2:/* top and left */
                /* Note that we really MEAN >>1 and NOT /2. in
                particular, if the sum is -1, we want the
                result to also be -1. That extra stuff after
                the "|" is there to make sure the sign bit is
                not lost. Also, the chroma planes for YUV42X
                formats round *up* the mean, where all other
                planes round down. */
                if (ch>0 && (image->use_clr_fmt==1/*YUV420*/||image->use_clr_fmt==2/*YUV422*/))
                    MACROBLK_UP_DC(image,ch,tx,mx) -= (left+top+1) >> 1 | ((left+top+1)&~INT_MAX);
                else
                    MACROBLK_UP_DC(image,ch,tx,mx) -= (left+top) >> 1 | ((left+top)&~INT_MAX);
                break;
            default:
                break;
        }
        CHECK1(image->lwf_test, MACROBLK_UP_DC(image,ch,tx,mx));

    }

    /* The MBLPMode decides the LP prediction within the
    macroblock. It is generally the same as the prediction
    direction for DC in the macroblock, but we modify it based
    on the MBLP QP Index. In particular, if MBDC mode is
    predicting from the left, bug the MBLP QP Index from the
    left is different from that of the current MB, then cancel
    prediction. Same for prediction from above. */
    int mblp_mode = 2;
    unsigned char mblp_index = _jxr_select_lp_index(image, tx, ty, mx, my);
    if (mbdc_mode == 0) {
        unsigned char mblp_index_left = _jxr_select_lp_index(image, tx, ty, mx-1, my);
        if (mblp_index == mblp_index_left)
            mblp_mode = 0;
        else
            mblp_mode = 2;

        DEBUG(" MBLP_MODE=%d for MBx=%d, MBy=%d (lp_quant=%d,lp_quant_ctx=%d)\n", mblp_mode, mx, my,
            mblp_index, mblp_index_left);

    } else if (mbdc_mode == 1) {
        unsigned char mblp_index_up = _jxr_select_lp_index(image, tx, ty, mx, my-1);
        if (mblp_index == mblp_index_up)
            mblp_mode = 1;
        else
            mblp_mode = 2;

        DEBUG(" MBLP_MODE=%d for MBx=%d, MBy=%d (lp_quant=%d,lp_quant_ctx=%d)\n", mblp_mode, mx, my,
            mblp_index, mblp_index_up);
    } else {

        mblp_mode = 2;
        DEBUG(" MBLP_MODE=%d for MBx=%d, MBy=%d (lp_quant=%d,lp_quant_ctx=-1)\n", mblp_mode, mx, my,
            mblp_index);
    }

    w_predict_lp444(image, tx, mx, my, 0, mblp_mode);
    for (ch = 1 ; ch < image->num_channels ; ch += 1) {
        switch (image->use_clr_fmt) {
            case 1: /* YUV420 */
                w_predict_lp420(image, tx, mx, my, ch, mblp_mode);
                break;
            case 2: /* YUV422 */
                w_predict_lp422(image, tx, mx, my, ch, mblp_mode, mbdc_mode);
                break;
            default:
                w_predict_lp444(image,tx, mx, my, ch, mblp_mode);
                break;
        }
    }
}

static int w_calculate_mbhp_mode_up1(jxr_image_t image, int tx, int mx)
{
    long strength_hor = 0;
    long strength_ver = 0;
    const long orientation_weight = 4;

    /* Add up the LP magnitudes along the top edge */
    strength_hor += abs(MACROBLK_UP_LP(image, 0, tx, mx, 0));
    strength_hor += abs(MACROBLK_UP_LP(image, 0, tx, mx, 1));
    strength_hor += abs(MACROBLK_UP_LP(image, 0, tx, mx, 2));

    /* Add up the LP magnitudes along the left edge */
    strength_ver += abs(MACROBLK_UP_LP(image, 0, tx, mx, 3));
    strength_ver += abs(MACROBLK_UP_LP(image, 0, tx, mx, 7));
    strength_ver += abs(MACROBLK_UP_LP(image, 0, tx, mx, 11));

    switch (image->use_clr_fmt) {
        case 0: /*YONLY*/
        case 6: /*NCOMPONENT*/
            break;
        case 3: /*YUV444*/
        case 4: /*YUVK */
            strength_hor += abs(MACROBLK_UP_LP(image, 1, tx, mx, 0));
            strength_hor += abs(MACROBLK_UP_LP(image, 2, tx, mx, 0));
            strength_ver += abs(MACROBLK_UP_LP(image, 1, tx, mx, 3));
            strength_ver += abs(MACROBLK_UP_LP(image, 2, tx, mx, 3));
            break;
        case 2: /*YUV422*/
            strength_hor += abs(MACROBLK_UP_LP(image, 1, tx, mx, 0));
            strength_hor += abs(MACROBLK_UP_LP(image, 2, tx, mx, 0));
            strength_ver += abs(MACROBLK_UP_LP(image, 1, tx, mx, 1));
            strength_ver += abs(MACROBLK_UP_LP(image, 2, tx, mx, 1));
            strength_hor += abs(MACROBLK_UP_LP(image, 1, tx, mx, 4));
            strength_hor += abs(MACROBLK_UP_LP(image, 2, tx, mx, 4));
            strength_ver += abs(MACROBLK_UP_LP(image, 1, tx, mx, 5));
            strength_ver += abs(MACROBLK_UP_LP(image, 2, tx, mx, 5));
            break;
        case 1: /*YUV420*/
            strength_hor += abs(MACROBLK_UP_LP(image, 1, tx, mx, 0));
            strength_hor += abs(MACROBLK_UP_LP(image, 2, tx, mx, 0));
            strength_ver += abs(MACROBLK_UP_LP(image, 1, tx, mx, 1));
            strength_ver += abs(MACROBLK_UP_LP(image, 2, tx, mx, 1));
            break;
        default:
            assert(0);
    }

    if (strength_hor * orientation_weight < strength_ver)
        return 0; /* predict from left */
    if (strength_ver * orientation_weight < strength_hor)
        return 1; /* predict from top */

    /* There is no strong weight from top or left, so do not
    bother with prediction. */
    return 2;
}

static void w_predict_up1_hp(jxr_image_t image, int ch, int tx, int mx, int mbhp_pred_mode)
{
    if (mbhp_pred_mode == 0) { /* Prediction left to right */
        int idx;
        for (idx = 15 ; idx > 0 ; idx -= 1) {
            if (idx%4 == 0)
                continue;

            MACROBLK_UP1_HP(image,ch,tx,mx,idx, 3) -= MACROBLK_UP1_HP(image,ch,tx,mx,idx-1, 3);
            MACROBLK_UP1_HP(image,ch,tx,mx,idx, 7) -= MACROBLK_UP1_HP(image,ch,tx,mx,idx-1, 7);
            MACROBLK_UP1_HP(image,ch,tx,mx,idx,11) -= MACROBLK_UP1_HP(image,ch,tx,mx,idx-1,11);
            CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,ch,tx,mx,idx, 3), MACROBLK_CUR_HP(image,ch,tx,mx,idx, 7), MACROBLK_CUR_HP(image,ch,tx,mx,idx, 11));
        }
    } else if (mbhp_pred_mode == 1) { /* Prediction top to bottom */
        int idx;
        for (idx = 15 ; idx >= 4 ; idx -= 1) {
            MACROBLK_UP1_HP(image,ch,tx,mx,idx,0) -= MACROBLK_UP1_HP(image,ch,tx,mx,idx-4,0);
            MACROBLK_UP1_HP(image,ch,tx,mx,idx,1) -= MACROBLK_UP1_HP(image,ch,tx,mx,idx-4,1);
            MACROBLK_UP1_HP(image,ch,tx,mx,idx,2) -= MACROBLK_UP1_HP(image,ch,tx,mx,idx-4,2);
            CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,ch,tx,mx,idx, 0), MACROBLK_CUR_HP(image,ch,tx,mx,idx, 1), MACROBLK_CUR_HP(image,ch,tx,mx,idx, 2));
        }
    }

    switch (image->use_clr_fmt) {
        case 1:/*YUV420*/
            assert(ch == 0);
            if (mbhp_pred_mode == 0) {
                int idx;
                for (idx = 3 ; idx > 0 ; idx -= 2) {
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,3) -= MACROBLK_UP1_HP(image,1,tx,mx,idx-1,3);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,3) -= MACROBLK_UP1_HP(image,2,tx,mx,idx-1,3);
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,7) -= MACROBLK_UP1_HP(image,1,tx,mx,idx-1,7);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,7) -= MACROBLK_UP1_HP(image,2,tx,mx,idx-1,7);
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,11)-= MACROBLK_UP1_HP(image,1,tx,mx,idx-1,11);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,11)-= MACROBLK_UP1_HP(image,2,tx,mx,idx-1,11);
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,1,tx,mx,idx, 3), MACROBLK_CUR_HP(image,1,tx,mx,idx, 7), MACROBLK_CUR_HP(image,1,tx,mx,idx, 11));
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,2,tx,mx,idx, 3), MACROBLK_CUR_HP(image,2,tx,mx,idx, 7), MACROBLK_CUR_HP(image,2,tx,mx,idx, 11));
                }
            } else if (mbhp_pred_mode == 1) {
                int idx;
                for (idx = 3 ; idx >= 2 ; idx -= 1) {
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,0) -= MACROBLK_UP1_HP(image,1,tx,mx,idx-2,0);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,0) -= MACROBLK_UP1_HP(image,2,tx,mx,idx-2,0);
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,1) -= MACROBLK_UP1_HP(image,1,tx,mx,idx-2,1);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,1) -= MACROBLK_UP1_HP(image,2,tx,mx,idx-2,1);
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,2) -= MACROBLK_UP1_HP(image,1,tx,mx,idx-2,2);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,2) -= MACROBLK_UP1_HP(image,2,tx,mx,idx-2,2);
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,1,tx,mx,idx, 0), MACROBLK_CUR_HP(image,1,tx,mx,idx, 1), MACROBLK_CUR_HP(image,1,tx,mx,idx, 2));
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,2,tx,mx,idx, 0), MACROBLK_CUR_HP(image,2,tx,mx,idx, 1), MACROBLK_CUR_HP(image,2,tx,mx,idx, 2));
                }
            }
            break;

        case 2:/*YUV422*/
            assert(ch == 0);
            if (mbhp_pred_mode == 0) {
                int idx;
                for (idx = 7 ; idx > 0 ; idx -= 2) {
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,3) -= MACROBLK_UP1_HP(image,1,tx,mx,idx-1,3);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,3) -= MACROBLK_UP1_HP(image,2,tx,mx,idx-1,3);
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,7) -= MACROBLK_UP1_HP(image,1,tx,mx,idx-1,7);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,7) -= MACROBLK_UP1_HP(image,2,tx,mx,idx-1,7);
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,11)-= MACROBLK_UP1_HP(image,1,tx,mx,idx-1,11);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,11)-= MACROBLK_UP1_HP(image,2,tx,mx,idx-1,11);
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,1,tx,mx,idx, 3), MACROBLK_CUR_HP(image,1,tx,mx,idx, 7), MACROBLK_CUR_HP(image,1,tx,mx,idx, 11));
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,2,tx,mx,idx, 3), MACROBLK_CUR_HP(image,2,tx,mx,idx, 7), MACROBLK_CUR_HP(image,2,tx,mx,idx, 11));
                }
            } else if (mbhp_pred_mode == 1) {
                int idx;
                for (idx = 7; idx >= 2 ; idx -= 1) {
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,0) -= MACROBLK_UP1_HP(image,1,tx,mx,idx-2,0);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,0) -= MACROBLK_UP1_HP(image,2,tx,mx,idx-2,0);
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,1) -= MACROBLK_UP1_HP(image,1,tx,mx,idx-2,1);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,1) -= MACROBLK_UP1_HP(image,2,tx,mx,idx-2,1);
                    MACROBLK_UP1_HP(image,1,tx,mx,idx,2) -= MACROBLK_UP1_HP(image,1,tx,mx,idx-2,2);
                    MACROBLK_UP1_HP(image,2,tx,mx,idx,2) -= MACROBLK_UP1_HP(image,2,tx,mx,idx-2,2);
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,1,tx,mx,idx, 0), MACROBLK_CUR_HP(image,1,tx,mx,idx, 1), MACROBLK_CUR_HP(image,1,tx,mx,idx, 2));
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,2,tx,mx,idx, 0), MACROBLK_CUR_HP(image,2,tx,mx,idx, 1), MACROBLK_CUR_HP(image,2,tx,mx,idx, 2));
                }
            }
#if defined(DETAILED_DEBUG) && 0
            { 
                int idx;
                for (idx = 0 ; idx < 8 ; idx += 1) {
                    int k;
                    DEBUG(" HP val difference[ch=1, tx=%u, mx=%d, block=%d] ==", tx, mx, idx);
                    for (k = 1 ; k<16; k+=1) {
                        DEBUG(" 0x%x", MACROBLK_UP1_HP(image,1,tx,mx,idx,k-1));
                    }
                    DEBUG("\n");
                    DEBUG(" HP val difference[ch=2, tx=%u, mx=%d, block=%d] ==", tx, mx, idx);
                    for (k = 1 ; k<16; k+=1) {
                        DEBUG(" 0x%x", MACROBLK_UP1_HP(image,2,tx,mx,idx,k-1));
                    }
                    DEBUG("\n");
                }
            }
#endif
            break;
        default:
            break;
    }
}

static void w_predict_lp444(jxr_image_t image, int tx, int mx, int /*my*/, int ch, int mblp_mode)
{
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[1] = MACROBLK_UP_LP(image,ch,tx,mx,0);
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[2] = MACROBLK_UP_LP(image,ch,tx,mx,1);
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[3] = MACROBLK_UP_LP(image,ch,tx,mx,2);
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[4] = MACROBLK_UP_LP(image,ch,tx,mx,3);
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[5] = MACROBLK_UP_LP(image,ch,tx,mx,7);
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[6] = MACROBLK_UP_LP(image,ch,tx,mx,11);

#if defined(DETAILED_DEBUG)
    {
        int jdx;
        DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) Predicted:", my, mx, ch);
        DEBUG(" 0x%08x", MACROBLK_UP1(image,ch,tx,mx).pred_dclp[0]);
        for (jdx = 0; jdx < 15 ; jdx += 1) {
            DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,tx,mx,jdx));
            if ((jdx+1)%4 == 3 && jdx != 14)
                DEBUG("\n%*s:", 45, "");
        }
        DEBUG("\n");
    }
#endif

    switch (mblp_mode) {
        case 0: /* left */
            MACROBLK_UP_LP(image,ch,tx,mx,3) -= MACROBLK_UP1(image,ch,tx,mx-1).pred_dclp[4];
            MACROBLK_UP_LP(image,ch,tx,mx,7) -= MACROBLK_UP1(image,ch,tx,mx-1).pred_dclp[5];
            MACROBLK_UP_LP(image,ch,tx,mx,11) -= MACROBLK_UP1(image,ch,tx,mx-1).pred_dclp[6];
            CHECK3(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,3), MACROBLK_CUR_LP(image,ch,tx,mx,7), MACROBLK_CUR_LP(image,ch,tx,mx,11));
            break;
        case 1: /* up */
            MACROBLK_UP_LP(image,ch,tx,mx,0) -= MACROBLK_CUR(image,ch,tx,mx).pred_dclp[1];
            MACROBLK_UP_LP(image,ch,tx,mx,1) -= MACROBLK_CUR(image,ch,tx,mx).pred_dclp[2];
            MACROBLK_UP_LP(image,ch,tx,mx,2) -= MACROBLK_CUR(image,ch,tx,mx).pred_dclp[3];
            CHECK3(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,0), MACROBLK_CUR_LP(image,ch,tx,mx,1), MACROBLK_CUR_LP(image,ch,tx,mx,2));
            break;
        case 2:
            break;
            }
#if defined(DETAILED_DEBUG)
        {
            int jdx;
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) Difference:", my, mx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,tx,mx));
            for (jdx = 0; jdx < 15 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,tx,mx,jdx));
                if ((jdx+1)%4 == 3 && jdx != 14)
                    DEBUG("\n%*s:", 46, "");
            }
            DEBUG("\n");
        }
#endif
}

static void w_predict_lp422(jxr_image_t image, int tx, int mx, int /*my*/, int ch, int mblp_mode, int mbdc_mode)
{
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[1] = MACROBLK_UP_LP(image,ch,tx,mx,0);
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[2] = MACROBLK_UP_LP(image,ch,tx,mx,1);
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[4] = MACROBLK_UP_LP(image,ch,tx,mx,3);
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[5] = MACROBLK_UP_LP(image,ch,tx,mx,4);
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[6] = MACROBLK_UP_LP(image,ch,tx,mx,5);

#if defined(DETAILED_DEBUG)
    {
        int jdx;
        DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) Predicted:", my, mx, ch);
        DEBUG(" 0x%08x", MACROBLK_UP1(image,ch,tx,mx).pred_dclp[0]);
        for (jdx = 0; jdx < 7 ; jdx += 1) {
            DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,tx,mx,jdx));
            if ((jdx+1)%4 == 3 && jdx != 6)
                DEBUG("\n%*s:", 45, "");
        }
        DEBUG("\n");
    }
#endif
    switch (mblp_mode) {
        case 0: /* left */
            MACROBLK_UP_LP(image,ch,tx,mx,3) -= MACROBLK_UP1(image,ch,tx,mx-1).pred_dclp[4];
            MACROBLK_UP_LP(image,ch,tx,mx,1) -= MACROBLK_UP1(image,ch,tx,mx-1).pred_dclp[2];
            MACROBLK_UP_LP(image,ch,tx,mx,5) -= MACROBLK_UP1(image,ch,tx,mx-1).pred_dclp[6];
            CHECK3(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,3), MACROBLK_CUR_LP(image,ch,tx,mx,1), MACROBLK_CUR_LP(image,ch,tx,mx,5));
            break;
        case 1: /* up */
            MACROBLK_UP_LP(image,ch,tx,mx,3) -= MACROBLK_CUR(image,ch,tx,mx).pred_dclp[4];
            MACROBLK_UP_LP(image,ch,tx,mx,0) -= MACROBLK_CUR(image,ch,tx,mx).pred_dclp[5];
            MACROBLK_UP_LP(image,ch,tx,mx,4) -= MACROBLK_UP1(image,ch,tx,mx).pred_dclp[1];
            CHECK3(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,3), MACROBLK_CUR_LP(image,ch,tx,mx,0), MACROBLK_CUR_LP(image,ch,tx,mx,4));
            break;
        case 2:
            if (mbdc_mode == 1) {
                MACROBLK_UP_LP(image,ch,tx,mx,4) -= MACROBLK_UP_LP(image,ch,tx,mx,0);
                CHECK1(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,4));
            }
            break;
            }
#if defined(DETAILED_DEBUG)
        {
            int jdx;
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) Difference:", my, mx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,tx,mx));
            for (jdx = 0; jdx < 7 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,tx,mx,jdx));
                if ((jdx+1)%4 == 3 && jdx != 6)
                    DEBUG("\n%*s:", 46, "");
            }
            DEBUG("\n");
        }
#endif
}

static void w_predict_lp420(jxr_image_t image, int tx, int mx, int /*my*/, int ch, int mblp_mode)
{
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[1] = MACROBLK_UP_LP(image,ch,tx,mx,0);
    MACROBLK_UP1(image,ch,tx,mx).pred_dclp[2] = MACROBLK_UP_LP(image,ch,tx,mx,1);

    switch (mblp_mode) {
        case 0: /* left */
            MACROBLK_UP_LP(image,ch,tx,mx,1) -= MACROBLK_UP1(image,ch,tx,mx-1).pred_dclp[2];
            CHECK1(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,1));
            break;
        case 1: /* up */
            MACROBLK_UP_LP(image,ch,tx,mx,0) -= MACROBLK_CUR(image,ch,tx,mx).pred_dclp[1];
            CHECK1(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,0));
            break;
    }
}

/*
* This calculations the HP CBP for the macroblock. It also calculates
* and saves the HP model_bits values for this block and stores them
* away in the macroblock for later use by the MB_HP formatter.
*/
static void calculate_hpcbp_up1(jxr_image_t image, int tx, int ty, int mx)
{
    uint32_t use_my = image->cur_my + 1;

    /* make adjustments based on tiling */
    if (use_my == image->tile_row_height[ty]) {
        use_my = 0;
        ty += 1;
    }

    if (_jxr_InitContext(image, tx, ty, mx, use_my)) {
        _jxr_InitializeModelMB(&image->model_hp, 2/*band=HP*/);
    }

    int lap_mean[2];
    lap_mean[0] = 0;
    lap_mean[1] = 0;

    int ch;
    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        int chroma_flag = (ch>0)? 1 : 0;
        unsigned model_bits = image->model_hp.bits[chroma_flag];

        int cbp_mask = 0;

        int blk_count = 16;
        if (chroma_flag && image->use_clr_fmt==2/*YUV422*/)
            blk_count = 8;
        if (chroma_flag && image->use_clr_fmt==1/*YUV420*/)
            blk_count = 4;

        int blk;
        for (blk = 0 ; blk < blk_count ; blk += 1) {
            int bpos = blk;
            if (blk_count==16)
                bpos = _jxr_hp_scan_map[bpos];
            int idx;
            for (idx = 0 ; idx < 15 ; idx += 1) {
                int value = MACROBLK_UP1_HP(image,ch,tx,mx,bpos,idx);
                value = abs(value) >> model_bits;
                if (value != 0) {
                    cbp_mask |= 1<<blk;
                    lap_mean[chroma_flag] += 1;
                }
            }
        }

        MACROBLK_UP1_HPCBP(image, ch, tx, mx) = cbp_mask;
    }

    MACROBLK_UP1(image,0,tx,mx).hp_model_bits[0] = image->model_hp.bits[0];
    MACROBLK_UP1(image,0,tx,mx).hp_model_bits[1] = image->model_hp.bits[1];

    DEBUG(" MP_HP: lap_mean={%u, %u}, model_hp.bits={%u %u}, model_hp.state={%d %d}\n",
        lap_mean[0], lap_mean[1],
        image->model_hp.bits[0], image->model_hp.bits[1],
        image->model_hp.state[0], image->model_hp.state[1]);

    _jxr_UpdateModelMB(image, lap_mean, &image->model_hp, 2/*band=HP*/);

    DEBUG(" MP_HP: Updated: lap_mean={%u, %u}, model_hp.bits={%u %u}, model_hp.state={%d %d}\n",
        lap_mean[0], lap_mean[1],
        image->model_hp.bits[0], image->model_hp.bits[1],
        image->model_hp.state[0], image->model_hp.state[1]);

}

static void w_PredCBP(jxr_image_t image, unsigned tx, unsigned ty, unsigned mx)
{
    uint32_t use_my = image->cur_my+1;
    if (use_my == image->tile_row_height[ty]) {
        use_my = 0;
        ty += 1;
    }

    if (_jxr_InitContext(image, tx, ty, mx, use_my)) {
        _jxr_InitializeCBPModel(image);
    }

    int ch;
    switch (image->use_clr_fmt) {
        case 1: /* YUV420 */
            _jxr_w_PredCBP444(image, 0, tx, mx, use_my);
            _jxr_w_PredCBP420(image, 1, tx, mx, use_my);
            _jxr_w_PredCBP420(image, 2, tx, mx, use_my);
            break;
        case 2: /* YUV422 */
            _jxr_w_PredCBP444(image, 0, tx, mx, use_my);
            _jxr_w_PredCBP422(image, 1, tx, mx, use_my);
            _jxr_w_PredCBP422(image, 2, tx, mx, use_my);
            break;
        default:
            for (ch = 0 ; ch < image->num_channels ; ch += 1) {
                DEBUG(" PredCBP: Predicted HPCBP[ch=%d]: 0x%04x\n",
                    ch, MACROBLK_UP1_HPCBP(image,ch,tx,mx));
                _jxr_w_PredCBP444(image, ch, tx, mx, use_my);
            }
            break;
    }
}


/*
* $Log: w_strip.c,v $
* Revision 1.50 2009/09/16 12:00:00 microsoft
* Reference Software v1.8 updates.
*
* Revision 1.49 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.48 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.47 2008/03/24 18:06:56 steve
* Imrpove DEBUG messages around quantization.
*
* Revision 1.46 2008/03/21 18:05:53 steve
* Proper CMYK formatting on input.
*
* Revision 1.45 2008/03/21 00:08:56 steve
* Handle missing LP/HP QP maps.
*
* Revision 1.44 2008/03/20 22:39:41 steve
* Fix various debug prints of QP data.
*
* Revision 1.43 2008/03/20 18:11:50 steve
* Clarify MBLPMode calculations.
*
* Revision 1.42 2008/03/19 00:35:28 steve
* Fix MBLPMode prediction during distributed QP
*
* Revision 1.41 2008/03/18 21:09:12 steve
* Fix distributed color prediction.
*
* Revision 1.40 2008/03/18 18:36:56 steve
* Support compress of CMYK images.
*
* Revision 1.39 2008/03/14 17:08:51 gus
* *** empty log message ***
*
* Revision 1.38 2008/03/14 16:10:47 steve
* Clean up some signed/unsigned warnings.
*
* Revision 1.37 2008/03/14 00:54:09 steve
* Add second prefilter for YUV422 and YUV420 encode.
*
* Revision 1.36 2008/03/14 00:12:00 steve
* Fix YUV420 UV subsampling at bottom of MB.
*
* Revision 1.35 2008/03/13 23:46:24 steve
* First level filter encoding.
*
* Revision 1.34 2008/03/13 21:23:27 steve
* Add pipeline step for YUV420.
*
* Revision 1.33 2008/03/13 18:56:37 steve
* Fix compile-time support for dumb 422-to-420
*
* Revision 1.32 2008/03/13 18:52:01 steve
* Pipeline 422 to 420 handling.
*
* Revision 1.31 2008/03/13 18:37:45 steve
* Better YUV422 to YUV420 scaling
*
* Revision 1.30 2008/03/13 17:49:31 steve
* Fix problem with YUV422 CBP prediction for UV planes
*
* Add support for YUV420 encoding.
*
* Revision 1.29 2008/03/13 00:07:23 steve
* Encode HP of YUV422
*
* Revision 1.28 2008/03/12 21:10:27 steve
* Encode LP of YUV422
*
* Revision 1.27 2008/03/12 00:39:11 steve
* Rework yuv444_to_yuv422 to match WmpEncApp.
*
* Revision 1.26 2008/03/11 22:12:49 steve
* Encode YUV422 through DC.
*
* Revision 1.25 2008/03/06 02:05:49 steve
* Distributed quantization
*
* Revision 1.24 2008/03/05 06:58:10 gus
* *** empty log message ***
*
* Revision 1.23 2008/03/03 02:04:05 steve
* Get encoding of BD16 correct.
*
* Revision 1.22 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.21 2008/02/04 19:21:36 steve
* Fix rounding of scaled chroma DC/LP values.
*
* Revision 1.20 2008/02/01 22:49:53 steve
* Handle compress of YUV444 color DCONLY
*
* Revision 1.19 2008/01/08 23:56:22 steve
* Add remaining 444 overlap filtering.
*
* Revision 1.18 2008/01/08 01:06:20 steve
* Add first pass overlap filtering.
*
* Revision 1.17 2008/01/05 01:19:55 steve
* Use secret-handshake version of quantization.
*
* Revision 1.16 2008/01/04 22:48:55 steve
* Quantization division rounds, not truncates.
*
* Revision 1.15 2008/01/04 17:07:36 steve
* API interface for setting QP values.
*
* Revision 1.14 2008/01/03 18:07:31 steve
* Detach prediction mode calculations from prediction.
*
* Revision 1.13 2008/01/01 01:07:26 steve
* Add missing HP prediction.
*
* Revision 1.12 2007/12/30 00:16:00 steve
* Add encoding of HP values.
*
* Revision 1.11 2007/12/17 23:02:57 steve
* Implement MB_CBP encoding.
*
* Revision 1.10 2007/12/14 17:10:39 steve
* HP CBP Prediction
*
* Revision 1.9 2007/12/12 00:38:07 steve
* Encode LP data.
*
* Revision 1.8 2007/12/07 16:34:52 steve
* Use repeat bytes to pad to MB boundary.
*
* Revision 1.7 2007/12/06 17:54:59 steve
* Fix input data block ordering.
*
* Revision 1.6 2007/11/30 01:50:58 steve
* Compression of DCONLY GRAY.
*
* Revision 1.5 2007/11/26 01:47:16 steve
* Add copyright notices per MS request.
*
* Revision 1.4 2007/11/16 20:03:58 steve
* Store MB Quant, not qp_index.
*
* Revision 1.3 2007/11/12 23:21:55 steve
* Infrastructure for frequency mode ordering.
*
* Revision 1.2 2007/11/09 22:15:41 steve
* DC prediction for the encoder.
*
* Revision 1.1 2007/11/09 01:18:59 steve
* Stub strip input processing.
*
*/

