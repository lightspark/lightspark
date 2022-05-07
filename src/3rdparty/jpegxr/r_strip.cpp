
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
#pragma comment (user,"$Id: r_strip.c,v 1.51 2008/03/24 18:06:56 steve Exp $")
#else
#ident "$Id: r_strip.c,v 1.51 2008/03/24 18:06:56 steve Exp $"
#endif

# include "jxr_priv.h"
# include <limits.h>
# include <assert.h>
# include <math.h>
# include <memory.h>

static void dclphp_shuffle(int*data, int dclp_count);
static void unblock_shuffle444(int*data);
static void unblock_shuffle422(int*data);
static void unblock_shuffle420(int*data);

static void dequantize_up_dclp(jxr_image_t image, int use_my, int ch)
{
    int tx, ty = 0;
    int dc_quant = 0;

    int lp_coeff_count = 16;
    if (ch > 0) {
        if (image->use_clr_fmt == 2/*YUV422*/)
            lp_coeff_count = 8;
        else if (image->use_clr_fmt == 1/*YUV420*/)
            lp_coeff_count = 4;
    }

    unsigned int strip = use_my -1;
    unsigned int i;
    for(i=0; i < image->tile_rows; i++)
    {
        /* Figure out what ty is */
        if(strip >= image->tile_row_position[i] && strip <image->tile_row_position[i] + image->tile_row_height[i])
        {
            ty = i;
            break;
        }
    }

    /* The current "cur" is now made up into DC coefficients, so
    we no longer need the strip_up levels. Dequantize them,
    inverse tranform then and deliver them to output. */
    for (tx = 0 ; tx < (int) image->tile_columns ; tx += 1) {
        int mx;
        if(image->dc_frame_uniform)
            dc_quant = image->dc_quant_ch[ch];
        else
            dc_quant = image->tile_quant[ty *(image->tile_columns) + tx].dc_quant_ch[ch];
        dc_quant = _jxr_quant_map(image, dc_quant, ch==0? 1 : 0/* iShift for YONLY */);

        for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
            int lp_quant_idx = MACROBLK_UP1_LP_QUANT(image,ch,tx,mx);

            int lp_quant_raw = 0;
            if(image->lp_frame_uniform)
                lp_quant_raw = image->lp_quant_ch[ch][lp_quant_idx];
            else
                lp_quant_raw = image->tile_quant[ty *(image->tile_columns) + tx].lp_quant_ch[ch][lp_quant_idx];

            int lp_quant_use = _jxr_quant_map(image, lp_quant_raw, ch==0? 1 : 0/* iShift for YONLY */);
            MACROBLK_UP_DC(image,ch,tx,mx) *= dc_quant;
            CHECK1(image->lwf_test, MACROBLK_CUR_DC(image,ch,tx,mx));

            DEBUG(" Dequantize strip=%d tx=%d MBx=%d ch=%d with lp_quant=%d lp_quant_use=%d\n",
                use_my-1, tx, mx, ch, lp_quant_raw, lp_quant_use);
            int k;
            for (k = 1 ; k < lp_coeff_count ; k += 1)
            {
                MACROBLK_UP_LP(image,ch,tx,mx,k-1) *= lp_quant_use;
                CHECK1(image->lwf_test, MACROBLK_UP_LP(image,ch,tx,mx,k-1));
            }
        }
    }

#if defined(DETAILED_DEBUG)
    for (tx = 0 ; tx < (int) image->tile_columns ; tx += 1) {
        int mx;
        for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
            int jdx;
            DEBUG(" DC/LP (strip=%3d, tx=%d mbx=%4d, ch=%d) Dequant:", use_my-1, tx, mx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,tx,mx));
            for (jdx = 0; jdx < lp_coeff_count-1 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,tx,mx,jdx));
                if ((jdx+1)%4 == 3 && jdx != (lp_coeff_count-2))
                    DEBUG("\n%*s:", 48, "");
            }
            DEBUG("\n");
        }
    }
#endif
}


static void IPCT_level1_up1(jxr_image_t image, int use_my, int ch)
{
    int idx;

    dequantize_up_dclp(image, use_my, ch);

    DEBUG(" DC-LP IPCT transforms (first level) for strip %d channel %d\n", use_my-1, ch);

    /* Reverse transform the DC/LP to 16 DC values. */

    for (idx = 0 ; idx < (int) EXTENDED_WIDTH_BLOCKS(image); idx += 1) {
        DEBUG(" DC-LP IPCT transforms for mb[%d %d]\n", idx, use_my-1);

        if (ch > 0 && image->use_clr_fmt == 1/*YUV420*/) {

            _jxr_2x2IPCT(image->strip[ch].up1[idx].data+0);
            _jxr_InvPermute2pt(image->strip[ch].up1[idx].data+1,
                image->strip[ch].up1[idx].data+2);

            /* Scale up the chroma channel */
            if (image->scaled_flag) {
                int jdx;
                for (jdx = 0 ; jdx < 4 ; jdx += 1)
                    image->strip[ch].up1[idx].data[jdx] *= 2;
            }

#if defined(DETAILED_DEBUG)
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) IPCT:", use_my-1, idx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,0,idx));
            DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,0,idx,0));
            DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,0,idx,1));
            DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,0,idx,2));
            DEBUG("\n");
#endif
        } else if (ch > 0 && image->use_clr_fmt == 2/*YUV422*/) {
#if defined(DETAILED_DEBUG)
            int jdx;
            DEBUG(" DC/LP scaled_flag=%d\n", image->scaled_flag);
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) Pre-IPCT:", use_my-1, idx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,0,idx));
            for (jdx = 0; jdx < 7 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,0,idx,jdx));
                if ((jdx+1)%4 == 3 && jdx != 6)
                    DEBUG("\n%*s:", 44, "");
            }
            DEBUG("\n");
#endif

            _jxr_2ptT(image->strip[ch].up1[idx].data+0,
                image->strip[ch].up1[idx].data+4);
            _jxr_2x2IPCT(image->strip[ch].up1[idx].data+0);
            _jxr_2x2IPCT(image->strip[ch].up1[idx].data+4);

            _jxr_InvPermute2pt(image->strip[ch].up1[idx].data+1,
                image->strip[ch].up1[idx].data+2);
            _jxr_InvPermute2pt(image->strip[ch].up1[idx].data+5,
                image->strip[ch].up1[idx].data+6);

#if defined(DETAILED_DEBUG)
            DEBUG(" DC/LP scaled_flag=%d\n", image->scaled_flag);
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) scaled:", use_my-1, idx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,0,idx));
            for (jdx = 0; jdx < 7 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,0,idx,jdx));
                if ((jdx+1)%4 == 3 && jdx != 6)
                    DEBUG("\n%*s:", 42, "");
            }
            DEBUG("\n");
#endif
            /* Scale up the chroma channel */
            if (image->scaled_flag) {
                int jdx;
                for (jdx = 0 ; jdx < 8 ; jdx += 1)
                    image->strip[ch].up1[idx].data[jdx] *= 2;
            }

#if defined(DETAILED_DEBUG)
            DEBUG(" DC/LP scaled_flag=%d\n", image->scaled_flag);
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) IPCT:", use_my-1, idx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,0,idx));
            for (jdx = 0; jdx < 7 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,0,idx,jdx));
                if ((jdx+1)%4 == 3 && jdx != 6)
                    DEBUG("\n%*s:", 40, "");
            }
            DEBUG("\n");
#endif
        } else {

            /* Channel 0 of everything, and Channel-N of full
            resolution colors, are processed here. */
            _jxr_4x4IPCT(image->strip[ch].up1[idx].data);

            /* Scale up the chroma channel */
            if (ch > 0 && image->scaled_flag) {
                int jdx;
                for (jdx = 0 ; jdx < 16 ; jdx += 1)
                    image->strip[ch].up1[idx].data[jdx] *= 2;
            }

#if defined(DETAILED_DEBUG)
            int jdx;
            DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) IPCT:", use_my-1, idx, ch);
            DEBUG(" 0x%08x", MACROBLK_UP_DC(image,ch,0,idx));
            for (jdx = 0; jdx < 15 ; jdx += 1) {
                DEBUG(" 0x%08x", MACROBLK_UP_LP(image,ch,0,idx,jdx));
                if ((jdx+1)%4 == 3 && jdx != 14)
                    DEBUG("\n%*s:", 40, "");
            }
            DEBUG("\n");
#endif
        }

    }

}

static void IPCT_level2_up2(jxr_image_t image, int /*use_my*/, int ch)
{
    int idx;

    for (idx = 0 ; idx < (int) EXTENDED_WIDTH_BLOCKS(image); idx += 1) {
        int jdx;
        /* Reshuffle the DCLP with the HP data to get
        DC-LP stretches in the data stream. */
        int dclp_count = 16;
        if (ch>0 && image->use_clr_fmt == 2/*YUV422*/)
            dclp_count = 8;
        else if (ch>0 && image->use_clr_fmt == 1/*YUV420*/)
            dclp_count = 4;

        dclphp_shuffle(image->strip[ch].up2[idx].data, dclp_count);

        DEBUG(" DC-LP-HP IPCT transforms for (second level) strip %d MBx=%d ch=%d\n",
            use_my-2, idx, ch);

        int hp_quant_raw = MACROBLK_UP2_HP_QUANT(image,ch,0,idx);
        int hp_quant = _jxr_quant_map(image, hp_quant_raw, 1);

        /* IPCT transform to absorb HP band data. */
        for (jdx = 0 ; jdx < 16*dclp_count ; jdx += 16) {
#if defined(DETAILED_DEBUG)
            {
                int pix;
                DEBUG(" DC-LP-HP (strip=%3d, mbx=%4d ch=%d, block=%2d) pre-IPCT:",
                    use_my-2, idx, ch, jdx/16);
                for (pix = 0; pix < 16 ; pix += 1) {
                    DEBUG(" 0x%08x", image->strip[ch].up2[idx].data[jdx+pix]);
                    if (pix%4 == 3 && pix != 15)
                        DEBUG("\n%*s:", 56, "");
                }
                DEBUG("\n");
            }
#endif
            DEBUG(" Dequantize strip=%d MBx=%d ch=%d block=%d with hp_quant=%d (raw=%d)\n",
                use_my-2, idx, ch, jdx/16, hp_quant, hp_quant_raw);
            int k;
            for (k = 1 ; k < 16 ; k += 1)
            {
                image->strip[ch].up2[idx].data[jdx+k] *= hp_quant;
                CHECK1(image->lwf_test, image->strip[ch].up2[idx].data[jdx+k]);
            }

            _jxr_4x4IPCT(image->strip[ch].up2[idx].data+jdx);
#if defined(DETAILED_DEBUG)
            {
                int pix;
                DEBUG(" DC-LP-HP (strip=%3d, mbx=%4d ch=%d block=%2d) IPCT:",
                    use_my-2, idx, ch, jdx/16);
                for (pix = 0; pix < 16 ; pix += 1) {
                    DEBUG(" 0x%08x", image->strip[ch].up2[idx].data[jdx+pix]);
                    if (pix%4 == 3 && pix != 15)
                        DEBUG("\n%*s:", 51, "");
                }
                DEBUG("\n");
            }
#endif
        }

    }
}

#define TOP_Y(y) ( y == image->tile_row_position[ty])
#define BOTTOM_Y(y) ( y == image->tile_row_position[ty] + image->tile_row_height[ty] - 1)
#define LEFT_X(idx) ( idx == 0)
#define RIGHT_X(idx) ( idx == image->tile_column_width[tx] -1 )


