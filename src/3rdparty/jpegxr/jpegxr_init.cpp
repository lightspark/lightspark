

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
#pragma comment (user,"$Id: init.c,v 1.23 2008/03/13 21:23:27 steve Exp $")
#else
#ident "$Id: init.c,v 1.23 2008/03/13 21:23:27 steve Exp $"
#endif

# include "jxr_priv.h"
# include <stdlib.h>
# include <assert.h>

const int _jxr_abslevel_index_delta[7] = { 1, 0, -1, -1, -1, -1, -1 };

static void clear_vlc_tables(jxr_image_t image)
{
    int idx;
    for (idx = 0 ; idx < AbsLevelInd_COUNT ; idx += 1) {
        image->vlc_table[idx].discriminant = 0;
        image->vlc_table[idx].discriminant2 = 0;
        image->vlc_table[idx].table = 0;
        image->vlc_table[idx].deltatable = 0;
        image->vlc_table[idx].delta2table = 0;
    }
}

static struct jxr_image* __make_jxr(void)
{
    struct jxr_image*image = (struct jxr_image*) jpegxr_calloc(1, sizeof(struct jxr_image));
    image->user_flags = 0;
    image->width1 = 0;
    image->height1 = 0;
    image->extended_width = 0;
    image->extended_height = 0;
    image->header_flags1 = 0;
    image->header_flags2 = 0x80; /* SHORT_HEADER_FLAG=1 */
    image->header_flags_fmt = 0;
    image->bands_present = 0; /* Default ALL bands present */
    image->num_channels = 0;
    image->tile_index_table = 0;
    image->tile_index_table_length = 0;
    image->primary = 1;

    clear_vlc_tables(image);

    int idx;
    for (idx = 0 ; idx < MAX_CHANNELS ; idx += 1) {
        image->strip[idx].up4 = 0;
        image->strip[idx].up3 = 0;
        image->strip[idx].up2 = 0;
        image->strip[idx].up1 = 0;
        image->strip[idx].cur = 0;
    }

    image->scaled_flag = 1;

    image->out_fun = 0;

    return image;
}

static void make_mb_row_buffer(jxr_image_t image, unsigned use_height)
{
    size_t block_count = EXTENDED_WIDTH_BLOCKS(image) * use_height;
    int*data, *pred_dclp;
    size_t idx;

    image->mb_row_buffer[0] = (struct macroblock_s*) jpegxr_calloc(block_count, sizeof(struct macroblock_s));
    data = (int*) jpegxr_calloc(block_count*256, sizeof(int));
    pred_dclp = (int*) jpegxr_calloc(block_count*7, sizeof(int));
    assert(image->mb_row_buffer[0]);
    assert(data);
    assert(pred_dclp);

    for (idx = 0 ; idx < block_count ; idx += 1) {
        image->mb_row_buffer[0][idx].data = data + 256*idx;
        /* 7 (used as mutilpier) = 1 DC + 3 top LP + 3 left LP coefficients used for prediction */
        image->mb_row_buffer[0][idx].pred_dclp = pred_dclp + 7*idx;
    }

    int format_scale = 256;
    if (image->use_clr_fmt == 2 /* YUV422 */) {
        format_scale = 16 + 8*15;
    } else if (image->use_clr_fmt == 1 /* YUV420 */) {
        format_scale = 16 + 4*15;
    }

    int ch;
    for (ch = 1 ; ch < image->num_channels ; ch += 1) {
        image->mb_row_buffer[ch] = (struct macroblock_s*) jpegxr_calloc(block_count, sizeof(struct macroblock_s));
        data = (int*) jpegxr_calloc(block_count*format_scale, sizeof(int));
        pred_dclp = (int*) jpegxr_calloc(block_count*7, sizeof(int));
        assert(image->mb_row_buffer[ch]);
        assert(data);
        assert(pred_dclp);

        for (idx = 0 ; idx < block_count ; idx += 1) {
            image->mb_row_buffer[ch][idx].data = data + format_scale*idx;
            image->mb_row_buffer[ch][idx].pred_dclp = pred_dclp + 7*idx;
        }
    }
}

