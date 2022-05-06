/*
**
** $Id: algo.c,v 1.3 2008-05-13 13:47:11 thor Exp $
**
**
*/

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
#pragma comment (user,"$Id: algo.c,v 1.3 2008-05-13 13:47:11 thor Exp $")
#else
#ident "$Id: algo.c,v 1.3 2008-05-13 13:47:11 thor Exp $"
#endif

/*
* This file contains many common algorithms of JPEGXR processing.
*/

# include "jxr_priv.h"
# include <stdio.h>
# include <stdlib.h>
# include <limits.h>
# include <assert.h>

static void InitVLCTable2(jxr_image_t image, int vlc_select);

const int _jxr_hp_scan_map[16] = { 0, 1, 4, 5,
2, 3, 6, 7,
8, 9,12,13,
10,11,14,15 };

static const unsigned ScanTotals[15] ={32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4};
static int long_word_flag = 0;

/*
* These two functions implemented floor(x/2) and ceil(x/2). Note that
* in C/C++ x/2 is NOT the same as floor(x/2) if x<0. It may be on
* some systems, but not in general. So we do all arithmetic with
* positive values and get the floor/ceil right by manipulating signs
* and rounding afterwards. The YUV444 --> RGB transform must get this
* rounding exactly right or there may be losses in the lossless transform.
*/
int _jxr_floor_div2(int x)
{
    if (x >= 0)
        return x/2;
    else
        return -((-x+1)/2);
}

int _jxr_ceil_div2(int x)
{
    if (x >= 0)
        return (x+1)/2;
    else
        return -((-x)/2);
}

int _jxr_quant_map(jxr_image_t image, int x, int shift)
{
    if (x == 0)
        return 1;

    int man, exp;

    if (image->scaled_flag) {
        if (x < 16) {
            man = x;
            exp = shift;
        } else {
            man = 16 + (x%16);
            exp = ((x>>4) - 1) + shift;
        }
    } else {
        if (x < 32) {
            man = (x + 3) >> 2;
            exp = 0;
        } else if (x < 48) {
            man = (16 + (x%16) + 1) >> 1;
            exp = (x>>4) - 2;
        } else {
            man = 16 + (x%16);
            exp = (x>>4) - 3;
        }
    }

    return man << exp;
}

int _jxr_vlc_select(int band, int chroma_flag)
{
    int vlc_select = 0;

    /* Based on the band and the chroma flag, select the vlc table
    that we want to use for the ABSLEVEL_INDEX decoder. */
    switch (band) {
        case 0: /* DC */
            if (chroma_flag)
                vlc_select = AbsLevelIndDCChr;
            else
                vlc_select = AbsLevelIndDCLum;
            break;
        case 1: /* LP */
            if (chroma_flag)
                vlc_select = AbsLevelIndLP1;
            else
                vlc_select = AbsLevelIndLP0;
            break;
        case 2: /* HP */
            if (chroma_flag)
                vlc_select = AbsLevelIndHP1;
            else
                vlc_select = AbsLevelIndHP0;
            break;
        default:
            assert(0);
            break;
    }

    return vlc_select;
}

void _jxr_InitVLCTable(jxr_image_t image, int vlc_select)
{
    struct adaptive_vlc_s*table = image->vlc_table + vlc_select;
    table->table = 0;
    table->deltatable = 0;
    table->discriminant = 0;
}

static void InitVLCTable2(jxr_image_t image, int vlc_select)
{
    struct adaptive_vlc_s*table = image->vlc_table + vlc_select;
    table->table = 1;
    table->deltatable = 0;
    table->delta2table = 1;
    table->discriminant = 0;
    table->discriminant2 = 0;
}

/*
* This function is for running the adapt for VLC machines that have
* only 2 tables. In this case, only the table and discriminant
* members are used, and the table is 0 or 1.
*/
void _jxr_AdaptVLCTable(jxr_image_t image, int vlc_select)
{
    const int cLowerBound = -8;
    const int cUpperBound = 8;

    struct adaptive_vlc_s*table = image->vlc_table + vlc_select;
    const int max_index = 1; /* Only 2 code tables. */

    table->deltatable = 0;
    if ((table->discriminant < cLowerBound) && (table->table != 0)) {
        table->table -= 1;
        table->discriminant = 0;
    } else if ((table->discriminant > cUpperBound) && (table->table != max_index)) {
        table->table += 1;
        table->discriminant = 0;
    } else {
        if (table->discriminant < -64) table->discriminant = -64;
        if (table->discriminant > 64) table->discriminant = 64;
    }
}

static void AdaptVLCTable2(jxr_image_t image, int vlc_select, int max_index)
{
    const int LOWER_BOUND = -8;
    const int UPPER_BOUND = 8;

    struct adaptive_vlc_s*table = image->vlc_table + vlc_select;

    int disc_lo = table->discriminant;
    int disc_hi = table->discriminant2;

    int change_flag = 0;

    if (disc_lo < LOWER_BOUND && table->table > 0) {
        table->table -= 1;
        change_flag = 1;

    } else if (disc_hi > UPPER_BOUND && table->table < max_index) {
        table->table += 1;
        change_flag = 1;
    }


    if (change_flag) {
        table->discriminant = 0;
        table->discriminant2 = 0;

        if (table->table == max_index) {
            table->deltatable = table->table - 1;
            table->delta2table = table->table - 1;
        } else if (table->table == 0) {
            table->deltatable = table->table;
            table->delta2table = table->table;
        } else {
            table->deltatable = table->table - 1;
            table->delta2table = table->table;
        }

    } else {
        if (table->discriminant < -64) table->discriminant = -64;
        if (table->discriminant > 64) table->discriminant = 64;
        if (table->discriminant2 < -64) table->discriminant2 = -64;
        if (table->discriminant2 > 64) table->discriminant2 = 64;
    }
}

void _jxr_AdaptLP(jxr_image_t image)
{
    AdaptVLCTable2(image, DecFirstIndLPLum, 4);
    AdaptVLCTable2(image, DecIndLPLum0, 3);
    AdaptVLCTable2(image, DecIndLPLum1, 3);
    AdaptVLCTable2(image, DecFirstIndLPChr, 4);
    AdaptVLCTable2(image, DecIndLPChr0, 3);
    AdaptVLCTable2(image, DecIndLPChr1, 3);

    _jxr_AdaptVLCTable(image, AbsLevelIndLP0);
    _jxr_AdaptVLCTable(image, AbsLevelIndLP1);

    DEBUG(" AdaptLP: DecFirstIndLPLum=%d\n", image->vlc_table[DecFirstIndLPLum].table);
    DEBUG(" : DecIndLPLum0=%d\n", image->vlc_table[DecIndLPLum0].table);
    DEBUG(" : DecIndLPLum1=%d\n", image->vlc_table[DecIndLPLum1].table);
    DEBUG(" : DecFirstIndLPChr=%d\n", image->vlc_table[DecFirstIndLPChr].table);
    DEBUG(" : DecIndLPChr0=%d\n", image->vlc_table[DecIndLPChr0].table);
    DEBUG(" : DecIndLPChr1=%d\n", image->vlc_table[DecIndLPChr1].table);
}

void _jxr_AdaptHP(jxr_image_t image)
{
    AdaptVLCTable2(image, DecFirstIndHPLum, 4);
    AdaptVLCTable2(image, DecIndHPLum0, 3);
    AdaptVLCTable2(image, DecIndHPLum1, 3);
    AdaptVLCTable2(image, DecFirstIndHPChr, 4);
    AdaptVLCTable2(image, DecIndHPChr0, 3);
    AdaptVLCTable2(image, DecIndHPChr1, 3);

    _jxr_AdaptVLCTable(image, AbsLevelIndHP0);
    _jxr_AdaptVLCTable(image, AbsLevelIndHP1);

    _jxr_AdaptVLCTable(image, DecNumCBP);
    _jxr_AdaptVLCTable(image, DecNumBlkCBP);
}