static void overlap_level1_up2_444(jxr_image_t image, int use_my, int ch)
{
    /* 16 Coeffs per MB */
    assert(ch == 0 || (image->use_clr_fmt != 2/*YUV422*/ && image->use_clr_fmt !=1/* YUV420*/));
    uint32_t tx = 0; /* XXXX */
    assert(use_my >= 2);
    uint32_t top_my = use_my - 2;
    uint32_t idx;

    uint32_t ty = 0;
    /* Figure out which tile row the current strip of macroblocks belongs to. */
    while(top_my > image->tile_row_position[ty]+image->tile_row_height[ty] - 1)
        ty++;

    for(tx = 0; tx < image->tile_columns; tx++)
    {
        /* Top edge */
        if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my) ))
        {
            /* If this is the very first strip of blocks, then process the
            first two scan lines with the smaller 4Overlap filter. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                /* Top edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                    int*tp0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                    int*tp1 = MACROBLK_UP2(image,ch,tx,idx-1).data; /* Macroblock to the right */

                    _jxr_4OverlapFilter(tp1+2, tp1+3, tp0+0, tp0+1);
                    _jxr_4OverlapFilter(tp1+6, tp1+7, tp0+4, tp0+5);
                }
            }
            /* Top left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                _jxr_4OverlapFilter(tp0+0, tp0+1, tp0+4, tp0+5);
            }
            /* Top right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
                _jxr_4OverlapFilter(tp0+2, tp0+3, tp0+6, tp0+7);
            }
        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. finish up with 4Overlap filters. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                /* Bottom edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {

                        int*tp0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                        int*tp1 = MACROBLK_UP2(image,ch,tx,idx-1).data;
                        _jxr_4OverlapFilter(tp1+10, tp1+11, tp0+8, tp0+9);
                        _jxr_4OverlapFilter(tp1+14, tp1+15, tp0+12, tp0+13);
                }
            }

            /* Bottom left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                _jxr_4OverlapFilter(tp0+8, tp0+9, tp0+12, tp0+13);
            }
            /* Bottom right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
                _jxr_4OverlapFilter(tp0+10, tp0+11, tp0+14, tp0+15);
            }
        }

        for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {
            if ((top_my+1) < EXTENDED_HEIGHT_BLOCKS(image)) {

                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                        int*up0 = MACROBLK_UP1(image,ch,tx,0).data;

                        /* Left edge Across Vertical MBs */
                        _jxr_4OverlapFilter(tp0+8, tp0+12, up0+0, up0+4);
                        _jxr_4OverlapFilter(tp0+9, tp0+13, up0+1, up0+5);
                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    ) {
                        /* This assumes that the DCLP coefficients are the first
                        16 values in the array, and ordered properly. */
                        int*tp0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                        int*tp1 = MACROBLK_UP2(image,ch,tx,idx+1).data;
                        int*up0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                        int*up1 = MACROBLK_UP1(image,ch,tx,idx+1).data;

                        /* MB below, right, right-below */
                        _jxr_4x4OverlapFilter(tp0+10, tp0+11, tp1+ 8, tp1+ 9,
                            tp0+14, tp0+15, tp1+12, tp1+13,
                            up0+ 2, up0+ 3, up1+ 0, up1+ 1,
                            up0+ 6, up0+ 7, up1+ 4, up1+ 5);
                }
                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    int*tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
                    int*up0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;

                    /* Right edge Across Vertical MBs */
                    _jxr_4OverlapFilter(tp0+10, tp0+14, up0+2, up0+6);
                    _jxr_4OverlapFilter(tp0+11, tp0+15, up0+3, up0+7);
                }
            }
        }
    }
}

/*
*/

static void overlap_level1_up2_422(jxr_image_t image, int use_my, int ch)
{
    assert(ch > 0 && image->use_clr_fmt == 2/*YUV422*/);
    uint32_t tx = 0; /* XXXX */
    assert(use_my >= 2);
    uint32_t top_my = use_my - 2;
    uint32_t idx;

    uint32_t ty = 0;
    /* Figure out which tile row the current strip of macroblocks belongs to. */
    while(top_my > image->tile_row_position[ty]+image->tile_row_height[ty] - 1)
        ty++;


    /* Top edge */
    if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my)))
    {
        for(tx = 0; tx < image->tile_columns; tx++)
        {
            /* Top Left Corner Difference */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                tp0[0] = tp0[0] -tp0[1];
                CHECK1(image->lwf_test, tp0[0]);
            }
            /* Top Right Corner Difference */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
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
                int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                tp0[6] = tp0[6] -tp0[7];
                CHECK1(image->lwf_test, tp0[6]);
            }
            /* Bottom Right Corner Difference */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
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
            int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
            _jxr_2OverlapFilter(tp0+2, tp0+4);
        }

        /* Right edge */
        if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
        {
            int*tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
            /* Interior Right edge */
            _jxr_2OverlapFilter(tp0+3, tp0+5);
        }


        /* Top edge */
        if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my) ))
        {
            /* If this is the very first strip of blocks, then process the
            first two scan lines with the smaller 4Overlap filter. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                /* Top edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                    int*tp0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                    int*tp1 = MACROBLK_UP2(image,ch,tx,idx-1).data; /* The macroblock to the right */

                    _jxr_2OverlapFilter(tp1+1, tp0+0);
                }
            }
        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. finish up with 4Overlap filters. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                /* Bottom edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                        int*tp0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                        int*tp1 = MACROBLK_UP2(image,ch,tx,idx - 1).data;
                        _jxr_2OverlapFilter(tp1+7, tp0+6);
                }
            }
        }

        for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {
            if(top_my< EXTENDED_HEIGHT_BLOCKS(image) -1)
            {
                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        /* Across vertical blocks, left edge */
                        int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                        int*up0 = MACROBLK_UP1(image,ch,tx,0).data;

                        /* Left edge across vertical MBs */
                        _jxr_2OverlapFilter(tp0+6, up0+0);
                }

                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    int*tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
                    int*up0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;

                    /* Right edge across MBs */
                    _jxr_2OverlapFilter(tp0+7, up0+1);
                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    )
                {
                    /* This assumes that the DCLP coefficients are the first
                    16 values in the array, and ordered properly. */
                    int*tp0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                    int*tp1 = MACROBLK_UP2(image,ch,tx,idx+1).data;
                    int*up0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                    int*up1 = MACROBLK_UP1(image,ch,tx,idx+1).data;

                    /* MB below, right, right-below */
                    _jxr_2x2OverlapFilter(tp0+7, tp1+6, up0+1, up1+0);
                }
            }

            if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                ( image->disableTileOverlapFlag && !RIGHT_X(idx) )
                )
            {
                /* This assumes that the DCLP coefficients are the first
                16 values in the array, and ordered properly. */
                int*tp0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                int*tp1 = MACROBLK_UP2(image,ch,tx,idx+1).data;

                /* MB to the right */
                _jxr_2x2OverlapFilter(tp0+3, tp1+2, tp0+5, tp1+4);
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
                int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                tp0[0] = tp0[0] + tp0[1];
                CHECK1(image->lwf_test, tp0[0]);
            }
            /* Top Right Corner Addition */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
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
                int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                tp0[6] = tp0[6] + tp0[7];
                CHECK1(image->lwf_test, tp0[6]);
            }
            /* Bottom Right Corner Addition */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
                tp0[7] = tp0[7] + tp0[6];
                CHECK1(image->lwf_test, tp0[7]);
            }
        }
    }
}

static void overlap_level1_up2_420(jxr_image_t image, int use_my, int ch)
{
    /* 4 coeffs*/
    assert(ch > 0 && image->use_clr_fmt == 1/*YUV420*/);
    uint32_t tx = 0; /* XXXX */
    assert(use_my >= 2);
    uint32_t top_my = use_my - 2;

    uint32_t idx;
    uint32_t ty = 0;
    /* Figure out which tile row the current strip of macroblocks belongs to. */
    while(top_my > image->tile_row_position[ty]+image->tile_row_height[ty] - 1)
        ty++;

    if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my)))
    {
        for(tx = 0; tx < image->tile_columns; tx++)
        {
            /* Top Left Corner Difference*/
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                tp0[0] = tp0[0] -tp0[1];
                CHECK1(image->lwf_test, tp0[0]);
            }
            /* Top Right Corner Difference*/
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
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
                int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                tp0[2] = tp0[2] -tp0[3];
                CHECK1(image->lwf_test, tp0[2]);
            }
            /* Bottom Right Corner Difference*/
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
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
                    int*tp0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                    int*tp1 = MACROBLK_UP2(image,ch,tx,idx-1).data;
                    _jxr_2OverlapFilter(tp1+1, tp0+0);
                }
            }
        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. finish up with 4Overlap filters. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                /* Bottom edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                        int*tp0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                        int*tp1 = MACROBLK_UP2(image,ch,tx,idx-1).data;
                        _jxr_2OverlapFilter(tp1+3, tp0+2);
                }
            }
        }
        else
        {
            for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {

                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        /* Left edge across vertical MBs */
                        int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                        int*up0 = MACROBLK_UP1(image,ch,tx,0).data;

                        _jxr_2OverlapFilter(tp0+2, up0+0);
                }

                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    /* Right edge across vertical MBs */
                    int*tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
                    int*up0 = MACROBLK_UP1(image,ch,tx,image->tile_column_width[tx]-1).data;

                    _jxr_2OverlapFilter(tp0+3, up0+1);
                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    )
                {
                    /* This assumes that the DCLP coefficients are the first
                    16 values in the array, and ordered properly. */
                    /* MB below, right, right-below */
                    int*tp0 = MACROBLK_UP2(image,ch,tx,idx+0).data;
                    int*tp1 = MACROBLK_UP2(image,ch,tx,idx+1).data;
                    int*up0 = MACROBLK_UP1(image,ch,tx,idx+0).data;
                    int*up1 = MACROBLK_UP1(image,ch,tx,idx+1).data;

                    _jxr_2x2OverlapFilter(tp0+3, tp1+2,
                        up0+1, up1+0);
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
                int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                tp0[0] = tp0[0] + tp0[1];
                CHECK1(image->lwf_test, tp0[0]);
            }
            /* Top Right Corner Addition */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
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
                int*tp0 = MACROBLK_UP2(image,ch,tx,0).data;
                tp0[2] = tp0[2] + tp0[3];
                CHECK1(image->lwf_test, tp0[2]);
            }
            /* Bottom Right Corner Addition*/
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *tp0 = MACROBLK_UP2(image,ch,tx,image->tile_column_width[tx]-1).data;
                tp0[3] = tp0[3] + tp0[2];
                CHECK1(image->lwf_test, tp0[3]);
            }
        }
    }
}


