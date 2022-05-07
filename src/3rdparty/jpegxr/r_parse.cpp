
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
#pragma comment (user,"$Id: r_parse.c,v 1.39 2008/03/24 18:06:56 steve Exp $")
#else
#ident "$Id: r_parse.c,v 1.39 2008/03/24 18:06:56 steve Exp $"
#endif

# include "jxr_priv.h"
# include <stdlib.h>
# include <memory.h>
# include <assert.h>

static int r_image_header(jxr_image_t image, struct rbitstream*str);
static int r_image_plane_header(jxr_image_t image, struct rbitstream*str, int alpha);
static int r_INDEX_TABLE(jxr_image_t image, struct rbitstream*str);
static int64_t r_PROFILE_LEVEL_INFO(jxr_image_t image, struct rbitstream*str);
static int r_TILE(jxr_image_t image, struct rbitstream*str);

static int r_HP_QP(jxr_image_t image, struct rbitstream*str);

static int32_t r_DEC_DC(jxr_image_t image, struct rbitstream*str,
                        unsigned tx, unsigned ty,
                        unsigned mx, unsigned my,
                        int model_bits, int chroma_flag, int is_dc_ch);
static uint32_t r_DECODE_ABS_LEVEL(jxr_image_t image, struct rbitstream*str,
                                   int band, int chroma_flag);
static int r_DECODE_FIRST_INDEX(jxr_image_t image, struct rbitstream*str,
                                int chroma_flag, int band);
static int r_DECODE_INDEX(jxr_image_t image, struct rbitstream*str,
                          int location, int chroma_flag, int band, int context);
static int r_DECODE_RUN(jxr_image_t image, struct rbitstream*str, int max_run);
static int r_REFINE_LP(struct rbitstream*str, int coeff, int model_bits);
static int r_REFINE_CBP(struct rbitstream*str, int cbp);
static void r_PredCBP(jxr_image_t image, int*diff_cbp,
                      unsigned tx, unsigned ty,
                      unsigned mx, unsigned my);
static int r_DECODE_BLOCK_ADAPTIVE(jxr_image_t image, struct rbitstream*str,
                                   unsigned tx, unsigned mx,
                                   int cbp_flag, int chroma_flag,
                                   int channel, int block, int mbhp_pred_mode,
                                   unsigned model_bits);
static void r_BLOCK_FLEXBITS(jxr_image_t image, struct rbitstream*str,
                             unsigned tx, unsigned ty,
                             unsigned mx, unsigned my,
                             unsigned chan, unsigned bl, unsigned model_bits);
static int r_calculate_mbhp_mode(jxr_image_t image, int tx, int mx);
static int get_is_dc_yuv(struct rbitstream*str);
static int dec_cbp_yuv_lp1(jxr_image_t image, struct rbitstream*str);
static int dec_abslevel_index(jxr_image_t image, struct rbitstream*str, int vlc_select);
static int get_num_cbp(struct rbitstream*str, struct adaptive_vlc_s*vlc);
static int get_num_blkcbp(jxr_image_t image, struct rbitstream*str, struct adaptive_vlc_s*vlc);
static int get_value_012(struct rbitstream*str);
static int get_num_ch_blk(struct rbitstream*str);



int jxr_read_image_bitstream(jxr_image_t image
#ifdef JPEGXR_ADOBE_EXT
	, const unsigned char *data, int len
#else //#ifdef JPEGXR_ADOBE_EXT
	, FILE*fd
#endif //#ifdef JPEGXR_ADOBE_EXT
)
{
    int rc;
    struct rbitstream bits
#ifdef JPEGXR_ADOBE_EXT
	(data,len)
#endif //#ifndef JPEGXR_ADOBE_EXT
    ;
    _jxr_rbitstream_initialize(&bits
#ifndef JPEGXR_ADOBE_EXT
		, fd
#endif //#ifndef JPEGXR_ADOBE_EXT
	);

    /* Image header for the image overall */
    rc = r_image_header(image, &bits);
    if (rc < 0) return rc;

    /* Image plane. */
    rc = r_image_plane_header(image, &bits, 0);
    if (rc < 0) return rc;

    /* Make image structures that need header details. */
    _jxr_make_mbstore(image, 0);

    /* If there is an alpa channel, process the image place header
    for it. */
    if (ALPHACHANNEL_FLAG(image)) {
        int ch;

        image->alpha = jxr_create_input();
        *image->alpha = *image;

        rc = r_image_plane_header(image->alpha, &bits, 1);
        if (rc < 0) return rc;

        for(ch = 0; ch < image->num_channels; ch ++)
            memset(&image->alpha->strip[ch], 0, sizeof(image->alpha->strip[ch]));

        _jxr_make_mbstore(image->alpha, 0);
        image->alpha->primary = 0;
    }

    rc = r_INDEX_TABLE(image, &bits);

#ifndef JPEGXR_ADOBE_EXT
    /* Store command line input values for later comparison */
    uint8_t input_profile = image->profile_idc;
    uint8_t input_level = image->level_idc;
#endif //#ifndef JPEGXR_ADOBE_EXT

    /* inferred value as per Appendix B */
    image->profile_idc = 111; 
    image->level_idc = 255;

    int64_t subsequent_bytes = _jxr_rbitstream_intVLW(&bits);
    DEBUG(" Subsequent bytes with %ld bytes\n", subsequent_bytes);
    if (subsequent_bytes > 0) {
        int64_t read_bytes = r_PROFILE_LEVEL_INFO(image,&bits);
        int64_t additional_bytes = subsequent_bytes - read_bytes;
        int64_t idx;
        for (idx = 0 ; idx < additional_bytes ; idx += 1) {
            _jxr_rbitstream_uint8(&bits); /* RESERVED_A_BYTE */
        }
    }

#ifndef JPEGXR_ADOBE_EXT
    assert(image->profile_idc <= input_profile);
    assert(image->level_idc <= input_level);
#endif //#ifndef JPEGXR_ADOBE_EXT

    rc = jxr_test_PROFILE_IDC(image, 1);
    rc = jxr_test_LEVEL_IDC(image, 1);

    DEBUG("MARK HERE as the tile base. bitpos=%zu\n", _jxr_rbitstream_bitpos(&bits));
    _jxr_rbitstream_mark(&bits);

    /* The image data is in a TILE element even if there is no
    tiling. No tiling just means 1 big tile. */
    rc = r_TILE(image, &bits);

    DEBUG("Consumed %zu bytes of the bitstream\n", bits.read_count);

#ifdef VERIFY_16BIT
    if(image->lwf_test == 0)
        DEBUG("Meet conditions for LONG_WORD_FLAG == 0!");
    else {
        DEBUG("Don't meet conditions for LONG_WORD_FLAG == 0!");
        if (LONG_WORD_FLAG(image) == 0)
            return JXR_EC_BADFORMAT;
    }
#endif

    return rc;
}

int jxr_test_LONG_WORD_FLAG(jxr_image_t image, int flag)
{
#ifdef VERIFY_16BIT
    if (flag == 0 && image->lwf_test != 0) {
        DEBUG("Using LONG_WORD_FLAG decoder but did not meet LONG_WORD_FLAG == 0 conditions!");
        return JXR_EC_BADFORMAT;
    }
    else 
#endif
        return 0;
    
}

#if defined(DETAILED_DEBUG)
static const char*bitdepth_names[16] = {
    "BD1WHITE1", "BD8", "BD16", "BD16S",
    "BD16F", "RESERVED5", "BD32S", "BD32F",
    "BD5", "BD10", "BD565", "RESERVED11"
    "RESERVED12", "RESERVED12", "RESERVED12","BD1BLACK1"
};

#endif

static int r_image_header(jxr_image_t image, struct rbitstream*str)
{
    const char GDI_SIG[] = "WMPHOTO\0";
    unsigned idx;

    unsigned version_info, version_sub_info;

    /* Read and test the GDI_SIGNATURE magic number */
    for (idx = 0 ; idx < 8 ; idx += 1) {
        uint8_t byte = _jxr_rbitstream_uint8(str);
        if (byte != GDI_SIG[idx]) {
            return JXR_EC_BADMAGIC;
        }
    }

    DEBUG("Got magic number.\n");
    DEBUG("START IMAGE_HEADER (bitpos=%zu)\n", _jxr_rbitstream_bitpos(str));

    /* Get the version info */
    version_info = _jxr_rbitstream_uint4(str);

    image->disableTileOverlapFlag = _jxr_rbitstream_uint1(str);
    DEBUG("  disableTileOverlapFlag: %d\n", image->disableTileOverlapFlag);

    version_sub_info = _jxr_rbitstream_uint3(str);
    DEBUG("  Version: %u.%u\n", version_info, version_sub_info);
	(void)version_info;
	(void)version_sub_info;
    /* Read some of the flags as a group. There are a bunch of
    small flag values together here, so it is economical to
    just collect them all at once. */
    image->header_flags1 = _jxr_rbitstream_uint8(str);
    image->header_flags2 = _jxr_rbitstream_uint8(str);
    image->header_flags_fmt = _jxr_rbitstream_uint8(str);

    /* check container conformance */
    if (image->container_current_separate_alpha == 0)
        assert(SOURCE_CLR_FMT(image) == image->container_color);
    assert(((image->header_flags_fmt & 0x0f) == 15 ? 0 : (image->header_flags_fmt & 0x0f)) == image->container_bpc);
    if (image->container_separate_alpha == 0)
        assert(image->container_alpha == ALPHACHANNEL_FLAG(image));
    else 
        assert(ALPHACHANNEL_FLAG(image) == 0);

    DEBUG(" Flags group1=0x%02x\n", image->header_flags1);
    DEBUG(" Flags group2=0x%02x\n", image->header_flags2);
    DEBUG(" OUTPUT_CLR_FMT=%d\n", SOURCE_CLR_FMT(image));
    DEBUG(" OUTPUT_BITDEPTH=%d (%s)\n", SOURCE_BITDEPTH(image), bitdepth_names[SOURCE_BITDEPTH(image)]);

    /* Get the configured image dimensions. */
    if (SHORT_HEADER_FLAG(image)) {
        DEBUG(" SHORT_HEADER_FLAG=true\n");
        image->width1 = _jxr_rbitstream_uint16(str);
        image->height1 = _jxr_rbitstream_uint16(str);
    } else {
        DEBUG(" SHORT_HEADER_FLAG=false\n");
        image->width1 = _jxr_rbitstream_uint32(str);
        image->height1 = _jxr_rbitstream_uint32(str);
    }

    /* check container conformance */
    assert(image->width1 + 1 == image->container_width);
    assert(image->height1 + 1 == image->container_height);

    DEBUG(" Image dimensions: %u x %u\n", image->width1+1, image->height1+1);

    assert(image->tile_row_height == 0);
    assert(image->tile_column_width == 0);
    assert(image->tile_column_position == 0);
    if (jxr_get_TILING_FLAG(image)) {
        image->tile_columns = _jxr_rbitstream_uint12(str) + 1;
        image->tile_rows = _jxr_rbitstream_uint12(str) + 1;
        DEBUG(" TILING %u columns, %u rows (bitpos=%zu)\n",
            image->tile_columns, image->tile_rows,
            _jxr_rbitstream_bitpos(str));


    } else {
        /* NO TILING means that the entire image is exactly 1
        tile. Configure the single tile to be the size of the
        entire image. */
        image->tile_columns = 1;
        image->tile_rows = 1;
        DEBUG(" NO TILING\n");
    }

    /* Collect the widths of the tile columns. All but the last
    column width are encoded in the input stream. The last is
    inferred from the accumulated width of the columns and the
    total width of the image. If there is no tiling, then there
    is exactly 1 tile, and this degenerates to the width of the
    image.

    The heights of tile rows is processed exactly the same way. */

    image->tile_column_width = (unsigned*)jpegxr_calloc(2*image->tile_columns, sizeof(unsigned));
    image->tile_column_position = image->tile_column_width + image->tile_columns;
    image->tile_row_height = (unsigned*)jpegxr_calloc(2*image->tile_rows, sizeof(unsigned));
    image->tile_row_position = image->tile_row_height + image->tile_rows;

    unsigned wid_sum = 0;
    if (SHORT_HEADER_FLAG(image)) {
        for (idx = 0 ; idx < image->tile_columns-1 ; idx += 1) {
            image->tile_column_width[idx] = _jxr_rbitstream_uint8(str);
            image->tile_column_position[idx] = wid_sum;
            wid_sum += image->tile_column_width[idx];
        }

    } else {
        for (idx = 0 ; idx < image->tile_columns-1 ; idx += 1) {
            image->tile_column_width[idx] = _jxr_rbitstream_uint16(str);
            image->tile_column_position[idx] = wid_sum;
            wid_sum += image->tile_column_width[idx];
        }
    }
    /* calculate final tile width after windowing parameters are found */

    unsigned hei_sum = 0;
    if (SHORT_HEADER_FLAG(image)) {
        for (idx = 0 ; idx < image->tile_rows-1 ; idx += 1) {
            image->tile_row_height[idx] = _jxr_rbitstream_uint8(str);
            image->tile_row_position[idx] = hei_sum;
            hei_sum += image->tile_row_height[idx];
        }

    } else {
        for (idx = 0 ; idx < image->tile_rows-1 ; idx += 1) {
            image->tile_row_height[idx] = _jxr_rbitstream_uint16(str);
            image->tile_row_position[idx] = hei_sum;
            hei_sum += image->tile_row_height[idx];
        }
    }
    /* calculate final tile height after windowing parameters are found */

    if (WINDOWING_FLAG(image)) {
        image->window_extra_top = _jxr_rbitstream_uint6(str);
        image->window_extra_left = _jxr_rbitstream_uint6(str);
        image->window_extra_bottom = _jxr_rbitstream_uint6(str);
        image->window_extra_right = _jxr_rbitstream_uint6(str);
    } else {
        image->window_extra_top = 0;
        image->window_extra_left = 0;
        if ((image->height1 + 1) % 16 == 0)
            image->window_extra_bottom = 0;
        else
            image->window_extra_bottom = 16 - ((image->height1 + 1) % 16);
        if ((image->width1 + 1) % 16 == 0)
            image->window_extra_right = 0;
        else
            image->window_extra_right = 16 - ((image->width1 + 1) % 16);
        DEBUG(" NO WINDOWING\n");
    }
    image->extended_width = image->width1 + 1 + image->window_extra_left + image->window_extra_right;
    image->extended_height = image->height1 + 1 + image->window_extra_top + image->window_extra_bottom;

    image->lwf_test = 0;

    image->tile_column_width[image->tile_columns-1] = (image->extended_width >> 4)-wid_sum;
    image->tile_column_position[image->tile_columns-1] = wid_sum;

    image->tile_row_height[image->tile_rows-1] = (image->extended_height >> 4)-hei_sum;
    image->tile_row_position[image->tile_rows-1] = hei_sum;

#if defined(DETAILED_DEBUG)
    DEBUG(" Tile widths:");
    for (idx = 0 ; idx < image->tile_columns ; idx += 1)
        DEBUG(" %u", image->tile_column_width[idx]);
    DEBUG("\n");
    DEBUG(" Tile heights:");
    for (idx = 0 ; idx < image->tile_rows ; idx += 1)
        DEBUG(" %u", image->tile_row_height[idx]);
    DEBUG("\n");
#endif

    /* Perform some checks */
    assert(image->extended_width % 16 == 0);
    if ((OVERLAP_INFO(image) >= 2) && (image->use_clr_fmt == 1 || image->use_clr_fmt == 2))
    {
        assert(image->extended_width >= 32);
        if (image->disableTileOverlapFlag) {
            unsigned int idx = 0;
            for (idx = 0; idx < image->tile_columns ; idx += 1)
                assert(image->tile_column_width[idx] > 1);
        }
    }
    assert(image->extended_height % 16 == 0);

    DEBUG("END IMAGE_HEADER (%zu bytes)\n", str->read_count);
    return 0;
}