/*
* Allocate the macroblock strip store. Each macroblock points to 256
* values that are either the 256 pixel values, or the DC, LP and HP
* coefficients.
*
* DC/LP/HP arrangement -
* The first word is the DC.
* The next 15 are the LP coefficients.
* The remaining 240 are HP coefficients.
*/
void _jxr_make_mbstore(jxr_image_t image, int up4_flag)
{
    int ch;

    assert(image->strip[0].up4 == 0);
    assert(image->strip[0].up3 == 0);
    assert(image->strip[0].up2 == 0);
    assert(image->strip[0].up1 == 0);
    assert(image->strip[0].cur == 0);

    assert(image->num_channels > 0);

    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        unsigned idx;
        if (up4_flag)
            image->strip[ch].up4 = (struct macroblock_s*)
            jpegxr_calloc(EXTENDED_WIDTH_BLOCKS(image), sizeof(struct macroblock_s));
        image->strip[ch].up3 = (struct macroblock_s*)
            jpegxr_calloc(EXTENDED_WIDTH_BLOCKS(image), sizeof(struct macroblock_s));
        image->strip[ch].up2 = (struct macroblock_s*)
            jpegxr_calloc(EXTENDED_WIDTH_BLOCKS(image), sizeof(struct macroblock_s));
        image->strip[ch].up1 = (struct macroblock_s*)
            jpegxr_calloc(EXTENDED_WIDTH_BLOCKS(image), sizeof(struct macroblock_s));
        image->strip[ch].cur = (struct macroblock_s*)
            jpegxr_calloc(EXTENDED_WIDTH_BLOCKS(image), sizeof(struct macroblock_s));

        if (up4_flag) {
            image->strip[ch].up4[0].data = (int*)jpegxr_calloc(256 * EXTENDED_WIDTH_BLOCKS(image), sizeof(int));
            for (idx = 1 ; idx < EXTENDED_WIDTH_BLOCKS(image) ; idx += 1)
                image->strip[ch].up4[idx].data = image->strip[ch].up4[idx-1].data + 256;
        }
        image->strip[ch].up3[0].data = (int*)jpegxr_calloc(256 * EXTENDED_WIDTH_BLOCKS(image), sizeof(int));
        for (idx = 1 ; idx < EXTENDED_WIDTH_BLOCKS(image) ; idx += 1)
            image->strip[ch].up3[idx].data = image->strip[ch].up3[idx-1].data + 256;

        image->strip[ch].up2[0].data = (int*)jpegxr_calloc(256 * EXTENDED_WIDTH_BLOCKS(image), sizeof(int));
        for (idx = 1 ; idx < EXTENDED_WIDTH_BLOCKS(image) ; idx += 1)
            image->strip[ch].up2[idx].data = image->strip[ch].up2[idx-1].data + 256;

        image->strip[ch].up1[0].data = (int*)jpegxr_calloc(256 * EXTENDED_WIDTH_BLOCKS(image), sizeof(int));
        for (idx = 1 ; idx < EXTENDED_WIDTH_BLOCKS(image) ; idx += 1)
            image->strip[ch].up1[idx].data = image->strip[ch].up1[idx-1].data + 256;

        image->strip[ch].cur[0].data = (int*)jpegxr_calloc(256 * EXTENDED_WIDTH_BLOCKS(image), sizeof(int));
        for (idx = 1 ; idx < EXTENDED_WIDTH_BLOCKS(image) ; idx += 1)
            image->strip[ch].cur[idx].data = image->strip[ch].cur[idx-1].data + 256;

        if (up4_flag) {
            image->strip[ch].up4[0].pred_dclp = (int*)jpegxr_calloc(7*EXTENDED_WIDTH_BLOCKS(image), sizeof(int));
            for (idx = 1 ; idx < EXTENDED_WIDTH_BLOCKS(image) ; idx += 1)
                image->strip[ch].up4[idx].pred_dclp = image->strip[ch].up4[idx-1].pred_dclp + 7;
        }

        image->strip[ch].up3[0].pred_dclp = (int*)jpegxr_calloc(7*EXTENDED_WIDTH_BLOCKS(image), sizeof(int));
        for (idx = 1 ; idx < EXTENDED_WIDTH_BLOCKS(image) ; idx += 1)
            image->strip[ch].up3[idx].pred_dclp = image->strip[ch].up3[idx-1].pred_dclp + 7;

        image->strip[ch].up2[0].pred_dclp = (int*)jpegxr_calloc(7*EXTENDED_WIDTH_BLOCKS(image), sizeof(int));
        for (idx = 1 ; idx < EXTENDED_WIDTH_BLOCKS(image) ; idx += 1)
            image->strip[ch].up2[idx].pred_dclp = image->strip[ch].up2[idx-1].pred_dclp + 7;

        image->strip[ch].up1[0].pred_dclp = (int*)jpegxr_calloc(7*EXTENDED_WIDTH_BLOCKS(image), sizeof(int));
        for (idx = 1 ; idx < EXTENDED_WIDTH_BLOCKS(image) ; idx += 1)
            image->strip[ch].up1[idx].pred_dclp = image->strip[ch].up1[idx-1].pred_dclp + 7;

        image->strip[ch].cur[0].pred_dclp = (int*)jpegxr_calloc(7*EXTENDED_WIDTH_BLOCKS(image), sizeof(int));
        for (idx = 1 ; idx < EXTENDED_WIDTH_BLOCKS(image) ; idx += 1)
            image->strip[ch].cur[idx].pred_dclp = image->strip[ch].cur[idx-1].pred_dclp + 7;

        if(ch!= 0)
        {
            if(image->use_clr_fmt == 2 || image->use_clr_fmt == 1) /* 422 or 420 */
                image->strip[ch].upsample_memory_x = (int*)jpegxr_calloc(16, sizeof(int));

            if(image->use_clr_fmt == 1)/* 420 */
                image->strip[ch].upsample_memory_y = (int*)jpegxr_calloc(8*EXTENDED_WIDTH_BLOCKS(image), sizeof(int));
        }
        
    }

    /* If there is tiling (in columns) then allocate a tile buffer
    that can hold an entire row of tiles. */
    if (FREQUENCY_MODE_CODESTREAM_FLAG(image)) { /* FREQUENCY MODE */

        make_mb_row_buffer(image, EXTENDED_HEIGHT_BLOCKS(image));

    } else { /* SPATIAL */
        if (INDEXTABLE_PRESENT_FLAG(image)) { 
            /* This means that the tiling flag is used */
            unsigned max_tile_height = 0;
            unsigned idx;
            for (idx = 0 ; idx < image->tile_rows ; idx += 1) {
                if (image->tile_row_height[idx] > max_tile_height)
                    max_tile_height = image->tile_row_height[idx];
            }

            make_mb_row_buffer(image, max_tile_height);

            /* Save enough context MBs for 4 rows of
            macroblocks. */

            int format_scale = 256;
            if (image->use_clr_fmt == 2 /* YUV422 */) {
                format_scale = 16 + 8*15;
            } else if (image->use_clr_fmt == 1 /* YUV420 */) {
                format_scale = 16 + 4*15;
            }

            int ch;
            for (ch = 0 ; ch < image->num_channels ; ch += 1) {
                int count = (ch==0)? 256 : format_scale;
                image->mb_row_context[ch] = (struct macroblock_s*)
                    jpegxr_calloc(4*EXTENDED_WIDTH_BLOCKS(image), sizeof(struct macroblock_s));
                image->mb_row_context[ch][0].data = (int*)
                    jpegxr_calloc(4*EXTENDED_WIDTH_BLOCKS(image)*count, sizeof(int));
                for (idx = 1 ; idx < 4*EXTENDED_WIDTH_BLOCKS(image) ; idx += 1)
                    image->mb_row_context[ch][idx].data = image->mb_row_context[ch][idx-1].data+count;
            }

        }
    }

    /* since CBP processing is done at each row, need to save contexts is multiple tile columns */
    image->model_hp_buffer = 0;
    image->hp_cbp_model_buffer = 0;
    if (image->tile_columns > 1) {
        image->model_hp_buffer = (struct model_s*)
            jpegxr_calloc(image->tile_columns, sizeof(struct model_s));
        image->hp_cbp_model_buffer = (struct cbp_model_s*)
            jpegxr_calloc(image->tile_columns, sizeof(struct cbp_model_s));
    }

    image->cur_my = -1;
}