/*
* Despite the name, this function doesn't actually reset a
* context. It uses the tx (X position of the time) and mx (X position
* of the macroblock in the tile) to calculate true or false that the
* caller should invoke a context reset.
*/
int _jxr_InitContext(jxr_image_t /*image*/, unsigned /*tx*/, unsigned /*ty*/,
                     unsigned mx, unsigned my)
{
    if (mx == 0 && my == 0)
        return 1;
    else
        return 0;
}

/*
* This function guards Adapt?? functions that are called during the
* parse. At the end of a MB where this function returns true, the
* processing function will call the Adapt?? function.
*
* NOTE: It is *correct* that this function is true for the first and
* the last macroblock. This allows some far right context of a tile
* to inform the first column of the next line, but also allows crazy
* differences to be adapted away.
*/
int _jxr_ResetContext(jxr_image_t image, unsigned tx, unsigned mx)
{
    assert(tx < image->tile_columns);
    assert(image->tile_column_width);
    /* Return true for every 16 macroblocks in the tile. */
    if (mx%16 == 0)
        return 1;
    /* Return true for the final macroblock in the tile. */
    if (image->tile_column_width[tx] == mx+1)
        return 1;

    return 0;
}

int _jxr_ResetTotals(jxr_image_t /*image*/, unsigned mx)
{
    if (mx%16 == 0)
        return 1;
    else return 0;
}

void _jxr_InitializeModelMB(struct model_s*model, int band)
{
    assert(band <= 2);

    model->state[0] = 0;
    model->state[1] = 0;
    model->bits[0] = (2-band) * 4;
    model->bits[1] = (2-band) * 4;
}

void _jxr_UpdateModelMB(jxr_image_t image, int lap_mean[2], struct model_s*model, int band)
{
    const int modelweight = 70;
    static const int weight0[3] = { 240, 12, 1 };
    static const int weight1[3][MAX_CHANNELS] = {
        {0,240,120,80, 60,48,40,34, 30,27,24,22, 20,18,17,16 },
        {0,12,6,4, 3,2,2,2, 2,1,1,1, 1,1,1,1 },
        {0,16,8,5, 4,3,3,2, 2,2,2,1, 1,1,1,1 }
    };
    static const int weight2[6] = { 120,37,2, 120,18,1 };

    assert(band < 3);

    lap_mean[0] *= weight0[band];
    switch (image->use_clr_fmt) {
        case 1: /* YUV420 */
            lap_mean[1] *= weight2[band];
            break;
        case 2: /* YUV422 */
            lap_mean[1] *= weight2[3+band];
            break;
        default:
            lap_mean[1] *= weight1[band][image->num_channels-1];
            if (band == 2 /*HP*/)
                lap_mean[1] >>= 4;
            break;
    }

    int j;
    for (j = 0; j < 2; j += 1) {
        int ms = model->state[j];
        int delta = (lap_mean[j] - modelweight) >> 2;

        if (delta <= -8) {
            delta += 4;
            if (delta < -16)
                delta = -16;
            ms += delta;
            if (ms < -8) {
                if (model->bits[j] == 0) {
                    ms = -8;
                } else {
                    ms = 0;
                    model->bits[j] -= 1;
                }
            }
        } else if (delta >= 8) {
            delta -= 4;
            if (delta > 15)
                delta = 15;
            ms += delta;
            if (ms > 8) {
                if (model->bits[j] >= 15) {
                    model->bits[j] = 15;
                    ms = 8;
                } else {
                    ms = 0;
                    model->bits[j] += 1;
                }
            }
        }
        model->state[j] = ms;
        /* If clr_fmt == YONLY, then we really only want to do
        this loop once, for j==0. */
        if (image->use_clr_fmt == 0/*YONLY*/)
            break;
    }
}

void _jxr_InitializeCountCBPLP(jxr_image_t image)
{
    image->count_max_CBPLP = 1;
    image->count_zero_CBPLP = 1;
}

void _jxr_UpdateCountCBPLP(jxr_image_t image, int cbplp, int max)
{
    image->count_zero_CBPLP += 1;
    if (cbplp == 0)
        image->count_zero_CBPLP -= 4;
    if (image->count_zero_CBPLP > 7)
        image->count_zero_CBPLP = 7;
    if (image->count_zero_CBPLP < -8)
        image->count_zero_CBPLP = -8;

    image->count_max_CBPLP += 1;
    if (cbplp == max)
        image->count_max_CBPLP -= 4;
    if (image->count_max_CBPLP > 7)
        image->count_max_CBPLP = 7;
    if (image->count_max_CBPLP < -8)
        image->count_max_CBPLP = -8;
}

void _jxr_InitLPVLC(jxr_image_t image)
{
    DEBUG(" ... InitLPVLC\n");
    InitVLCTable2(image, DecFirstIndLPLum);
    InitVLCTable2(image, DecIndLPLum0);
    InitVLCTable2(image, DecIndLPLum1);
    InitVLCTable2(image, DecFirstIndLPChr);
    InitVLCTable2(image, DecIndLPChr0);
    InitVLCTable2(image, DecIndLPChr1);

    _jxr_InitVLCTable(image, AbsLevelIndLP0);
    _jxr_InitVLCTable(image, AbsLevelIndLP1);
}

void _jxr_InitializeAdaptiveScanLP(jxr_image_t image)
{
    static const int ScanOrderLP[15] = { 4, 1, 5,
        8, 2, 9, 6,
        12, 3, 10, 13,
        7, 14, 11, 15};
    int idx;
    for (idx = 0 ; idx < 15 ; idx += 1) {
        image->lopass_scanorder[idx] = ScanOrderLP[idx];
        image->lopass_scantotals[idx] = ScanTotals[idx];
    }
}

/*
*/
void _jxr_InitializeAdaptiveScanHP(jxr_image_t image)
{
    static const unsigned ScanOrderHor[15] ={ 4, 1, 5,
        8, 2, 9, 6,
        12, 3, 10, 13,
        7, 14, 11, 15};
    static const unsigned ScanOrderVer[15] ={ 1, 2, 5,
        4, 3, 6, 9,
        8, 7, 12, 15,
        13, 10, 11, 14};
    int idx;
    for (idx = 0 ; idx < 15 ; idx += 1) {
        image->hipass_hor_scanorder[idx] = ScanOrderHor[idx];
        image->hipass_hor_scantotals[idx] = ScanTotals[idx];
        image->hipass_ver_scanorder[idx] = ScanOrderVer[idx];
        image->hipass_ver_scantotals[idx] = ScanTotals[idx];
    }
}

void _jxr_InitializeCBPModel(jxr_image_t image)
{
    image->hp_cbp_model.state[0] = 0;
    image->hp_cbp_model.state[1] = 0;
    image->hp_cbp_model.count0[0] = -4;
    image->hp_cbp_model.count0[1] = -4;
    image->hp_cbp_model.count1[0] = 4;
    image->hp_cbp_model.count1[1] = 4;
}

void _jxr_InitHPVLC(jxr_image_t image)
{
    InitVLCTable2(image, DecFirstIndHPLum);
    InitVLCTable2(image, DecIndHPLum0);
    InitVLCTable2(image, DecIndHPLum1);
    InitVLCTable2(image, DecFirstIndHPChr);
    InitVLCTable2(image, DecIndHPChr0);
    InitVLCTable2(image, DecIndHPChr1);

    _jxr_InitVLCTable(image, AbsLevelIndHP0);
    _jxr_InitVLCTable(image, AbsLevelIndHP1);

    /* _jxr_InitVLCTable(image, DecNumCBP); */
    /* _jxr_InitVLCTable(image, DecNumBlkCBP); */
}