static int r_image_plane_header(jxr_image_t image, struct rbitstream*str, int alpha)
{
#ifndef JPEGXR_ADOBE_EXT
    size_t save_count = str->read_count;
#endif //#ifndef JPEGXR_ADOBE_EXT
    DEBUG("START IMAGE_PLANE_HEADER (bitpos=%zu)\n", _jxr_rbitstream_bitpos(str));

    /* NOTE: The "use_clr_fmt" is the encoded color format, and is
    not necessarily the same as the image color format
    signaled in the image header. All of our processing of an
    image plane is handled using the "use_clr_fmt", and only
    transformed to the image color format on the way out. */

    image->use_clr_fmt = _jxr_rbitstream_uint3(str); /* INTERNAL_CLR_FMT */
    image->scaled_flag = _jxr_rbitstream_uint1(str); /* NO_SCALED_FLAG */
    image->bands_present = _jxr_rbitstream_uint4(str); /* BANDS_PRESENT */

    /* for alpha image plane, INTERNAL_CLR_FMT == YONLY */
    if (alpha)
        assert(image->use_clr_fmt == 0);

    DEBUG(" INTERNAL_CLR_FMT = %d\n", image->use_clr_fmt);
    DEBUG(" SCALED_FLAG = %s\n", image->scaled_flag? "true" : "false");
    DEBUG(" BANDS_PRESENT = %d\n", image->bands_present);

    uint16_t num_components;
    switch (image->use_clr_fmt) {
        case 0: /* YONLY */
            image->num_channels = 1;
            break;
        case 1: /* YUV420 */
            _jxr_rbitstream_uint1(str); /* RESERVED_E_BIT */
            image->chroma_centering_x = _jxr_rbitstream_uint3(str); /* CHROMA_CENTERING_X */
            _jxr_rbitstream_uint1(str); /* RESERVED_G_BIT */
            image->chroma_centering_y = _jxr_rbitstream_uint3(str); /* CHROMA_CENTERING_Y */
            image->num_channels = 3;
            break;
        case 2: /* YUV422 */
            _jxr_rbitstream_uint1(str); /* RESERVED_E_BIT */
            image->chroma_centering_x = _jxr_rbitstream_uint3(str); /* CHROMA_CENTERING_X */
            _jxr_rbitstream_uint4(str); /* RESERVED_H */
            image->chroma_centering_y = 0;
            image->num_channels = 3;
            break;
        case 3: /* YUV444 */
            _jxr_rbitstream_uint4(str); /* RESERVED_F */
            _jxr_rbitstream_uint4(str); /* RESERVED_H */
            image->num_channels = 3;
            break;
        case 4: /* YUVK */
            image->num_channels = 4;
            break;
        case 6: /* NCOMPONENT */
            num_components = _jxr_rbitstream_uint4(str);
            if (num_components == 0xf) {
                image->num_channels = 16 + _jxr_rbitstream_uint12(str);
            }
            else {
                image->num_channels = 1 + num_components;
                _jxr_rbitstream_uint4(str); /* RESERVED_H */
            }
            break;
        case 5: /* RESERVED */
        case 7: /* RESERVED */
            break;
    }

    /* 
    check container conformance - specific for tag based container
    this section should be modified when the container is
    */
    if (image->container_alpha) {
        if (image->container_separate_alpha) {
            if (image->container_current_separate_alpha) {
                assert(image->num_channels == 1);
                assert(image->bands_present == image->container_alpha_band_presence  || image->container_alpha_band_presence > 3 || image->container_alpha_band_presence < 0);
            }
            else {
                assert(image->num_channels == image->container_nc - 1);
                assert((image->bands_present == image->container_image_band_presence) || (image->container_image_band_presence > 3) || (image->container_image_band_presence < 0));
            }
        }
        else {
            if (alpha) {
                assert(image->num_channels == 1);
                assert(image->bands_present == image->container_alpha_band_presence || image->container_alpha_band_presence > 3 || image->container_alpha_band_presence < 0);
            }
            else {
                assert(image->num_channels == image->container_nc - 1);
                assert(image->bands_present == image->container_image_band_presence || image->container_image_band_presence > 3 || (image->container_image_band_presence < 0));
            }
        }
    }
    else {
        assert(image->num_channels == image->container_nc);
        assert(image->bands_present == image->container_image_band_presence || image->container_image_band_presence > 3 || (image->container_image_band_presence < 0));
    }


    switch (SOURCE_BITDEPTH(image)) {
        case 0: /* BD1WHITE1 */
        case 1: /* BD8 */
        case 4: /* BD16F */
        case 8: /* BD5 */
        case 9: /* BD10 */
        case 15: /* BD1BLACK1 */
            image->shift_bits = 0;
            break;
        case 2: /* BD16 */
        case 3: /* BD16S */
        case 6: /* BD32S */
            image->shift_bits = _jxr_rbitstream_uint8(str); /* SHIFT_BITS */
            DEBUG(" SHIFT_BITS = %u\n", image->shift_bits);
            break;
        case 7: /* BD32F */
            image->len_mantissa = _jxr_rbitstream_uint8(str); /* LEN_MANTISSA */
            image->exp_bias = _jxr_rbitstream_uint8(str); /* EXP_BIAS */
            DEBUG(" LEN_MANTISSA = %u\n", image->len_mantissa);
            DEBUG(" EXP_BIAS = %u\n", image->exp_bias);
            break;
        default: /* RESERVED */
            DEBUG(" XXXX Inexplicable SOURCE_BITDEPTH=%u\n", SOURCE_BITDEPTH(image));
            break;
    }

    /* If the stream signals that the DC frames use a uniform
    quantization parameter, then collect that parameter
    here. In this case, DC quantization parameters elsewhere in
    the image are suppressed. Note that per macroblock, there
    is only 1 DC value, so only 1 DC QP is needed. */
    image->dc_frame_uniform = _jxr_rbitstream_uint1(str);
    DEBUG(" DC_FRAME_UNIFORM = %s\n", image->dc_frame_uniform?"true":"false");
    if (image->dc_frame_uniform) {
        _jxr_r_DC_QP(image, str);
    }

    if (image->bands_present != 3 /*DCONLY*/) {
        _jxr_rbitstream_uint1(str); /* RESERVED_I_BIT */

        image->lp_frame_uniform = _jxr_rbitstream_uint1(str);
        DEBUG(" LP_FRAME_UNIFORM = %s\n", image->lp_frame_uniform?"true":"false");
        if (image->lp_frame_uniform) {
            image->num_lp_qps = 1;
            _jxr_r_LP_QP(image, str);
        }

        if (image->bands_present != 2 /*NOHIGHPASS*/) {
            _jxr_rbitstream_uint1(str); /* RESERVED_J_BIT */

            image->hp_frame_uniform = _jxr_rbitstream_uint1(str);
            DEBUG(" HP_FRAME_UNIFORM = %s\n", image->hp_frame_uniform?"true":"false");
            if (image->hp_frame_uniform) {
                image->num_hp_qps = 1;
                r_HP_QP(image, str);
            }
        }

    }

    _jxr_rbitstream_syncbyte(str);
    DEBUG("END IMAGE_PLANE_HEADER (%zd bytes, bitpos=%zu)\n",
        str->read_count - save_count, _jxr_rbitstream_bitpos(str));

    return 0;
}

static int get_ch_mode(jxr_image_t image, struct rbitstream*str)
{
    int ch_mode;
    if (image->num_channels == 1) {
        ch_mode = 0; /* UNIFORM */
    } else {
        ch_mode = _jxr_rbitstream_uint2(str);
    }
    return ch_mode;
}

int _jxr_r_DC_QP(jxr_image_t image, struct rbitstream*str)
{
    unsigned idx;

    int ch_mode = get_ch_mode(image, str);
    DEBUG(" DC_QP CH_MODE=%d ", ch_mode);

    switch (ch_mode) {
        case 0: /* UNIFORM */
            image->dc_quant_ch[0] = _jxr_rbitstream_uint8(str);
            DEBUG(" DC_QUANT UNIFORM =%u", image->dc_quant_ch[0]);
            for (idx = 1 ; idx < image->num_channels ; idx += 1)
                image->dc_quant_ch[idx] = image->dc_quant_ch[0];
            break;
        case 1: /* SEPARATE */
            image->dc_quant_ch[0] = _jxr_rbitstream_uint8(str);
            image->dc_quant_ch[1] = _jxr_rbitstream_uint8(str);
            image->dc_quant_ch[2] = image->dc_quant_ch[1];
            DEBUG(" DC_QUANT SEPARATE Y=%u, Chr=%u", image->dc_quant_ch[0],image->dc_quant_ch[1]);
            break;
        case 2: /* INDEPENDENT */
            assert(image->num_channels <= MAX_CHANNELS);
            for (idx = 0 ; idx < image->num_channels ; idx += 1) {
                image->dc_quant_ch[idx] = _jxr_rbitstream_uint8(str);
                DEBUG(" DC_QUANT INDEPENDENT[%d] = %u", idx, image->dc_quant_ch[idx]);
            }
            break;
        case 3: /* Reserved */
            break;
        default:
            assert(0);
            break;
    }
    DEBUG("\n");

    return 0;
}

int _jxr_r_LP_QP(jxr_image_t image, struct rbitstream*str)
{
    unsigned q;

    for (q = 0 ; q < image->num_lp_qps ; q += 1) {
        unsigned idx;
        int ch_mode = get_ch_mode(image, str);
        DEBUG(" LP_QP[%u] CH_MODE=%d LP_QUANT=", q, ch_mode);

        switch (ch_mode) {
            case 0: /* UNIFORM */
                image->lp_quant_ch[0][q] = _jxr_rbitstream_uint8(str);
                DEBUG("%d", image->lp_quant_ch[0][q]);
                for (idx = 1 ; idx < image->num_channels ; idx += 1)
                    image->lp_quant_ch[idx][q] = image->lp_quant_ch[0][q];
                break;
            case 1: /* SEPARATE */
                image->lp_quant_ch[0][q] = _jxr_rbitstream_uint8(str);
                image->lp_quant_ch[1][q] = _jxr_rbitstream_uint8(str);
                DEBUG("SEPARATE Y=%d Chr=%d", image->lp_quant_ch[0][q], image->lp_quant_ch[1][q]);
                for (idx = 2 ; idx < image->num_channels ; idx += 1)
                    image->lp_quant_ch[idx][q] = image->lp_quant_ch[1][q];
                break;
            case 2: /* INDEPENDENT */
                DEBUG("INDEPENDENT =");
                for (idx = 0 ; idx < image->num_channels ; idx += 1) {
                    image->lp_quant_ch[idx][q] = _jxr_rbitstream_uint8(str);
                    DEBUG(" %d", image->lp_quant_ch[idx][q]);
                }
                break;
            case 3: /* Reserved */
                break;
            default:
                assert(0);
                break;
        }
        DEBUG("\n");
    }

    return 0;
}

static int r_HP_QP(jxr_image_t image, struct rbitstream*str)
{
    unsigned q;

    for (q = 0 ; q < image->num_hp_qps ; q += 1) {
        unsigned idx;
        int ch_mode = get_ch_mode(image, str);
        DEBUG("HP_QP[%u] CH_MODE: %d ", q, ch_mode);

        switch (ch_mode) {
            case 0: /* UNIFORM */
                image->HP_QUANT_Y[q] = _jxr_rbitstream_uint8(str);
                DEBUG("UNIFORM %d", image->hp_quant_ch[0][q]);
                for (idx = 1 ; idx < image->num_channels ; idx += 1)
                    image->hp_quant_ch[idx][q] = image->hp_quant_ch[0][q];
                break;
            case 1: /* SEPARATE */
                image->HP_QUANT_Y[q] = _jxr_rbitstream_uint8(str);
                image->hp_quant_ch[1][q] = _jxr_rbitstream_uint8(str);
                DEBUG("SEPARATE Y=%d Chr=%d", image->hp_quant_ch[0][q], image->hp_quant_ch[1][q]);
                for (idx = 2 ; idx < image->num_channels ; idx += 1)
                    image->hp_quant_ch[idx][q] = image->hp_quant_ch[1][q];
                break;
            case 2: /* INDEPENDENT */
                DEBUG("INDEPENDENT =");
                for (idx = 0 ; idx < image->num_channels ; idx += 1) {
                    image->hp_quant_ch[idx][q] = _jxr_rbitstream_uint8(str);
                    DEBUG(" %d", image->hp_quant_ch[idx][q]);
                }
                break;
            case 3: /* Reserved */
                break;
            default:
                assert(0);
                break;
        }
        DEBUG(" bitpos=%zu\n", _jxr_rbitstream_bitpos(str));
    }

    return 0;
}

static int r_INDEX_TABLE(jxr_image_t image, struct rbitstream*str)
{
    DEBUG("INDEX_TABLE START bitpos=%zu\n", _jxr_rbitstream_bitpos(str));
    if (INDEXTABLE_PRESENT_FLAG(image)) {
        uint8_t s0 = _jxr_rbitstream_uint8(str);
        uint8_t s1 = _jxr_rbitstream_uint8(str);
        DEBUG(" STARTCODE = 0x%02x 0x%02x\n", s0, s1);
        if (s0 != 0x00 || s1 != 0x01)
            return JXR_EC_ERROR;

        int num_index_table_entries;
        int idx;

        if (FREQUENCY_MODE_CODESTREAM_FLAG(image) == 0 /* SPATIALMODE */) {
            num_index_table_entries = image->tile_rows * image->tile_columns;

        } else {
            num_index_table_entries = image->tile_rows * image->tile_columns;
            switch (image->bands_present) {
                case 4: /* ISOLATED */
                    num_index_table_entries *= 4;
                    break;
                default:
                    num_index_table_entries *= 4 - image->bands_present;
                    break;
            }
        }
        image->tile_index_table_length = num_index_table_entries;

        assert(image->tile_index_table == 0);
        image->tile_index_table = (int64_t*)jpegxr_calloc(num_index_table_entries, sizeof(int64_t));
        DEBUG(" INDEX_TABLE has %d table entries\n", num_index_table_entries);

        for (idx = 0 ; idx < num_index_table_entries ; idx += 1) {
            int64_t off = _jxr_rbitstream_intVLW(str);
            DEBUG(" ... %ld\n", off);
            image->tile_index_table[idx] = off;
        }
    }

    DEBUG("INTEX_TABLE DONE bitpos=%zu\n", _jxr_rbitstream_bitpos(str));
    return 0;
}

static int64_t r_PROFILE_LEVEL_INFO(jxr_image_t image, struct rbitstream*str) 
{
    int64_t num_bytes = 0;
    uint16_t reserved_l;
    unsigned last_flag;

    int64_t last;
    for (last = 0 ; last == 0 ; last = last_flag) {
        image->profile_idc = _jxr_rbitstream_uint8(str); /* PROFILE_IDC */
        DEBUG(" Profile signaled in file %ld bytes\n", image->profile_idc);
        image->level_idc = _jxr_rbitstream_uint8(str); /* LEVEL_IDC */
        DEBUG(" Level signaled in file %ld bytes\n", image->level_idc);
        reserved_l = _jxr_rbitstream_uint15(str); /* RESERVED_L */
        last_flag = _jxr_rbitstream_uint1(str); /* LAST_FLAG */
        num_bytes += 4;
    }
	(void)reserved_l;

    return num_bytes;
}