jxr_image_t jxr_create_input(void)
{
    struct jxr_image*image = __make_jxr();

    return image;
}

jxr_image_t jxr_create_image(int width, int height, unsigned char * windowing)
{
    if (width == 0 || height == 0)
        return 0;

    struct jxr_image*image = __make_jxr();

    if (windowing[0] == 1) {
        assert(((width+windowing[2]+windowing[4]) & 0x0f) == 0);
        assert(((height+windowing[1]+windowing[3]) & 0x0f) == 0);
    }
    else {
        windowing[1] = windowing[2] = 0;
        windowing[3] = (((height + 15) >> 4) << 4) - height;
        windowing[4] = (((width + 15) >> 4) << 4) - width;
    }

    image->width1 = width-1;
    image->height1 = height-1;
    image->extended_width = image->width1 + 1 + windowing[2] + windowing[4];
    image->extended_height = image->height1 + 1 + windowing[1] + windowing[3];

    image->dc_frame_uniform = 1;
    image->lp_frame_uniform = 1;
    image->lp_use_dc_qp = 0;
    image->num_lp_qps = 1;
    image->hp_use_lp_qp = 0;
    image->hp_frame_uniform = 1;
    image->num_hp_qps = 1;

    image->window_extra_top = windowing[1];
    image->window_extra_left = windowing[2];
    image->window_extra_bottom = windowing[3];
    image->window_extra_right = windowing[4];

    return image;
}