static void overlap_level1_up2(jxr_image_t image, int use_my, int ch)
{
    if (ch == 0) {
        overlap_level1_up2_444(image, use_my, ch);

    } 
    else {
        switch (image->use_clr_fmt) {
            case 1: /*YUV420*/
                overlap_level1_up2_420(image, use_my, ch);
                break;
            case 2: /*YUV422*/
                overlap_level1_up2_422(image, use_my, ch);
                break;
            default:
                overlap_level1_up2_444(image, use_my, ch);
                break;
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

static void overlap_level2_up3_444(jxr_image_t image, int use_my, int ch)
{
    assert(ch == 0 || (image->use_clr_fmt != 2/*YUV422*/ && image->use_clr_fmt !=1/* YUV420*/));
    uint32_t tx = 0; /* XXXX */
    assert(use_my >= 3);
    uint32_t top_my = use_my - 3;
    uint32_t idx;

    DEBUG("Overlap Level2 for row %d\n", top_my);

    uint32_t ty = 0;
    /* Figure out which tile row the current strip of macroblocks belongs to. */
    while(top_my > image->tile_row_position[ty]+image->tile_row_height[ty] - 1)
        ty++;

    for(tx = 0; tx < image->tile_columns; tx++)
    {
        int jdx;
        /* Left edge */
        if (tx == 0 || image->disableTileOverlapFlag)
        {
            int*dp = MACROBLK_UP3(image,ch,tx,0).data;
            for (jdx = 2 ; jdx < 14 ; jdx += 4) {
                _jxr_4OverlapFilter(R2B(dp,0,jdx+0),R2B(dp,0,jdx+1),R2B(dp,0,jdx+2),R2B(dp,0,jdx+3));
                _jxr_4OverlapFilter(R2B(dp,1,jdx+0),R2B(dp,1,jdx+1),R2B(dp,1,jdx+2),R2B(dp,1,jdx+3));
            }
        }

        /* Right edge */
        if(tx == image->tile_columns -1 || image->disableTileOverlapFlag){
            int*dp = MACROBLK_UP3(image,ch,tx,image->tile_column_width[tx]-1).data;
            for (jdx = 2 ; jdx < 14 ; jdx += 4) {
                _jxr_4OverlapFilter(R2B(dp,14,jdx+0),R2B(dp,14,jdx+1),R2B(dp,14,jdx+2),R2B(dp,14,jdx+3));
                _jxr_4OverlapFilter(R2B(dp,15,jdx+0),R2B(dp,15,jdx+1),R2B(dp,15,jdx+2),R2B(dp,15,jdx+3));
            }
        }

        /* Top edge */
        if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my) ))
        {
            /* If this is the very first strip of blocks, then process the
            first two scan lines with the smaller 4Overlap filter. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*dp = MACROBLK_UP3(image,ch,tx,idx).data;
                _jxr_4OverlapFilter(R2B(dp, 2,0),R2B(dp, 3,0),R2B(dp, 4,0),R2B(dp, 5,0));
                _jxr_4OverlapFilter(R2B(dp, 6,0),R2B(dp, 7,0),R2B(dp, 8,0),R2B(dp, 9,0));
                _jxr_4OverlapFilter(R2B(dp,10,0),R2B(dp,11,0),R2B(dp,12,0),R2B(dp,13,0));

                _jxr_4OverlapFilter(R2B(dp, 2,1),R2B(dp, 3,1),R2B(dp, 4,1),R2B(dp, 5,1));
                _jxr_4OverlapFilter(R2B(dp, 6,1),R2B(dp, 7,1),R2B(dp, 8,1),R2B(dp, 9,1));
                _jxr_4OverlapFilter(R2B(dp,10,1),R2B(dp,11,1),R2B(dp,12,1),R2B(dp,13,1));

                /* Top edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                    int*pp = MACROBLK_UP3(image,ch,tx,idx-1).data;
                    _jxr_4OverlapFilter(R2B(pp,14,0),R2B(pp,15,0),R2B(dp,0,0),R2B(dp,1,0));
                    _jxr_4OverlapFilter(R2B(pp,14,1),R2B(pp,15,1),R2B(dp,0,1),R2B(dp,1,1));
                }
            }

            /* Top left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch, tx, 0).data;
                _jxr_4OverlapFilter(R2B(dp, 0,0),R2B(dp, 1,0),R2B(dp, 0,1),R2B(dp, 1,1));
            }
            /* Top right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4OverlapFilter(R2B(dp, 14,0),R2B(dp, 15,0),R2B(dp, 14,1),R2B(dp, 15,1));
            }

        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. finish up with 4Overlap filters. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*tp = MACROBLK_UP3(image,ch,tx,idx).data;

                _jxr_4OverlapFilter(R2B(tp, 2,14),R2B(tp, 3,14),R2B(tp, 4,14),R2B(tp, 5,14));
                _jxr_4OverlapFilter(R2B(tp, 6,14),R2B(tp, 7,14),R2B(tp, 8,14),R2B(tp, 9,14));
                _jxr_4OverlapFilter(R2B(tp,10,14),R2B(tp,11,14),R2B(tp,12,14),R2B(tp,13,14));

                _jxr_4OverlapFilter(R2B(tp, 2,15),R2B(tp, 3,15),R2B(tp, 4,15),R2B(tp, 5,15));
                _jxr_4OverlapFilter(R2B(tp, 6,15),R2B(tp, 7,15),R2B(tp, 8,15),R2B(tp, 9,15));
                _jxr_4OverlapFilter(R2B(tp,10,15),R2B(tp,11,15),R2B(tp,12,15),R2B(tp,13,15));

                /* Bottom edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                        int*tn = MACROBLK_UP3(image,ch,tx,idx-1).data;
                        _jxr_4OverlapFilter(R2B(tn,14,14),R2B(tn,15,14),R2B(tp, 0,14),R2B(tp, 1,14));
                        _jxr_4OverlapFilter(R2B(tn,14,15),R2B(tn,15,15),R2B(tp, 0,15),R2B(tp, 1,15));
                }
            }

            /* Bottom left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch,tx,0).data;
                _jxr_4OverlapFilter(R2B(dp, 0,14),R2B(dp, 1, 14),R2B(dp, 0,15),R2B(dp, 1, 15));
            }
            /* Bottom right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4OverlapFilter(R2B(dp, 14, 14),R2B(dp, 15, 14),R2B(dp, 14,15),R2B(dp, 15, 15));
            }

        }

        for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {
            int jdx;

            for (jdx = 2 ; jdx < 14 ; jdx += 4) {

                int*dp = MACROBLK_UP3(image,ch,tx,idx).data;
                /* Fully interior 4x4 filter blocks... */
                _jxr_4x4OverlapFilter(R2B(dp, 2,jdx+0),R2B(dp, 3,jdx+0),R2B(dp, 4,jdx+0),R2B(dp, 5,jdx+0),
                    R2B(dp, 2,jdx+1),R2B(dp, 3,jdx+1),R2B(dp, 4,jdx+1),R2B(dp, 5,jdx+1),
                    R2B(dp, 2,jdx+2),R2B(dp, 3,jdx+2),R2B(dp, 4,jdx+2),R2B(dp, 5,jdx+2),
                    R2B(dp, 2,jdx+3),R2B(dp, 3,jdx+3),R2B(dp, 4,jdx+3),R2B(dp, 5,jdx+3));
                _jxr_4x4OverlapFilter(R2B(dp, 6,jdx+0),R2B(dp, 7,jdx+0),R2B(dp, 8,jdx+0),R2B(dp, 9,jdx+0),
                    R2B(dp, 6,jdx+1),R2B(dp, 7,jdx+1),R2B(dp, 8,jdx+1),R2B(dp, 9,jdx+1),
                    R2B(dp, 6,jdx+2),R2B(dp, 7,jdx+2),R2B(dp, 8,jdx+2),R2B(dp, 9,jdx+2),
                    R2B(dp, 6,jdx+3),R2B(dp, 7,jdx+3),R2B(dp, 8,jdx+3),R2B(dp, 9,jdx+3));
                _jxr_4x4OverlapFilter(R2B(dp,10,jdx+0),R2B(dp,11,jdx+0),R2B(dp,12,jdx+0),R2B(dp,13,jdx+0),
                    R2B(dp,10,jdx+1),R2B(dp,11,jdx+1),R2B(dp,12,jdx+1),R2B(dp,13,jdx+1),
                    R2B(dp,10,jdx+2),R2B(dp,11,jdx+2),R2B(dp,12,jdx+2),R2B(dp,13,jdx+2),
                    R2B(dp,10,jdx+3),R2B(dp,11,jdx+3),R2B(dp,12,jdx+3),R2B(dp,13,jdx+3));

                if ( (image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && !RIGHT_X(idx))) {
                        /* 4x4 at the right */
                        int*np = MACROBLK_UP3(image,ch,tx,idx+1).data;

                        _jxr_4x4OverlapFilter(R2B(dp,14,jdx+0),R2B(dp,15,jdx+0),R2B(np, 0,jdx+0),R2B(np, 1,jdx+0),
                            R2B(dp,14,jdx+1),R2B(dp,15,jdx+1),R2B(np, 0,jdx+1),R2B(np, 1,jdx+1),
                            R2B(dp,14,jdx+2),R2B(dp,15,jdx+2),R2B(np, 0,jdx+2),R2B(np, 1,jdx+2),
                            R2B(dp,14,jdx+3),R2B(dp,15,jdx+3),R2B(np, 0,jdx+3),R2B(np, 1,jdx+3));
                }
            }

            if ((top_my+1) < EXTENDED_HEIGHT_BLOCKS(image)) {

                int*dp = MACROBLK_UP3(image,ch,tx,idx).data;
                int*up = MACROBLK_UP2(image,ch,tx,idx).data;

                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        /* Across vertical blocks, left edge */
                        _jxr_4OverlapFilter(R2B(dp,0,14),R2B(dp,0,15),R2B(up,0,0),R2B(up,0,1));
                        _jxr_4OverlapFilter(R2B(dp,1,14),R2B(dp,1,15),R2B(up,1,0),R2B(up,1,1));
                }
                if((!image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !BOTTOM_Y(top_my)))
                {
                    /* 4x4 bottom */
                    _jxr_4x4OverlapFilter(R2B(dp, 2,14),R2B(dp, 3,14),R2B(dp, 4,14),R2B(dp, 5,14),
                        R2B(dp, 2,15),R2B(dp, 3,15),R2B(dp, 4,15),R2B(dp, 5,15),
                        R2B(up, 2, 0),R2B(up, 3, 0),R2B(up, 4, 0),R2B(up, 5, 0),
                        R2B(up, 2, 1),R2B(up, 3, 1),R2B(up, 4, 1),R2B(up, 5, 1));
                    _jxr_4x4OverlapFilter(R2B(dp, 6,14),R2B(dp, 7,14),R2B(dp, 8,14),R2B(dp, 9,14),
                        R2B(dp, 6,15),R2B(dp, 7,15),R2B(dp, 8,15),R2B(dp, 9,15),
                        R2B(up, 6, 0),R2B(up, 7, 0),R2B(up, 8, 0),R2B(up, 9, 0),
                        R2B(up, 6, 1),R2B(up, 7, 1),R2B(up, 8, 1),R2B(up, 9, 1));
                    _jxr_4x4OverlapFilter(R2B(dp,10,14),R2B(dp,11,14),R2B(dp,12,14),R2B(dp,13,14),
                        R2B(dp,10,15),R2B(dp,11,15),R2B(dp,12,15),R2B(dp,13,15),
                        R2B(up,10, 0),R2B(up,11, 0),R2B(up,12, 0),R2B(up,13, 0),
                        R2B(up,10, 1),R2B(up,11, 1),R2B(up,12, 1),R2B(up,13, 1));
                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    ) {
                        /* Blocks that span the MB to the right */
                        int*dn = MACROBLK_UP3(image,ch,tx,idx+1).data;
                        int*un = MACROBLK_UP2(image,ch,tx,idx+1).data;

                        /* 4x4 on right, below, below-right */
                        _jxr_4x4OverlapFilter(R2B(dp,14,14),R2B(dp,15,14),R2B(dn, 0,14),R2B(dn, 1,14),
                            R2B(dp,14,15),R2B(dp,15,15),R2B(dn, 0,15),R2B(dn, 1,15),
                            R2B(up,14, 0),R2B(up,15, 0),R2B(un, 0, 0),R2B(un, 1, 0),
                            R2B(up,14, 1),R2B(up,15, 1),R2B(un, 0, 1),R2B(un, 1, 1));
                }
                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    /* Across vertical blocks, right edge */
                    _jxr_4OverlapFilter(R2B(dp,14,14),R2B(dp,14,15),R2B(up,14,0),R2B(up,14,1));
                    _jxr_4OverlapFilter(R2B(dp,15,14),R2B(dp,15,15),R2B(up,15,0),R2B(up,15,1));
                }
            }
        }
    }
}