static int r_TILE(jxr_image_t image, struct rbitstream*str)
{
int rc = 0;
    image->tile_quant = (struct jxr_tile_qp *) jpegxr_calloc(image->tile_columns*image->tile_rows, sizeof(*(image->tile_quant)));
    assert(image->tile_quant);

    if (FREQUENCY_MODE_CODESTREAM_FLAG(image) == 0 /* SPATIALMODE */) {

        unsigned tx, ty, tt=0;
        for (ty = 0 ; ty < image->tile_rows ; ty += 1) {
            for (tx = 0 ; tx < image->tile_columns ; tx += 1) {
                if(INDEXTABLE_PRESENT_FLAG(image))
                {
                    _jxr_rbitstream_seek(str, image->tile_index_table[tt]);
                    tt++;
                }
                rc = _jxr_r_TILE_SPATIAL(image, str, tx, ty);
                if (rc < 0) goto RET;
            }
        }
    } else { /* FREQUENCYMODE */

        int num_bands = 0;
        switch (image->bands_present) {
            case 0: /* ALL */
                num_bands = 4;
                break;
            case 1: /* NOFLEXBITS */
                num_bands = 3;
                break;
            case 2: /* NOHIGHPASS */
                num_bands = 2;
                break;
            case 3: /* DCONLY */
                num_bands = 1;
                break;
            case 4: /* ISOLATED */
                break;
        }

        unsigned tx, ty, tt;
        for (ty = 0, tt=0 ; ty < image->tile_rows ; ty += 1) {
            for (tx = 0 ; tx < image->tile_columns ; tx += 1) {
                _jxr_rbitstream_seek(str, image->tile_index_table[tt*num_bands+0]);
                rc = _jxr_r_TILE_DC(image, str, tx, ty);
                if (rc < 0) goto RET;
                tt += 1;
            }
        }

        if (num_bands > 1) {
            for (ty = 0, tt=0 ; ty < image->tile_rows ; ty += 1) {
                for (tx = 0 ; tx < image->tile_columns ; tx += 1) {
                    _jxr_rbitstream_seek(str, image->tile_index_table[tt*num_bands+1]);
                    rc = _jxr_r_TILE_LP(image, str, tx, ty);
                    if (rc < 0) goto RET;
                    tt += 1;
                }
            }
        }

        if (num_bands > 2) {
            for (ty = 0, tt=0 ; ty < image->tile_rows ; ty += 1) {
                for (tx = 0 ; tx < image->tile_columns ; tx += 1) {
                    _jxr_rbitstream_seek(str, image->tile_index_table[tt*num_bands+2]);
                    rc = _jxr_r_TILE_HP(image, str, tx, ty);
                    if (rc < 0) goto RET;
                    tt += 1;
                }
            }
        }

        if (num_bands > 3) {
            for (ty = 0, tt=0 ; ty < image->tile_rows ; ty += 1) {
                for (tx = 0 ; tx < image->tile_columns ; tx += 1) {
                    int64_t off = image->tile_index_table[tt*num_bands+3];
                    if (off >= 0) {
                        _jxr_rbitstream_seek(str, off);
                        rc = _jxr_r_TILE_FLEXBITS(image, str, tx, ty);
                        if (rc < 0) goto RET;
                    } else {
                        _jxr_r_TILE_FLEXBITS_ESCAPE(image, tx, ty);
                    }
                    tt += 1;
                }
            }
        }

        _jxr_frequency_mode_render(image);
    }

RET:
    jpegxr_free(image->tile_quant);
    return rc;
}

void _jxr_r_TILE_HEADER_DC(jxr_image_t image, struct rbitstream*str,
                           int /*alpha_flag*/, unsigned tx, unsigned ty)
{
    DEBUG(" TILE_HEADER_DC START bitpos=%zu\n", _jxr_rbitstream_bitpos(str));
    if (image->dc_frame_uniform == 0) {
        DEBUG(" TILE_HEADER_DC: parse non-uniform DC_QP\n");
        _jxr_r_DC_QP(image, str);
        memcpy(image->tile_quant[ty*(image->tile_columns) + tx ].dc_quant_ch, image->dc_quant_ch, MAX_CHANNELS);
    }
}

void _jxr_r_TILE_HEADER_LOWPASS(jxr_image_t image, struct rbitstream*str,
                                int /*alpha_flag*/,
                                unsigned tx, unsigned ty)
{
    DEBUG(" TILE_HEADER_LOWPASS START bitpos=%zu\n", _jxr_rbitstream_bitpos(str));
    if (image->lp_frame_uniform == 0) {
        image->lp_use_dc_qp = _jxr_rbitstream_uint1(str);
        DEBUG(" TILE_HEADER_LP: parse non-uniform LP_QP: USE_DC_QP=%u\n",
            image->lp_use_dc_qp);
        if (image->lp_use_dc_qp == 0) {
            image->num_lp_qps = _jxr_rbitstream_uint4(str) + 1;
            DEBUG(" TILE_HEADER_LP: NUM_LP_QPS = %d\n", image->num_lp_qps);
            _jxr_r_LP_QP(image, str);
            memcpy(image->tile_quant[ty*(image->tile_columns) + tx].lp_quant_ch, image->lp_quant_ch, MAX_CHANNELS*MAX_LP_QPS);
        }
        else
        {
            /* Use the same quantization index as the dc band (the dc quantization step size could be different for each tile, so store it */
            int ch;
            for(ch = 0; ch < image->num_channels; ch++)
                image->tile_quant[ty*(image->tile_columns) + tx].lp_quant_ch[ch][0] = image->dc_quant_ch[ch];
        }
    }
}


void _jxr_r_TILE_HEADER_HIGHPASS(jxr_image_t image, struct rbitstream*str,
                                 int /*alpha_flag*/,
                                 unsigned tx, unsigned ty)
{
    if (image->hp_frame_uniform == 0) {
        image->hp_use_lp_qp = _jxr_rbitstream_uint1(str);
        DEBUG(" TILE_HEADER_HP: parse non-uniform HP_QP: USE_LP_QP=%u\n",
            image->hp_use_lp_qp);

        if (image->hp_use_lp_qp == 0) {
            image->num_hp_qps = _jxr_rbitstream_uint4(str) + 1;
            DEBUG(" TILE_HEADER_HIGHPASS: NUM_HP_QPS = %d\n", image->num_hp_qps);
            r_HP_QP(image, str);
            memcpy(image->tile_quant[ty*(image->tile_columns) + tx].hp_quant_ch, image->lp_quant_ch, MAX_CHANNELS*MAX_HP_QPS);
        }
        else
        {
            /* Use the same quantization index as the lp band (the lp quantization step size could be different for each tile, so store it */
            int ch;
            image->num_hp_qps = image->num_lp_qps;
            for(ch = 0; ch < image->num_channels; ch++) {
                memcpy(image->hp_quant_ch[ch], image->lp_quant_ch[ch], MAX_LP_QPS);
                memcpy(image->tile_quant[ty*(image->tile_columns) + tx].hp_quant_ch[ch], image->lp_quant_ch[ch], MAX_LP_QPS);
            }
        }
    }
}

unsigned _jxr_DECODE_QP_INDEX(struct rbitstream*str, unsigned index_count)
{
    static const int bits_per_qp_index[] = {0,0,1,1,2,2,3,3, 3,3,4,4,4,4,4,4,4};
    assert(index_count <= 16);

    int nonzero_flag = _jxr_rbitstream_uint1(str);

    if (nonzero_flag == 0)
        return 0;

    int bits_count = bits_per_qp_index[index_count];
    /* DECODE_QP_INDEX is onny called if the index count is
    greater then 1. Therefore, the bits_count here must be more
    then zero. */
    assert(bits_count > 0);

    return _jxr_rbitstream_uintN(str, bits_count)+1;
}

/*
* Decode the single DC component for the macroblock.
*/
void _jxr_r_MB_DC(jxr_image_t image, struct rbitstream*str,
                  int /*alpha_flag*/,
                  unsigned tx, unsigned ty,
                  unsigned mx, unsigned my)
{
    int lap_mean[2];
    lap_mean[0] = 0;
    lap_mean[1] = 0;

    DEBUG(" MB_DC tile=[%u %u] mb=[%u %u] bitpos=%zu\n",
        tx, ty, mx, my, _jxr_rbitstream_bitpos(str));

    if (_jxr_InitContext(image, tx, ty, mx, my)) {
        DEBUG(" MB_DC: Initialize Context\n");
        _jxr_InitVLCTable(image, AbsLevelIndDCLum);
        _jxr_InitVLCTable(image, AbsLevelIndDCChr);
        _jxr_InitializeModelMB(&image->model_dc, 0/*DC*/);
    }

    if (image->use_clr_fmt==0 || image->use_clr_fmt==4 || image->use_clr_fmt==6) {
        /* clr_fmt == YONLY, YUVK or NCOMPONENT */
        unsigned idx;
        for (idx = 0 ; idx < image->num_channels ; idx += 1) {
            int m = (idx == 0)? 0 : 1;
            int model_bits = image->model_dc.bits[m];
            unsigned is_dc_ch = _jxr_rbitstream_uint1(str);
            DEBUG(" MB_DC: IS_DC_CH=%u, model_bits=%d\n",
                is_dc_ch, model_bits);
            if (is_dc_ch) {
                lap_mean[m] += 1;
            }
            uint32_t dc_val = r_DEC_DC(image, str, tx, ty, mx, my,
                model_bits, 0/*chroma_flag==FALSE*/,
                is_dc_ch);

            MACROBLK_CUR_DC(image,idx,tx, mx) = dc_val;
            DEBUG(" dc_val at t=[%u %u], m=[%u %u] == %d (0x%08x)\n",
                tx, ty, mx, my, (int32_t)dc_val, dc_val);
        }
    } else {
        assert(image->num_channels == 3);
        int is_dc_yuv = get_is_dc_yuv(str);
        int model_bits_y = image->model_dc.bits[0];
        int model_bits_uv = image->model_dc.bits[1];
        DEBUG(" MB_DC: IS_DC_YUV=0x%x, model_bits[0]=%d, model_bits[1]=%d\n",
            is_dc_yuv, model_bits_y, model_bits_uv);

        if (is_dc_yuv&4)
            lap_mean[0] += 1;
        uint32_t dc_val_y = r_DEC_DC(image, str, tx, ty, mx, my,
            model_bits_y, 0/*chroma_flag==FALSE*/,
            is_dc_yuv&4);

        if (is_dc_yuv&2)
            lap_mean[1] += 1;
        uint32_t dc_val_u = r_DEC_DC(image, str, tx, ty, mx, my,
            model_bits_uv, 1/*chroma_flag==TRUE*/,
            is_dc_yuv&2);

        if (is_dc_yuv&1)
            lap_mean[1] += 1;
        uint32_t dc_val_v = r_DEC_DC(image, str, tx, ty, mx, my,
            model_bits_uv, 1/*chroma_flag==TRUE*/,
            is_dc_yuv&1);

        MACROBLK_CUR_DC(image,0,tx, mx) = dc_val_y;
        MACROBLK_CUR_DC(image,1,tx, mx) = dc_val_u;
        MACROBLK_CUR_DC(image,2,tx, mx) = dc_val_v;
        DEBUG(" dc_val at t=[%u %u], m=[%u %u] == %d (0x%08x), %d (0x%08x), %d (0x%08x)\n",
            tx, ty, mx, my, (int)dc_val_y, dc_val_y, (int)dc_val_u, dc_val_u, (int)dc_val_v, dc_val_v);
    }

    /* */
    DEBUG(" MB_DC: UpdateModelMB: lap_mean={%u %u}\n", lap_mean[0], lap_mean[1]);
    _jxr_UpdateModelMB(image, lap_mean, &image->model_dc, 0/*DC*/);
    if (_jxr_ResetContext(image, tx, mx)) {
        DEBUG(" MB_DC: Reset Context\n");
        /* AdaptDC */
        _jxr_AdaptVLCTable(image, AbsLevelIndDCLum);
        _jxr_AdaptVLCTable(image, AbsLevelIndDCChr);
    }
    DEBUG(" MB_DC DONE tile=[%u %u] mb=[%u %u]\n", tx, ty, mx, my);
}

/*
* When the LP value is input from the stream, it is delivered into
* the target array based on a scan order. The "lopass_scanorder"
* array maps the list of LP values (actually the position in the
* list) to the location in the scan. Thus the scan order places the
* value into the lpinput array.
*
* A property of the lpinput is that it is sparse. The adpative scan
* order tries to adapt the scan order so that the most frequent value
* is pressed to the beginning of the input stream. It does this by
* counting the arrival of each value, and bubbling frequent values
* forward.
*
* Note in the code below that the "i" value ranges from 1-16 but the
* tables are numbered from 0-15. Thus "i-1" is used to index tables.
*
* Note that the scanorder is adapted while we go, but the only
* adjustment is to swap the current position with the previous. Thus,
* it is not possible to effect the current pass with the adaptation.
*/
static void AdaptiveLPScan(jxr_image_t image, int lpinput_n[], int i, int value)
{
    assert(i > 0);
    int k = image->lopass_scanorder[i-1];
    lpinput_n[k] = value;
    image->lopass_scantotals[i-1] += 1;
    if (i>1 && image->lopass_scantotals[i-1] > image->lopass_scantotals[i-2]) {
        SWAP(image->lopass_scantotals[i-1], image->lopass_scantotals[i-2]);
        SWAP(image->lopass_scanorder[i-1], image->lopass_scanorder[i-2]);
    }
}