void _jxr_InitCBPVLC(jxr_image_t image)
{
    _jxr_InitVLCTable(image, DecNumCBP);
    _jxr_InitVLCTable(image, DecNumBlkCBP);
}

static int num_ones(int val)
{
    assert(val >= 0);

    int cnt = 0;
    while (val > 0) {
        if (val&1) cnt += 1;
        val >>= 1;
    }
    return cnt;
}

# define SAT(x) do { if ((x) > 15) (x)=15 ; else if ((x) < -16) (x)=-16; } while(0);

static void update_cbp_model(jxr_image_t image, int c1, int norig)
{
    const int ndiff = 3;

    struct cbp_model_s*hp_cbp_model = & (image->hp_cbp_model);

    hp_cbp_model->count0[c1] += norig - ndiff;
    SAT(hp_cbp_model->count0[c1]);

    hp_cbp_model->count1[c1] += 16 - norig - ndiff;
    SAT(hp_cbp_model->count1[c1]);

    if (hp_cbp_model->count0[c1] < 0) {
        if (hp_cbp_model->count0[c1] < hp_cbp_model->count1[c1])
            hp_cbp_model->state[c1] = 1;
        else
            hp_cbp_model->state[c1] = 2;

    } else if (hp_cbp_model->count1[c1] < 0) {
        hp_cbp_model->state[c1] = 2;

    } else {
        hp_cbp_model->state[c1] = 0;
    }
}

int _jxr_PredCBP444(jxr_image_t image, int*diff_cbp,
                    int channel, unsigned tx,
                    unsigned mx, unsigned my)
{
    int chroma_flag = 0;
    if (channel > 0)
        chroma_flag = 1;

    DEBUG(" PredCBP444: Prediction mode = %d\n", image->hp_cbp_model.state[chroma_flag]);
    int cbp = diff_cbp[channel];
    if (image->hp_cbp_model.state[chroma_flag] == 0) {
        if (mx == 0) {
            if (my == 0)
                cbp ^= 1;
            else
                cbp ^= (MACROBLK_UP1_HPCBP(image, channel, tx, mx)>>10)&1;
        } else {
            cbp ^= (MACROBLK_CUR_HPCBP(image, channel, tx, mx-1)>>5)&1;
        }

        cbp ^= 0x02 & (cbp<<1);
        cbp ^= 0x10 & (cbp<<3);
        cbp ^= 0x20 & (cbp<<1);
        cbp ^= (cbp&0x33) << 2;
        cbp ^= (cbp&0xcc) << 6;
        cbp ^= (cbp&0x3300) << 2;

    } else if (image->hp_cbp_model.state[chroma_flag] == 2) {
        cbp ^= 0xffff;
    }

    int norig = num_ones(cbp);
    DEBUG(" PredCBP444: NOrig=%d, CBPModel.Count0/1[%d]= %d/%d\n", norig, chroma_flag,
        image->hp_cbp_model.count0[chroma_flag],
        image->hp_cbp_model.count1[chroma_flag]);
    update_cbp_model(image, chroma_flag, norig);
    DEBUG(" PredCBP444: ...becomes CBPModel.Count0/1[%d]= %d/%d, new state=%d\n",
        chroma_flag, image->hp_cbp_model.count0[chroma_flag],
        image->hp_cbp_model.count1[chroma_flag], image->hp_cbp_model.state[chroma_flag]);
    return cbp;
}

void _jxr_w_PredCBP444(jxr_image_t image, int ch, unsigned tx, unsigned mx, int my)
{
    int chroma_flag = 0;
    if (ch > 0)
        chroma_flag = 1;

    DEBUG(" PredCBP444: Prediction mode = %d\n", image->hp_cbp_model.state[chroma_flag]);
    int cbp = MACROBLK_UP1_HPCBP(image,ch,tx,mx);
    int norig = num_ones(cbp);

    DEBUG(" PredCBP444: ... cbp starts as 0x%x\n", cbp);

    if (image->hp_cbp_model.state[chroma_flag] == 0) {

        cbp ^= (cbp&0x3300) << 2;
        cbp ^= (cbp&0xcc) << 6;
        cbp ^= (cbp&0x33) << 2;
        cbp ^= 0x20 & (cbp<<1);
        cbp ^= 0x10 & (cbp<<3);
        cbp ^= 0x02 & (cbp<<1);

        if (mx == 0) {
            if (my == 0)
                cbp ^= 1;
            else
                cbp ^= (MACROBLK_CUR_HPCBP(image,ch, tx, mx)>>10)&1;
        } else {
            cbp ^= (MACROBLK_UP1_HPCBP(image,ch, tx, mx-1)>>5)&1;
        }


    } else if (image->hp_cbp_model.state[chroma_flag] == 2){
        cbp ^= 0xffff;
    }

    DEBUG(" PredCBP444: ... diff_cbp 0x%04x\n", cbp);
    MACROBLK_UP1(image,ch,tx,mx).hp_diff_cbp = cbp;

    update_cbp_model(image, chroma_flag, norig);
}

int _jxr_PredCBP422(jxr_image_t image, int*diff_cbp,
                    int channel, unsigned tx,
                    unsigned mx, unsigned my)
{
    assert(channel > 0);
    DEBUG(" PredCBP422: Prediction mode = %d, channel=%d, cbp_mode.State[1]=%d\n",
        image->hp_cbp_model.state[1], channel, image->hp_cbp_model.state[1]);
    int cbp = diff_cbp[channel];

    if (image->hp_cbp_model.state[1] == 0) {
        if (mx == 0) {
            if (my == 0)
                cbp ^= 1;
            else
                cbp ^= (MACROBLK_UP1_HPCBP(image, channel, tx, mx)>>6)&1;
        } else {
            cbp ^= (MACROBLK_CUR_HPCBP(image, channel, tx, mx-1)>>1)&1;
        }

        cbp ^= 0x02 & (cbp<<1);
        cbp ^= 0x0c & (cbp<<2);
        cbp ^= 0x30 & (cbp<<2);
        cbp ^= 0xc0 & (cbp<<2);
    } else if (image->hp_cbp_model.state[1] == 2) {
        cbp ^= 0xff;
    }

    int norig = num_ones(cbp) * 2;
    update_cbp_model(image, 1, norig);

    return cbp;
}

void _jxr_w_PredCBP422(jxr_image_t image, int ch, unsigned tx, unsigned mx, int my)
{
    assert(ch > 0);
    DEBUG(" PredCBP422: Prediction mode = %d\n", image->hp_cbp_model.state[1]);
    int cbp = MACROBLK_UP1_HPCBP(image,ch,tx,mx);
    int norig = num_ones(cbp) * 2;

    DEBUG(" PredCBP422: ... cbp[%d] starts as 0x%x\n", ch, cbp);

    if (image->hp_cbp_model.state[1] == 0) {

        cbp ^= 0xc0 & (cbp<<2);
        cbp ^= 0x30 & (cbp<<2);
        cbp ^= 0x0c & (cbp<<2);
        cbp ^= 0x02 & (cbp<<1);

        if (mx == 0) {
            if (my == 0)
                cbp ^= 1;
            else
                cbp ^= (MACROBLK_CUR_HPCBP(image, ch, tx, mx)>>6)&1;
        } else {
            cbp ^= (MACROBLK_UP1_HPCBP(image, ch, tx, mx-1)>>1)&1;
        }

    } else if (image->hp_cbp_model.state[1] == 2) {
        cbp ^= 0xff;
    }

    DEBUG(" PredCBP422: ... diff_cbp 0x%04x\n", cbp);
    MACROBLK_UP1(image,ch,tx,mx).hp_diff_cbp = cbp;

    update_cbp_model(image, 1, norig);
}