static void overlap_level2_up3_422(jxr_image_t image, int use_my, int ch)
{
    assert(ch > 0 && image->use_clr_fmt == 2/*YUV422*/);
    uint32_t tx = 0; /* XXXX */
    assert(use_my >= 3);
    uint32_t top_my = use_my - 3;
    uint32_t idx;


    DEBUG("Overlap Level2 for row %d\n", top_my);

    uint32_t ty = 0;
    /* Figure out which tile row the current strip of macroblocks belongs to. */
    while(top_my > image->tile_row_position[ty]+image->tile_row_height[ty] - 1)
        ty++;



    for(tx = 0; tx < image->tile_columns; tx++)
    {
        /* Left edge */
        if (tx == 0 || image->disableTileOverlapFlag)
        {
            int*dp = MACROBLK_UP3(image,ch,tx,0).data;
            _jxr_4OverlapFilter(R2B42(dp,0, 2),R2B42(dp,0, 3),R2B42(dp,0, 4),R2B42(dp,0, 5));
            _jxr_4OverlapFilter(R2B42(dp,0, 6),R2B42(dp,0, 7),R2B42(dp,0, 8),R2B42(dp,0, 9));
            _jxr_4OverlapFilter(R2B42(dp,0,10),R2B42(dp,0,11),R2B42(dp,0,12),R2B42(dp,0,13));

            _jxr_4OverlapFilter(R2B42(dp,1, 2),R2B42(dp,1, 3),R2B42(dp,1, 4),R2B42(dp,1, 5));
            _jxr_4OverlapFilter(R2B42(dp,1, 6),R2B42(dp,1, 7),R2B42(dp,1, 8),R2B42(dp,1, 9));
            _jxr_4OverlapFilter(R2B42(dp,1,10),R2B42(dp,1,11),R2B42(dp,1,12),R2B42(dp,1,13));
        }

        /* Right edge */
        if(tx == image->tile_columns -1 || image->disableTileOverlapFlag){

            int*dp = MACROBLK_UP3(image,ch,tx,image->tile_column_width[tx]-1).data;
            _jxr_4OverlapFilter(R2B42(dp,6,2),R2B42(dp,6,3),R2B42(dp,6,4),R2B42(dp,6,5));
            _jxr_4OverlapFilter(R2B42(dp,7,2),R2B42(dp,7,3),R2B42(dp,7,4),R2B42(dp,7,5));

            _jxr_4OverlapFilter(R2B42(dp,6,6),R2B42(dp,6,7),R2B42(dp,6,8),R2B42(dp,6,9));
            _jxr_4OverlapFilter(R2B42(dp,7,6),R2B42(dp,7,7),R2B42(dp,7,8),R2B42(dp,7,9));

            _jxr_4OverlapFilter(R2B42(dp,6,10),R2B42(dp,6,11),R2B42(dp,6,12),R2B42(dp,6,13));
            _jxr_4OverlapFilter(R2B42(dp,7,10),R2B42(dp,7,11),R2B42(dp,7,12),R2B42(dp,7,13));
        }

        /* Top edge */
        if(top_my == 0 || (image->disableTileOverlapFlag && TOP_Y(top_my) ))
        {
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*dp = MACROBLK_UP3(image,ch,tx,idx).data;

                _jxr_4OverlapFilter(R2B42(dp, 2,0),R2B42(dp, 3,0),R2B42(dp, 4,0),R2B42(dp, 5,0));
                _jxr_4OverlapFilter(R2B42(dp, 2,1),R2B42(dp, 3,1),R2B42(dp, 4,1),R2B42(dp, 5,1));

                /* Top across for soft tiles */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                    int*pp = MACROBLK_UP3(image,ch,tx,idx-1).data;
                    _jxr_4OverlapFilter(R2B42(pp,6,0),R2B42(pp,7,0),R2B(dp,0,0),R2B42(dp,1,0));
                    _jxr_4OverlapFilter(R2B42(pp,6,1),R2B42(pp,7,1),R2B(dp,0,1),R2B42(dp,1,1));
                }
            }

            /* Top left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch, tx, 0).data;
                _jxr_4OverlapFilter(R2B42(dp,0,0),R2B42(dp,1,0),R2B42(dp,0,1),R2B42(dp,1,1));
            }
            /* Top right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4OverlapFilter(R2B42(dp,6,0),R2B42(dp,7,0),R2B42(dp,6,1),R2B42(dp,7,1));
            }
        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. finish up with 4Overlap filters. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*tp = MACROBLK_UP3(image,ch,tx,idx).data;

                _jxr_4OverlapFilter(R2B42(tp,2,14),R2B42(tp,3,14),R2B42(tp,4,14),R2B42(tp,5,14));
                _jxr_4OverlapFilter(R2B42(tp,2,15),R2B42(tp,3,15),R2B42(tp,4,15),R2B42(tp,5,15));

                /* Bottom across for soft tiles */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                        /* Blocks that span the MB to the right */
                        int*tn = MACROBLK_UP3(image,ch,tx,idx-1).data;
                        _jxr_4OverlapFilter(R2B42(tn,6,14),R2B42(tn,7,14),R2B42(tp,0,14),R2B42(tp,1,14));
                        _jxr_4OverlapFilter(R2B42(tn,6,15),R2B42(tn,7,15),R2B42(tp,0,15),R2B42(tp,1,15));
                }
            }

            /* Bottom left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch,tx,0).data;
                _jxr_4OverlapFilter(R2B42(dp,0,14),R2B42(dp,1,14),R2B42(dp,0,15),R2B42(dp,1,15));
            }
            /* Bottom right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4OverlapFilter(R2B42(dp,6,14),R2B42(dp,7,14),R2B42(dp,6,15),R2B42(dp,7,15));
            }
        }

        for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {
            int*dp = MACROBLK_UP3(image,ch,tx,idx).data;

            /* Fully interior 4x4 filter blocks... */
            _jxr_4x4OverlapFilter(R2B42(dp,2,2),R2B42(dp,3,2),R2B42(dp,4,2),R2B42(dp,5,2),
                R2B42(dp,2,3),R2B42(dp,3,3),R2B42(dp,4,3),R2B42(dp,5,3),
                R2B42(dp,2,4),R2B42(dp,3,4),R2B42(dp,4,4),R2B42(dp,5,4),
                R2B42(dp,2,5),R2B42(dp,3,5),R2B42(dp,4,5),R2B42(dp,5,5));

            _jxr_4x4OverlapFilter(R2B42(dp,2,6),R2B42(dp,3,6),R2B42(dp,4,6),R2B42(dp,5,6),
                R2B42(dp,2,7),R2B42(dp,3,7),R2B42(dp,4,7),R2B42(dp,5,7),
                R2B42(dp,2,8),R2B42(dp,3,8),R2B42(dp,4,8),R2B42(dp,5,8),
                R2B42(dp,2,9),R2B42(dp,3,9),R2B42(dp,4,9),R2B42(dp,5,9));

            _jxr_4x4OverlapFilter(R2B42(dp,2,10),R2B42(dp,3,10),R2B42(dp,4,10),R2B42(dp,5,10),
                R2B42(dp,2,11),R2B42(dp,3,11),R2B42(dp,4,11),R2B42(dp,5,11),
                R2B42(dp,2,12),R2B42(dp,3,12),R2B42(dp,4,12),R2B42(dp,5,12),
                R2B42(dp,2,13),R2B42(dp,3,13),R2B42(dp,4,13),R2B42(dp,5,13));

            if ( (image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                (image->disableTileOverlapFlag && !RIGHT_X(idx))) {
                    /* Blocks that span the MB to the right */
                    int*np = MACROBLK_UP3(image,ch,tx,idx+1).data;
                    _jxr_4x4OverlapFilter(R2B42(dp,6,2),R2B42(dp,7,2),R2B42(np,0,2),R2B42(np,1,2),
                        R2B42(dp,6,3),R2B42(dp,7,3),R2B42(np,0,3),R2B42(np,1,3),
                        R2B42(dp,6,4),R2B42(dp,7,4),R2B42(np,0,4),R2B42(np,1,4),
                        R2B42(dp,6,5),R2B42(dp,7,5),R2B42(np,0,5),R2B42(np,1,5));

                    _jxr_4x4OverlapFilter(R2B42(dp,6,6),R2B42(dp,7,6),R2B42(np,0,6),R2B42(np,1,6),
                        R2B42(dp,6,7),R2B42(dp,7,7),R2B42(np,0,7),R2B42(np,1,7),
                        R2B42(dp,6,8),R2B42(dp,7,8),R2B42(np,0,8),R2B42(np,1,8),
                        R2B42(dp,6,9),R2B42(dp,7,9),R2B42(np,0,9),R2B42(np,1,9));

                    _jxr_4x4OverlapFilter(R2B42(dp,6,10),R2B42(dp,7,10),R2B42(np,0,10),R2B42(np,1,10),
                        R2B42(dp,6,11),R2B42(dp,7,11),R2B42(np,0,11),R2B42(np,1,11),
                        R2B42(dp,6,12),R2B42(dp,7,12),R2B42(np,0,12),R2B42(np,1,12),
                        R2B42(dp,6,13),R2B42(dp,7,13),R2B42(np,0,13),R2B42(np,1,13));
            }

            if ((top_my+1) < EXTENDED_HEIGHT_BLOCKS(image)) {

                /* Blocks that MB below */
                int*dp = MACROBLK_UP3(image,ch,tx,idx).data;
                int*up = MACROBLK_UP2(image,ch,tx,idx).data;

                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        _jxr_4OverlapFilter(R2B42(dp,0,14),R2B42(dp,0,15),R2B42(up,0,0),R2B42(up,0,1));
                        _jxr_4OverlapFilter(R2B42(dp,1,14),R2B42(dp,1,15),R2B42(up,1,0),R2B42(up,1,1));
                }
                if((!image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !BOTTOM_Y(top_my)))
                {
                    _jxr_4x4OverlapFilter(R2B42(dp,2,14),R2B42(dp,3,14),R2B42(dp,4,14),R2B42(dp,5,14),
                        R2B42(dp,2,15),R2B42(dp,3,15),R2B42(dp,4,15),R2B42(dp,5,15),
                        R2B42(up,2, 0),R2B42(up,3, 0),R2B42(up,4, 0),R2B42(up,5, 0),
                        R2B42(up,2, 1),R2B42(up,3, 1),R2B42(up,4, 1),R2B42(up,5, 1));

                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    ) {
                        /* Blocks that span the MB to the right, below, below-right */
                        int*dn = MACROBLK_UP3(image,ch,tx,idx+1).data;
                        int*un = MACROBLK_UP2(image,ch,tx,idx+1).data;

                        _jxr_4x4OverlapFilter(R2B42(dp,6,14),R2B42(dp,7,14),R2B42(dn,0,14),R2B42(dn,1,14),
                            R2B42(dp,6,15),R2B42(dp,7,15),R2B42(dn,0,15),R2B42(dn,1,15),
                            R2B42(up,6, 0),R2B42(up,7, 0),R2B42(un,0, 0),R2B42(un,1, 0),
                            R2B42(up,6, 1),R2B42(up,7, 1),R2B42(un,0, 1),R2B42(un,1, 1));
                }
                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    _jxr_4OverlapFilter(R2B42(dp,6,14),R2B42(dp,6,15),R2B42(up,6,0),R2B42(up,6,1));
                    _jxr_4OverlapFilter(R2B42(dp,7,14),R2B42(dp,7,15),R2B42(up,7,0),R2B42(up,7,1));
                }
            }
        }
    }
}
/*
*/