void _jxr_r_MB_LP(jxr_image_t image, struct rbitstream*str,
                  int /*alpha_flag*/,
                  unsigned tx, unsigned ty,
                  unsigned mx, unsigned my)
{
    static const int transpose420[4] = {0, 2,
        1, 3 };
    static const int transpose422[8] = {0, 2, 1, 3, 4, 6, 5, 7};
    int LPInput[8][16];
    int idx;

    for (idx = 0 ; idx < 8 ; idx += 1) {
        int k;
        for (k = 0 ; k < 16 ; k += 1)
            LPInput[idx][k] = 0;
    }

    int lap_mean[2];
    lap_mean[0] = 0;
    lap_mean[1] = 0;

    DEBUG(" MB_LP tile=[%u %u] mb=[%u %u] bitpos=%zu\n",
        tx, ty, mx, my, _jxr_rbitstream_bitpos(str));

    if (_jxr_InitContext(image, tx, ty, mx, my)) {
        DEBUG(" Init contexts\n");
        _jxr_InitializeCountCBPLP(image);
        _jxr_InitLPVLC(image);
        _jxr_InitializeAdaptiveScanLP(image);
        _jxr_InitializeModelMB(&image->model_lp, 1/*LP*/);
    }

    if (_jxr_ResetTotals(image, mx)) {
        _jxr_ResetTotalsAdaptiveScanLP(image);
    }

    int full_planes = image->num_channels;
    if (image->use_clr_fmt==2 || image->use_clr_fmt==1)
        full_planes = 2;

    /* The CBPLP signals whether any non-zero coefficients are
    present in the LP band for this macroblock. It is a bitmask
    with a bit for each channel. So for example, YONLY, which
    has 1 channel, has a 1-bit cbplp. */

    int cbplp = 0;
    /* if CLR_FMT is YUV420, YUV422 or YUV444... */
    if (image->use_clr_fmt==1 || image->use_clr_fmt==2 || image->use_clr_fmt==3) {
        DEBUG(" MB_LP: Calculate YUV CBP using CountZeroCBPLP=%d, CountMaxCBPLP=%d bitpos=%zu\n",
            image->count_zero_CBPLP, image->count_max_CBPLP, _jxr_rbitstream_bitpos(str));
        int max = full_planes * 4 - 5;
        if (image->count_zero_CBPLP <= 0 || image->count_max_CBPLP < 0) {
            int cbp_yuv_lp1 = dec_cbp_yuv_lp1(image, str);
            if (image->count_max_CBPLP < image->count_zero_CBPLP)
                cbplp = max - cbp_yuv_lp1;
            else
                cbplp = cbp_yuv_lp1;
        } else {
            uint32_t cbp_yuv_lp2 = _jxr_rbitstream_uintN(str, full_planes);
            cbplp = cbp_yuv_lp2;
        }
        _jxr_UpdateCountCBPLP(image, cbplp, max);

    } else {
        int idx;
        cbplp = 0;
        for (idx = 0 ; idx < image->num_channels ; idx += 1) {
            int cbp_ch_lp = _jxr_rbitstream_uint1(str);
            cbplp |= cbp_ch_lp << idx;
        }
    }

    DEBUG(" MB_LP: cbplp = 0x%x (full_planes=%u)\n", cbplp, full_planes);

    int ndx;
    for (ndx = 0 ; ndx < full_planes ; ndx += 1) {
        int idx;
        const int chroma_flag = ndx>0? 1 : 0;
        int num_nonzero = 0;

        DEBUG(" MB_LP: process full_plane %u, CBPLP for plane=%d, bitpos=%zu\n",
            ndx, (cbplp>>ndx)&1, _jxr_rbitstream_bitpos(str));
        if ((cbplp>>ndx) & 1) {
            /* If the CBPLP bit is set for this plane, then we
            have parsing to do. Decode the (15) coeffs and
            arrange them for use in the MB. */
            int RLCoeffs[32] = {0};
            for (idx = 0 ; idx < 32 ; idx += 1)
                RLCoeffs[idx] = 0;

            int location = 1;
            /* if CLR_FMT is YUV420 or YUV422 */
            if (image->use_clr_fmt==1/*YUV420*/ && chroma_flag)
                location = 10;
            if (image->use_clr_fmt==2/*YUV422*/ && chroma_flag)
                location = 2;

            num_nonzero = r_DECODE_BLOCK(image, str,
                chroma_flag, RLCoeffs, 1/*LP*/, location);
            DEBUG(" : num_nonzero = %d\n", num_nonzero);
            assert(num_nonzero <= 16);

            if ((image->use_clr_fmt==1 || image->use_clr_fmt==2) && chroma_flag) {
                static const int remap_arr[] = {4, 1, 2, 3, 5, 6, 7};
                int temp[14];
                int idx;
                for (idx = 0 ; idx < 14 ; idx += 1)
                    temp[idx] = 0;

                int remap_off = 0;
                if (image->use_clr_fmt==1/*YUV420*/)
                    remap_off = 1;

                int count_chr = 14;
                if (image->use_clr_fmt==1/*YUV420*/)
                    count_chr = 6;

                int k, i = 0;
                for (k = 0; k < num_nonzero; k+=1) {
                    i += RLCoeffs[k*2+0];
                    temp[i] = RLCoeffs[k*2+1];
                    i += 1;
                }
                for (k = 0; k < count_chr; k+=1) {
                    int remap = remap_arr[(k>>1) + remap_off];
                    int plane = (k&1) + 1;
                    if (image->use_clr_fmt==1)
                        remap = transpose420[remap];
                    if (image->use_clr_fmt==2)
                        remap = transpose422[remap];
                    LPInput[plane][remap] = temp[k];
                }
#if defined(DEBUG)
                {
                    int k;
                    DEBUG(" RLCoeffs[ndx=%d] ==", ndx);
                    for (k = 0 ; k<(num_nonzero*2); k+=2) {
                        DEBUG(" %d/0x%x", RLCoeffs[k+0], RLCoeffs[k+1]);
                    }
                    DEBUG("\n");
                    DEBUG(" temp ==");
                    for (k = 0 ; k<14; k+=1) {
                        DEBUG(" 0x%x", temp[k]);
                    }
                    DEBUG("\n");
                }
#endif
            } else {
                /* "i" is the current position in the LP
                array. It is adjusted based in the run
                each time around. This implines that the
                run value is the number of 0 elements in
                the LP array between non-zero values. */
                int k, i = 1;
                for (k = 0; k < num_nonzero; k+=1) {
                    i += RLCoeffs[k*2];
                    AdaptiveLPScan(image, LPInput[ndx], i, RLCoeffs[k*2+1]);
                    i += 1;
                }
            }
        }

#if defined(DEBUG)
        if (image->use_clr_fmt == 2/*YUV422*/) {
            int k;
            DEBUG(" lp val[ndx=%d] before refine ==", ndx);
            for (k = 1 ; k<8; k+=1) {
                DEBUG(" 0x%x/0x%x", LPInput[1][k], LPInput[2][k]);
            }
            DEBUG("\n");

        } else if (image->use_clr_fmt == 1/*YUV420*/) {
            int k;
            DEBUG(" lp val[ndx=%d] before refine ==", ndx);
            for (k = 1 ; k<4; k+=1) {
                DEBUG(" 0x%x/0x%x", LPInput[1][k], LPInput[2][k]);
            }
            DEBUG("\n");

        } else {
            int k;
            DEBUG(" lp val[ndx=%d] before refine ==", ndx);
            for (k = 1 ; k<16; k+=1) {
                DEBUG(" 0x%x", LPInput[ndx][k]);
            }
            DEBUG("\n");
            DEBUG(" adapted scan order ==");
            for (k = 0 ; k<15; k+=1) {
                DEBUG(" %2d", image->lopass_scanorder[k]);
            }
            DEBUG("\n");
            DEBUG(" adapted scan totals ==");
            for (k = 0 ; k<15; k+=1) {
                DEBUG(" %2d", image->lopass_scantotals[k]);
            }
            DEBUG("\n");
        }
#endif

        int model_bits = image->model_lp.bits[chroma_flag];
        lap_mean[chroma_flag] += num_nonzero;
        DEBUG(" MB_LP: start refine, model_bits=%d, bitpos=%zu\n",
            model_bits, _jxr_rbitstream_bitpos(str));
        if (model_bits) {
            static const int transpose444[16] = { 0, 4, 8,12,
                1, 5, 9,13,
                2, 6,10,14,
                3, 7,11,15 };
            if (chroma_flag == 0) {
                int k;
                for (k=1 ;k<16; k+=1) {
                    int k_ptr = transpose444[k];
                    LPInput[ndx][k_ptr] = r_REFINE_LP(str, LPInput[ndx][k_ptr], model_bits);
                }
            } else {
                int k;
                switch (image->use_clr_fmt) {
                    case 1: /* YUV420 */
                        for (k=1 ; k<4; k+=1) {
                            int k_ptr = transpose420[k];
                            LPInput[1][k_ptr] = r_REFINE_LP(str, LPInput[1][k_ptr], model_bits);
                            LPInput[2][k_ptr] = r_REFINE_LP(str, LPInput[2][k_ptr], model_bits);
                        }
                        break;
                    case 2: /* YUV422 */
                        for (k=1 ; k<8; k+=1) {
                            int k_ptr = transpose422[k];
                            DEBUG(" MP_LP: Refine LP_Input[1/2][%d] = 0x%x/0x%x bitpos=%zu\n",
                                k_ptr, LPInput[1][k_ptr], LPInput[2][k_ptr],
                                _jxr_rbitstream_bitpos(str));
                            LPInput[1][k_ptr] = r_REFINE_LP(str, LPInput[1][k_ptr], model_bits);
                            LPInput[2][k_ptr] = r_REFINE_LP(str, LPInput[2][k_ptr], model_bits);
                        }
                        break;
                    default: /* All others */
                        for (k=1 ;k<16; k+=1) {
                            int k_ptr = transpose444[k];
                            LPInput[ndx][k_ptr] = r_REFINE_LP(str, LPInput[ndx][k_ptr], model_bits);
                        }
                        break;
                }
            }
        }

        /* Stash the calculated LP values into the current
        MACROBLK strip */
        if (chroma_flag == 0) {
            /* All luma planes are simply copied into the macroblk. */
            int k;
            DEBUG(" lp val ==");
            for (k = 1 ; k<16; k+=1) {
                DEBUG(" 0x%x", LPInput[ndx][k]);
                MACROBLK_CUR_LP(image, ndx, tx, mx, k-1) = LPInput[ndx][k];
            }
            DEBUG("\n");
        } else {
            int k;
            DEBUG(" lp val (ch=%d) ==", ndx);
            switch (image->use_clr_fmt) {
                case 1:/* YUV420 */
                    /* The chroma for YUV420 is interleaved. */
                    for (k = 1 ; k < 4 ; k+=1) {
                        DEBUG(" 0x%x/0x%x", LPInput[1][k], LPInput[2][k]);
                        MACROBLK_CUR_LP(image, 1, tx, mx, k-1) = LPInput[1][k];
                        MACROBLK_CUR_LP(image, 2, tx, mx, k-1) = LPInput[2][k];
                    }
                    break;
                case 2:/* YUV422 */
                    /* The chroma for YUV422 is interleaved. */
                    for (k = 1 ; k < 8 ; k+=1) {
                        DEBUG(" 0x%x/0x%x", LPInput[1][k], LPInput[2][k]);
                        MACROBLK_CUR_LP(image, 1, tx, mx, k-1) = LPInput[1][k];
                        MACROBLK_CUR_LP(image, 2, tx, mx, k-1) = LPInput[2][k];
                    }
                    break;
                default:
                    for (k = 1 ; k < 16 ; k += 1) {
                        DEBUG(" 0x%x", LPInput[ndx][k]);
                        MACROBLK_CUR_LP(image, ndx, tx, mx, k-1) = LPInput[ndx][k];
                    }
                    break;
            }
            DEBUG("\n");
        }
    }

    DEBUG(" MB_LP: UpdateModelMB lap_mean={%d, %d}\n", lap_mean[0], lap_mean[1]);
    _jxr_UpdateModelMB(image, lap_mean, &image->model_lp, 1/*band=LP*/);
    if (_jxr_ResetContext(image, tx, mx)) {
        DEBUG(" AdaptLP at the end of mx=%d (my=%d, ndx=%u)\n", mx, my, ndx);
        _jxr_AdaptLP(image);
    }

    DEBUG(" MB_LP DONE tile=[%u %u] mb=[%u %u]\n", tx, ty, mx, my);
}

/*
* This decides the MBCBP for the macroblock. This value is then used
* by the MB_HP to know how to decide the HP values for the macroblock.
*/
int _jxr_r_MB_CBP(jxr_image_t image, struct rbitstream*str, int /*alpha_flag*/,
                  unsigned tx, unsigned ty, unsigned mx, unsigned my)
{
    static const int flc_table[] = {0, 2, 1, 2, 2, 0};
    static const int off_table[] = {0, 4, 2, 8, 12, 1};
    static const int out_table[] = {
        0, 15, 3, 12,
        1, 2, 4, 8,
        5, 6, 9, 10,
        7, 11, 13, 14 };

        int diff_cbp[MAX_CHANNELS];
        int idx;
        for (idx = 0 ; idx < MAX_CHANNELS ; idx += 1)
            diff_cbp[idx] = 0;

        DEBUG(" MB_CBP tile=[%u %u] mb=[%u %u] bitpos=%zu\n",
            tx, ty, mx, my, _jxr_rbitstream_bitpos(str));

        if (_jxr_InitContext(image, tx, ty, mx, my)) {
            DEBUG(" MB_CBP: InitContext\n");
            /* This happens only at the top left edge of the tile. */
            _jxr_InitCBPVLC(image);
        }

        /* "Channels" is not quite the same as "planes". For the
        purposes of CBP parsing, a color image has 1 channel. */
        int channels = 1;
        if (image->use_clr_fmt==4/*YUVK*/ || image->use_clr_fmt==6/*NCOMPONENT*/)
            channels = image->num_channels;

        /* This "for" loop decides not the code block pattern itself,
        but the encoded difference values. These are then added to
        the predicted values that are calculated later to make the
        actual MBCBP values. */
        int chan;
        for (chan = 0 ; chan < channels ; chan += 1) {
            DEBUG(" MB_CBP: Decode CBP for channel %d bitpos=%zu\n", chan, _jxr_rbitstream_bitpos(str));
            struct adaptive_vlc_s*vlc = image->vlc_table + DecNumCBP;
            int num_cbp = get_num_cbp(str, vlc);

            assert(vlc->deltatable == 0 && num_cbp < 5);
            static const int Num_CBP_Delta[5] = {0, -1, 0, 1, 1};
            vlc->discriminant += Num_CBP_Delta[num_cbp];

            DEBUG(" MB_CBP: Got num_cbp=%d, start REFINE_CBP at bitpos=%zu\n",
                num_cbp, _jxr_rbitstream_bitpos(str));

            int cbp = r_REFINE_CBP(str, num_cbp);

            DEBUG(" MB_CBP: Refined CBP=0x%x (num=%d)\n", cbp, num_cbp);

            /* The cbp is a "block present" bit hask for a group of
            4 blocks. This is used to inform the loop below that
            then tries to fill discern the 4 bits for the range
            enabled by this first level cbp. For example, if
            cbp=0x5, then the 16 diff_cbp values are 0x0?0? where
            the ? nibbles are yet to be resolved by the loop
            below. */

            int blk;
            for (blk = 0 ; blk < 4 ; blk += 1) {
                if ( (cbp & (1<<blk)) == 0 )
                    continue;

                vlc = image->vlc_table + DecNumBlkCBP;
                DEBUG(" MB_CBP: block=%d Use DecNumBlkCBP table=%d, discriminant=%d, bitpos=%zu\n",
                    blk, vlc->table, vlc->discriminant, _jxr_rbitstream_bitpos(str));

                int num_blkcbp = get_num_blkcbp(image, str, vlc);

                assert(vlc->deltatable == 0);

                if (image->use_clr_fmt==0 || image->use_clr_fmt==4 || image->use_clr_fmt==6) {
                    assert(num_blkcbp < 5);
                    static const int Num_BLKCBP_Delta5[5] = {0, -1, 0, 1, 1};
                    vlc->discriminant += Num_BLKCBP_Delta5[num_blkcbp];
                } else {
                    assert(num_blkcbp < 9);
                    static const int Num_BLKCBP_Delta9[9] = {2, 2, 1, 1, -1, -2, -2, -2, -3};
                    vlc->discriminant += Num_BLKCBP_Delta9[num_blkcbp];
                }

                DEBUG(" MB_CBP: NUM_BLKCBP=%d, discriminant becomes=%d, \n",
                    num_blkcbp, vlc->discriminant);

                int val = num_blkcbp + 1;

                int blkcbp = 0;

                /* Should only be true if this is chroma data. */
                if (val >= 6) {
                    int chr_cbp = get_value_012(str);
                    blkcbp = 0x10 * (chr_cbp+1);
                    if (val >= 9) {
                        int val_inc = get_value_012(str);
                        val += val_inc;
                    }
                    DEBUG(" MB_CBP: iVal=%d, CHR_CBP=%x\n", val, chr_cbp);
                    val -= 6;
                }
                assert(val < 6);

                int code = off_table[val];
                if (flc_table[val]) {
                    code += _jxr_rbitstream_uintN(str, flc_table[val]);
                }

                assert(code < 16);
                blkcbp += out_table[code];

                DEBUG(" MB_CBP: NUM_BLKCBP=%d, iCode=%d\n", num_blkcbp, code);
                DEBUG(" MB_CBP: blkcbp=0x%x for chunk blk=%d\n", blkcbp, blk);

                /* blkcbp is done. Now calculate the
                diff_cbp. How this is done (and how many
                there are) depend on the color format. */

                switch (image->use_clr_fmt) {
                    case 3: /*YUV444*/
                        diff_cbp[0] |= (blkcbp&0xf) << (blk * 4);
                        if (blkcbp & 0x10) {
                            int num_ch_blk = get_num_ch_blk(str);
                            int cbp_chr = r_REFINE_CBP(str, num_ch_blk+1);
                            DEBUG(" MB_CBP: Refined CBP_U=0x%x (num=%d)\n", cbp_chr, num_ch_blk);
                            diff_cbp[1] |= cbp_chr << (blk*4);
                        }
                        if (blkcbp & 0x20) {
                            int num_ch_blk = get_num_ch_blk(str);
                            int cbp_chr = r_REFINE_CBP(str, num_ch_blk+1);
                            DEBUG(" MB_CBP: Refined CBP_V=0x%x (num=%d)\n", cbp_chr, num_ch_blk);
                            diff_cbp[2] |= cbp_chr << (blk*4);
                        }
                        break;

                    case 2: /*YUV422*/
                        diff_cbp[0] |= (blkcbp&0xf) << (blk*4);
                        if (blkcbp & 0x10) {
                            const int shift[4] = {0, 1, 4, 5};
                            int cbp_ch_blk = get_value_012(str);
                            int cbp_chr = shift[cbp_ch_blk+1];
                            diff_cbp[1] |= cbp_chr << shift[blk];
                            DEBUG(" MB_CBP: Refined CBP_U=0x%x (cbp_ch_blk=%d, blk=%d)\n",
                                diff_cbp[1], cbp_ch_blk, blk);
                        }
                        if (blkcbp & 0x20) {
                            const int shift[4] = {0, 1, 4, 5};
                            int cbp_ch_blk = get_value_012(str);
                            int cbp_chr = shift[cbp_ch_blk+1];
                            diff_cbp[2] |= cbp_chr << shift[blk];
                            DEBUG(" MB_CBP: Refined CBP_V=0x%x (cbp_ch_blk=%d, blk=%d)\n",
                                diff_cbp[2], cbp_ch_blk, blk);
                        }
                        break;

                    case 1: /*YUV420*/
                        diff_cbp[0] |= (blkcbp & 0xf) << (blk*4);
                        diff_cbp[1] |= ((blkcbp >> 4) & 1) << blk;
                        diff_cbp[2] += ((blkcbp >> 5) & 1) << blk;
                        break;

                    default:
                        diff_cbp[chan] |= blkcbp << (blk*4);
                        break;
                }
            }
            DEBUG(" MB_CBP: chan=%d, num_cbp=%d, cbp=0x%1x\n", chan, num_cbp, cbp);
        }

#if defined(DETAILED_DEBUG)
        for (chan = 0 ; chan < image->num_channels ; chan += 1) {
            DEBUG(" MB_CBP: diff_cbp[%d]=0x%04x\n", chan, diff_cbp[chan]);
        }
#endif

        r_PredCBP(image, diff_cbp, tx, ty, mx, my);

        DEBUG(" MB_CBP done tile=[%u %u] mb=[%u %u]\n", tx, ty, mx, my);
        return 0;
}