int _jxr_PredCBP420(jxr_image_t image, int*diff_cbp,
                    int channel, unsigned tx,
                    unsigned mx, unsigned my)
{
    assert(channel > 0);
    DEBUG(" PredCBP420: Prediction mode = %d, channel=%d, cbp_mode.State[1]=%d\n",
        image->hp_cbp_model.state[1], channel, image->hp_cbp_model.state[1]);
    int cbp = diff_cbp[channel];

    if (image->hp_cbp_model.state[1] == 0) {
        if (mx == 0) {
            if (my == 0)
                cbp ^= 1;
            else
                cbp ^= (MACROBLK_UP1_HPCBP(image, channel, tx, mx)>>2)&1;
        } else {
            cbp ^= (MACROBLK_CUR_HPCBP(image, channel, tx, mx-1)>>1)&1;
        }

        cbp ^= 0x02 & (cbp<<1);
        cbp ^= 0x0c & (cbp<<2);
    } else if (image->hp_cbp_model.state[1] == 2) {
        cbp ^= 0xf;
    }

    int norig = num_ones(cbp) * 4;
    update_cbp_model(image, 1, norig);

    return cbp;
}

void _jxr_w_PredCBP420(jxr_image_t image, int ch, unsigned tx, unsigned mx, int my)
{
    assert(ch > 0);
    DEBUG(" PredCBP420: Prediction mode = %d\n", image->hp_cbp_model.state[1]);
    int cbp = MACROBLK_UP1_HPCBP(image,ch,tx,mx);
    int norig = num_ones(cbp) * 4;

    DEBUG(" PredCBP420: ... cbp[%d] starts as 0x%x\n", ch, cbp);

    if (image->hp_cbp_model.state[1] == 0) {

        cbp ^= 0x0c & (cbp<<2);
        cbp ^= 0x02 & (cbp<<1);

        if (mx == 0) {
            if (my == 0)
                cbp ^= 1;
            else
                cbp ^= (MACROBLK_CUR_HPCBP(image, ch, tx, mx)>>2)&1;
        } else {
            cbp ^= (MACROBLK_UP1_HPCBP(image, ch, tx, mx-1)>>1)&1;
        }

    } else if (image->hp_cbp_model.state[1] == 2) {
        cbp ^= 0xf;
    }

    DEBUG(" PredCBP420: ... diff_cbp 0x%04x\n", cbp);
    MACROBLK_UP1(image,ch,tx,mx).hp_diff_cbp = cbp;

    update_cbp_model(image, 1, norig);
}

void _jxr_ResetTotalsAdaptiveScanLP(jxr_image_t image)
{
    int idx;
    for (idx = 0 ; idx < 15 ; idx += 1) {
        image->lopass_scantotals[idx] = ScanTotals[idx];
    }
}

void _jxr_ResetTotalsAdaptiveScanHP(jxr_image_t image)
{
    int idx;
    for (idx = 0 ; idx < 15 ; idx += 1) {
        image->hipass_hor_scantotals[idx] = ScanTotals[idx];
        image->hipass_ver_scantotals[idx] = ScanTotals[idx];
    }
}

static int
calculate_mbdc_mode(jxr_image_t image, int tx, int mx, int my)
{
    if (mx == 0 && my == 0)
        return 3; /* No prediction. */

    if (mx == 0)
        return 1; /* Predictions from top only. */

    if (my == 0)
        return 0; /* prediction from left only */

    long left = MACROBLK_CUR_DC(image, 0, tx, mx-1);
    long top = MACROBLK_UP_DC(image, 0, tx, mx);
    long topleft = MACROBLK_UP_DC(image, 0, tx, mx-1);

    long strhor = 0;
    long strvert = 0;
    if (image->use_clr_fmt==0 || image->use_clr_fmt==6) {/* YONLY or NCOMPONENT */

        strhor = labs(topleft - left);
        strvert = labs(topleft - top);
    } else {
        long left_u = MACROBLK_CUR_DC(image, 1, tx, mx-1);
        long top_u = MACROBLK_UP_DC(image, 1, tx, mx);
        long topleft_u = MACROBLK_UP_DC(image, 1, tx, mx-1);
        long left_v = MACROBLK_CUR_DC(image, 2, tx, mx-1);
        long top_v = MACROBLK_UP_DC(image, 2, tx, mx);
        long topleft_v = MACROBLK_UP_DC(image, 2, tx, mx-1);

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

static void predict_lp444(jxr_image_t image, int tx, int mx, int my, int ch, int mblp_mode);
static void predict_lp422(jxr_image_t image, int tx, int mx, int my, int ch, int mblp_mode, int mbdc_mode);
static void predict_lp420(jxr_image_t image, int tx, int mx, int my, int ch, int mblp_mode);

void _jxr_complete_cur_dclp(jxr_image_t image, int tx, int mx, int my)
{
    /* Calculate the mbcd prediction mode. This mode is used for
    all the planes of DC data. */
    int mbdc_mode = calculate_mbdc_mode(image, tx, mx, image->cur_my);
    /* Now process all the planes of DC data. */
    int ch;
    for (ch = 0 ; ch < image->num_channels ; ch += 1) {
        long left = mx>0? MACROBLK_CUR_DC(image,ch,tx,mx-1) : 0;
        long top = MACROBLK_UP_DC(image,ch,tx,mx);

        DEBUG(" MBDC_MODE=%d for TX=%d, MBx=%d, MBy=%d (cur_my=%d), ch=%d, left=0x%lx, top=0x%lx, cur=0x%x\n",
            mbdc_mode, tx, mx, my, image->cur_my, ch, left, top, MACROBLK_CUR_DC(image,ch,tx,mx));

        MACROBLK_CUR(image,ch,tx,mx).pred_dclp[0] = MACROBLK_CUR_DC(image,ch,tx,mx);
        CHECK1(image->lwf_test, MACROBLK_CUR_DC(image,ch,tx,mx));
        switch (mbdc_mode) {
            case 0: /* left */
                MACROBLK_CUR_DC(image,ch,tx,mx) += left;
                break;
            case 1: /* top */
                MACROBLK_CUR_DC(image,ch,tx,mx) += top;
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
                    MACROBLK_CUR_DC(image,ch,tx,mx) += (left+top+1) >> 1 | ((left+top+1)&~INT_MAX);
                else
                    MACROBLK_CUR_DC(image,ch,tx,mx) += (left+top) >> 1 | ((left+top)&~INT_MAX);
                break;
            default:
                break;
        }
    }

    int mblp_mode = 0;
    if (mbdc_mode==0 && MACROBLK_CUR_LP_QUANT(image,0,tx,mx) == MACROBLK_CUR_LP_QUANT(image,0,tx,mx-1)) {
        mblp_mode = 0;
    } else if (mbdc_mode==1 && MACROBLK_CUR_LP_QUANT(image,0,tx,mx) == MACROBLK_UP1_LP_QUANT(image,0,tx,mx)) {
        mblp_mode = 1;

    } else {
        mblp_mode = 2;
    }

    DEBUG(" MBLP_MODE=%d for MBx=%d, MBy=%d (lp_quant=%d,lp_quant_ctx=%d)\n", mblp_mode, mx, image->cur_my,
        MACROBLK_CUR_LP_QUANT(image,0,tx,mx),
        mbdc_mode==0? MACROBLK_CUR_LP_QUANT(image,0,tx,mx-1) : mbdc_mode==1 ? MACROBLK_UP1_LP_QUANT(image,0,tx,mx) : -1);

    predict_lp444(image, tx, mx, my, 0, mblp_mode);
    for (ch = 1 ; ch < image->num_channels ; ch += 1) {
        switch (image->use_clr_fmt) {
            case 1: /* YUV420 */
                predict_lp420(image, tx, mx, my, ch, mblp_mode);
                break;
            case 2: /* YUV422 */
                predict_lp422(image, tx, mx, my, ch, mblp_mode, mbdc_mode);
                break;
            default:
                predict_lp444(image,tx, mx, my, ch, mblp_mode);
                break;
        }
    }
}

static void predict_lp444(jxr_image_t image, int tx, int mx, int /*my*/, int ch, int mblp_mode)
{
#if defined(DETAILED_DEBUG)
    {
        int jdx;
        DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) Difference:", my, mx, ch);
        DEBUG(" 0x%08x", MACROBLK_CUR(image,ch,tx,mx).pred_dclp[0]);
        for (jdx = 0; jdx < 15 ; jdx += 1) {
            DEBUG(" 0x%08x", MACROBLK_CUR_LP(image,ch,tx,mx,jdx));
            if ((jdx+1)%4 == 3 && jdx != 14)
                DEBUG("\n%*s:", 46, "");
        }
        DEBUG("\n");
    }
#endif

    switch (mblp_mode) {
        case 0: /* left */
            CHECK3(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,3), MACROBLK_CUR_LP(image,ch,tx,mx,7), MACROBLK_CUR_LP(image,ch,tx,mx,11));
            MACROBLK_CUR_LP(image,ch,tx,mx,3) += MACROBLK_CUR(image,ch,tx,mx-1).pred_dclp[4];
            MACROBLK_CUR_LP(image,ch,tx,mx,7) += MACROBLK_CUR(image,ch,tx,mx-1).pred_dclp[5];
            MACROBLK_CUR_LP(image,ch,tx,mx,11)+= MACROBLK_CUR(image,ch,tx,mx-1).pred_dclp[6];
            break;
        case 1: /* up */
            CHECK3(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,0), MACROBLK_CUR_LP(image,ch,tx,mx,1), MACROBLK_CUR_LP(image,ch,tx,mx,2));
            MACROBLK_CUR_LP(image,ch,tx,mx,0) += MACROBLK_UP1(image,ch,tx,mx).pred_dclp[1];
            MACROBLK_CUR_LP(image,ch,tx,mx,1) += MACROBLK_UP1(image,ch,tx,mx).pred_dclp[2];
            MACROBLK_CUR_LP(image,ch,tx,mx,2) += MACROBLK_UP1(image,ch,tx,mx).pred_dclp[3];
            break;
        case 2:
            break;
    }