static void overlap_level2_up3_420(jxr_image_t image, int use_my, int ch)
{
    assert(ch > 0 && image->use_clr_fmt == 1/*YUV420*/);
    uint32_t tx = 0; /* XXXX */
    assert(use_my >= 3);
    uint32_t top_my = use_my - 3;
    uint32_t idx;


    DEBUG("Overlap Level2 (YUV420) for row %d\n", top_my);

    uint32_t ty = 0;

    /* Figure out which tile row the current strip of macroblocks belongs to. */
    while(top_my > image->tile_row_position[ty]+image->tile_row_height[ty] - 1)
        ty++;

    for(tx = 0; tx < image->tile_columns; tx++)
    {

        /* Left edge */
        if (tx == 0 || image->disableTileOverlapFlag)
        {
            int*dp = MACROBLK_UP3(image,ch,tx,0).data;
            _jxr_4OverlapFilter(R2B42(dp,0,2),R2B42(dp,0,3),R2B42(dp,0,4),R2B42(dp,0,5));
            _jxr_4OverlapFilter(R2B42(dp,1,2),R2B42(dp,1,3),R2B42(dp,1,4),R2B42(dp,1,5));
        }

        /* Right edge */
        if(tx == image->tile_columns -1 || image->disableTileOverlapFlag){
            int*dp = MACROBLK_UP3(image,ch,tx,image->tile_column_width[tx]-1).data;
            _jxr_4OverlapFilter(R2B42(dp,6,2),R2B42(dp,6,3),R2B42(dp,6,4),R2B42(dp,6,5));
            _jxr_4OverlapFilter(R2B42(dp,7,2),R2B42(dp,7,3),R2B42(dp,7,4),R2B42(dp,7,5));
        }

        /* Top edge */
        if(top_my == 0 )/* || (image->disableTileOverlapFlag && TOP_Y(top_my) )) */
        {
            /* If this is the very first strip of blocks, then process the
            first two scan lines with the smaller 4Overlap filter. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*dp = MACROBLK_UP3(image,ch,tx,idx).data;
                _jxr_4OverlapFilter(R2B42(dp, 2,0),R2B42(dp, 3,0),R2B42(dp, 4,0),R2B42(dp, 5,0));
                _jxr_4OverlapFilter(R2B42(dp, 2,1),R2B42(dp, 3,1),R2B42(dp, 4,1),R2B42(dp, 5,1));
                /* Top edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                    int*pp = MACROBLK_UP3(image,ch,tx,idx-1).data;
                    _jxr_4OverlapFilter(R2B42(pp,6,0),R2B42(pp,7,0),R2B(dp,0,0),R2B42(dp,1,0));
                    _jxr_4OverlapFilter(R2B42(pp,6,1),R2B42(pp,7,1),R2B(dp,0,1),R2B42(dp,1,1));
                }
            }

            /* Top left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch,tx,0).data;
                _jxr_4OverlapFilter(R2B42(dp, 0,0),R2B42(dp, 1, 0),R2B42(dp, 0 ,1),R2B42(dp, 1,1));
            }
            /* Top right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4OverlapFilter(R2B42(dp, 6,0),R2B42(dp, 7,0),R2B42(dp, 6,1),R2B42(dp, 7,1));;
            }

        }

        /* Bottom edge */
        if ((top_my+1) == EXTENDED_HEIGHT_BLOCKS(image) || (image->disableTileOverlapFlag && BOTTOM_Y(top_my))) {

            /* This is the last row, so there is no UP below
            TOP. finish up with 4Overlap filters. */
            for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
            {
                int*tp = MACROBLK_UP3(image,ch,tx,idx).data;

                _jxr_4OverlapFilter(R2B42(tp,2,6),R2B42(tp,3,6),R2B42(tp,4,6),R2B42(tp,5,6));
                _jxr_4OverlapFilter(R2B42(tp,2,7),R2B42(tp,3,7),R2B42(tp,4,7),R2B42(tp,5,7));


                /* Bottom edge across */
                if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag)
                    || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                        int*tn = MACROBLK_UP3(image,ch,tx,idx-1).data;
                        _jxr_4OverlapFilter(R2B42(tn,6,6),R2B42(tn,7,6),R2B42(tp,0,6),R2B42(tp,1,6));
                        _jxr_4OverlapFilter(R2B42(tn,6,7),R2B42(tn,7,7),R2B42(tp,0,7),R2B42(tp,1,7));
                }
            }

            /* Bottom left corner */
            if(tx == 0 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch,tx,0).data;
                _jxr_4OverlapFilter(R2B42(dp, 0,6),R2B42(dp, 1, 6),R2B42(dp, 0,7),R2B42(dp, 1, 7));
            }

            /* Bottom right corner */
            if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
            {
                int *dp = MACROBLK_UP3(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                _jxr_4OverlapFilter(R2B42(dp, 6, 6),R2B42(dp, 7, 6),R2B42(dp, 6, 7),R2B42(dp, 7, 7));
            }

            if(image->disableTileOverlapFlag && BOTTOM_Y(top_my) && top_my <EXTENDED_HEIGHT_BLOCKS(image)-1)
            {
                /* Also process Top edge of next macroblock row */
                /* In the case of YUV 420, the next row of macroblocks needs to be transformed */
                /* before yuv420_to_yuv422 is called */
                /* In the soft tile case the top 2 lines of the MB below are processed by the 2x2 operators spanning the MB below*/
                /* In the case of hard tiles, if this is the bottom most row of MBs in the Hard Tile */
                /* we need to process the top edge of the next hard tile */
                /* Also see HARDTILE_NOTE in yuv420_to_yuv422() */

                for (idx = 0; idx < image->tile_column_width[tx] ; idx += 1)
                {
                    int*dp = MACROBLK_UP2(image,ch,tx,idx).data;
                    _jxr_4OverlapFilter(R2B42(dp, 2,0),R2B42(dp, 3,0),R2B42(dp, 4,0),R2B42(dp, 5,0));
                    _jxr_4OverlapFilter(R2B42(dp, 2,1),R2B42(dp, 3,1),R2B42(dp, 4,1),R2B42(dp, 5,1));
                    /* Top edge across */
                    if ( (image->tile_column_position[tx] + idx > 0 && !image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !LEFT_X(idx))) {
                        int*pp = MACROBLK_UP2(image,ch,tx,idx-1).data;
                        _jxr_4OverlapFilter(R2B42(pp,6,0),R2B42(pp,7,0),R2B(dp,0,0),R2B42(dp,1,0));
                        _jxr_4OverlapFilter(R2B42(pp,6,1),R2B42(pp,7,1),R2B(dp,0,1),R2B42(dp,1,1));
                    }
                }

                /* Top left corner */
                if(tx == 0 || image->disableTileOverlapFlag)
                {
                    int *dp = MACROBLK_UP2(image,ch,tx,0).data;
                    _jxr_4OverlapFilter(R2B42(dp, 0,0),R2B42(dp, 1, 0),R2B42(dp, 0 ,1),R2B42(dp, 1,1));
                }

                /* Top right corner */
                if(tx == image->tile_columns -1 || image->disableTileOverlapFlag)
                {
                    int *dp = MACROBLK_UP2(image,ch,tx, image->tile_column_width[tx] - 1 ).data;
                    _jxr_4OverlapFilter(R2B42(dp, 6,0),R2B42(dp, 7,0),R2B42(dp, 6,1),R2B42(dp, 7,1));;
                }
            }
        }

        for (idx = 0 ; idx < image->tile_column_width[tx] ; idx += 1) {

            int*dp = MACROBLK_UP3(image,ch,tx,idx).data;
#ifndef JPEGXR_ADOBE_EXT
            int*up = MACROBLK_UP2(image,ch,tx,idx).data;
#endif //#ifndef JPEGXR_ADOBE_EXT

            /* Fully interior 4x4 filter blocks... */
            _jxr_4x4OverlapFilter(R2B42(dp,2,2),R2B42(dp,3,2),R2B42(dp,4,2),R2B42(dp,5,2),
                R2B42(dp,2,3),R2B42(dp,3,3),R2B42(dp,4,3),R2B42(dp,5,3),
                R2B42(dp,2,4),R2B42(dp,3,4),R2B42(dp,4,4),R2B42(dp,5,4),
                R2B42(dp,2,5),R2B42(dp,3,5),R2B42(dp,4,5),R2B42(dp,5,5));

            if ( (image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                (image->disableTileOverlapFlag && !RIGHT_X(idx)))
            {
                /* 4x4 at the right */
                int*np = MACROBLK_UP3(image,ch,tx,idx+1).data;

                _jxr_4x4OverlapFilter(R2B42(dp,6,2),R2B42(dp,7,2),R2B42(np,0,2),R2B42(np,1,2),
                    R2B42(dp,6,3),R2B42(dp,7,3),R2B42(np,0,3),R2B42(np,1,3),
                    R2B42(dp,6,4),R2B42(dp,7,4),R2B42(np,0,4),R2B42(np,1,4),
                    R2B42(dp,6,5),R2B42(dp,7,5),R2B42(np,0,5),R2B42(np,1,5));
            }

            if ((top_my+1) < EXTENDED_HEIGHT_BLOCKS(image)) {

                int*dp = MACROBLK_UP3(image,ch,tx,idx).data;
                int*up = MACROBLK_UP2(image,ch,tx,idx).data;

                if ((tx == 0 && idx==0 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && LEFT_X(idx) && !BOTTOM_Y(top_my))) {
                        /* Across vertical blocks, left edge */
                        _jxr_4OverlapFilter(R2B42(dp,0,6),R2B42(dp,0,7),R2B42(up,0,0),R2B42(up,0,1));
                        _jxr_4OverlapFilter(R2B42(dp,1,6),R2B42(dp,1,7),R2B42(up,1,0),R2B42(up,1,1));
                }
                if((!image->disableTileOverlapFlag) || (image->disableTileOverlapFlag && !BOTTOM_Y(top_my)))
                {
                    /* 4x4 straddling lower MB */
                    _jxr_4x4OverlapFilter(R2B42(dp,2,6),R2B42(dp,3,6),R2B42(dp,4,6),R2B42(dp,5,6),
                        R2B42(dp,2,7),R2B42(dp,3,7),R2B42(dp,4,7),R2B42(dp,5,7),
                        R2B42(up,2,0),R2B42(up,3,0),R2B42(up,4,0),R2B42(up,5,0),
                        R2B42(up,2,1),R2B42(up,3,1),R2B42(up,4,1),R2B42(up,5,1));
                }

                if (((image->tile_column_position[tx] + idx < EXTENDED_WIDTH_BLOCKS(image)-1) && !image->disableTileOverlapFlag ) ||
                    ( image->disableTileOverlapFlag && !RIGHT_X(idx) && !BOTTOM_Y(top_my) )
                    ) {
                        /* Blocks that span the MB to the right */
                        int*dn = MACROBLK_UP3(image,ch,tx,idx+1).data;
                        int*un = MACROBLK_UP2(image,ch,tx,idx+1).data;

                        /* 4x4 right, below, below-right */
                        _jxr_4x4OverlapFilter(R2B42(dp,6,6),R2B42(dp,7,6),R2B42(dn,0,6),R2B42(dn,1,6),
                            R2B42(dp,6,7),R2B42(dp,7,7),R2B42(dn,0,7),R2B42(dn,1,7),
                            R2B42(up,6,0),R2B42(up,7,0),R2B42(un,0,0),R2B42(un,1,0),
                            R2B42(up,6,1),R2B42(up,7,1),R2B42(un,0,1),R2B42(un,1,1));
                }
                if((image->tile_column_position[tx] + idx == EXTENDED_WIDTH_BLOCKS(image)-1 && !image->disableTileOverlapFlag) ||
                    (image->disableTileOverlapFlag && RIGHT_X(idx) && !BOTTOM_Y(top_my)))
                {
                    /* Across vertical blocks, right edge */
                    _jxr_4OverlapFilter(R2B42(dp,6,6),R2B42(dp,6,7),R2B42(up,6,0),R2B42(up,6,1));
                    _jxr_4OverlapFilter(R2B42(dp,7,6),R2B42(dp,7,7),R2B42(up,7,0),R2B42(up,7,1));
                }
            }
        }
    }
}


static void overlap_level2_up3(jxr_image_t image, int use_my, int ch)
{
    if (ch == 0) {
        overlap_level2_up3_444(image, use_my, ch);
    } 
    else {
        switch (image->use_clr_fmt) {
            case 1: /*YUV420*/
                overlap_level2_up3_420(image, use_my, ch);
                break;
            case 2: /*YUV422*/
                overlap_level2_up3_422(image, use_my, ch);
                break;
            default:
                overlap_level2_up3_444(image, use_my, ch);
                break;
        }
    }
}


static void yuv444_to_rgb(jxr_image_t image, int mx)
{
    int px;
    for (px = 0 ; px < 256 ; px += 1) {
        int Y = image->strip[0].up3[mx].data[px];
        int U = image->strip[1].up3[mx].data[px];
        int V = image->strip[2].up3[mx].data[px];
        int G = Y - _jxr_floor_div2(-U);
        int R = G - U - _jxr_ceil_div2(V);
        int B = V + R;

        image->strip[0].up3[mx].data[px] = R;
        image->strip[1].up3[mx].data[px] = G;
        image->strip[2].up3[mx].data[px] = B;
    }
}

static const int iH[5][4] = {{4, 4 , 0, 8}, {5, 3, 1, 7}, {6, 2, 2, 6}, {7, 1, 3, 5}, {8, 0, 4, 4}};

static void upsample(int inbuf[], int outbuf[], int upsamplelen, int chroma_center)
{    
    int k;
    if(chroma_center == 5 || chroma_center == 6 || chroma_center == 7)
    {
        chroma_center = 0;
        DEBUG("Treating chroma_center as 0 in upsample\n");
    }

    for (k = 0; k <= (upsamplelen - 2) / 2; k++)
        outbuf[2*k+1] = (( iH[chroma_center][0]*inbuf[k+1] + iH[chroma_center][1]*inbuf[k+2] + 4) >> 3);
    for (k = -1; k <= (upsamplelen - 4) / 2; k++)
        outbuf[2*k+2] = ((iH[chroma_center][2] * inbuf[k+1] + iH[chroma_center][3] * inbuf[k+2] + 4) >> 3);
}

static void yuv422_to_yuv444(jxr_image_t image, int mx)
{
    int buf[256];    

    int ch;
    int px, py, idx;

    for(ch =1; ch < 3; ch ++) {

        for (py = 0 ; py < 16 ; py += 1)
        {
            if(mx == 0) /* Repeat to the left */
                image->strip[ch].upsample_memory_x[py] = image->strip[ch].up3[mx].data[8*py];

            int inbuf [10];
            /* Prep input array */
            for(px =0; px <=7; px++)
            {
                inbuf[px+1] = image->strip[ch].up3[mx].data[8*py+ px];

            }
            inbuf[0] =  image->strip[ch].upsample_memory_x[py];
            if(mx+1 < (int) EXTENDED_WIDTH_BLOCKS(image))
                inbuf[9] = image->strip[ch].up3[mx+1].data[8*py];
            else
                inbuf[9] = inbuf[8]; /* Repeat to the right */

            /* Call upsample */
            upsample(inbuf, buf + 16*py, 16, image->chroma_centering_x);

            /* Remember right most vals */
            image->strip[ch].upsample_memory_x[py] = image->strip[ch].up3[mx].data[8*py+7];
        }

        for (idx = 0 ; idx < 256 ; idx += 1)
            image->strip[ch].up3[mx].data[idx] = buf[idx];

    }
}

static void yuv420_to_yuv444(jxr_image_t image, int use_my, int mx)
{
    int buf[256];
    int intermediatebuf[2][16 * 8];    

    int ch;
    int inbuf [10];
    int px, py, idx;

    /* Upsample in the y direction */
    for (ch = 1 ; ch < 3 ; ch += 1) {
        for(px = 0; px < 8; px ++)
        {
            if(use_my-2 == 1) /* Repeat to the top */
                image->strip[ch].upsample_memory_y[8*mx+ px] = image->strip[ch].up3[mx].data[px];/* Store the top most values */

            /* Prep input buffer */
            for(py =0; py < 8 ; py++)
                inbuf[py+1] = image->strip[ch].up3[mx].data[8*py+ px];
            inbuf[0] = image->strip[ch].upsample_memory_y[8*mx + px];
            if((use_my-2) < (int) EXTENDED_HEIGHT_BLOCKS(image))
            {
                if(px <= 3)
                    inbuf[9] = image->strip[ch].up2[mx].data[px]; /* Get the lower MB sample */
                else
                    inbuf[9] = image->strip[ch].up2[mx].data[px + 12]; /* Since unblock_shuffle420 has not been called on up2 */
            }
            else
                inbuf[9] = inbuf[8]; /* Repeat to the right */

            /* Call upsample */
            upsample(inbuf, buf, 16, image->chroma_centering_y);

            for(py =0; py < 16; py ++)
                intermediatebuf[ch-1][8*py + px] = buf[py];

            image->strip[ch].upsample_memory_y[8*mx + px] = image->strip[ch].up3[mx].data[8*7+px];/* Store the bottom most values */
        }
    }

    /* Upsample in the X direction */
    for (ch = 1 ; ch < 3 ; ch += 1) {
        int nextmbrow[16];

        /* To upsample in the X direction, we need the Y-direction upsampled values from the left most row of the next MB */
        /* Prep input buffer */        
        for(py = 0; py < 8; py ++)
        {
            if(mx + 1 < (int) EXTENDED_WIDTH_BLOCKS(image))
                inbuf[py + 1] = image->strip[ch].up3[mx+1].data[8*py];
            else
                inbuf[py + 1] = image->strip[ch].up3[mx].data[8*py];
        }
        if(use_my - 2 < (int) EXTENDED_HEIGHT_BLOCKS(image) && mx + 1 < (int) EXTENDED_WIDTH_BLOCKS(image))
            inbuf[9] = image->strip[ch].up2[mx+1].data[0];
        else
            inbuf[9] = inbuf[8];/* Repeat to the right */
        if(use_my -2 != 1 && mx + 1 < (int) EXTENDED_WIDTH_BLOCKS(image))
            inbuf[0] = image->strip[ch].upsample_memory_y[8*(mx+1)];
        else
            inbuf[0] = inbuf[1];

        /*Call upsample */
        upsample(inbuf, nextmbrow, 16, image->chroma_centering_y);

        for(py = 0; py < 16; py ++)
        {
            if(mx == 0) /* Repeat to the left */
                image->strip[ch].upsample_memory_x[py] = intermediatebuf[ch-1][8*py];

            /* Prepare the input buffer */
            for(px =0; px <=7; px++)
                inbuf[px+1] = intermediatebuf[ch-1][8*py+ px];

            inbuf[0] = image->strip[ch].upsample_memory_x[py];
            if(mx + 1 < (int) EXTENDED_WIDTH_BLOCKS(image))
            {
                inbuf[9] = nextmbrow[py];
            }
            else
                inbuf[9] = inbuf[8]; /* Repeat to the right */

            /* Call upsample */
            upsample(inbuf, buf + 16*py, 16, image->chroma_centering_x);
            image->strip[ch].upsample_memory_x[py] = intermediatebuf[ch-1][8*py + 7];/* Store the right most values */
        }

        for(idx =0; idx < 256; idx ++)
            image->strip[ch].up3[mx].data[idx] = buf[idx];
    }
}