void jxr_flag_SKIP_HP_DATA(jxr_image_t image, int flag)
{
    if (flag)
        image->user_flags |= 0x0001;
    else
        image->user_flags &= ~0x0001;
}

void jxr_flag_SKIP_FLEX_DATA(jxr_image_t image, int flag)
{
    if (flag)
        image->user_flags |= 0x0002;
    else
        image->user_flags &= ~0x0002;
}

void jxr_destroy(jxr_image_t image)
{
    int idx, plane_idx = 1;
    if(image == NULL)
        return;

    if (ALPHACHANNEL_FLAG(image))
        plane_idx = 2;
    
    for (; plane_idx > 0; plane_idx --) {
        jxr_image_t plane = (plane_idx == 1 ? image : image->alpha);

        for (idx = 0 ; idx < plane->num_channels ; idx += 1) {
            if (plane->strip[idx].up4) {
                jpegxr_free(plane->strip[idx].up4[0].data);
                jpegxr_free(plane->strip[idx].up4);
            }
            if (plane->strip[idx].up3) {
                jpegxr_free(plane->strip[idx].up3[0].data);
                jpegxr_free(plane->strip[idx].up3);
            }
            if (plane->strip[idx].up2) {
                jpegxr_free(plane->strip[idx].up2[0].data);
                jpegxr_free(plane->strip[idx].up2);
            }
            if (plane->strip[idx].up1) {
                jpegxr_free(plane->strip[idx].up1[0].data);
                jpegxr_free(plane->strip[idx].up1);
            }
            if (plane->strip[idx].cur) {
                jpegxr_free(plane->strip[idx].cur[0].data);
                jpegxr_free(plane->strip[idx].cur);
            }
            if(plane->strip[idx].upsample_memory_x)
                jpegxr_free(plane->strip[idx].upsample_memory_x);
            if(plane->strip[idx].upsample_memory_y)
                jpegxr_free(plane->strip[idx].upsample_memory_y);

        }

        for (idx = 0 ; idx < plane->num_channels ; idx += 1) {
            if (plane->mb_row_buffer[idx]) {
                jpegxr_free(plane->mb_row_buffer[idx][0].data);
                jpegxr_free(plane->mb_row_buffer[idx]);
            }

            if (plane->mb_row_context[idx]) {
                jpegxr_free(plane->mb_row_context[idx][0].data);
                jpegxr_free(plane->mb_row_context[idx]);
            }
        }

        if (plane->model_hp_buffer) {
            jpegxr_free(plane->model_hp_buffer);
        }

        if (plane->hp_cbp_model_buffer) {
            jpegxr_free(plane->hp_cbp_model_buffer);
        }

        if(plane_idx == 1){
            if (plane->tile_index_table)
                jpegxr_free(plane->tile_index_table);
            if (plane->tile_column_width)
                jpegxr_free(plane->tile_column_width);
            if (plane->tile_row_height)
                jpegxr_free(plane->tile_row_height);
        }
        jpegxr_free(plane);
    }    
}