#if defined(DETAILED_DEBUG)
    {
        int jdx;
        DEBUG(" DC/LP (strip=%3d, mbx=%4d, ch=%d) Predicted:", my, mx, ch);
        DEBUG(" 0x%08x", MACROBLK_CUR_DC(image,ch,tx,mx));
        for (jdx = 0; jdx < 15 ; jdx += 1) {
            DEBUG(" 0x%08x", MACROBLK_CUR_LP(image,ch,tx,mx,jdx));
            if ((jdx+1)%4 == 3 && jdx != 14)
                DEBUG("\n%*s:", 45, "");
        }
        DEBUG("\n");
    }
#endif

    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[1] = MACROBLK_CUR_LP(image,ch,tx,mx,0);
    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[2] = MACROBLK_CUR_LP(image,ch,tx,mx,1);
    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[3] = MACROBLK_CUR_LP(image,ch,tx,mx,2);
    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[4] = MACROBLK_CUR_LP(image,ch,tx,mx,3);
    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[5] = MACROBLK_CUR_LP(image,ch,tx,mx,7);
    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[6] = MACROBLK_CUR_LP(image,ch,tx,mx,11);
}

static void predict_lp422(jxr_image_t image, int tx, int mx, int /*my*/, int ch, int mblp_mode, int mbdc_mode)
{
#if defined(DETAILED_DEBUG)
    {
        int jdx;
        DEBUG(" DC/LP (strip=%3d, tx=%d, mbx=%4d, ch=%d) Difference:", my, tx, mx, ch);
        DEBUG(" 0x%08x", MACROBLK_CUR(image,ch,tx,mx).pred_dclp[0]);
        for (jdx = 0; jdx < 7 ; jdx += 1) {
            DEBUG(" 0x%08x", MACROBLK_CUR_LP(image,ch,tx,mx,jdx));
            if ((jdx+1)%4 == 3 && jdx != 6)
                DEBUG("\n%*s:", 52, "");
        }
        DEBUG("\n");
    }
#endif

    switch (mblp_mode) {
case 0: /* left */
    CHECK3(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,3), MACROBLK_CUR_LP(image,ch,tx,mx,1), MACROBLK_CUR_LP(image,ch,tx,mx,5));
    MACROBLK_CUR_LP(image,ch,tx,mx,3) += MACROBLK_CUR(image,ch,tx,mx-1).pred_dclp[4];
    MACROBLK_CUR_LP(image,ch,tx,mx,1) += MACROBLK_CUR(image,ch,tx,mx-1).pred_dclp[2];
    MACROBLK_CUR_LP(image,ch,tx,mx,5) += MACROBLK_CUR(image,ch,tx,mx-1).pred_dclp[6];
    break;
case 1: /* up */
    CHECK3(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,3), MACROBLK_CUR_LP(image,ch,tx,mx,0), MACROBLK_CUR_LP(image,ch,tx,mx,4));
    MACROBLK_CUR_LP(image,ch,tx,mx,3) += MACROBLK_UP1(image,ch,tx,mx).pred_dclp[4];
    MACROBLK_CUR_LP(image,ch,tx,mx,0) += MACROBLK_UP1(image,ch,tx,mx).pred_dclp[5];
    MACROBLK_CUR_LP(image,ch,tx,mx,4) += MACROBLK_CUR_LP(image,ch,tx,mx,0);
    break;
case 2:
    if (mbdc_mode == 1)
    {
        CHECK1(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,4));
        MACROBLK_CUR_LP(image,ch,tx,mx,4) += MACROBLK_CUR_LP(image,ch,tx,mx,0);
    }
    break;
    }

#if defined(DETAILED_DEBUG)
    {
        int jdx;
        DEBUG(" DC/LP (strip=%3d, tx=%d, mbx=%4d, ch=%d) Predicted:", my, tx, mx, ch);
        DEBUG(" 0x%08x", MACROBLK_CUR_DC(image,ch,tx,mx));
        for (jdx = 0; jdx < 7 ; jdx += 1) {
            DEBUG(" 0x%08x", MACROBLK_CUR_LP(image,ch,tx,mx,jdx));
            if ((jdx+1)%4 == 3 && jdx != 6)
                DEBUG("\n%*s:", 51, "");
        }
        DEBUG("\n");
    }
#endif

    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[1] = MACROBLK_CUR_LP(image,ch,tx,mx,0);
    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[2] = MACROBLK_CUR_LP(image,ch,tx,mx,1);
    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[4] = MACROBLK_CUR_LP(image,ch,tx,mx,3);
    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[5] = MACROBLK_CUR_LP(image,ch,tx,mx,4);
    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[6] = MACROBLK_CUR_LP(image,ch,tx,mx,5);
}

static void predict_lp420(jxr_image_t image, int tx, int mx, int /*my*/, int ch, int mblp_mode)
{
    switch (mblp_mode) {
        case 0: /* left */
            CHECK1(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,1));
            MACROBLK_CUR_LP(image,ch,tx,mx,1) += MACROBLK_CUR(image,ch,tx,mx-1).pred_dclp[2];
            break;
        case 1: /* up */
            CHECK1(image->lwf_test, MACROBLK_CUR_LP(image,ch,tx,mx,0));
            MACROBLK_CUR_LP(image,ch,tx,mx,0) += MACROBLK_UP1(image,ch,tx,mx).pred_dclp[1];
            break;
    }

#if defined(DETAILED_DEBUG)
    {
        int jdx;
        DEBUG(" DC/LP (strip=%3d, tx=%d, mbx=%4d, ch=%d) Predicted:", tx, my, mx, ch);
        DEBUG(" 0x%08x", MACROBLK_CUR_DC(image,ch,tx,mx));
        for (jdx = 0; jdx < 3 ; jdx += 1) {
            DEBUG(" 0x%08x", MACROBLK_CUR_LP(image,ch,tx,mx,jdx));
        }
        DEBUG("\n");
    }