static void yuvk_to_cmyk(jxr_image_t image, int mx)
{
    int px;
    for (px = 0 ; px < 256 ; px += 1) {
        int Y = image->strip[0].up3[mx].data[px];
        int U = image->strip[1].up3[mx].data[px];
        int V = image->strip[2].up3[mx].data[px];
        int K = image->strip[3].up3[mx].data[px];
        int k = K + _jxr_floor_div2(Y);
        int m = k - Y - _jxr_floor_div2(U);
        int c = U + m + _jxr_floor_div2(V);
        int y = c - V;

        image->strip[0].up3[mx].data[px] = c;
        image->strip[1].up3[mx].data[px] = m;
        image->strip[2].up3[mx].data[px] = y;
        image->strip[3].up3[mx].data[px] = k;
    }
}

static int PostScalingFloat(int iPixVal, unsigned int expBias, unsigned char lenMantissa, int bitdepth)
{
    int convVal = 0;
    if (bitdepth == JXR_BD16F) 
    {
        uint8_t iS = 0;
#ifndef JPEGXR_ADOBE_EXT
        unsigned int fV = 0;                                              
#endif //#ifndef JPEGXR_ADOBE_EXT
        unsigned int iEM = abs(iPixVal);
        if(iPixVal < 0)
            iS = 1;
        if (iEM > 0x7FFF)
            iEM = 0x7FFF;
        convVal = ((iS << 15) | iEM); /* Concatenate these fields*/
    }
    else
    {        
        int sign, iTempH, mantissa, exp, lmshift = (1 << lenMantissa);

        assert (expBias <= 127);

        iTempH = (int) iPixVal ;
        sign = (iTempH >> 31);
        iTempH = (iTempH ^ sign) - sign; /* abs(iTempH) */

        exp = (unsigned int) iTempH >> lenMantissa;/* & ((1 << (31 - lenMantissa)) - 1); */
        mantissa = (iTempH & (lmshift - 1)) | lmshift; /* actual mantissa, with normalizer */
        if (exp == 0) { /* denormal land */
            mantissa ^= lmshift; /* normalizer removed */
            exp = 1; /* actual exponent */
        }

        exp += (127 - expBias);
        while (mantissa < lmshift && exp > 1 && mantissa > 0) { /* denormal originally, see if normal is possible */
            exp--;
            mantissa <<= 1;
        }
        if (mantissa < lmshift) /* truly denormal */
            exp = 0;
        else
            mantissa ^= lmshift;
        mantissa <<= (23 - lenMantissa);

        convVal = (sign & 0x80000000) | (exp << 23) | mantissa;        
    }
    return convVal;
}

static void PostScalingFl2(int arrayOut[], int arrayIn[]) {
	/* arrayIn[ ]= {R, G, B} */
	/* arrayOut[ ]= {Rrgbe, Grgbe, Brgbe, Ergbe} */
    int iEr, iEg, iEb;
    int iShift;

    if (arrayIn[0] <= 0) {
        arrayOut[0] = 0;
        iEr = 0;
    }
    else if ((arrayIn[0] >> 7) > 1) {
        arrayOut[0] = (arrayIn[0] & 0x7F) + 0x80;
        iEr = (arrayIn[0] >> 7);
    }
    else {
        arrayOut[0] = arrayIn[0];
        iEr = 1;
    }
    if (arrayIn[1] <= 0) {
        arrayOut[1] = 0;
        iEg = 0;
    }
    else if ((arrayIn[1] >> 7) > 1) {
        arrayOut[1] = (arrayIn[1] & 0x7F) + 0x80;
        iEg = (arrayIn[1] >> 7);
    }
    else {
        arrayOut[1] = arrayIn[1];
        iEg = 1;
    }
    if (arrayIn[2] <= 0) {
        arrayOut[2] = 0;
        iEb = 0;
    }
    else if ((arrayIn[2] >> 7) > 1) {
        arrayOut[2] = (arrayIn[2] & 0x7F) + 0x80;
        iEb = (arrayIn[2] >> 7);
    }
    else {
        arrayOut[2] = arrayIn[2];
        iEb = 1;
    }

    /* Max(iEr, iEg, iEb) */
    arrayOut[3] = iEr> iEg? (iEr>iEb?iEr:iEb):(iEg>iEb?iEg:iEb);

    if( arrayOut[3] > iEr){
        iShift = ( arrayOut[3] - iEr);
        arrayOut[0] = (unsigned char)((((int) arrayOut[0]) * 2 + 1) >> (iShift + 1));
    }
    if( arrayOut[3] > iEg){
        iShift = ( arrayOut[3]- iEg);
        arrayOut[1] = (unsigned char)((((int) arrayOut[1]) * 2 + 1) >> (iShift + 1));
    }
    if( arrayOut[3] > iEb){
        iShift = ( arrayOut[3]- iEb);
        arrayOut[2] = (unsigned char)((((int) arrayOut[2]) * 2 + 1) >> (iShift + 1));
    }
}

static void scale_and_emit_top(jxr_image_t image, int /*tx*/, int use_my)
{
    int scale = image->scaled_flag? 3 : 0;
    int bias;
    int round;
    int shift_bits = image->shift_bits;
    /* Clipping based on 8bit values. */
    int clip_low = 0;
    int clip_hig = 255;
    int idx;
    int buffer[(MAX_CHANNELS + 1)*256];
    memset(buffer, 0,(MAX_CHANNELS + 1)*256*sizeof(int));
    unsigned int bSkipColorTransform = 0;

    
    switch (SOURCE_BITDEPTH(image)) {
        case 0: /* BD1WHITE1*/
          
        case 15: /* BD1BLACK1 */
            bias = 0;
            round = image->scaled_flag? 4 : 0;
            clip_low = 0;
            clip_hig = 1;
            break;
        case 1: /* BD8 */
            bias = 1 << 7;
            round = image->scaled_flag? 3 : 0;
            clip_low = 0;
            clip_hig = 255;
            break;
        case 2: /* BD16 */
            bias = 1 << 15;
            round = image->scaled_flag? 4 : 0;
            clip_low = 0;
            clip_hig = 65535;
            break;
        case 3: /* BD16S */
            bias = 0;
            round = image->scaled_flag? 3 : 0;
            clip_low = -32768;
            clip_hig = 32767;
            break;

        case 6: /*BD32S */
            bias = 0;
            round = image->scaled_flag? 3 : 0;
            clip_hig = 0x7fffffff;
            clip_low = -(clip_hig + 1);
            break;

        case 4: /* BD16F */

        case 7: /* BD32F */
            bias = 0;
            round = image->scaled_flag? 3 : 0;
            break;

        case 8: /* BD5 */
            bias = 16;
            round = image->scaled_flag? 3 : 0;
            clip_hig = 31;
            clip_low = 0;
            break;

        case 9: /* BD10 */
            bias = 512;
            round = image->scaled_flag? 3 : 0;
            clip_hig = 1023;
            clip_low = 0;
            break;

        case 10: /* BD565 */
            bias = 32;
            round = image->scaled_flag? 3 : 0;
            clip_hig = 31;
            clip_low = 0;
            break;

        case 5: /* Reserved */

        default:
            fprintf(stderr, "XXXX Don't know how to scale bit depth %d?\n", SOURCE_BITDEPTH(image));
            bias = 0;
            round = image->scaled_flag? 3 : 0;
            clip_low = 0;
            clip_hig = 255;
            break;
    }

    DEBUG("scale_and_emit_top: scale=%d, bias=%d, round=%d, shift_bits=%d, clip_low=%d, clip_hig=%d\n",
        scale, bias, round, shift_bits, clip_low, clip_hig);

    /* Up 'til this point, the MB contains 4x4 sub-blocks. We are
    now ready for the MB to contain only raster data within, so
    this loop rasterizes all the MBs in this strip. */
    for (idx = 0 ; idx < (int) EXTENDED_WIDTH_BLOCKS(image); idx += 1) {

        int ch;
        int*dp = image->strip[0].up3[idx].data;
        unblock_shuffle444(dp);
        for (ch = 1 ; ch < image->num_channels ; ch += 1) {
            dp = image->strip[ch].up3[idx].data;
            switch (image->use_clr_fmt) {
                case 1: /* YUV420 */
                    unblock_shuffle420(dp);
                    break;
                case 2: /* YUV422 */
                    unblock_shuffle422(dp);
                    break;
                default:
                    unblock_shuffle444(dp);
                    break;
            }
        }
    }

    for (idx = 0 ; idx < (int) EXTENDED_WIDTH_BLOCKS(image); idx += 1) {

        int ch;
#if defined(DETAILED_DEBUG) && 1
        for (ch = 0 ; ch < image->num_channels ; ch += 1) {
            int count = 256;
            if (ch > 0 && image->use_clr_fmt==2/*YUV422*/)
                count = 128;
            if (ch > 0 && image->use_clr_fmt==1/*YUV420*/)
                count = 64;

            DEBUG("image yuv mx=%3d my=%3d ch=%d:", idx, use_my-3, ch);
            int jdx;
            for (jdx = 0 ; jdx < count ; jdx += 1) {
                if (jdx%8 == 0 && jdx != 0)
                    DEBUG("\n%*s", 29, "");
                DEBUG(" %08x", image->strip[ch].up3[idx].data[jdx]);
            }
            DEBUG("\n");
        }
#endif

        if(SOURCE_CLR_FMT(image)  == JXR_OCF_YUV420 || SOURCE_CLR_FMT(image)  == JXR_OCF_YUV422 || SOURCE_CLR_FMT(image)  == JXR_OCF_YUV444 || SOURCE_CLR_FMT(image) == JXR_OCF_CMYKDIRECT)
        {
            bSkipColorTransform = 1;            
        }

        /* Perform transform in place, if needed. */
        /* For YCC output, no color transform is needed */ 
        if(!bSkipColorTransform)
        {
            switch (image->use_clr_fmt ) {

                case 1: /* YUV420 */
                    yuv420_to_yuv444(image, use_my, idx);
                    yuv444_to_rgb(image, idx);
                    break;

                case 2: /* YUV422 */
                    yuv422_to_yuv444(image, idx);
                    yuv444_to_rgb(image, idx);
                    break;

                case 3: /* YUV444 */
                    yuv444_to_rgb(image, idx);
                    break;

                case 4: /* CMYK */
                    yuvk_to_cmyk(image, idx);
                    break;
            }
        }

        /* The strip data is now in the output color space. */

        /* AddBias and ComputeScaling */
        if (image->use_clr_fmt == 4 && SOURCE_CLR_FMT(image) != JXR_OCF_CMYKDIRECT/*CMYK*/) {
            /* The CMYK format has a different and unique set
            of bias/rounding calculations. Treat it as a
            special case. And treat the K plane even more
            special. */
            int*dp;
            int jdx;
            for (ch = 0 ; ch < 3 ; ch += 1) {
                dp = image->strip[ch].up3[idx].data;
                for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                    dp[jdx] = (dp[jdx] + ((bias>>(shift_bits+1))<<scale) + round) >> scale;
                    dp[jdx] <<= shift_bits;
                    if (dp[jdx] > clip_hig)
                        dp[jdx] = clip_hig;
                    if (dp[jdx] < clip_low)
                        dp[jdx] = clip_low;
                }
            }
            dp = image->strip[3].up3[idx].data;
            for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                dp[jdx] = (dp[jdx] - ((bias>>(shift_bits+1))<<scale) + round) >> scale;
                dp[jdx] <<= shift_bits;
                if (dp[jdx] > clip_hig)
                    dp[jdx] = clip_hig;
                if (dp[jdx] < clip_low)
                    dp[jdx] = clip_low;
            }
        }
        else
        {
            for (ch = 0 ; ch < image->num_channels ; ch += 1) {
                /* PostScalingInt and clip, 16s and 32s */
                if( SOURCE_BITDEPTH(image)!= JXR_BD565 && SOURCE_BITDEPTH(image) != JXR_BD16F && SOURCE_BITDEPTH(image) != JXR_BD32F && SOURCE_CLR_FMT(image) != JXR_OCF_RGBE)
                {
                    int*dp = image->strip[ch].up3[idx].data;
                    int jdx;
                    for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                        dp[jdx] = (dp[jdx] + ((bias>>shift_bits)<<scale) + round) >> scale;
                        dp[jdx] <<= shift_bits;
                        if (dp[jdx] > clip_hig)
                            dp[jdx] = clip_hig;
                        if (dp[jdx] < clip_low)
                            dp[jdx] = clip_low;
                    }
#if defined(DETAILED_DEBUG) && 0
                    DEBUG("scale_and_emit: block at mx=%d, my=%d, ch=%d:", idx, use_my-3, ch);
                    for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                        if (jdx%8 == 0 && jdx > 0)
                            DEBUG("\n%*s:", 41, "");
                        DEBUG(" %04x", dp[jdx]);
                    }
                    DEBUG("\n");
#endif
                }
                else if(SOURCE_BITDEPTH(image) == JXR_BD565)
                {
                    assert(image->num_channels == 3);
                    /* Special case where R and B have different clip thresholds from G */
					int*dp = image->strip[ch].up3[idx].data;
                    int jdx;
                    if(ch != 1)
                    {
                        clip_hig = 31;
                        clip_low = 0;
                    }
                    else
                    {
                        clip_hig = 63;
                        clip_low = 0;
                    }
                    for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                        if(ch == 1)
							dp[jdx] = (dp[jdx] + ((bias)<<scale) + round) >> scale;
                        else
							dp[jdx] = (dp[jdx] + ((bias)<<scale) + round) >> (scale + 1);
						if (dp[jdx] > clip_hig)
                            dp[jdx] = clip_hig;
                        if (dp[jdx] < clip_low)
							dp[jdx] = clip_low;
					}
                }

                /* PostScalingFl */
                else
                {
                    int* dp = image->strip[ch].up3[idx].data;
                    int jdx;
                    if(SOURCE_BITDEPTH(image) == JXR_BD16F || SOURCE_BITDEPTH(image) == JXR_BD32F)
                    {
                        for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                            dp[jdx] = (dp[jdx] + round) >> scale;
                            dp[jdx] = PostScalingFloat(dp[jdx], image->exp_bias, image->len_mantissa, SOURCE_BITDEPTH(image));                                                 
                        }
                    }
                    else /* RGBE : PostScalingFl2 requires one extra sample per pixel - Write directly into buffer */
                    {                        
                        int *dp0 = image->strip[0].up3[idx].data;
                        int *dp1 = image->strip[1].up3[idx].data;
                        int *dp2 = image->strip[2].up3[idx].data;
                        assert(image->num_channels == 3);
                        assert(ch == 0);

                        for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                            /* There is no bias in this case */
                            int idp0 = (dp0[jdx] + round) >> scale;
                            int idp1 = (dp1[jdx] + round) >> scale;
                            int idp2 = (dp2[jdx] + round) >> scale;
                        
                            int arrIn[3] = {idp0, idp1, idp2};                            
                                                     
                            PostScalingFl2(buffer + (image->num_channels + 1) * jdx, arrIn);                            
                            ch = 3;/* We have taken care of all channels in one go */
                        }

                    }

                    
                    