static int r_REFINE_CBP(struct rbitstream*str, int num)
{
    switch (num) {
        case 1:
            return 1 << _jxr_rbitstream_uint2(str);

        case 2:
            /*
            * table value
            * 00 3
            * 01 5
            * 100 6
            * 101 9
            * 110 10
            * 111 12
            */
            if (_jxr_rbitstream_uint1(str) == 0) {
                if (_jxr_rbitstream_uint1(str) == 0)
                    return 3;
                else
                    return 5;
            } else { /* 1xx */
                if (_jxr_rbitstream_uint1(str) == 0) { /* 10x */
                    if (_jxr_rbitstream_uint1(str) == 0)
                        return 6;
                    else
                        return 9;
                } else { /* 11x */
                    if (_jxr_rbitstream_uint1(str) == 0)
                        return 10;
                    else
                        return 12;
                }
            }

        case 3:
            return 0x0f ^ (1 << _jxr_rbitstream_uint2(str));

        case 4:
            return 0x0f;

        default:
            return 0x00;
    }
}


int _jxr_r_MB_HP(jxr_image_t image, struct rbitstream*str,
                 int /*alpha_flag*/,
                 unsigned tx, unsigned ty,
                 unsigned mx, unsigned my)
{
    DEBUG(" MB_HP tile=[%u %u] mb=[%u %u] bitpos=%zu\n",
        tx, ty, mx, my, _jxr_rbitstream_bitpos(str));

    if (_jxr_InitContext(image, tx, ty, mx, my)) {
        DEBUG(" MB_HP: InitContext\n");
        /* This happens only at the top left edge of the tile. */
        _jxr_InitHPVLC(image);
        _jxr_InitializeAdaptiveScanHP(image);
        _jxr_InitializeModelMB(&image->model_hp, 2/*band=HP*/);
    }

    if (_jxr_ResetTotals(image, mx)) {
        _jxr_ResetTotalsAdaptiveScanHP(image);
    }

    /* FLEXBITS are embedded in the HP data if there are FLEXBITS
    present in the bitstream AND we are in SPATIAL (not
    FREQUENCY) mode. */
    int flex_flag = 1;
    if (image->bands_present == 1) /* NOFLEXBITS */
        flex_flag = 0;
    if (FREQUENCY_MODE_CODESTREAM_FLAG(image) != 0) /* FREQUENCY_MODE */
        flex_flag = 0;

    int lap_mean[2];
    lap_mean[0] = 0;
    lap_mean[1] = 0;

    /* Calculate the MB HP prediction mode. This uses only local
    information, namely the LP values. */
    int mbhp_pred_mode = r_calculate_mbhp_mode(image, tx, mx);
    assert(mbhp_pred_mode < 4);

    int idx;
    for (idx = 0 ; idx < image->num_channels; idx += 1) {
        int chroma_flag = idx>0? 1 : 0;
        int nblocks = 4;
        if (chroma_flag && image->use_clr_fmt==1/*YUV420*/)
            nblocks = 1;
        else if (chroma_flag && image->use_clr_fmt==2/*YUV422*/)
            nblocks = 2;

        unsigned model_bits = image->model_hp.bits[chroma_flag];
        int cbp = MACROBLK_CUR_HPCBP(image, idx, tx, mx);
        int block;
        DEBUG(" MB_HP channel=%d, cbp=0x%x, model_bits=%u MBHPMode=%d\n",
            idx, cbp, model_bits, mbhp_pred_mode);
        for (block=0 ; block<(nblocks*4) ; block += 1, cbp >>= 1) {
            int bpos = block;
            /* Only remap the Y plane of YUV42X data. */
            if (nblocks == 4)
                bpos = _jxr_hp_scan_map[block];
            int num_nonzero = r_DECODE_BLOCK_ADAPTIVE(image, str, tx, mx,
                cbp&1, chroma_flag,
                idx, bpos, mbhp_pred_mode,
                model_bits);
            if (num_nonzero < 0) {
                DEBUG("ERROR: r_DECODE_BLOCK_ADAPTIVE returned rc=%d\n", num_nonzero);
                return JXR_EC_ERROR;
            }
            if (flex_flag)
                r_BLOCK_FLEXBITS(image, str, tx, ty, mx, my,
                idx, bpos, model_bits);
            lap_mean[chroma_flag] += num_nonzero;
        }

    }

    int use_num_channels = image->num_channels;
    if (image->use_clr_fmt == 1/*YUV420*/ || image->use_clr_fmt == 2/*YUV422*/)
        use_num_channels = 1;

    /* Propagate HP predictions only in SPATIAL MODE. If this is
    FREQUENCY mode, and there is a FLEXBITS pass later, then do
    *not* do the predictions, leaving them to the FLEXBITS tile. */
    if (FREQUENCY_MODE_CODESTREAM_FLAG(image) == 0 || image->bands_present == 1) {
        DEBUG(" MB_HP: propagate hp predictions within MB_HP function\n");
        for (idx = 0 ; idx < use_num_channels ; idx += 1)
            _jxr_propagate_hp_predictions(image, idx, tx, mx, mbhp_pred_mode);
    }

    DEBUG(" MP_HP: lap_mean={%u, %u}, model_hp.bits={%u %u}, model_hp.state={%d %d}\n",
        lap_mean[0], lap_mean[1],
        image->model_hp.bits[0], image->model_hp.bits[1],
        image->model_hp.state[0], image->model_hp.state[1]);

    MACROBLK_CUR(image,0,tx,mx).mbhp_pred_mode = mbhp_pred_mode;
    MACROBLK_CUR(image,0,tx,mx).hp_model_bits[0] = image->model_hp.bits[0];
    MACROBLK_CUR(image,0,tx,mx).hp_model_bits[1] = image->model_hp.bits[1];

    _jxr_UpdateModelMB(image, lap_mean, &image->model_hp, 2/*band=HP*/);
    if (_jxr_ResetContext(image, tx, mx)) {
        DEBUG(" MB_HP: Run AdaptHP\n");
        _jxr_AdaptHP(image);
    }

    DEBUG(" MP_HP: Updated: lap_mean={%u, %u}, model_hp.bits={%u %u}, model_hp.state={%d %d}\n",
        lap_mean[0], lap_mean[1],
        image->model_hp.bits[0], image->model_hp.bits[1],
        image->model_hp.state[0], image->model_hp.state[1]);

    DEBUG(" MB_HP DONE tile=[%u %u] mb=[%u %u]\n", tx, ty, mx, my);
    return 0;
}

int _jxr_r_MB_FLEXBITS(jxr_image_t image, struct rbitstream*str,
                       int /*alpha_flag*/,
                       unsigned tx, unsigned ty,
                       unsigned mx, unsigned my)
{
    int idx;
    for (idx = 0 ; idx < image->num_channels ; idx += 1) {
        int chroma_flag = idx>0? 1 : 0;
        int nblocks = 4;
        if (chroma_flag && image->use_clr_fmt==1/*YUV420*/)
            nblocks = 1;
        else if (chroma_flag && image->use_clr_fmt==2/*YUV422*/)
            nblocks = 2;

        unsigned model_bits = MACROBLK_CUR(image,0,tx,mx).hp_model_bits[chroma_flag];
        int block;
        for (block=0 ; block<(nblocks*4) ; block += 1) {
            int bpos = block;
            /* Only remap the Y plane of YUV42X data. */
            if (nblocks == 4)
                bpos = _jxr_hp_scan_map[block];

            r_BLOCK_FLEXBITS(image, str, tx, ty, mx, my,
                idx, bpos, model_bits);
        }
    }

    return 0;
}

/*
* Decode a single DC component value from the input stream.
*/
static int32_t r_DEC_DC(jxr_image_t image, struct rbitstream*str,
                        unsigned /*tx*/, unsigned /*ty*/,
                        unsigned /*mx*/, unsigned /*my*/,
                        int model_bits, int chroma_flag, int is_dc_ch)
{
    int32_t dc_val = 0;

    if (is_dc_ch) {
        dc_val = r_DECODE_ABS_LEVEL(image, str, 0/*DC*/, chroma_flag) -1;
        DEBUG(" DEC_DC: DECODE_ABS_LEVEL = %u (0x%08x)\n", dc_val, dc_val);
    }

    /* If there are model_bits, then read them literally from the
    bitstream and use them as the LSB bits for the DC value. */
    if (model_bits > 0) {
        DEBUG(" DEC_DC: Collect %u model_bits\n", model_bits);
        int idx;
        for (idx = 0 ; idx < model_bits ; idx += 1) {
            dc_val <<= 1;
            dc_val |= _jxr_rbitstream_uint1(str);
        }
    }

    /* If the dc_val is non-zero, it may have a sign so decode the
    sign bit and apply it. */
    if (dc_val != 0) {
        int sign_flag = _jxr_rbitstream_uint1(str);
        DEBUG(" DEC_DC: sign_flag=%s\n", sign_flag? "true":"false");
        if (sign_flag)
            dc_val = - dc_val;
    }

    DEBUG(" DEC_DC: DC value is %d (0x%08x)\n", dc_val, dc_val);
    return dc_val;
}

/*
* This function decodes one sample from one of the bands. The code is
* the same for any of the bands. The band (and chroma_flag) is only
* used to select the vlc_table.
*
* Note that the chroma_flag is only interpreted as the "chroma_flag"
* when the band is DC. Otherwise, the chroma_flag argument is taken
* as the "context" argument described in the specification.
*/
static uint32_t r_DECODE_ABS_LEVEL(jxr_image_t image, struct rbitstream*str,
                                   int band, int chroma_flag)
{
    int vlc_select = _jxr_vlc_select(band, chroma_flag);
    DEBUG(" Use vlc_select = %s (table=%d) to decode level index, bitpos=%zu\n",
        _jxr_vlc_index_name(vlc_select), image->vlc_table[vlc_select].table,
        _jxr_rbitstream_bitpos(str));

    const int remap[] = {2, 3, 4, 6, 10, 14};
    const int fixed_len[] = {0, 0, 1, 2, 2, 2};

    int abslevel_index = dec_abslevel_index(image, str, vlc_select);
    DEBUG(" ABSLEVEL_INDEX = %d\n", abslevel_index);

    image->vlc_table[vlc_select].discriminant += _jxr_abslevel_index_delta[abslevel_index];

    uint32_t level;
    if (abslevel_index < 6) {
        int fixed = fixed_len[abslevel_index];
        level = remap[abslevel_index];
        uint32_t level_ref = 0;
        if (fixed > 0) {
            assert(fixed <= 32);
            int idx;
            for (idx = 0 ; idx < fixed ; idx += 1) {
                level_ref <<= 1;
                level_ref |= _jxr_rbitstream_uint1(str);
            }
            level += level_ref;
        }
        DEBUG(" ABS_LEVEL = 0x%x (fixed = %d, level_ref = %d)\n",
            level, fixed, level_ref);
    } else {
        int fixed = 4 + _jxr_rbitstream_uint4(str);
        if (fixed == 19) {
            fixed += _jxr_rbitstream_uint2(str);
            if (fixed == 22) {
                fixed += _jxr_rbitstream_uint3(str);
            }
        }

        assert(fixed <= 32);

        uint32_t level_ref = 0;
        int idx;
        for (idx = 0 ; idx < fixed ; idx += 1) {
            level_ref <<= 1;
            level_ref |= _jxr_rbitstream_uint1(str);
        }
        level = 2 + (1 << fixed) + level_ref;
        DEBUG(" ABS_LEVEL = 0x%x (fixed = %d, level_ref = %d)\n",
            level, fixed, level_ref);
    }

    return level;
}

/*
* The DECODE_BLOCK decodes the block as run/coefficient pairs. The
* run is the distance to the next coefficient, and is followed by the
* coefficient itself. The skipped values are implicitly zeros. A
* later process takes these pairs including adaptation of their position.
*/
int r_DECODE_BLOCK(jxr_image_t image, struct rbitstream*str,
                   int chroma_flag, int coeff[32], int band, int location)
{
    DEBUG(" DECODE_BLOCK chroma_flag=%d, band=%d, location=%d bitpos=%zu\n",
        chroma_flag, band, location, _jxr_rbitstream_bitpos(str));
    int num_nz = 1;

    /* The index is a bit field that encodes three values:
    * index[0] : 1 means the run to the next coeff is == 0
    * index[1] : 1 means the next coefficient is >1
    * index[3:2]: 0 This is the last coefficient
    * 1 the next non-zero coefficient immediately follows
    * 2 there are zero coefficients before the next.
    */
    int index = r_DECODE_FIRST_INDEX(image, str, chroma_flag, band);

    DEBUG(" first index=0x%x\n", index);

    int sr = index & 1;
    int srn = index >> 2;
    int context = sr & srn;

    /* Decode the first coefficient. Note that the chroma_flag
    argument to DECODE_ABS_LEVEL really is supposed to be the
    context. It is "chroma" for DC values (band==0) and context
    for LP and HP values. This DECODE_BLOCK is only called for
    LP and HP blocks. */
    int sign_flag = _jxr_rbitstream_uint1(str);
    if (index&2)
        coeff[1] = r_DECODE_ABS_LEVEL(image, str, band, context);
    else
        coeff[1] = 1;

    if (sign_flag)
        coeff[1] = -coeff[1];

    /* Decode the run to the first coefficient. */
    if (index&1) {
        coeff[0] = 0;
    } else {
        assert( location < 15 );
        coeff[0] = r_DECODE_RUN(image, str, 15-location);
    }

    DEBUG(" coeff[0] = %d (run)\n", coeff[0]);
    DEBUG(" coeff[1] = 0x%x (coeff)\n", coeff[1]);

    location += coeff[0] + 1;

    while (srn != 0) { /* While more coefficients are expected... */
        sr = srn & 1;

        /* Decode run to the next coefficient. */
        if (srn & 1) {
            coeff[num_nz*2] = 0;
        } else {
            coeff[num_nz*2] = r_DECODE_RUN(image, str, 15-location);
        }

        DEBUG(" coeff[%d*2+0] = %d (run)\n", num_nz, coeff[num_nz*2]);

        location += coeff[num_nz*2] + 1;

        /* The index is a bit field that encodes two values:
        * index[0] : 1 means the run to the next coeff is == 0
        * index[2:1]: 0 This is the last coefficient
        * 1 the next non-zero coefficient immediately follows
        * 2 there are zero coefficients before the next.
        * The location can clue the DECODE_INDEX that certain
        * constraints on the possible index values may exist,
        * and certain restricted tables are used.
        */
        index = r_DECODE_INDEX(image, str, location, chroma_flag, band, context);
        DEBUG(" next index=0x%x\n", index);

        srn = index >> 1;
        context &= srn;

        /* Decode the next coefficient. */
        sign_flag = _jxr_rbitstream_uint1(str);
        if (index & 1)
            coeff[num_nz*2+1] = r_DECODE_ABS_LEVEL(image, str,
            band, context);
        else
            coeff[num_nz*2+1] = 1;

        if (sign_flag)
            coeff[num_nz*2+1] = -coeff[num_nz*2+1];

        DEBUG(" coeff[%d*2+1] = 0x%x (coeff)\n", num_nz, coeff[num_nz*2+1]);
        num_nz += 1;
    }

    DEBUG(" DECODE_BLOCK done, num_nz=%d\n", num_nz);
    return num_nz;
}