#endif

    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[1] = MACROBLK_CUR_LP(image,ch,tx,mx,0);
    MACROBLK_CUR(image,ch,tx,mx).pred_dclp[2] = MACROBLK_CUR_LP(image,ch,tx,mx,1);
}

/*
* TRANSFORM functions, forward and inverse.
*/
static void _InvPermute(int*coeff)
{
    static const int inverse[16] = {0, 8, 4, 13,
        2, 15, 3, 14,
        1, 12, 5, 9,
        7, 11, 6, 10};
    int t[16];
    int idx;

    for (idx = 0 ; idx < 16 ; idx += 1)
        t[inverse[idx]] = coeff[idx];
    for (idx = 0 ; idx < 16 ; idx += 1)
        coeff[idx] = t[idx];
}

static void _FwdPermute(int*coeff)
{
    static const int fwd[16] = {0, 8, 4, 6,
        2, 10, 14, 12,
        1, 11, 15, 13,
        9, 3, 7, 5};

    int t[16];
    int idx;
    for (idx = 0 ; idx < 16 ; idx += 1)
        t[fwd[idx]] = coeff[idx];
    for (idx = 0 ; idx < 16 ; idx += 1)
        coeff[idx] = t[idx];
}

static void _2x2T_h(int*a, int*b, int*c, int*d, int R_flag)
{
    *a += *d;
    *b -= *c;

    int t1 = ((*a - *b + R_flag) >> 1);
    int t2 = *c;

    *c = t1 - *d;
    *d = t1 - t2;
    CHECK5(long_word_flag, *a, *b, t1, *c, *d);
    *a -= *d;
    *b += *c;
    CHECK2(long_word_flag, *a, *b);
}

static void _2x2T_h_POST(int*a, int*b, int*c, int*d)
{
    int t1;
    *b -= *c;
    *a += (*d * 3 + 4) >> 3;
    *d -= *b >> 1;
    t1 = ((*a - *b) >> 1) - *c;
    CHECK4(long_word_flag, *b, *a, *d, t1);
    *c = *d;
    *d = t1;
    *a -= *d;
    *b += *c;
    CHECK2(long_word_flag, *a, *b);
}

static void _2x2T_h_Enc(int*a, int*b, int*c, int*d)
{
    *a += *d;
    *b -= *c;
    CHECK2(long_word_flag, *a, *b);
    int t1 = *d;
    int t2 = *c;
    *c = ((*a - *b) >> 1) - t1;
    *d = t2 + (*b >> 1);
    *b += *c;
    *a -= (*d * 3 + 4) >> 3;
    CHECK4(long_word_flag, *c, *d, *b, *a);
}

static void _InvT_odd(int*a, int*b, int*c, int*d)
{
    *b += *d;
    *a -= *c;
    *d -= *b >> 1;
    *c += (*a + 1) >> 1;
    CHECK4(long_word_flag, *a, *b, *c, *d);

    *a -= ((*b)*3 + 4) >> 3;
    *b += ((*a)*3 + 4) >> 3;
    *c -= ((*d)*3 + 4) >> 3;
    *d += ((*c)*3 + 4) >> 3;
    CHECK4(long_word_flag, *a, *b, *c, *d);

    *c -= (*b + 1) >> 1;
    *d = ((*a + 1) >> 1) - *d;
    *b += *c;
    *a -= *d;
    CHECK4(long_word_flag, *a, *b, *c, *d);
}

static void _InvT_odd_odd(int*a, int*b, int*c, int*d)
{
    int t1, t2;
    *d += *a;
    *c -= *b;
    t1 = *d >> 1;
    t2 = *c >> 1;
    *a -= t1;
    *b += t2;
    CHECK4(long_word_flag, *a, *b, *c, *d);

    *a -= ((*b)*3 + 3) >> 3;
    *b += ((*a)*3 + 3) >> 2;
    CHECK2(long_word_flag, *a, *b);
    *a -= ((*b)*3 + 4) >> 3;

    *b -= t2;
    CHECK2(long_word_flag, *a, *b);
    *a += t1;
    *c += *b;
    *d -= *a;

    *b = -*b;
    *c = -*c;
    CHECK4(long_word_flag, *a, *b, *c, *d);
}

static void _InvT_odd_odd_POST(int*a, int*b, int*c, int*d)
{
    int t1, t2;

    *d += *a;
    *c -= *b;
    t1 = *d >> 1;
    t2 = *c >> 1;
    *a -= t1;
    *b += t2;
    CHECK4(long_word_flag, *d, *c, *a, *b);

    *a -= (*b * 3 + 6) >> 3;
    *b += (*a * 3 + 2) >> 2;
    CHECK2(long_word_flag, *a, *b);
    *a -= (*b * 3 + 4) >> 3;

    *b -= t2;
    CHECK2(long_word_flag, *a, *b);
    *a += t1;
    *c += *b;
    *d -= *a;
    CHECK3(long_word_flag, *a, *c, *d);
}

static void _T_odd(int*a, int*b, int*c, int*d)
{
    *b -= *c;
    *a += *d;
    *c += (*b + 1) >> 1;
    *d = ((*a + 1) >> 1) - *d;
    CHECK4(long_word_flag, *b, *a, *c, *d);

    *b -= (*a * 3 + 4) >> 3;
    *a += (*b * 3 + 4) >> 3;
    *d -= (*c * 3 + 4) >> 3;
    *c += (*d * 3 + 4) >> 3;
    CHECK4(long_word_flag, *b, *a, *d, *c);

    *d += *b >> 1;
    *c -= (*a + 1) >> 1;
    *b -= *d;
    *a += *c;
    CHECK4(long_word_flag, *d, *c, *b, *a);
}

static void _T_odd_odd(int*a, int*b, int*c, int*d)
{
    *b = -*b;
    *c = -*c;
    CHECK2(long_word_flag, *b, *c);

    *d += *a;
    *c -= *b;
    int t1 = *d >> 1;
    int t2 = *c >> 1;
    *a -= t1;
    *b += t2;
    CHECK4(long_word_flag, *d, *c, *a, *b);

    *a += (*b * 3 + 4) >> 3;
    *b -= (*a * 3 + 3) >> 2;
    CHECK2(long_word_flag, *a, *b);
    *a += (*b * 3 + 3) >> 3;

    *b -= t2;
    CHECK2(long_word_flag, *a, *b);
    *a += t1;
    *c += *b;
    *d -= *a;
    CHECK3(long_word_flag, *a, *c, *d);
}

void _jxr_InvPermute2pt(int*a, int*b)
{
    int t1 = *a;
    *a = *b;
    *b = t1;
}

void _jxr_2ptT(int*a, int*b)
{
    *a -= (*b + 1) >> 1;
    *b += *a;
    CHECK2(long_word_flag, *a, *b);
}

/* This is the inverse of the 2ptT function */
void _jxr_2ptFwdT(int*a, int*b)
{
    *b -= *a;
    *a += (*b + 1) >> 1;
    CHECK2(long_word_flag, *b, *a);
}

void _jxr_2x2IPCT(int*coeff)
{
    _2x2T_h(coeff+0, coeff+1, coeff+2, coeff+3, 0);
    /* _2x2T_h(coeff+0, coeff+2, coeff+1, coeff+3, 0); */
}