#if defined(DETAILED_DEBUG) && 0
                    DEBUG("scale_and_emit: block at mx=%d, my=%d, ch=%d:", idx, use_my-3, ch);
                    for (jdx = 0 ; jdx < 256 ; jdx += 1) {
                        if (jdx%8 == 0 && jdx > 0)
                            DEBUG("\n%*s:", 41, "");
                        DEBUG(" %04x", dp[jdx]);
                    }
                    DEBUG("\n");
#endif
                }
            }
        }

         if ( image->primary == 1 ) {  /* alpha channel output is combined with primary channel */
            int px;
            int channels = image->num_channels;
            
            if(!bSkipColorTransform) /* Interleave channels in buffer */
            {
                if (ALPHACHANNEL_FLAG(image))
                    channels ++;

                for (px = 0 ; px < 256 && (SOURCE_CLR_FMT(image) != JXR_OCF_RGBE) ; px += 1) /*RGBE is a special case that is already taken care of */
                    for (ch = 0 ; ch < image->num_channels ; ch += 1)
                        buffer[channels*px + ch] = image->strip[ch].up3[idx].data[px];

                if (ALPHACHANNEL_FLAG(image))
                    for (px = 0 ; px < 256 ; px += 1)
                        buffer[channels*px + image->num_channels] = image->alpha->strip[0].up3[idx].data[px];
            }
            else
            {
                int size =  256*sizeof(uint32_t);
                int i = 0;
                for(i = 0; i < image->num_channels; i ++)
                {
                    memcpy(((uint8_t *)buffer + i*size), image->strip[i].up3[idx].data, size);

                }                
                if(ALPHACHANNEL_FLAG(image))
                    memcpy(((uint8_t *)buffer) + image->num_channels*size, image->alpha->strip[0].up3[idx].data, size);                                    
            }


            _jxr_send_mb_to_output(image, idx, use_my-3, buffer);
        }
    }
}

/*
* The tile_row_buffer holds flushed mb data in image raster order,
* along with other per-mb data. This is in support of SPATIAL processing.
*/
static void rflush_to_tile_buffer(jxr_image_t image, int tx, int my)
{
    DEBUG("rflush_mb_strip: rflush_to_tile_buffer tx=%d, my=%d\n", tx, my);

    int format_scale = 256;
    if (image->use_clr_fmt == 2 /* YUV422 */) {
        format_scale = 16 + 8*15;
    } else if (image->use_clr_fmt == 1 /* YUV420 */) {
        format_scale = 16 + 4*15;
    }

    int mx;
    for (mx = 0 ; mx < (int) image->tile_column_width[tx] ; mx += 1) {
        DEBUG("rflush_mb_strip: rflush_to_tile_buffer tx=%d, mx=%d, CUR=0x%08x UP1=0x%08x, UP2=0x%08x, UP3=0x%08x, LP_QUANT=%d\n",
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
            int count = (ch==0)? 256 : format_scale;
            int idx;
            for (idx = 0 ; idx < count ; idx += 1)
                mb->data[idx] = MACROBLK_CUR(image,ch,tx,mx).data[idx];
        }
    }
}

/*
* Recover a strip of data from all but the last column of data. Skip
* the last column because this function is called while the last
* column is being processed.
*/
static void rflush_collect_mb_strip_data(jxr_image_t image, int my)
{
    DEBUG("rflush_mb_strip: rflush_collect_mb_strip_data my=%d\n", my);

    int format_scale = 256;
    if (image->use_clr_fmt == 2 /* YUV422 */) {
        format_scale = 16 + 8*15;
    } else if (image->use_clr_fmt == 1 /* YUV420 */) {
        format_scale = 16 + 4*15;
    }

    int tx;
    for (tx = 0; tx < (int) image->tile_columns-1 ; tx += 1) {
        int mx;
        for (mx = 0; mx < (int) image->tile_column_width[tx]; mx += 1) {
            int off = my * EXTENDED_WIDTH_BLOCKS(image) + image->tile_column_position[tx] + mx;
            int ch;
            for (ch = 0; ch < image->num_channels; ch += 1) {
                struct macroblock_s*mb = image->mb_row_buffer[ch] + off;
                MACROBLK_CUR_LP_QUANT(image,ch,tx,mx) = mb->lp_quant;
                MACROBLK_CUR(image,ch,tx,mx).hp_quant = mb->hp_quant;
                int count = (ch==0)? 256 : format_scale;
                int idx;
                for (idx = 0 ; idx < count; idx += 1)
                    MACROBLK_CUR(image,ch,tx,mx).data[idx] = mb->data[idx];
            }
            DEBUG("rflush_mb_strip: rflush_collect_mb_strip_data tx=%d, mx=%d, CUR=0x%08x UP1=0x%08x, UP2=0x%08x, UP3=0x%08x lp_quant=%d\n",
                tx, mx, MACROBLK_CUR(image,0,tx,mx).data[0],
                MACROBLK_UP1(image,0,tx,mx).data[0],
                MACROBLK_UP2(image,0,tx,mx).data[0],
                MACROBLK_UP3(image,0,tx,mx).data[0],
                MACROBLK_CUR_LP_QUANT(image,0,tx,mx));
        }
    }
}

/*
* The save_ and recover_context functions save the 3 strips of data
* currently in the strip buffer. This is used at the end of a tile
* row and beginning of the next tile row to save context while
* columns of tiles are collected, and restore it when processing the
* last tile column.
*/
static void rflush_save_context(jxr_image_t image)
{
    DEBUG("rflush_mb_strip: rflush_save_context\n");

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
            int off0 = image->tile_column_position[tx] + mx;
            int off1 = off0 + EXTENDED_WIDTH_BLOCKS(image);
            int off2 = off1 + EXTENDED_WIDTH_BLOCKS(image);
            int off3 = off2 + EXTENDED_WIDTH_BLOCKS(image);
            int ch;
            DEBUG("rflush_mb_strip: rflush_save_context tx=%d, mx=%d, CUR=0x%08x UP1=0x%08x, UP2=0x%08x, UP3=0x%08x\n",
                tx, mx, MACROBLK_CUR(image,0,tx,mx).data[0],
                MACROBLK_UP1(image,0,tx,mx).data[0],
                MACROBLK_UP2(image,0,tx,mx).data[0],
                MACROBLK_UP3(image,0,tx,mx).data[0]);
            for (ch = 0; ch < image->num_channels; ch += 1) {
                image->mb_row_context[ch][off0].lp_quant = MACROBLK_CUR_LP_QUANT(image,ch,tx,mx);
                image->mb_row_context[ch][off1].lp_quant = MACROBLK_UP1_LP_QUANT(image,ch,tx,mx);
                image->mb_row_context[ch][off2].lp_quant = MACROBLK_UP2(image,ch,tx,mx).lp_quant;
                image->mb_row_context[ch][off3].lp_quant = MACROBLK_UP3(image,ch,tx,mx).lp_quant;
                image->mb_row_context[ch][off0].hp_quant = MACROBLK_CUR(image,ch,tx,mx).hp_quant;
                image->mb_row_context[ch][off1].hp_quant = MACROBLK_UP1(image,ch,tx,mx).hp_quant;
                image->mb_row_context[ch][off2].hp_quant = MACROBLK_UP2(image,ch,tx,mx).hp_quant;
                image->mb_row_context[ch][off3].hp_quant = MACROBLK_UP3(image,ch,tx,mx).hp_quant;
                int count = (ch==0)? 256 : format_scale;
                int idx;
                for (idx = 0 ; idx < count; idx += 1)
                    image->mb_row_context[ch][off0].data[idx] = MACROBLK_CUR(image,ch,tx,mx).data[idx];
                for (idx = 0 ; idx < count; idx += 1)
                    image->mb_row_context[ch][off1].data[idx] = MACROBLK_UP1(image,ch,tx,mx).data[idx];
                for (idx = 0 ; idx < count; idx += 1)
                    image->mb_row_context[ch][off2].data[idx] = MACROBLK_UP2(image,ch,tx,mx).data[idx];
                for (idx = 0 ; idx < count; idx += 1)
                    image->mb_row_context[ch][off3].data[idx] = MACROBLK_UP3(image,ch,tx,mx).data[idx];
            }
        }
    }
}

static void rflush_recover_context(jxr_image_t image)
{
    DEBUG("rflush_mb_strip: recover contex\n");

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
            int off0 = image->tile_column_position[tx] + mx;
            int off1 = off0 + EXTENDED_WIDTH_BLOCKS(image);
            int off2 = off1 + EXTENDED_WIDTH_BLOCKS(image);
            int off3 = off2 + EXTENDED_WIDTH_BLOCKS(image);
            int ch;
            for (ch = 0; ch < image->num_channels; ch += 1) {
                MACROBLK_CUR_LP_QUANT(image,ch,tx,mx) = image->mb_row_context[ch][off0].lp_quant;
                MACROBLK_UP1_LP_QUANT(image,ch,tx,mx) = image->mb_row_context[ch][off1].lp_quant;
                MACROBLK_UP2(image,ch,tx,mx).lp_quant = image->mb_row_context[ch][off2].lp_quant;
                MACROBLK_UP3(image,ch,tx,mx).lp_quant = image->mb_row_context[ch][off3].lp_quant;
                MACROBLK_CUR(image,ch,tx,mx).hp_quant = image->mb_row_context[ch][off0].hp_quant;
                MACROBLK_UP1(image,ch,tx,mx).hp_quant = image->mb_row_context[ch][off1].hp_quant;
                MACROBLK_UP2(image,ch,tx,mx).hp_quant = image->mb_row_context[ch][off2].hp_quant;
                MACROBLK_UP3(image,ch,tx,mx).hp_quant = image->mb_row_context[ch][off3].hp_quant;
                int count = (ch==0)? 256 : format_scale;
                int idx;
                for (idx = 0 ; idx < count; idx += 1)
                    MACROBLK_CUR(image,ch,tx,mx).data[idx] = image->mb_row_context[ch][off0].data[idx];
                for (idx = 0 ; idx < count; idx += 1)
                    MACROBLK_UP1(image,ch,tx,mx).data[idx] = image->mb_row_context[ch][off1].data[idx];
                for (idx = 0 ; idx < count; idx += 1)
                    MACROBLK_UP2(image,ch,tx,mx).data[idx] = image->mb_row_context[ch][off2].data[idx];
                for (idx = 0 ; idx < count; idx += 1)
                    MACROBLK_UP3(image,ch,tx,mx).data[idx] = image->mb_row_context[ch][off3].data[idx];
            }
        }
    }
}