static int r_DECODE_FIRST_INDEX(jxr_image_t image, struct rbitstream*str,
                                int chroma_flag, int band)
{
    /* VALUE TABLE0
    * 0 0000 1..
    * 1 0000 01.
    * 2 0000 000
    * 3 0000 001
    * 4 0010 0..
    * 5 010. ...
    * 6 0010 1..
    * 7 1... ...
    * 8 0011 0..
    * 9 0001 ...
    * 10 0011 1..
    * 11 011. ...
    * (Table 59: Note that the first bit is handled as a special
    * case, so the array only needs to account for the last 6 bits.)
    */
    static const unsigned char c0b[64] = {
        6, 6, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
    };
    static const signed char c0v[64] = {
        2, 3, 1, 1, 0, 0, 0, 0, 9, 9, 9, 9, 9, 9, 9, 9,
        4, 4, 4, 4, 6, 6, 6, 6, 8, 8, 8, 8, 10,10,10,10,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        11,11,11,11, 11,11,11,11, 11,11,11,11, 11,11,11,11
    };
    /* VALUE TABLE1
    * 0 0010 ..
    * 1 0001 0.
    * 2 0000 00
    * 3 0000 01
    * 4 0011 ..
    * 5 010. ..
    * 6 0001 1.
    * 7 11.. ..
    * 8 011. ..
    * 9 100. ..
    * 10 0000 1.
    * 11 101. ..
    */
    static const unsigned char c1b[64] = {
        6, 6, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
    };
    static const signed char c1v[64] = {
        2, 3,10,10, 1, 1, 6, 6, 0, 0, 0, 0, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 8, 8, 8, 8, 8, 8, 8, 8,
        9, 9, 9, 9, 9, 9, 9, 9, 11,11,11,11, 11,11,11,11,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
    };

    /* VALUE TABLE2
    * 0 11.. ...
    * 1 001. ...
    * 2 0000 000
    * 3 0000 001
    * 4 0000 1..
    * 5 010. ...
    * 6 0000 010
    * 7 011. ...
    * 8 100. ...
    * 9 101. ...
    * 10 0000 011
    * 11 0001 ...
    */
    static const unsigned char c2b[128] = {
        7, 7, 7, 7, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,

        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
    };
    static const signed char c2v[128] = {
        2, 3, 6,10, 4, 4, 4, 4, 11,11,11,11, 11,11,11,11,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,

        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    /* VALUE TABLE3
    * 0 001. ...
    * 1 11.. ...
    * 2 0000 000
    * 3 0000 1..
    * 4 0001 0..
    * 5 010. ...
    * 6 0000 001
    * 7 011. ...
    * 8 0001 1..
    * 9 100. ...
    * 10 0000 01.
    * 11 101. ...
    */
    static const unsigned char c3b[128] = {
        7, 7, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,

        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
    };
    static const signed char c3v[128] = {
        2, 6,10,10, 3, 3, 3, 3, 4, 4, 4, 4, 8, 8, 8, 8,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,

        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        11,11,11,11, 11,11,11,11, 11,11,11,11, 11,11,11,11,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };

    /* VALUE TABLE4
    * 0 010. ....
    * 1 1... ....
    * 2 0000 001.
    * 3 0001 ....
    * 4 0000 010.
    * 5 011. ....
    * 6 0000 0000
    * 7 0010 ....
    * 8 0000 011.
    * 9 0011 ....
    * 10 0000 0001
    * 11 0000 1...
    */
    static const unsigned char c4b[128] = {
        7, 7, 6, 6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,

        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
    };
    static const signed char c4v[128] = {
        6,10, 2, 2, 4, 4, 8, 8, 11,11,11,11, 11,11,11,11,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,

        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
    };

    abs_level_vlc_index_t vlc_select = (abs_level_vlc_index_t)0;

    switch (band) {
        case 1: /* LP */
            if (chroma_flag)
                vlc_select = DecFirstIndLPChr;
            else
                vlc_select = DecFirstIndLPLum;
            break;
        case 2: /* HP */
            if (chroma_flag)
                vlc_select = DecFirstIndHPChr;
            else
                vlc_select = DecFirstIndHPLum;
            break;
        default:
            assert(0);
            break;
    }

    int vlc_table = image->vlc_table[vlc_select].table;
    int first_index = 0;

    switch (vlc_table) {
        case 0:
            if (_jxr_rbitstream_uint1(str)) {
                first_index = 7;
            } else {
                first_index = _jxr_rbitstream_intE(str, 6, c0b, c0v);
            }
            break;

        case 1:
            first_index = _jxr_rbitstream_intE(str, 6, c1b, c1v);
            break;

        case 2:
            first_index = _jxr_rbitstream_intE(str, 7, c2b, c2v);
            break;

        case 3:
            first_index = _jxr_rbitstream_intE(str, 7, c3b, c3v);
            break;

        case 4:
            if (_jxr_rbitstream_uint1(str)) {
                first_index = 1;
            } else {
                first_index = _jxr_rbitstream_intE(str, 7, c4b, c4v);
            }
            break;

        default:
            assert(0);
            break;
    }

    int delta_table = image->vlc_table[vlc_select].deltatable;
    int delta2table = image->vlc_table[vlc_select].delta2table;

    typedef int deltatable_t[12];
    const deltatable_t FirstIndexDelta[4] = {
        { 1, 1, 1, 1, 1, 0, 0,-1, 2, 1, 0, 0 },
        { 2, 2,-1,-1,-1, 0,-2,-1, 0, 0,-2,-1 },
        {-1, 1, 0, 2, 0, 0, 0, 0,-2, 0, 1, 1 },
        { 0, 1, 0, 1,-2, 0,-1,-1,-2,-1,-2,-2 }
    };
    assert(delta_table < 4);
    assert(delta2table < 4);
    assert(first_index < 12);
    image->vlc_table[vlc_select].discriminant += FirstIndexDelta[delta_table][first_index];
    image->vlc_table[vlc_select].discriminant2 += FirstIndexDelta[delta2table][first_index];
    DEBUG(" DECODE_FIRST_INDEX: vlc_select = %s, vlc_table = %d, deltatable/2 = %d/%d, discriminant/2 = %d/%d, first_index=%d\n",
        _jxr_vlc_index_name(vlc_select), vlc_table,
        delta_table, delta2table,
        image->vlc_table[vlc_select].discriminant,
        image->vlc_table[vlc_select].discriminant2, first_index);

    return first_index;
}

static int r_DECODE_INDEX(jxr_image_t image, struct rbitstream*str,
                          int location, int chroma_flag, int band, int context)
{
    int vlc_select = 0;

    switch (band) {
        case 1: /* LP */
            if (chroma_flag)
                vlc_select = context? DecIndLPChr1 : DecIndLPChr0;
            else
                vlc_select = context? DecIndLPLum1 : DecIndLPLum0;
            break;
        case 2: /* HP */
            if (chroma_flag)
                vlc_select = context? DecIndHPChr1 : DecIndHPChr0;
            else
                vlc_select = context? DecIndHPLum1 : DecIndHPLum0;
            break;
        default:
            assert(0);
            break;
    }

    int index = 0;

    /* If location > 15, then there can't possibly be coefficients
    after the next, so the encoding will only encode the low
    bit, that hints the run is zero or not. */
    if (location > 15) {
        index = _jxr_rbitstream_uint1(str);
        DEBUG(" DECODE_INDEX: location=%d, index=%d\n", location, index);
        return index;
    }

    /* If location == 15, then this is probably the last
    coefficient, but there may be more. We do know that there
    are no zero coefficients before the next (if there is one).
    Use a fixed table to decode the index with reduced alphabet. */
    if (location == 15) {
        /* Table 61
        * INDEX2 table
        * 0 0
        * 2 10
        * 1 110
        * 3 111
        */
        if (_jxr_rbitstream_uint1(str) == 0)
            index = 0; /* 0 */
        else if (_jxr_rbitstream_uint1(str) == 0)
            index = 2; /* 10 */
        else if (_jxr_rbitstream_uint1(str) == 0)
            index = 1; /* 110 */
        else
            index = 3; /* 111 */
        DEBUG(" DECODE_INDEX: location=%d, index=%d\n", location, index);
        return index;
    }

    /* For more general cases, use adaptive table selections to
    decode the full set of index possibilities. */
    int vlc_table = image->vlc_table[vlc_select].table;
    DEBUG(" DECODE_INDEX: vlc_select = %s, vlc_table = %d chroma_flag=%d\n",
        _jxr_vlc_index_name(vlc_select), vlc_table, chroma_flag);

    /* Table 60 is implemented in this switch. */
    switch (vlc_table) {
        case 0:
            /* INDEX1 table0
            * 0 1
            * 1 00000
            * 2 001
            * 3 00001
            * 4 01
            * 5 0001
            */
            if (_jxr_rbitstream_uint1(str) == 1)
                index = 0; /* 1 */
            else if (_jxr_rbitstream_uint1(str) == 1)
                index = 4; /* 01 */
            else if (_jxr_rbitstream_uint1(str) == 1)
                index = 2; /* 001 */
            else if (_jxr_rbitstream_uint1(str) == 1)
                index = 5; /* 0001 */
            else if (_jxr_rbitstream_uint1(str) == 1)
                index = 3; /* 00001 */
            else
                index = 1; /* 00000 */
            break;

        case 1:
            /* INDEX1 table1
            * 0 01
            * 1 0000
            * 2 10
            * 3 0001
            * 4 11
            * 5 001
            */
            switch (_jxr_rbitstream_uint2(str)) {
                case 1: /* 01 */
                    index = 0;
                    break;
                case 2: /* 10 */
                    index = 2;
                    break;
                case 3: /* 11 */
                    index = 4;
                    break;
                case 0: /* 00... */
                    if (_jxr_rbitstream_uint1(str) == 1)
                        index = 5; /* 001 */
                    else if (_jxr_rbitstream_uint1(str) == 1)
                        index = 3; /* 0001 */
                    else
                        index = 1; /* 0000 */
                    break;
            }
            break;

        case 2:
            /* INDEX1 table2
            * 0 0000
            * 1 0001
            * 2 01
            * 3 10
            * 4 11
            * 5 001
            */
            switch (_jxr_rbitstream_uint2(str)) {
                case 1: /* 01 */
                    index = 2;
                    break;
                case 2: /* 10 */
                    index = 3;
                    break;
                case 3: /* 11 */
                    index = 4;
                    break;
                case 0: /* 00... */
                    if (_jxr_rbitstream_uint1(str))
                        index = 5; /* 001 */
                    else if (_jxr_rbitstream_uint1(str))
                        index = 1; /* 0001 */
                    else
                        index = 0; /* 0000 */
                    break;
            }
            break;

        case 3:
            /* INDEX1 table3
            * 0 00000
            * 1 00001
            * 2 01
            * 3 1
            * 4 0001
            * 5 001
            */
            if (_jxr_rbitstream_uint1(str))
                index = 3; /* 1 */
            else if (_jxr_rbitstream_uint1(str))
                index = 2; /* 01 */
            else if (_jxr_rbitstream_uint1(str))
                index = 5; /* 001 */
            else if (_jxr_rbitstream_uint1(str))
                index = 4; /* 0001 */
            else if (_jxr_rbitstream_uint1(str))
                index = 1; /* 00001 */
            else
                index = 0; /* 00000 */
            break;

        default:
            assert(0);
    }

    int vlc_delta = image->vlc_table[vlc_select].deltatable;
    int vlc_delta2 = image->vlc_table[vlc_select].delta2table;

    typedef int deltatable_t[6];
    const deltatable_t Index1Delta[3] = {
        {-1, 1, 1, 1, 0, 1 },
        {-2, 0, 0, 2, 0, 0 },
        {-1,-1, 0, 1,-2, 0 }
    };

    image->vlc_table[vlc_select].discriminant += Index1Delta[vlc_delta][index];
    image->vlc_table[vlc_select].discriminant2+= Index1Delta[vlc_delta2][index];

    DEBUG(" DECODE_INDEX: vlc_select = %s, deltatable/2 = %d/%d, discriminant/2 becomes %d/%d\n",
        _jxr_vlc_index_name(vlc_select), vlc_delta, vlc_delta2,
        image->vlc_table[vlc_select].discriminant,
        image->vlc_table[vlc_select].discriminant2);

    return index;
}

static int r_DECODE_RUN(jxr_image_t /*image*/, struct rbitstream*str, int max_run)
{
    int run=0;

    if (max_run < 5) {
        DEBUG(" DECODE_RUN max_run=%d (<5) bitpos=%zu\n",
            max_run, _jxr_rbitstream_bitpos(str));
        switch (max_run) {
            case 1:
                run = 1;
                break;
            case 2:
                if (_jxr_rbitstream_uint1(str))
                    run = 1;
                else
                    run = 2;
                break;
            case 3:
                if (_jxr_rbitstream_uint1(str))
                    run = 1;
                else if (_jxr_rbitstream_uint1(str))
                    run = 2;
                else
                    run = 3;
                break;
            case 4:
                if (_jxr_rbitstream_uint1(str))
                    run = 1;
                else if (_jxr_rbitstream_uint1(str))
                    run = 2;
                else if (_jxr_rbitstream_uint1(str))
                    run = 3;
                else
                    run = 4;
                break;
        }

    } else {
        static const int RunBin[15] = {-1,-1,-1,-1, 2,2,2, 1,1,1,1, 0,0,0,0 };
        static const int RunFixedLen[15] = {0,0,1,1,3, 0,0,1,1,2, 0,0,0,0,1 };
        static const int Remap[15] = {1,2,3,5,7, 1,2,3,5,7, 1,2,3,4,5 };
        int run_index = 0;
        if (_jxr_rbitstream_uint1(str))
            run_index = 0; /* 1 */
        else if (_jxr_rbitstream_uint1(str))
            run_index = 1; /* 01 */
        else if (_jxr_rbitstream_uint1(str))
            run_index = 2; /* 001 */
        else if (_jxr_rbitstream_uint1(str))
            run_index = 4; /* 0001 */
        else
            run_index = 3; /* 0000 */

        DEBUG(" DECODE_RUN max_run=%d, RUN_INDEX=%d\n", max_run, run_index);

        assert(max_run < 15);
        int index = run_index + 5*RunBin[max_run];
        DEBUG(" DECODE_RUN index=%d\n", index);

        assert(run_index < 15);
        int fixed = RunFixedLen[index];
        DEBUG(" DECODE_RUN fixed=%d (bitpos=%zu)\n",
            fixed, _jxr_rbitstream_bitpos(str));

        assert(run_index < 15);
        run = Remap[index];
        if (fixed) {
            run += _jxr_rbitstream_uintN(str, fixed);
        }
    }

    DEBUG(" DECODE_RUN max_run=%d, run=%d\n", max_run, run);

    return run;
}


static int r_REFINE_LP(struct rbitstream*str, int coeff, int model_bits)
{
    int coeff_refinement = _jxr_rbitstream_uintN(str, model_bits);

    if (coeff > 0) {
        coeff <<= model_bits;
        coeff += coeff_refinement;
    } else if (coeff < 0) {
        coeff <<= model_bits;
        coeff -= coeff_refinement;
    } else {
        coeff = coeff_refinement;
        if (coeff) {
            int sign_flag = _jxr_rbitstream_uint1(str);
            if (sign_flag)
                coeff = -coeff;
        }
    }

    return coeff;
}

static void r_PredCBP(jxr_image_t image, int*diff_cbp,
                      unsigned tx, unsigned ty,
                      unsigned mx, unsigned my)
{
    int idx;

    if (_jxr_InitContext(image, tx, ty, mx, my)) {
        _jxr_InitializeCBPModel(image);
    }

    assert(my == (unsigned)image->cur_my);
    switch (image->use_clr_fmt) {
        case 1: /*YUV420*/
            MACROBLK_CUR_HPCBP(image, 0, tx, mx)
                = _jxr_PredCBP444(image, diff_cbp, 0, tx, mx, my);
            MACROBLK_CUR_HPCBP(image, 1, tx, mx)
                = _jxr_PredCBP420(image, diff_cbp, 1, tx, mx, my);
            MACROBLK_CUR_HPCBP(image, 2, tx, mx)
                = _jxr_PredCBP420(image, diff_cbp, 2, tx, mx, my);
            DEBUG(" PredCBP: Predicted HPCBP[ch=0]: 0x%04x (YUV420)\n",
                MACROBLK_CUR_HPCBP(image, 0, tx, mx));
            DEBUG(" PredCBP: Predicted HPCBP[ch=1]: 0x%04x (YUV420)\n",
                MACROBLK_CUR_HPCBP(image, 1, tx, mx));
            DEBUG(" PredCBP: Predicted HPCBP[ch=2]: 0x%04x (YUV420)\n",
                MACROBLK_CUR_HPCBP(image, 2, tx, mx));
            break;
        case 2: /*YUV422*/
            MACROBLK_CUR_HPCBP(image, 0, tx, mx)
                = _jxr_PredCBP444(image, diff_cbp, 0, tx, mx, my);
            MACROBLK_CUR_HPCBP(image, 1, tx, mx)
                = _jxr_PredCBP422(image, diff_cbp, 1, tx, mx, my);
            MACROBLK_CUR_HPCBP(image, 2, tx, mx)
                = _jxr_PredCBP422(image, diff_cbp, 2, tx, mx, my);
            DEBUG(" PredCBP: Predicted HPCBP[ch=0]: 0x%04x (YUV422)\n",
                MACROBLK_CUR_HPCBP(image, 0, tx, mx));
            DEBUG(" PredCBP: Predicted HPCBP[ch=1]: 0x%04x (YUV422)\n",
                MACROBLK_CUR_HPCBP(image, 1, tx, mx));
            DEBUG(" PredCBP: Predicted HPCBP[ch=2]: 0x%04x (YUV422)\n",
                MACROBLK_CUR_HPCBP(image, 2, tx, mx));
            break;
        default:
            for (idx = 0; idx<image->num_channels; idx += 1) {
                MACROBLK_CUR_HPCBP(image, idx, tx, mx)
                    = _jxr_PredCBP444(image, diff_cbp, idx, tx, mx, my);
                DEBUG(" PredCBP: Predicted HPCBP[ch=%d]: 0x%04x\n",
                    idx, MACROBLK_CUR_HPCBP(image, idx, tx, mx));
            }
            break;
    }
}

static void AdaptiveHPScan(jxr_image_t image, int hpinput_n[],
                           int i, int value, int MBHPMode)
{
    assert(i > 0);

    if (MBHPMode == 1) {
        int k = image->hipass_ver_scanorder[i-1];
        image->hipass_ver_scantotals[i-1] += 1;
        assert(k < 16);
        hpinput_n[k] = value;

        if ((i>1) && (image->hipass_ver_scantotals[i-1] > image->hipass_ver_scantotals[i-2])) {
            SWAP(image->hipass_ver_scantotals[i-1], image->hipass_ver_scantotals[i-2]);
            SWAP(image->hipass_ver_scanorder[i-1], image->hipass_ver_scanorder[i-2]);
        }
    } else {
        int k = image->hipass_hor_scanorder[i-1];
        image->hipass_hor_scantotals[i-1] += 1;
        assert(k < 16);
        hpinput_n[k] = value;

        if ((i>1) && (image->hipass_hor_scantotals[i-1] > image->hipass_hor_scantotals[i-2])) {
            SWAP(image->hipass_hor_scantotals[i-1], image->hipass_hor_scantotals[i-2]);
            SWAP(image->hipass_hor_scanorder[i-1], image->hipass_hor_scanorder[i-2]);
        }
    }
}

/*
* For each block within the macroblk, there are 15 HP values and the
* DECODE_BLOCK_ADAPTIVE function is called to collect those values.
*/
static int r_DECODE_BLOCK_ADAPTIVE(jxr_image_t image, struct rbitstream*str,
                                   unsigned tx, unsigned mx,
                                   int cbp_flag, int chroma_flag,
                                   int channel, int block, int mbhp_pred_mode,
                                   unsigned model_bits)
{
    int RLCoeffs[32] = {0};

    int num_nonzero = 0;
    if (cbp_flag) {
        int idx, k;
        int hpinput[16];
        for (k = 0 ; k < 16 ; k += 1)
            hpinput[k] = 0;

        num_nonzero = r_DECODE_BLOCK(image, str, chroma_flag, RLCoeffs, 2/*HP*/, 1);

        for (idx = 0, k = 1 ; idx < num_nonzero ; idx += 1) {
            assert(idx < 16);
            k += RLCoeffs[idx*2];
            if (k >= 16) {
                DEBUG("ERROR: r_DECODE_BLOCK returned bogus RLCoeffs table. ch=%d, tx=%u, mx=%u, k=%d\n",
                    channel, tx, mx, k);
                for (idx = 0 ; idx < num_nonzero ; idx += 1) {
                    DEBUG(" : RLCoeffs[%d] = %d\n", idx*2, RLCoeffs[idx*2]);
                    DEBUG(" : RLCoeffs[%d] = 0x%x\n", idx*2+1, RLCoeffs[idx*2+1]);
                }
                return JXR_EC_ERROR;
            }
            assert(k < 16);
            AdaptiveHPScan(image, hpinput, k, RLCoeffs[idx*2+1], mbhp_pred_mode);
            k += 1;
        }
#if defined(DETAILED_DEBUG)
        {
            DEBUG(" HP val[tx=%u, mx=%d, block=%d] ==", tx, mx, block);
            for (k = 1 ; k<16; k+=1) {
                DEBUG(" 0x%x", hpinput[k]);
            }
            DEBUG("\n");
            DEBUG(" adapted hor scan order (MBHPMode=%d) ==", mbhp_pred_mode);
            for (k = 0 ; k<15; k+=1) {
                DEBUG(" %2d", image->hipass_hor_scanorder[k]);
            }
            DEBUG("\n");
            DEBUG(" adapted hor scan totals ==");
            for (k = 0 ; k<15; k+=1) {
                DEBUG(" %2d", image->hipass_hor_scantotals[k]);
            }
            DEBUG("\n");
            DEBUG(" adapted ver scan order (MBHPMode=%d) ==", mbhp_pred_mode);
            for (k = 0 ; k<15; k+=1) {
                DEBUG(" %2d", image->hipass_ver_scanorder[k]);
            }
            DEBUG("\n");
            DEBUG(" adapted ver scan totals ==");
            for (k = 0 ; k<15; k+=1) {
                DEBUG(" %2d", image->hipass_ver_scantotals[k]);
            }
            DEBUG("\n");
        }
#endif
        if (SKIP_HP_DATA(image)) {
            for (idx = 1; idx < 16; idx += 1)
                MACROBLK_CUR_HP(image, channel, tx, mx, block, idx-1) = 0;
        } else {
            for (idx = 1; idx < 16; idx += 1)
                MACROBLK_CUR_HP(image, channel, tx, mx, block, idx-1) = hpinput[idx] << model_bits;
        }
    }

    return num_nonzero;
}



static void r_DECODE_FLEX(jxr_image_t image, struct rbitstream*str,
                          unsigned tx, unsigned mx,
                          int ch, unsigned block, unsigned k,
                          unsigned flexbits)
{
    /* NOTE: The model_bits shift was already applied, when the HP
    value was first parsed. */
    int coeff = MACROBLK_CUR_HP(image, ch, tx, mx, block, k);

    int flex_ref = _jxr_rbitstream_uintN(str, flexbits);
    DEBUG(" DECODE_FLEX: coeff=0x%08x, flex_ref=0x%08x\n", coeff, flex_ref);
    if (coeff > 0) {
        coeff += flex_ref << image->trim_flexbits;
    } else if (coeff < 0) {
        coeff -= flex_ref << image->trim_flexbits;
    } else {
        if (flex_ref != 0 && _jxr_rbitstream_uint1(str))
            coeff = (-flex_ref) << image->trim_flexbits;
        else
            coeff = (+flex_ref) << image->trim_flexbits;
    }

    if (! SKIP_FLEX_DATA(image))
        MACROBLK_CUR_HP(image, ch, tx, mx, block, k) = coeff;
}

static void r_BLOCK_FLEXBITS(jxr_image_t image, struct rbitstream*str,
                             unsigned tx, unsigned /*ty*/,
                             unsigned mx, unsigned /*my*/,
                             unsigned ch, unsigned bl, unsigned model_bits)
{
    const int transpose444 [16] = {0, 4, 8,12,
        1, 5, 9,13,
        2, 6,10,14,
        3, 7,11,15 };
    unsigned flexbits_left = model_bits;
    if (image->trim_flexbits > flexbits_left)
        flexbits_left = 0;
    else
        flexbits_left -= image->trim_flexbits;

    DEBUG(" BLOCK_FLEXBITS: flexbits_left=%u (model=%u, trim=%u) block=%u bitpos=%zu\n",
        flexbits_left, model_bits, image->trim_flexbits, bl, _jxr_rbitstream_bitpos(str));
    if (flexbits_left > 0) {
        int idx;
        for (idx = 1; idx < 16; idx += 1) {
            int idx_trans = transpose444[idx];
            r_DECODE_FLEX(image, str, tx, mx, ch, bl, idx_trans-1, flexbits_left);
        }
    }
    DEBUG(" BLOCK_FLEXBITS done\n");
}

static int r_calculate_mbhp_mode(jxr_image_t image, int tx, int mx)
{
    long strength_hor = 0;
    long strength_ver = 0;
    const long orientation_weight = 4;

    /* Add up the LP magnitudes along the top edge */
    strength_hor += abs(MACROBLK_CUR_LP(image, 0, tx, mx, 0));
    strength_hor += abs(MACROBLK_CUR_LP(image, 0, tx, mx, 1));
    strength_hor += abs(MACROBLK_CUR_LP(image, 0, tx, mx, 2));

    /* Add up the LP magnitudes along the left edge */
    strength_ver += abs(MACROBLK_CUR_LP(image, 0, tx, mx, 3));
    strength_ver += abs(MACROBLK_CUR_LP(image, 0, tx, mx, 7));
    strength_ver += abs(MACROBLK_CUR_LP(image, 0, tx, mx, 11));

    switch (image->use_clr_fmt) {
        case 0: /*YONLY*/
        case 6: /*NCOMPONENT*/
            break;
        case 3: /*YUV444*/
        case 4: /*YUVK */
            strength_hor += abs(MACROBLK_CUR_LP(image, 1, tx, mx, 0));
            strength_hor += abs(MACROBLK_CUR_LP(image, 2, tx, mx, 0));
            strength_ver += abs(MACROBLK_CUR_LP(image, 1, tx, mx, 3));
            strength_ver += abs(MACROBLK_CUR_LP(image, 2, tx, mx, 3));
            break;
        case 2: /*YUV422*/
            strength_hor += abs(MACROBLK_CUR_LP(image, 1, tx, mx, 0));
            strength_hor += abs(MACROBLK_CUR_LP(image, 2, tx, mx, 0));
            strength_ver += abs(MACROBLK_CUR_LP(image, 1, tx, mx, 1));
            strength_ver += abs(MACROBLK_CUR_LP(image, 2, tx, mx, 1));
            strength_hor += abs(MACROBLK_CUR_LP(image, 1, tx, mx, 4));
            strength_hor += abs(MACROBLK_CUR_LP(image, 2, tx, mx, 4));
            strength_ver += abs(MACROBLK_CUR_LP(image, 1, tx, mx, 5));
            strength_ver += abs(MACROBLK_CUR_LP(image, 2, tx, mx, 5));
            break;
        case 1: /*YUV420*/
            strength_hor += abs(MACROBLK_CUR_LP(image, 1, tx, mx, 0));
            strength_hor += abs(MACROBLK_CUR_LP(image, 2, tx, mx, 0));
            strength_ver += abs(MACROBLK_CUR_LP(image, 1, tx, mx, 1));
            strength_ver += abs(MACROBLK_CUR_LP(image, 2, tx, mx, 1));
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


/*
* This function does HP coefficient propagation within a completed
* macroblock.
*/
void _jxr_propagate_hp_predictions(jxr_image_t image, int ch, unsigned tx, unsigned mx,
                                   int mbhp_pred_mode)
{
    if (mbhp_pred_mode == 0) { /* Prediction left to right */
        int idx;
        for (idx = 1 ; idx < 16 ; idx += 1) {
            if (idx%4 == 0)
                continue;
            CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,ch,tx,mx,idx, 3), MACROBLK_CUR_HP(image,ch,tx,mx,idx, 7), MACROBLK_CUR_HP(image,ch,tx,mx,idx, 11));
            MACROBLK_CUR_HP(image,ch,tx,mx,idx, 3) += MACROBLK_CUR_HP(image,ch,tx,mx,idx-1, 3);
            MACROBLK_CUR_HP(image,ch,tx,mx,idx, 7) += MACROBLK_CUR_HP(image,ch,tx,mx,idx-1, 7);
            MACROBLK_CUR_HP(image,ch,tx,mx,idx,11) += MACROBLK_CUR_HP(image,ch,tx,mx,idx-1,11);

#if defined(DETAILED_DEBUG)
            {
                int k;
                DEBUG(" HP val predicted(l)[ch=%d, tx=%u, mx=%d, block=%d] ==", ch, tx, mx, idx);
                for (k = 1 ; k<16; k+=1) {
                    DEBUG(" 0x%x", MACROBLK_CUR_HP(image,ch,tx,mx,idx,k-1));
                }
                DEBUG("\n");
            }
#endif
        }
    } else if (mbhp_pred_mode == 1) { /* Prediction top to bottom. */
        int idx;
        for (idx = 4 ; idx < 16 ; idx += 1) {
            CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,ch,tx,mx,idx, 0), MACROBLK_CUR_HP(image,ch,tx,mx,idx, 1), MACROBLK_CUR_HP(image,ch,tx,mx,idx, 2));
            MACROBLK_CUR_HP(image,ch,tx,mx,idx,0) += MACROBLK_CUR_HP(image,ch,tx,mx, idx-4,0);
            MACROBLK_CUR_HP(image,ch,tx,mx,idx,1) += MACROBLK_CUR_HP(image,ch,tx,mx, idx-4,1);
            MACROBLK_CUR_HP(image,ch,tx,mx,idx,2) += MACROBLK_CUR_HP(image,ch,tx,mx, idx-4,2);
#if defined(DETAILED_DEBUG)
            {
                int k;
                DEBUG(" HP val predicted(t)[ch=%d, tx=%u, mx=%d, block=%d] ==", ch, tx, mx, idx);
                for (k = 1 ; k<16; k+=1) {
                    DEBUG(" 0x%x", MACROBLK_CUR_HP(image,ch,tx,mx,idx,k-1));
                }
                DEBUG("\n");
            }
#endif
        }
    }

    switch (image->use_clr_fmt) {
        case 1:/*YUV420*/
            assert(ch == 0);
            if (mbhp_pred_mode == 0) {
                int idx;
                for (idx = 1 ; idx <= 3 ; idx += 2) {
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,1,tx,mx,idx, 3), MACROBLK_CUR_HP(image,1,tx,mx,idx, 7), MACROBLK_CUR_HP(image,1,tx,mx,idx, 11));
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,2,tx,mx,idx, 3), MACROBLK_CUR_HP(image,2,tx,mx,idx, 7), MACROBLK_CUR_HP(image,2,tx,mx,idx, 11));
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,3) += MACROBLK_CUR_HP(image,1,tx,mx,idx-1,3);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,3) += MACROBLK_CUR_HP(image,2,tx,mx,idx-1,3);
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,7) += MACROBLK_CUR_HP(image,1,tx,mx,idx-1,7);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,7) += MACROBLK_CUR_HP(image,2,tx,mx,idx-1,7);
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,11)+= MACROBLK_CUR_HP(image,1,tx,mx,idx-1,11);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,11)+= MACROBLK_CUR_HP(image,2,tx,mx,idx-1,11);
#if defined(DETAILED_DEBUG)
                    int k;
                    DEBUG(" HP val predicted(l)[ch=1, tx=%u, mx=%d, block=%d] ==", tx, mx, idx);
                    for (k = 1 ; k<16; k+=1) {
                        DEBUG(" 0x%x", MACROBLK_CUR_HP(image,1,tx,mx,idx,k-1));
                    }
                    DEBUG("\n");
                    DEBUG(" HP val predicted(l)[ch=2, tx=%u, mx=%d, block=%d] ==", tx, mx, idx);
                    for (k = 1 ; k<16; k+=1) {
                        DEBUG(" 0x%x", MACROBLK_CUR_HP(image,2,tx,mx,idx,k-1));
                    }
                    DEBUG("\n");