/*
* $Log: init.c,v $
* Revision 1.25 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.24 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.23 2008/03/13 21:23:27 steve
* Add pipeline step for YUV420.
*
* Revision 1.22 2008/03/05 06:58:10 gus
* *** empty log message ***
*
* Revision 1.21 2008/02/28 18:50:31 steve
* Portability fixes.
*
* Revision 1.20 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.19 2008/01/04 17:07:35 steve
* API interface for setting QP values.
*
* Revision 1.18 2007/11/26 01:47:15 steve
* Add copyright notices per MS request.
*
* Revision 1.17 2007/11/22 19:02:05 steve
* More fixes of color plane buffer sizes.
*
* Revision 1.16 2007/11/21 23:26:14 steve
* make all strip buffers store MB data.
*
* Revision 1.15 2007/11/21 00:34:30 steve
* Rework spatial mode tile macroblock shuffling.
*
* Revision 1.14 2007/11/15 17:44:13 steve
* Frequency mode color support.
*
* Revision 1.13 2007/11/14 23:56:17 steve
* Fix TILE ordering, using seeks, for FREQUENCY mode.
*
* Revision 1.12 2007/11/12 23:21:54 steve
* Infrastructure for frequency mode ordering.
*
* Revision 1.11 2007/11/08 02:52:32 steve
* Some progress in some encoding infrastructure.
*
* Revision 1.10 2007/11/05 02:01:12 steve
* Add support for mixed row/column tiles.
*
* Revision 1.9 2007/11/01 21:09:40 steve
* Multiple rows of tiles.
*
* Revision 1.8 2007/10/30 21:32:46 steve
* Support for multiple tile columns.
*
* Revision 1.7 2007/09/08 01:01:43 steve
* YUV444 color parses properly.
*
* Revision 1.6 2007/09/04 19:10:46 steve
* Finish level1 overlap filtering.
*
* Revision 1.5 2007/08/15 01:54:11 steve
* Add level2 filter to decoder.
*
* Revision 1.4 2007/08/04 00:15:31 steve
* Allow width/height of 1 pixel.
*
* Revision 1.3 2007/07/21 00:25:48 steve
* snapshot 2007 07 20
*
* Revision 1.2 2007/06/28 20:03:11 steve
* LP processing seems to be OK now.
*
* Revision 1.1 2007/06/06 17:19:12 steve
* Introduce to CVS.
*
*/