void _jxr_4x4IPCT(int*coeff)
{
    /* Permute */
    _InvPermute(coeff);

#if defined(DETAILED_DEBUG) && 0
    {
        int idx;
        DEBUG(" InvPermute:\n%*s", 4, "");
        for (idx = 0 ; idx < 16 ; idx += 1) {
            DEBUG(" 0x%08x", coeff[idx]);
            if (idx%4 == 3 && idx != 15)
                DEBUG("\n%*s", 4, "");
        }
        DEBUG("\n");
    }
#endif
    _2x2T_h (coeff+0, coeff+ 1, coeff+4, coeff+ 5, 1);
    _InvT_odd(coeff+2, coeff+ 3, coeff+6, coeff+ 7);
    _InvT_odd(coeff+8, coeff+12, coeff+9, coeff+13);
    _InvT_odd_odd(coeff+10, coeff+11, coeff+14, coeff+15);
#if defined(DETAILED_DEBUG) && 0
    {
        int idx;
        DEBUG(" stage 1:\n%*s", 4, "");
        for (idx = 0 ; idx < 16 ; idx += 1) {
            DEBUG(" 0x%08x", coeff[idx]);
            if (idx%4 == 3 && idx != 15)
                DEBUG("\n%*s", 4, "");
        }
        DEBUG("\n");
    }
#endif

    _2x2T_h(coeff+0, coeff+3, coeff+12, coeff+15, 0);
    _2x2T_h(coeff+5, coeff+6, coeff+ 9, coeff+10, 0);
    _2x2T_h(coeff+1, coeff+2, coeff+13, coeff+14, 0);
    _2x2T_h(coeff+4, coeff+7, coeff+ 8, coeff+11, 0);
}

void _jxr_4x4PCT(int*coeff)
{
    _2x2T_h(coeff+0, coeff+3, coeff+12, coeff+15, 0);
    _2x2T_h(coeff+5, coeff+6, coeff+ 9, coeff+10, 0);
    _2x2T_h(coeff+1, coeff+2, coeff+13, coeff+14, 0);
    _2x2T_h(coeff+4, coeff+7, coeff+ 8, coeff+11, 0);

    _2x2T_h (coeff+0, coeff+ 1, coeff+4, coeff+ 5, 1);
    _T_odd(coeff+2, coeff+ 3, coeff+6, coeff+ 7);
    _T_odd(coeff+8, coeff+12, coeff+9, coeff+13);
    _T_odd_odd(coeff+10, coeff+11, coeff+14, coeff+15);

    _FwdPermute(coeff);
}

static void _InvRotate(int*a, int*b)
{
    *a -= (*b + 1) >> 1;
    *b += (*a + 1) >> 1;
    CHECK2(long_word_flag, *a, *b);
}

static void _InvScale(int*a, int*b)
{
    *a += *b;
    *b = (*a >> 1) - *b;
    CHECK2(long_word_flag, *a, *b);
    *a += (*b * 3 + 0) >> 3;
    *b -= *a >> 10;
    CHECK2(long_word_flag, *a, *b);
    *b += *a >> 7;
    *b += (*a * 3 + 0) >> 4;
    CHECK1(long_word_flag, *b);
}

void _jxr_4x4OverlapFilter(int*a, int*b, int*c, int*d,
                           int*e, int*f, int*g, int*h,
                           int*i, int*j, int*k, int*l,
                           int*m, int*n, int*o, int*p)
{
    _2x2T_h(a, d, m, p, 0);
    _2x2T_h(b, c, n, o, 0);
    _2x2T_h(e, h, i, l, 0);
    _2x2T_h(f, g, j, k, 0);

    _InvRotate(n, m);
    _InvRotate(j, i);
    _InvRotate(h, d);
    _InvRotate(g, c);
    _InvT_odd_odd_POST(k, l, o, p);

    _InvScale(a, p);
    _InvScale(b, o);
    _InvScale(e, l);
    _InvScale(f, k);

    _2x2T_h_POST(a, d, m, p);
    _2x2T_h_POST(b, c, n, o);
    _2x2T_h_POST(e, h, i, l);
    _2x2T_h_POST(f, g, j, k);
}

void _jxr_4OverlapFilter(int*a, int*b, int*c, int*d)
{
    *a += *d;
    *b += *c;
    *d -= ((*a + 1) >> 1);
    *c -= ((*b + 1) >> 1);
    CHECK4(long_word_flag, *a, *b, *d, *c);
    _InvScale(a, d);
    _InvScale(b, c);
    *a += ((*d * 3 + 4) >> 3);
    *b += ((*c * 3 + 4) >> 3);
    *d -= (*a >> 1);
    *c -= (*b >> 1);
    CHECK4(long_word_flag, *a, *b, *d, *c);
    *a += *d;
    *b += *c;
    *d *= -1;
    *c *= -1;
    CHECK4(long_word_flag, *a, *b, *d, *c);
    _InvRotate(c, d);
    *d += ((*a + 1) >> 1);
    *c += ((*b + 1) >> 1);
    *a -= *d;
    *b -= *c;
    CHECK4(long_word_flag, *a, *b, *d, *c);
}

void _jxr_2x2OverlapFilter(int*a, int*b, int*c, int*d)
{
    *a += *d;
    *b += *c;
    *d -= (*a + 1) >> 1;
    *c -= (*b + 1) >> 1;
    CHECK4(long_word_flag, *a, *b, *d, *c);
    *b += (*a + 2) >> 2;
    *a += (*b + 1) >> 1;

    *a += (*b >> 5);
    *a += (*b >> 9);
    *a += (*b >> 13);
    CHECK2(long_word_flag, *a, *b);

    *b += (*a + 2) >> 2;

    *d += (*a + 1) >> 1;
    *c += (*b + 1) >> 1;
    *a -= *d;
    CHECK4(long_word_flag, *a, *b, *d, *c);
    *b -= *c;
    CHECK1(long_word_flag, *b);
}

void _jxr_2OverlapFilter(int*a, int*b)
{
    *b += ((*a + 2) >> 2);
    *a += ((*b + 1) >> 1);
    *a += (*b >> 5);
    *a += (*b >> 9);
    CHECK2(long_word_flag, *a, *b);
    *a += (*b >> 13);
    *b += ((*a + 2) >> 2);
    CHECK2(long_word_flag, *a, *b);
}

/* Prefiltering... */

static void fwdT_Odd_Odd_PRE(int*a, int*b, int*c, int*d)
{
    *d += *a;
    *c -= *b;
    int t1 = *d >> 1;
    int t2 = *c >> 1;
    *a -= t1;
    *b += t2;
    CHECK4(long_word_flag, *d, *c, *a, *b);
    *a += (*b * 3 + 4) >> 3;
    *b -= (*a * 3 + 2) >> 2;
    CHECK2(long_word_flag, *a, *b);
    *a += (*b * 3 + 6) >> 3;
    *b -= t2;
    CHECK2(long_word_flag, *a, *b);
    *a += t1;
    *c += *b;
    *d -= *a;
    CHECK3(long_word_flag, *a, *c, *d);
}

static void fwdScale(int*a, int*b)
{
    *b -= (*a * 3 + 0) >> 4;
    CHECK1(long_word_flag, *b);
    *b -= *a >> 7;
    CHECK1(long_word_flag, *b);
    *b += *a >> 10;
    *a -= (*b * 3 + 0) >> 3;
    CHECK2(long_word_flag, *b, *a);
    *b = (*a >> 1) - *b;
    *a -= *b;
    CHECK2(long_word_flag, *b, *a);
}

static void fwdRotate(int*a, int*b)
{
    *b -= (*a + 1) >> 1;
    *a += (*b + 1) >> 1;
    CHECK2(long_word_flag, *b, *a);
}

void _jxr_4x4PreFilter(int*a, int*b, int*c, int*d,
                       int*e, int*f, int*g, int*h,
                       int*i, int*j, int*k, int*l,
                       int*m, int*n, int*o, int*p)
{
    _2x2T_h_Enc(a, d, m, p);
    _2x2T_h_Enc(b, c, n, o);
    _2x2T_h_Enc(e, h, i, l);
    _2x2T_h_Enc(f, g, j, k);

    fwdScale(a, p);
    fwdScale(b, o);
    fwdScale(e, l);
    fwdScale(f, k);

    fwdRotate(n, m);
    fwdRotate(j, i);
    fwdRotate(h, d);
    fwdRotate(g, c);
    fwdT_Odd_Odd_PRE(k, l, o, p);

    _2x2T_h(a, m, d, p, 0);
    _2x2T_h(b, c, n, o, 0);
    _2x2T_h(e, h, i, l, 0);
    _2x2T_h(f, g, j, k, 0);
}