#endif
                }
            } else if (mbhp_pred_mode == 1) {
                int idx;
                for (idx = 2; idx <= 3 ; idx += 1) {
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,1,tx,mx,idx, 0), MACROBLK_CUR_HP(image,1,tx,mx,idx, 1), MACROBLK_CUR_HP(image,1,tx,mx,idx, 2));
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,2,tx,mx,idx, 0), MACROBLK_CUR_HP(image,2,tx,mx,idx, 1), MACROBLK_CUR_HP(image,2,tx,mx,idx, 2));
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,0) += MACROBLK_CUR_HP(image,1,tx,mx,idx-2,0);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,0) += MACROBLK_CUR_HP(image,2,tx,mx,idx-2,0);
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,1) += MACROBLK_CUR_HP(image,1,tx,mx,idx-2,1);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,1) += MACROBLK_CUR_HP(image,2,tx,mx,idx-2,1);
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,2) += MACROBLK_CUR_HP(image,1,tx,mx,idx-2,2);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,2) += MACROBLK_CUR_HP(image,2,tx,mx,idx-2,2);
#if defined(DETAILED_DEBUG)
                    int k;
                    DEBUG(" HP val predicted(t)[ch=1, tx=%u, mx=%d, block=%d] ==", tx, mx, idx);
                    for (k = 1 ; k<16; k+=1) {
                        DEBUG(" 0x%x", MACROBLK_CUR_HP(image,1,tx,mx,idx,k-1));
                    }
                    DEBUG("\n");
                    DEBUG(" HP val predicted(t)[ch=2, tx=%u, mx=%d, block=%d] ==", tx, mx, idx);
                    for (k = 1 ; k<16; k+=1) {
                        DEBUG(" 0x%x", MACROBLK_CUR_HP(image,2,tx,mx,idx,k-1));
                    }
                    DEBUG("\n");