/*
* When the parser calls this function, the current strip is done
* being parsed, so it no longer needs the previous strip. Complete
* the processing of the previous ("up") strip and arrange for it to
* be delivered to the applications. Then shuffle the current strip to
* the "up" position for the next round.
*
* cur_my is the number of the current line. If this is -1, then there
* are no lines complete yet and this function is being called to get
* things started.
*/
void _jxr_rflush_mb_strip(jxr_image_t image, int tx, int ty, int my)
{
    /* This is the position within the image of the current
    line. It accounts for the current tile row. */
    const int use_my = my + (ty>=0? image->tile_row_position[ty] : 0) - 1;

    DEBUG("rflush_mb_strip: cur_my=%d, tile-x/y=%d/%d, my=%d, use_my=%d\n", image->cur_my, tx, ty, my, use_my);

    if (image->tile_columns > 1 && tx >= 0) {
        if (tx+1 < (int) image->tile_columns) {
            /* We're actually working on a tile, and this is
            not the last tile in the row. Deliver the data
            to the correct tile buffer and return. */

            if (my == 0 && image->cur_my >= 0) {
                /* starting a new tile, dump previous */

                if (tx == 0 && ty > 0) {
                    /* First column of a row */
                    /* Complete last line of previous row */
                    rflush_collect_mb_strip_data(image, image->cur_my);
                    /* Save previous strip context */
                    rflush_save_context(image);
                    /* Flush last column of previous row. */
                    rflush_to_tile_buffer(image, image->tile_columns-1, image->cur_my);
                } else if (tx > 0) {
                    /* Column within a row, dump previous column */
                    rflush_to_tile_buffer(image, tx-1, image->cur_my);
                }

            } else if (image->cur_my >= 0) {
                rflush_to_tile_buffer(image, tx, image->cur_my);
            }
            image->cur_my = my;
            _jxr_r_rotate_mb_strip(image);
            return;

        } else {
            /* We are tiling, and this is the last tile of the
            row, so collect rows from the left tiles to
            finish the row, and proceed to processing. */
            if (my == 0) {
                /* Starting last tile of row */
                /* Flush end of previous tile */
                rflush_to_tile_buffer(image, tx-1, image->cur_my);
                image->cur_my = -1;
                /* Recover previous strip context */
                if (ty > 0) {
                    rflush_recover_context(image);
                }
            } else if (my <= (int) image->tile_row_height[ty]) {
                rflush_collect_mb_strip_data(image, image->cur_my);
            }
        }
    }


    if (use_my >= 1) {
        int ch;

        /* Dequantize the PREVIOUS strip of macroblocks DC and LP. */

        /* Reverse transform the DC/LP to 16 DC values. */

        for (ch = 0 ; ch < image->num_channels ; ch += 1)
            IPCT_level1_up1(image, use_my, ch);

        if (use_my >= 2) {
            if (OVERLAP_INFO(image) >= 2)
                for (ch = 0 ; ch < image->num_channels ; ch += 1)
                    overlap_level1_up2(image, use_my, ch);

            /* Do the second level IPCT transform to include HP values. */
            for (ch = 0 ; ch < image->num_channels ; ch += 1)
                IPCT_level2_up2(image,use_my, ch);

            if (use_my >= 3) {

                /* Do the second level post filter */
                if (OVERLAP_INFO(image) >= 1)
                    for (ch = 0 ; ch < image->num_channels ; ch += 1)
                        overlap_level2_up3(image, use_my, ch);

                /* The reverse transformation is complete for the
                PREVIOUS strip, so perform the "Output Formatting"
                and deliver the data for the application. */

                scale_and_emit_top(image, tx, use_my);
            }
        }

        /* read lwf test flag into image container */
        if (image->lwf_test == 0)
            image->lwf_test = _jxr_read_lwf_test_flag();

    }

    /* Now completely done with strip_up. Rotate the storage to
    strip_down. */
    image->cur_my = my;

    _jxr_r_rotate_mb_strip(image);
}

/*
* The input to this function is 256 samples arranged like this:
*
* DC..DC (16 DC values) HP..HP (240 HP values)
*
* Shuffle the values so that there is 1 DC, then 15 HP, and so on 16
* times. This prepares the array for 16 calls to the 4x4IPCT transform.
*/
static void dclphp_shuffle(int*data, int dclp_count)
{
    int tmp[256];
    int dc, hp, dst;
    assert(dclp_count <= 16);

    for (dc=0, hp=16, dst=0; dc<dclp_count ; ) {
        int idx;
        tmp[dst++] = data[dc++];
        for (idx = 0 ; idx < 15 ; idx += 1)
            tmp[dst++] = data[hp++];
    }

    assert(dst == 16*dclp_count);
    assert(dc == dclp_count);
    assert(hp == 16+15*dclp_count);

    for (dst = 0 ; dst<256 ; dst+=1)
        data[dst] = tmp[dst];
}

/*
* The input to this function is 256 intensities arranged as blocks,
* with each 4x4 block in raster order is together, i.e.
*
* 00..0011..1122..22...
*
* It reorders the values into a raster order that is not blocked:
*
* 0000111122223333
* 0000111122223333
* 0000111122223333, etc.
*/
static void unblock_shuffle444(int*data)
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
        tmp[ptr+0] = data[idx+0];
        tmp[ptr+1] = data[idx+1];
        tmp[ptr+2] = data[idx+2];
        tmp[ptr+3] = data[idx+3];
    }

    for (idx = 0 ; idx < 256 ; idx += 1)
        data[idx] = tmp[idx];
}

/*
* 0 1 2 3 16 17 18 19
* 4 5 6 7 20 21 22 23
* 8 9 10 11 24 25 26 27
* 12 13 14 15 28 29 30 31
* 32 33 34 35 48 49 50 51
* 36 37 38 39 52 53 54 55
* 40 41 42 43 56 57 58 59
* 44 45 46 47 60 61 62 63 ...
*/
static void unblock_shuffle422(int*data)
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
        tmp[ptr+0] = data[idx+0];
        tmp[ptr+1] = data[idx+1];
        tmp[ptr+2] = data[idx+2];
        tmp[ptr+3] = data[idx+3];
    }

    for (idx = 0 ; idx < 128 ; idx += 1)
        data[idx] = tmp[idx];
}

/*
* 0 1 2 3 16 17 18 19
* 4 5 6 7 20 21 22 23
* 8 9 10 11 24 25 26 27
* 12 13 14 15 28 29 30 31
* 32 33 34 35 48 49 50 51
* 36 37 38 39 52 53 54 55
* 40 41 42 43 56 57 58 59
* 44 45 46 47 60 61 62 63
*/
static void unblock_shuffle420(int*data)
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
        tmp[ptr+0] = data[idx+0];
        tmp[ptr+1] = data[idx+1];
        tmp[ptr+2] = data[idx+2];
        tmp[ptr+3] = data[idx+3];
    }

    for (idx = 0 ; idx < 64 ; idx += 1)
        data[idx] = tmp[idx];
}

void _jxr_r_rotate_mb_strip(jxr_image_t image)
{
    if(image->primary) {
        int ch;

        for (ch = 0 ; ch < image->num_channels ; ch += 1) {
            struct macroblock_s*tmp = image->strip[ch].up3;
            image->strip[ch].up3 = image->strip[ch].up2;
            image->strip[ch].up2 = image->strip[ch].up1;
            image->strip[ch].up1 = image->strip[ch].cur;
            image->strip[ch].cur = tmp;
        }
        
        _jxr_clear_strip_cur(image);

        if (ALPHACHANNEL_FLAG(image)) {
            struct macroblock_s*tmp = image->alpha->strip[0].up3;
            image->alpha->strip[0].up3 = image->alpha->strip[0].up2;
            image->alpha->strip[0].up2 = image->alpha->strip[0].up1;
            image->alpha->strip[0].up1 = image->alpha->strip[0].cur;
            image->alpha->strip[0].cur = tmp;
            _jxr_clear_strip_cur(image->alpha);
        }
    }
}


/*
* $Log: r_strip.c,v $
* Revision 1.53 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.52 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.51 2008/03/24 18:06:56 steve
* Imrpove DEBUG messages around quantization.
*
* Revision 1.50 2008/03/20 22:38:53 steve
* Use MB HPQP instead of first HPQP in decode.
*
* Revision 1.49 2008/03/18 21:09:11 steve
* Fix distributed color prediction.
*
* Revision 1.48 2008/03/17 23:48:12 steve
* Bias and Scaling for CMYK
*
* Revision 1.47 2008/03/17 21:48:56 steve
* CMYK decode support
*
* Revision 1.46 2008/03/14 17:08:51 gus
* *** empty log message ***
*
* Revision 1.45 2008/03/13 17:49:31 steve
* Fix problem with YUV422 CBP prediction for UV planes
*
* Add support for YUV420 encoding.
*
* Revision 1.44 2008/03/11 22:12:49 steve
* Encode YUV422 through DC.
*
* Revision 1.43 2008/03/05 06:58:10 gus
* *** empty log message ***
*
* Revision 1.42 2008/03/03 23:33:53 steve
* Implement SHIFT_BITS functionality.
*
* Revision 1.41 2008/03/02 18:35:27 steve
* Add support for BD16
*
* Revision 1.40 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.39 2008/02/01 22:49:53 steve
* Handle compress of YUV444 color DCONLY
*
* Revision 1.38 2008/01/08 23:23:18 steve
* Clean up some DEBUG messages.
*
* Revision 1.37 2008/01/04 17:07:35 steve
* API interface for setting QP values.
*
* Revision 1.36 2007/11/26 01:47:15 steve
* Add copyright notices per MS request.
*
* Revision 1.35 2007/11/22 19:02:05 steve
* More fixes of color plane buffer sizes.
*
* Revision 1.34 2007/11/22 02:51:04 steve
* Fix YUV422 strip save byte count - buffer overrun
*
* Revision 1.33 2007/11/21 23:26:14 steve
* make all strip buffers store MB data.
*
* Revision 1.32 2007/11/21 00:34:30 steve
* Rework spatial mode tile macroblock shuffling.
*
* Revision 1.31 2007/11/20 17:08:02 steve
* Fix SPATIAL processing of QUANT values for color.
*
* Revision 1.30 2007/11/16 21:33:48 steve
* Store MB Quant, not qp_index.
*
* Revision 1.29 2007/11/16 20:03:57 steve
* Store MB Quant, not qp_index.
*
* Revision 1.28 2007/11/12 23:21:55 steve
* Infrastructure for frequency mode ordering.
*
* Revision 1.27 2007/11/09 01:18:58 steve
* Stub strip input processing.
*
* Revision 1.26 2007/11/06 21:45:04 steve
* Fix MB of previous tile in row.
*
* Revision 1.25 2007/11/06 01:39:22 steve
* Do not collect strip data for pad strips.
*
* Revision 1.24 2007/11/05 02:01:12 steve
* Add support for mixed row/column tiles.
*
* Revision 1.23 2007/11/02 21:06:07 steve
* Filtering when tile rows are present.
*
* Revision 1.22 2007/11/02 00:19:06 steve
* Fix Multiple rows of tiles strip flush.
*
* Revision 1.21 2007/11/01 21:09:40 steve
* Multiple rows of tiles.
*
* Revision 1.20 2007/10/30 21:32:46 steve
* Support for multiple tile columns.
*
* Revision 1.19 2007/10/23 00:34:12 steve
* Level1 filtering for YUV422 and YUV420
*
* Revision 1.18 2007/10/22 22:33:12 steve
* Level2 filtering for YUV422
*
* Revision 1.17 2007/10/22 21:52:37 steve
* Level2 filtering for YUV420
*
* Revision 1.16 2007/10/19 22:07:36 steve
* Clean up YUV420 to YUV444 conversion corner cases.
*
* Revision 1.15 2007/10/19 20:48:53 steve
* Convert YUV420 to YUV444 works.
*
* Revision 1.14 2007/10/17 23:43:20 steve
* Add support for YUV420
*
* Revision 1.13 2007/10/01 20:39:34 steve
* Add support for YUV422 LP bands.
*
* Revision 1.12 2007/09/20 18:04:11 steve
* support render of YUV422 images.
*
* Revision 1.11 2007/09/12 01:10:22 steve
* Fix rounding/floor/ceil of YUV to RGB transform.
*
* Revision 1.10 2007/09/11 00:40:06 steve
* Fix rendering of chroma to add the missing *2.
* Fix handling of the chroma LP samples
* Parse some of the HP CBP data in chroma.
*
* Revision 1.9 2007/09/10 23:02:48 steve
* Scale chroma channels?
*
* Revision 1.8 2007/09/08 01:01:44 steve
* YUV444 color parses properly.
*
* Revision 1.7 2007/09/04 19:10:46 steve
* Finish level1 overlap filtering.
*
* Revision 1.6 2007/08/15 01:54:11 steve
* Add level2 filter to decoder.
*
* Revision 1.5 2007/08/13 22:55:12 steve
* Cleanup rflush_md_strip function.
*
* Revision 1.4 2007/08/02 22:48:27 steve
* Add missing clip of calculated values.
*
* Revision 1.3 2007/07/21 00:25:48 steve
* snapshot 2007 07 20
*
* Revision 1.2 2007/07/11 00:53:36 steve
* HP adaptation and precition corrections.
*
* Revision 1.1 2007/06/28 20:03:11 steve
* LP processing seems to be OK now.
*
*/