void _jxr_4PreFilter(int*a, int*b, int*c, int*d)
{
    *a += *d;
    *b += *c;
    *d -= ((*a + 1) >> 1);
    *c -= ((*b + 1) >> 1);
    CHECK4(long_word_flag, *a, *b, *d, *c);
    fwdRotate(c, d);
    *d *= -1;
    *c *= -1;
    *a -= *d;
    *b -= *c;
    CHECK4(long_word_flag, *d, *c, *a, *b);
    *d += (*a >> 1);
    *c += (*b >> 1);
    *a -= ((*d * 3 + 4) >> 3);
    *b -= ((*c * 3 + 4) >> 3);
    CHECK4(long_word_flag, *d, *c, *a, *b);
    fwdScale(a, d);
    fwdScale(b, c);
    *d += ((*a + 1) >> 1);
    *c += ((*b + 1) >> 1);
    *a -= *d;
    *b -= *c;
    CHECK4(long_word_flag, *d, *c, *a, *b);
}

void _jxr_2x2PreFilter(int*a, int*b, int*c, int*d)
{
    *a += *d;
    *b += *c;
    *d -= ((*a + 1) >> 1);
    *c -= ((*b + 1) >> 1);
    CHECK4(long_word_flag, *a, *b, *d, *c);
    *b -= ((*a + 2) >> 2);
    *a -= (*b >> 5);
    CHECK2(long_word_flag, *b, *a);
    *a -= (*b >> 9);
    CHECK1(long_word_flag, *a);
    *a -= (*b >> 13);
    CHECK1(long_word_flag, *a);
    *a -= ((*b + 1) >> 1);
    *b -= ((*a + 2) >> 2);
    *d += ((*a + 1) >> 1);
    *c += ((*b + 1) >> 1);
    CHECK4(long_word_flag, *a, *b, *d, *c);
    *a -= *d;
    *b -= *c;
    CHECK2(long_word_flag, *a, *b);
}

void _jxr_2PreFilter(int*a, int*b)
{
    *b -= ((*a + 2) >> 2);
    *a -= (*b >> 13);
    CHECK2(long_word_flag, *b, *a);
    *a -= (*b >> 9);
    CHECK1(long_word_flag, *a);
    *a -= (*b >> 5);
    CHECK1(long_word_flag, *a);
    *a -= ((*b + 1) >> 1);
    *b -= ((*a + 2) >> 2);
    CHECK2(long_word_flag, *a, *b);
}

uint8_t _jxr_read_lwf_test_flag()
{
    return long_word_flag;
}

/*
* $Log: algo.c,v $
*
* Revision 1.58 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.57 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.56 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.55 2008-05-13 13:47:11 thor
* Some experiments with a smarter selection for the quantization size,
* does not yet compile.
*
* Revision 1.54 2008-05-09 19:57:48 thor
* Reformatted for unix LF.
*
* Revision 1.53 2008-04-15 14:28:12 thor
* Start of the repository for the jpegxr reference software.
*
* Revision 1.52 2008/03/20 18:11:50 steve
* Clarify MBLPMode calculations.
*
* Revision 1.51 2008/03/14 00:54:08 steve
* Add second prefilter for YUV422 and YUV420 encode.
*
* Revision 1.50 2008/03/13 22:32:26 steve
* Fix CBP prediction for YUV422, mode==0.
*
* Revision 1.49 2008/03/13 17:49:31 steve
* Fix problem with YUV422 CBP prediction for UV planes
*
* Add support for YUV420 encoding.
*
* Revision 1.48 2008/03/13 00:07:22 steve
* Encode HP of YUV422
*
* Revision 1.47 2008/03/12 21:10:27 steve
* Encode LP of YUV422
*
* Revision 1.46 2008/03/11 22:12:48 steve
* Encode YUV422 through DC.
*
* Revision 1.45 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.44 2008/02/22 23:01:33 steve
* Compress macroblock HP CBP packets.
*
* Revision 1.43 2008/02/01 22:49:52 steve
* Handle compress of YUV444 color DCONLY
*
* Revision 1.42 2008/01/08 23:23:32 steve
* Minor math error in overlap code.
*
* Revision 1.41 2008/01/08 01:06:20 steve
* Add first pass overlap filtering.
*
* Revision 1.40 2008/01/04 17:07:35 steve
* API interface for setting QP values.
*
* Revision 1.39 2008/01/01 01:08:23 steve
* Compile warning.
*
* Revision 1.38 2008/01/01 01:07:25 steve
* Add missing HP prediction.
*
* Revision 1.37 2007/12/30 00:16:00 steve
* Add encoding of HP values.
*
* Revision 1.36 2007/12/17 23:02:57 steve
* Implement MB_CBP encoding.
*
* Revision 1.35 2007/12/14 17:10:39 steve
* HP CBP Prediction
*
* Revision 1.34 2007/12/12 00:36:46 steve
* Use T_odd instead of InvT_odd for PCT transform.
*
* Revision 1.33 2007/12/06 23:12:40 steve
* Stubs for LP encode operations.
*
* Revision 1.32 2007/11/26 01:47:15 steve
* Add copyright notices per MS request.
*
* Revision 1.31 2007/11/21 23:24:09 steve
* Account for tiles selecting LP_QUANT for pred math.
*
* Revision 1.30 2007/11/16 20:03:57 steve
* Store MB Quant, not qp_index.
*
* Revision 1.29 2007/11/13 03:27:23 steve
* Add Frequency mode LP support.
*
* Revision 1.28 2007/10/30 21:32:46 steve
* Support for multiple tile columns.
*
* Revision 1.27 2007/10/23 00:34:12 steve
* Level1 filtering for YUV422 and YUV420
*
* Revision 1.26 2007/10/22 21:52:37 steve
* Level2 filtering for YUV420
*
* Revision 1.25 2007/10/19 22:07:11 steve
* Prediction of YUV420 chroma planes.
*
* Revision 1.24 2007/10/19 16:20:21 steve
* Parse YUV420 HP
*
* Revision 1.23 2007/10/17 23:43:19 steve
* Add support for YUV420
*
* Revision 1.22 2007/10/04 00:30:47 steve
* Fix prediction of HP CBP for YUV422 data.
*
* Revision 1.21 2007/10/02 20:36:29 steve
* Fix YUV42X DC prediction, add YUV42X HP parsing.
*
* Revision 1.20 2007/10/01 20:39:33 steve
* Add support for YUV422 LP bands.
*
* Revision 1.19 2007/09/20 18:04:10 steve
* support render of YUV422 images.
*
* Revision 1.18 2007/09/13 23:12:34 steve
* Support color HP bands.
*
* Revision 1.17 2007/09/11 00:40:06 steve
* Fix rendering of chroma to add the missing *2.
* Fix handling of the chroma LP samples
* Parse some of the HP CBP data in chroma.
*
* Revision 1.16 2007/09/08 01:01:43 steve
* YUV444 color parses properly.
*
* Revision 1.15 2007/09/04 19:10:46 steve
* Finish level1 overlap filtering.
*
* Revision 1.14 2007/08/31 23:31:49 steve
* Initialize CBP VLC tables at the right time.
*
* Revision 1.13 2007/08/28 21:58:52 steve
* Rewrite filtering to match rewritten 4.7
*
* Revision 1.12 2007/08/14 23:39:56 steve
* Fix UpdateModelMB / Add filtering functions.
*
* Revision 1.11 2007/08/04 00:10:51 steve
* Fix subtle loss of -1 values during DC prediction.
*/