#endif
                }
            }
            break;

        case 2:/*YUV422*/
            assert(ch == 0);
            if (mbhp_pred_mode == 0) {
                int idx;
                for (idx = 1 ; idx <= 7 ; idx += 2) {
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,1,tx,mx,idx, 3), MACROBLK_CUR_HP(image,1,tx,mx,idx, 7), MACROBLK_CUR_HP(image,1,tx,mx,idx, 11));
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,2,tx,mx,idx, 3), MACROBLK_CUR_HP(image,2,tx,mx,idx, 7), MACROBLK_CUR_HP(image,2,tx,mx,idx, 11));
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,3) += MACROBLK_CUR_HP(image,1,tx,mx,idx-1,3);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,3) += MACROBLK_CUR_HP(image,2,tx,mx,idx-1,3);
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,7) += MACROBLK_CUR_HP(image,1,tx,mx,idx-1,7);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,7) += MACROBLK_CUR_HP(image,2,tx,mx,idx-1,7);
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,11)+= MACROBLK_CUR_HP(image,1,tx,mx,idx-1,11);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,11)+= MACROBLK_CUR_HP(image,2,tx,mx,idx-1,11);
#if defined(DETAILED_DEBUG)
                    int k;
                    DEBUG(" HP val predicted(l)[ch=1, tx=%u, mx=%d, block=%d] ==", tx, mx, idx);
                    for (k = 1 ; k<16; k+=1) {
                        DEBUG(" 0x%x", MACROBLK_CUR_HP(image,1,tx,mx,idx,k-1));
                    }
                    DEBUG("\n");
                    DEBUG(" HP val predicted(l)[ch=2, tx=%u, mx=%d, block=%d] ==", tx, mx, idx);
                    for (k = 1 ; k<16; k+=1) {
                        DEBUG(" 0x%x", MACROBLK_CUR_HP(image,2,tx,mx,idx,k-1));
                    }
                    DEBUG("\n");
#endif
                }
            } else if (mbhp_pred_mode == 1) {
                int idx;
                for (idx = 2; idx <= 7 ; idx += 1) {
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,1,tx,mx,idx, 0), MACROBLK_CUR_HP(image,1,tx,mx,idx, 1), MACROBLK_CUR_HP(image,1,tx,mx,idx, 2));
                    CHECK3(image->lwf_test, MACROBLK_CUR_HP(image,2,tx,mx,idx, 0), MACROBLK_CUR_HP(image,2,tx,mx,idx, 1), MACROBLK_CUR_HP(image,2,tx,mx,idx, 2));
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,0) += MACROBLK_CUR_HP(image,1,tx,mx,idx-2,0);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,0) += MACROBLK_CUR_HP(image,2,tx,mx,idx-2,0);
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,1) += MACROBLK_CUR_HP(image,1,tx,mx,idx-2,1);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,1) += MACROBLK_CUR_HP(image,2,tx,mx,idx-2,1);
                    MACROBLK_CUR_HP(image,1,tx,mx,idx,2) += MACROBLK_CUR_HP(image,1,tx,mx,idx-2,2);
                    MACROBLK_CUR_HP(image,2,tx,mx,idx,2) += MACROBLK_CUR_HP(image,2,tx,mx,idx-2,2);
#if defined(DETAILED_DEBUG)
                    int k;
                    DEBUG(" HP val predicted(t)[ch=1, tx=%u, mx=%d, block=%d] ==", tx, mx, idx);
                    for (k = 1 ; k<16; k+=1) {
                        DEBUG(" 0x%x", MACROBLK_CUR_HP(image,1,tx,mx,idx,k-1));
                    }
                    DEBUG("\n");
                    DEBUG(" HP val predicted(t)[ch=2, tx=%u, mx=%d, block=%d] ==", tx, mx, idx);
                    for (k = 1 ; k<16; k+=1) {
                        DEBUG(" 0x%x", MACROBLK_CUR_HP(image,2,tx,mx,idx,k-1));
                    }
                    DEBUG("\n");
#endif
                }
            }
            break;

        default:
            break;
    }
}


/*
* Code IS_DC_YUV
* 10 0
* 001 1
* 00001 2
* 0001 3
* 11 4
* 010 5
* 00000 6
* 011 7
*/
static int get_is_dc_yuv(struct rbitstream*str)
{
    if (_jxr_rbitstream_uint1(str) == 1) { /* 1... */
        if (_jxr_rbitstream_uint1(str) == 1) /* 11 */
            return 4;
        else /* 10 */
            return 0;
    } 
    else { 
        switch (_jxr_rbitstream_uint2(str)) { /* 1... */
            case 0: /* 000... */
                if (_jxr_rbitstream_uint1(str) == 1) /* 0001 */
                    return 3;
                else if (_jxr_rbitstream_uint1(str) == 1) /* 00001 */
                    return 2;
                else /* 00000 */
                    return 6;
            case 1: /* 001 */
                return 1;

            case 2: /* 010 */
                return 5;

            case 3: /* 011 */
                return 7;
        }
    }

    assert(0); /* Should not get here. */
    return -1;
}

/*
* table0 table1 value
* 1 1 0
* 01 000 1
* 001 001 2
* 0000 010 3
* 0001 011 4
*/
static int get_num_cbp(struct rbitstream*str, struct adaptive_vlc_s*vlc)
{
    assert(vlc->table < 2);

    if (_jxr_rbitstream_uint1(str) == 1)
        return 0;

    if (vlc->table == 0) {
        if (_jxr_rbitstream_uint1(str) == 1)
            return 1;
        if (_jxr_rbitstream_uint1(str) == 1)
            return 2;
        if (_jxr_rbitstream_uint1(str) == 1)
            return 4;
        else
            return 3;
    } else {
        uint8_t tmp = _jxr_rbitstream_uint2(str);
        return tmp + 1;
    }
}

static int get_num_blkcbp(jxr_image_t image, struct rbitstream*str,
struct adaptive_vlc_s*vlc)
{
    switch (image->use_clr_fmt) {
        case 0: /*YONLY*/
        case 4: /*YUVK*/
        case 6: /*NCOMPONENT*/
            /*
            * table0 table1 value
            * 1 1 0
            * 01 000 1
            * 001 001 2
            * 0000 010 3
            * 0001 011 4
            *
            * NOTE that this is exactly the same table as for
            * NUM_CBP above.
            */
            return get_num_cbp(str, vlc);

        default:
            /*
            * table0 table1 value
            * 010 1 0
            * 00000 001 1
            * 0010 010 2
            * 00001 0001 3
            * 00010 000001 4
            * 1 011 5
            * 011 00001 6
            * 00011 0000000 7
            * 0011 0000001 8
            */
            if (vlc->table == 0) {
                static const unsigned char codeb[32] = {
                    5, 5, 5, 5, 4, 4, 4, 4,
                    3, 3, 3, 3, 3, 3, 3, 3,
                    1, 1, 1, 1, 1, 1, 1, 1, /* 1xxxx */
                    1, 1, 1, 1, 1, 1, 1, 1
                };
                static const signed char codev[32] = {
                    1, 3, 4, 7, 2, 2, 8, 8,
                    0, 0, 0, 0, 6, 6, 6, 6,
                    5, 5, 5, 5, 5, 5, 5, 5,
                    5, 5, 5, 5, 5, 5, 5, 5
                };
                return _jxr_rbitstream_intE(str, 5, codeb, codev);
            } else {
                static const unsigned char codeb[64] = {
                    6, 6, 5, 5, 4, 4, 4, 4,
                    3, 3, 3, 3, 3, 3, 3, 3,
                    3, 3, 3, 3, 3, 3, 3, 3,
                    3, 3, 3, 3, 3, 3, 3, 3,
                    1, 1, 1, 1, 1, 1, 1, 1,
                    1, 1, 1, 1, 1, 1, 1, 1,
                    1, 1, 1, 1, 1, 1, 1, 1,
                    1, 1, 1, 1, 1, 1, 1, 1
                };
                static const signed char codev[64] = {
                    7, 4, 6, 6, 3, 3, 3, 3,
                    1, 1, 1, 1, 1, 1, 1, 1,
                    2, 2, 2, 2, 2, 2, 2, 2,
                    5, 5, 5, 5, 5, 5, 5, 5,
                    0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0
                };
                int tmp = _jxr_rbitstream_intE(str, 6, codeb, codev);
                if (tmp == 7) tmp += _jxr_rbitstream_uint1(str);
                return tmp;
            }
    }
}



/*
* value table0 table1
* 0 01 1
* 1 10 01
* 2 11 001
* 3 001 0001
* 4 0001 00001
* 5 00000 000000
* 6 00001 000001
*/
static const unsigned char abslevel_code0b[64] = {
    5, 5, 5, 5, 4, 4, 4, 4, /* 00000x, 00001x, 0001xx */
    3, 3, 3, 3, 3, 3, 3, 3, /* 001xxx */
    2, 2, 2, 2, 2, 2, 2, 2, /* 01xxxx */
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, /* 10xxxx */
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, /* 11xxxx */
    2, 2, 2, 2, 2, 2, 2, 2
};
static const signed char abslevel_code0v[64] = {
    5, 5, 6, 6, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2
};

static const unsigned char abslevel_code1b[64] = {
    6, 6, 5, 5, 4, 4, 4, 4, /* 000000, 000001, 00001x, 0001xx */
    3, 3, 3, 3, 3, 3, 3, 3, /* 001xxx */
    2, 2, 2, 2, 2, 2, 2, 2, /* 01xxxx */
    2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, /* 1xxxxx */
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1
};
static const signed char abslevel_code1v[64] = {
    5, 6, 4, 4, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

static int dec_abslevel_index(jxr_image_t image, struct rbitstream*str, int vlc_select)
{
    const unsigned char*codeb = image->vlc_table[vlc_select].table? abslevel_code1b :abslevel_code0b;
    const signed char*codev = image->vlc_table[vlc_select].table? abslevel_code1v : abslevel_code0v;

    return _jxr_rbitstream_intE(str, 6, codeb, codev);
}

static int dec_cbp_yuv_lp1(jxr_image_t image, struct rbitstream*str)
{
    static const unsigned char codeb[16] = {
        1, 1, 1, 1,
        1, 1, 1, 1,
        3, 3, 4, 4,
        4, 4, 4, 4 };
        static const signed char codev[16] = {
            0, 0, 0, 0,
            0, 0, 0, 0,
            1, 1, 2, 3,
            4, 5, 6, 7 };

            switch (image->use_clr_fmt) {
                case 3: /* YUV444 */
                    return _jxr_rbitstream_intE(str, 4, codeb, codev);
                case 1: /* YUV420 */
                case 2: /* YUV422 */
                    if (_jxr_rbitstream_uint1(str) == 0) {
                        return 0;

                    } else if (_jxr_rbitstream_uint1(str) == 0) {
                        return 1;

                    } else if (_jxr_rbitstream_uint1(str) == 0) {
                        return 2;

                    } else {
                        return 3;
                    }
                default:
                    assert(0);
                    return 0;
            }
}

/*
* Bits Value
* 1 0
* 01 1
* 00 2
*/
static int get_value_012(struct rbitstream*str)
{
    if (_jxr_rbitstream_uint1(str) == 1)
        return 0;
    else if ( _jxr_rbitstream_uint1(str) == 1)
        return 1;
    else
        return 2;
}

/*
* Bits Value
* 1 0
* 01 1
* 000 2
* 001 3
*/
static int get_num_ch_blk(struct rbitstream*str)
{
    if (_jxr_rbitstream_uint1(str) == 1)
        return 0;
    if (_jxr_rbitstream_uint1(str) == 1)
        return 1;
    if (_jxr_rbitstream_uint1(str) == 1)
        return 3;
    else
        return 2;
}

/*
* $Log: r_parse.c,v $
* Revision 1.41 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.40 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.39 2008/03/24 18:06:56 steve
* Imrpove DEBUG messages around quantization.
*
* Revision 1.38 2008/03/21 18:30:21 steve
* Get HP Prediction right for YUVK (CMYK)
*
* Revision 1.37 2008/03/20 22:39:41 steve
* Fix various debug prints of QP data.
*
* Revision 1.36 2008/03/17 21:48:55 steve
* CMYK decode support
*
* Revision 1.35 2008/03/13 21:23:27 steve
* Add pipeline step for YUV420.
*
* Revision 1.34 2008/03/13 00:07:22 steve
* Encode HP of YUV422
*
* Revision 1.33 2008/03/06 22:47:39 steve
* Clean up parsing/encoding of QP counts
*
* Revision 1.32 2008/03/06 02:05:48 steve
* Distributed quantization
*
* Revision 1.31 2008/03/05 04:04:30 steve
* Clarify constraints on USE_DC_QP in image plane header.
*
* Revision 1.30 2008/03/05 00:31:17 steve
* Handle UNIFORM/IMAGEPLANE_UNIFORM compression.
*
* Revision 1.29 2008/02/28 18:50:31 steve
* Portability fixes.
*
* Revision 1.28 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.27 2008/02/23 01:55:51 steve
* CBP REFINE is more complex when CHR is involved.
*
* Revision 1.26 2008/02/22 23:01:33 steve
* Compress macroblock HP CBP packets.
*
* Revision 1.25 2008/01/01 01:07:26 steve
* Add missing HP prediction.
*
* Revision 1.24 2007/12/30 00:16:00 steve
* Add encoding of HP values.
*
* Revision 1.23 2007/12/13 18:01:09 steve
* Stubs for HP encoding.
*
* Revision 1.22 2007/12/12 00:37:33 steve
* Cleanup some debug messages.
*
* Revision 1.21 2007/12/06 23:12:41 steve
* Stubs for LP encode operations.
*
* Revision 1.20 2007/12/06 17:54:09 steve
* UpdateModelMB dump details.
*
* Revision 1.19 2007/11/26 01:47:15 steve
* Add copyright notices per MS request.
*
* Revision 1.18 2007/11/21 00:34:30 steve
* Rework spatial mode tile macroblock shuffling.
*
* Revision 1.17 2007/11/20 00:05:47 steve
* Complex handling of mbhp_pred_mode in frequency dmoe.
*
* Revision 1.16 2007/11/19 18:22:34 steve
* Skip ESCaped FLEXBITS tiles.
*
* Revision 1.15 2007/11/16 20:03:57 steve
* Store MB Quant, not qp_index.
*
* Revision 1.14 2007/11/16 17:33:24 steve
* Do HP prediction after FLEXBITS frequency tiles.
*
* Revision 1.13 2007/11/16 00:29:05 steve
* Support FREQUENCY mode HP and FLEXBITS
*
* Revision 1.12 2007/11/14 23:56:17 steve
* Fix TILE ordering, using seeks, for FREQUENCY mode.
*
* Revision 1.11 2007/11/14 00:17:26 steve
* Fix parsing of QP indices.
*
* Revision 1.10 2007/11/13 03:27:23 steve
* Add Frequency mode LP support.
*
* Revision 1.9 2007/11/12 23:21:55 steve
* Infrastructure for frequency mode ordering.
*
* Revision 1.8 2007/11/08 19:38:38 steve
* Get stub DCONLY compression to work.
*
* Revision 1.7 2007/11/01 21:09:40 steve
* Multiple rows of tiles.
*
* Revision 1.6 2007/10/30 21:32:46 steve
* Support for multiple tile columns.
*
* Revision 1.5 2007/09/08 01:01:43 steve
* YUV444 color parses properly.
*
* Revision 1.4 2007/07/30 23:09:57 steve
* Interleave FLEXBITS within HP block.
*
* Revision 1.3 2007/07/24 20:56:28 steve
* Fix HP prediction and model bits calculations.
*
* Revision 1.2 2007/06/07 18:53:06 steve
* Parse HP coeffs that are all 0.
*
* Revision 1.1 2007/06/06 17:19:12 steve
* Introduce to CVS.
*
*/

