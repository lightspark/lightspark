
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
#pragma comment (user,"$Id: w_emit.c,v 1.25 2008/03/24 18:06:56 steve Exp $")
#else
#ident "$Id: w_emit.c,v 1.25 2008/03/24 18:06:56 steve Exp $"
#endif

# include "jxr_priv.h"
# include <stdlib.h>
# include <assert.h>

void initialize_index_table(jxr_image_t image);

static int w_image_header(jxr_image_t image, struct wbitstream*str);
static int w_image_plane_header(jxr_image_t image, struct wbitstream*str, int alpha);
static void w_INDEX_TABLE(jxr_image_t image, struct wbitstream*str);
static uint64_t w_PROFILE_LEVEL_INFO(jxr_image_t image, struct wbitstream*str, uint64_t bytes);
static void w_TILE(jxr_image_t image, struct wbitstream*str);

static int short_header_ok(jxr_image_t image);
static int need_windowing_flag(jxr_image_t image);
static int need_trim_flexbits_flag(jxr_image_t image);


static void w_BLOCK_FLEXBITS(jxr_image_t image, struct wbitstream*str,
                             unsigned tx, unsigned ty,
                             unsigned mx, unsigned my,
                             unsigned ch, unsigned bl, unsigned model_bits);
static void w_DEC_DC(jxr_image_t image, struct wbitstream*str,
                     int model_bits, int chroma_flag, int is_dc_ch,
                     int32_t dc_val);
static void w_DECODE_ABS_LEVEL(jxr_image_t image, struct wbitstream*str,
                               int band, int chroma_flag, uint32_t level);
static void w_DECODE_BLOCK(jxr_image_t image, struct wbitstream*str, int band, int chroma_flag,
                           const int RLCoeffs[32], int num_non_zero);
static void w_DECODE_FIRST_INDEX(jxr_image_t image, struct wbitstream*str,
                                 int chroma_flag, int band, int index_code);
static void w_DECODE_INDEX(jxr_image_t image, struct wbitstream*str,
                           int location, int chroma_flag, int band, int context,
                           int index_code);
static void w_DECODE_RUN(jxr_image_t image, struct wbitstream*str, int max_run, int run);
static int w_DECODE_BLOCK_ADAPTIVE(jxr_image_t image, struct wbitstream*str,
                                   unsigned tx, unsigned mx,
                                   int cbp_flag, int chroma_flag,
                                   int channel, int block, int mbhp_pred_mode,
                                   unsigned model_bits);
static void w_REFINE_CBP(jxr_image_t image, struct wbitstream*str, int cbp_block_mask);
static void w_REFINE_CBP_CHR(jxr_image_t image, struct wbitstream*str, int cbp_block_mask);
static void refine_cbp_chr422(jxr_image_t image, struct wbitstream*str, int diff_cbp, int block);


static const int transpose420[4] = {0, 2,
1, 3 };
static const int transpose422[8] = {0, 2, 1, 3, 4, 6, 5, 7};


void initialize_index_table(jxr_image_t image)
{
    int num_index_table_entries;

    if (FREQUENCY_MODE_CODESTREAM_FLAG(image) == 0 /* SPATIAL MODE */) {
        num_index_table_entries = image->tile_columns * image->tile_rows;
    }
    else 
    {
        num_index_table_entries = image->tile_columns * image->tile_rows;
        switch (image->bands_present_of_primary) {
            case 4: /* ISOLATED */
                num_index_table_entries *= 4;
                break;
            default:
                num_index_table_entries *= (4 - image->bands_present_of_primary);
                break;
        }
    }
    image->tile_index_table_length = num_index_table_entries;

    assert(image->tile_index_table == 0);
    image->tile_index_table = (int64_t*)jpegxr_calloc(num_index_table_entries, sizeof(int64_t));
    DEBUG(" INDEX_TABLE has %d table entries\n", num_index_table_entries);
}

static int fill_in_image_defaults(jxr_image_t image)
{
    if (image->tile_columns == 0)
        image->tile_columns = 1;
    if (image->tile_rows == 0)
        image->tile_rows = 1;

    if (image->tile_columns > 1 || image->tile_rows > 1)
        image->header_flags1 |= 0x80; /* TILING FLAG */   

    if (short_header_ok(image))
        image->header_flags2 |= 0x80; /* SHORT_HEADER FLAG */
    else
        image->header_flags2 &= ~0x80;

    if (need_windowing_flag(image))
        image->header_flags2 |= 0x20; /* WINDOWING_FLAG */
    
    image->window_extra_bottom = 15 - ((image->height1 + image->window_extra_top) % 16);
    image->extended_height = image->height1 + 1 + image->window_extra_top + image->window_extra_bottom;
    image->window_extra_right = 15 - ((image->width1 + image->window_extra_left) % 16);
    image->extended_width = image->width1 + 1 + image->window_extra_left + image->window_extra_right;

    if (need_trim_flexbits_flag(image))
        image->header_flags2 |= 0x10; /* TRIM_FLEXBITS_FLAG */
    else
        image->header_flags2 &= ~0x10;

    /* Test OUTPUT_CLR_FMT against size requirements */
    switch(image->output_clr_fmt) {
        case JXR_OCF_YUV420: /* YUV420 */
            assert(image->height1 & 0x1);
            assert((image->window_extra_top & 0x1) == 0);
            assert((image->window_extra_bottom & 0x1) == 0);
        case JXR_OCF_YUV422: /* YUV422 */
            assert(image->width1 & 0x1);
            assert((image->window_extra_left & 0x1) == 0);
            assert((image->window_extra_right & 0x1) == 0);
            break;
		default:
			break;
    }

    /* Force scaling ON if we are using a subsampled color format. */
    switch (image->use_clr_fmt) {

        /* If external and internal formats are both YUV420 (or YUV422), don't change scaled_flag. 
           Otherwise, color format is subsampled(lossy) and scaled_flag should be set as 1. */
        case 1: /*YUV420*/
            if (OVERLAP_INFO(image) == 2)
                assert(image->extended_width >= 32);
            if (image->output_clr_fmt != JXR_OCF_YUV420) 
                image->scaled_flag = 1;
            break;
        case 2: /*YUV422*/
            if (OVERLAP_INFO(image) == 2)
                assert(image->extended_width >= 32);
            if (image->output_clr_fmt != JXR_OCF_YUV422) 
                image->scaled_flag = 1;
            break;
    }
    
    unsigned * temp_ptr, idx;
    temp_ptr = image->tile_column_width;
    image->tile_column_width = (unsigned*)jpegxr_calloc(2*image->tile_columns, sizeof(unsigned));
    for (idx = 0 ; idx < image->tile_columns ; idx++)
        image->tile_column_width[idx] = temp_ptr[idx];
    image->tile_column_position = image->tile_column_width + image->tile_columns;

    temp_ptr = image->tile_row_height;
    image->tile_row_height = (unsigned*)jpegxr_calloc(2*image->tile_rows, sizeof(unsigned));
    for (idx = 0 ; idx < image->tile_rows ; idx++)
        image->tile_row_height[idx] = temp_ptr[idx];
    image->tile_row_position = image->tile_row_height + image->tile_rows;

    if (TILING_FLAG(image)) {
        unsigned width_MB = EXTENDED_WIDTH_BLOCKS(image), height_MB = EXTENDED_HEIGHT_BLOCKS(image);
        unsigned min_width = 1, total_width = 0, min_height = 1, total_height = 0;

        if (image->tile_column_width[0] == 0) {
            total_width = 0;
            for ( idx = 0 ; idx < image->tile_columns - 1 ; idx++ ) {
                image->tile_column_width[idx] = width_MB / image->tile_columns;
                image->tile_column_position[idx] = total_width;
                total_width += image->tile_column_width[idx];
            }
            image->tile_column_width[image->tile_columns - 1] = width_MB - total_width;
            image->tile_column_position[image->tile_columns - 1] = total_width;
        }
        total_width = 0;

        if ((OVERLAP_INFO(image) == 2) && ((image->use_clr_fmt == 1/*YUV420*/) || (image->use_clr_fmt == 2/*YUV422*/)) && image->disableTileOverlapFlag)
            min_width = 2;
        for ( idx = 0 ; idx < image->tile_columns - 1 ; idx++ ) {
            if (image->tile_column_width[idx] < min_width) {
                DEBUG(" Tile %d width is below minimum width\n", idx);
                assert(0);
                break;
            }
            image->tile_column_position[idx] = total_width;
            total_width += image->tile_column_width[idx];
        }
        if ((total_width + min_width) > width_MB) {
            DEBUG(" Total specified tile width is above image width\n");
            assert(0);
        }
        image->tile_column_position[image->tile_columns - 1] = total_width;
        image->tile_column_width[image->tile_columns - 1] = (width_MB - total_width);

        if (image->tile_row_height[0] == 0) {
            total_height = 0;
            for ( idx = 0 ; idx < image->tile_rows - 1 ; idx++ ) {
                image->tile_row_height[idx] = height_MB / image->tile_rows;
                image->tile_row_position[idx] = total_height;
                total_height += image->tile_row_height[idx];
            }
            image->tile_row_height[image->tile_rows - 1] = height_MB - total_height;
            image->tile_row_position[image->tile_rows - 1] = total_height;
        }
        total_height = 0;

        for ( idx = 0 ; idx < image->tile_rows - 1 ; idx++ ) {
            if (image->tile_row_height[idx] < min_height) {
                DEBUG(" Tile %d height is below minimum height\n", idx);
                assert(0);
                break;
            }
            image->tile_row_position[idx] = total_height;
            total_height += image->tile_row_height[idx];
        }
        if ((total_height + min_height) > height_MB) {
            DEBUG(" Total specified tile height is above image height\n");
            assert(0);
        }
        image->tile_row_position[image->tile_rows - 1] = total_height;
        image->tile_row_height[image->tile_rows - 1] = (height_MB - total_height);

    } else {
        image->tile_column_width[0] = EXTENDED_WIDTH_BLOCKS(image);
        image->tile_column_position[0] = 0;

        image->tile_row_height[0] = EXTENDED_HEIGHT_BLOCKS(image);
        image->tile_row_position[0] = 0;
    }

    image->lwf_test = 0;

    _jxr_make_mbstore(image, 1);
    image->cur_my = -5;
    return 0;
}

int jxr_write_image_bitstream(jxr_image_t image
#ifdef JPEGXR_ADOBE_EXT
	, jxr_container_t container
#else //#ifdef JPEGXR_ADOBE_EXT
	, FILE*fd
#endif //#ifndef JPEGXR_ADOBE_EXT
)
{
    int rc, res = 0;

    struct wbitstream bits;
    _jxr_wbitstream_initialize(&bits
#ifndef JPEGXR_ADOBE_EXT
		, fd
#endif //#ifndef JPEGXR_ADOBE_EXT
	);

    /* Clean up the image structure in preparation for actually
    writing the image. This checks for and configures any
    values left to defaults, and checks for bogus settings. */
    rc = fill_in_image_defaults(image);
    if (rc < 0)
        return rc;

    /* Prepare index table storage */
    initialize_index_table(image);


    rc = w_image_header(image, &bits);
    assert(rc >= 0);

    rc = w_image_plane_header(image, &bits, 0);
    assert(rc >= 0);

    if (ALPHACHANNEL_FLAG(image)) {       
        unsigned char window_params[5];
        if (image->window_extra_top || image->window_extra_right) {
            window_params[0] = 1;
            window_params[1] = image->window_extra_top;
            window_params[2] = image->window_extra_left;
            window_params[3] = image->window_extra_bottom;
            window_params[4] = image->window_extra_right;
        }
        else {
            window_params[4] = window_params[3] = window_params[2] = window_params[1] = window_params[0] = 0;
        }

        image->alpha = jxr_create_image(image->width1 + 1, image->height1 + 1, window_params);

        *image->alpha = *image;
        image->alpha->strip[0].up4 = image->alpha->strip[0].up3 = image->alpha->strip[0].up2 =
            image->alpha->strip[0].up1 = image->alpha->strip[0].cur = NULL;
       
        jxr_set_INTERNAL_CLR_FMT(image->alpha, JXR_YONLY, 1);
        _jxr_make_mbstore(image->alpha, 1);
        image->alpha->dc_component_mode = image->alpha->lp_component_mode = image->alpha->hp_component_mode = JXR_CM_UNIFORM;
        image->alpha->primary = 0;
        image->alpha->cur_my = -5;

        rc = w_image_plane_header(image->alpha, &bits, 1);
        assert(rc >= 0);
    }

    if (INDEXTABLE_PRESENT_FLAG(image)) {
        struct wbitstream strCodedTiles;
#ifndef JPEGXR_ADOBE_EXT
        FILE*fdCodedTiles = fopen("codedtiles.tmp", "wb");
#endif //#ifndef JPEGXR_ADOBE_EXT
        _jxr_wbitstream_initialize(&strCodedTiles
#ifndef JPEGXR_ADOBE_EXT
			, fdCodedTiles
#endif //#ifndef JPEGXR_ADOBE_EXT
		);

        /* CODED_TILES() */
        w_TILE(image, &strCodedTiles);

        w_INDEX_TABLE(image, &bits);

        _jxr_wbitstream_flush(&strCodedTiles);
#ifndef JPEGXR_ADOBE_EXT
        fclose(fdCodedTiles);
#endif //#ifndef JPEGXR_ADOBE_EXT

        /* Profile / Level info */
        uint64_t subsequent_bytes = 4;
        _jxr_wbitstream_intVLW(&bits, subsequent_bytes);

        if (subsequent_bytes > 0) {
            uint64_t additional_bytes;
            additional_bytes = w_PROFILE_LEVEL_INFO(image, &bits, subsequent_bytes) ;

            uint64_t ibyte;
            for (ibyte = 0 ; ibyte < additional_bytes ; ibyte += 1)
                _jxr_wbitstream_uint8(&bits, 0); /* RESERVED_A_BYTE */
        }

        DEBUG("MARK HERE as the tile base. bitpos=%zu\n", _jxr_wbitstream_bitpos(&bits));
        _jxr_wbitstream_mark(&bits);

        struct rbitstream strCodedTilesRead
#ifdef JPEGXR_ADOBE_EXT
        (strCodedTiles.buffer(), strCodedTiles.len())
#endif //#ifndef JPEGXR_ADOBE_EXT
        ;
#ifndef JPEGXR_ADOBE_EXT
        FILE*fdCodedTilesRead = fopen("codedtiles.tmp", "rb");
#endif //#ifndef JPEGXR_ADOBE_EXT
        _jxr_rbitstream_initialize(&strCodedTilesRead
#ifndef JPEGXR_ADOBE_EXT
			, fdCodedTilesRead
#endif //#ifndef JPEGXR_ADOBE_EXT
		);

        size_t idx;
        for (idx = 0; idx < strCodedTiles.write_count; idx++) {
            _jxr_wbitstream_uint8(&bits, _jxr_rbitstream_uint8(&strCodedTilesRead));
        }
#ifndef JPEGXR_ADOBE_EXT
        fclose(fdCodedTilesRead);
        /* delete file associated with CodedTiles */
        remove("codedtiles.tmp");
#endif //#ifndef JPEGXR_ADOBE_EXT

    }
    else {
        /* Profile / Level info */
        uint64_t subsequent_bytes = 4;
        _jxr_wbitstream_intVLW(&bits, subsequent_bytes);

        if (subsequent_bytes > 0) {
            uint64_t additional_bytes;
            additional_bytes = w_PROFILE_LEVEL_INFO(image, &bits, subsequent_bytes) ;

            uint64_t ibyte;
            for (ibyte = 0 ; ibyte < additional_bytes ; ibyte += 1)
                _jxr_wbitstream_uint8(&bits, 0); /* RESERVED_A_BYTE */
        }

        DEBUG("MARK HERE as the tile base. bitpos=%zu\n", _jxr_wbitstream_bitpos(&bits));
        _jxr_wbitstream_mark(&bits);

        /* CODED_TILES() */
        w_TILE(image, &bits);
    }

    _jxr_wbitstream_flush(&bits);

#ifdef VERIFY_16BIT
    if(image->lwf_test == 0)
        DEBUG("Meets conditions for LONG_WORD_FLAG == 0!");
    else {
        DEBUG("Does not meet conditions for LONG_WORD_FLAG == 0!");
        if (LONG_WORD_FLAG(image) == 0)
            return JXR_EC_BADFORMAT;
    }
#endif

#ifdef JPEGXR_ADOBE_EXT
	container->wb.write(bits.buffer(),bits.len());
#endif //#ifdef JPEGXR_ADOBE_EXT

    return res;
}

#if defined(DETAILED_DEBUG)
static const char*bitdepth_names[16] = {
    "BD1WHITE1", "BD8", "BD16", "BD16S",
    "BD16F", "RESERVED5", "BD32S", "BD32F",
    "BD5", "BD10", "BD565", "RESERVED11"
    "RESERVED12", "RESERVED12", "RESERVED12","BD1BLACK1"
};

#endif

static int w_image_header(jxr_image_t image, struct wbitstream*str)
{
    const char GDI_SIG[] = "WMPHOTO\0";
    int res = 0;
    unsigned idx;

    /* GDI SIGNATURE */
    for (idx = 0 ; idx < 8 ; idx += 1) {
        _jxr_wbitstream_uint8(str, GDI_SIG[idx]);
    }

    DEBUG("START IMAGE_HEADER (bitpos=%zu)\n", _jxr_wbitstream_bitpos(str));

    _jxr_wbitstream_uint4(str, 1); /* VERSION_INFO */

    _jxr_wbitstream_uint1(str, image->disableTileOverlapFlag); /* HARD_TILING_FLAG */

    _jxr_wbitstream_uint3(str, 1); /* SUB_VERSION_INFO */

    DEBUG(" Flags group1=0x%02x\n", image->header_flags1);
    _jxr_wbitstream_uint8(str, image->header_flags1);

    DEBUG(" Flags group2=0x%02x\n", image->header_flags2);
    _jxr_wbitstream_uint8(str, image->header_flags2);

    DEBUG(" OUTPUT_CLR_FMT=%d\n", SOURCE_CLR_FMT(image));
    DEBUG(" OUTPUT_BITEDPTH=%d (%s)\n", SOURCE_BITDEPTH(image), bitdepth_names[SOURCE_BITDEPTH(image)]);
    _jxr_wbitstream_uint8(str, image->header_flags_fmt);

    if (SHORT_HEADER_FLAG(image)) {
        DEBUG(" SHORT_HEADER_FLAG=true\n");
        _jxr_wbitstream_uint16(str, image->width1);
        _jxr_wbitstream_uint16(str, image->height1);
    } else {
        DEBUG(" SHORT_HEADER_FLAG=false\n");
        _jxr_wbitstream_uint32(str, image->width1);
        _jxr_wbitstream_uint32(str, image->height1);
    }

    DEBUG(" Image dimensions: %u x %u\n", image->width1+1, image->height1+1);

    /* Write TILE geometry information, if there is TILING. */
    if (jxr_get_TILING_FLAG(image)) {

        DEBUG(" TILING %u columns, %u rows\n",
            image->tile_columns, image->tile_rows);

        _jxr_wbitstream_uint12(str, image->tile_columns -1);
        _jxr_wbitstream_uint12(str, image->tile_rows -1);

        for (idx = 0 ; idx < image->tile_columns-1 ; idx += 1) {
            if (SHORT_HEADER_FLAG(image))
                _jxr_wbitstream_uint8(str, image->tile_column_width[idx]);
            else
                _jxr_wbitstream_uint16(str,image->tile_column_width[idx]);
        }
        for (idx = 0 ; idx < image->tile_rows-1 ; idx += 1) {
            if (SHORT_HEADER_FLAG(image))
                _jxr_wbitstream_uint8(str, image->tile_row_height[idx]);
            else
                _jxr_wbitstream_uint16(str,image->tile_row_height[idx]);
        }
    } else {
        DEBUG(" NO TILING\n");
    }

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

    /* Write out windowing bits. */
    if (WINDOWING_FLAG(image)) {
        _jxr_wbitstream_uint6(str, (uint8_t)image->window_extra_top);
        _jxr_wbitstream_uint6(str, (uint8_t)image->window_extra_left);
        _jxr_wbitstream_uint6(str, (uint8_t)image->window_extra_bottom);
        _jxr_wbitstream_uint6(str, (uint8_t)image->window_extra_right);
    }

    DEBUG("END IMAGE_HEADER\n");
    return res;
}

static int w_image_plane_header(jxr_image_t image, struct wbitstream*str, int /*alpha*/)
{
    DEBUG("START IMAGE_PLANE_HEADER (bitpos=%zu)\n", _jxr_wbitstream_bitpos(str));

    DEBUG(" INTERNAL_CLR_FMT = %d\n", image->use_clr_fmt);
    DEBUG(" SCALED_FLAG = %s\n", image->scaled_flag? "true" : "false");
    DEBUG(" BANDS_PRESENT = %d\n", image->bands_present);

    _jxr_wbitstream_uint3(str, image->use_clr_fmt);
    _jxr_wbitstream_uint1(str, image->scaled_flag); /* SCALED_FLAG = 1 */
    _jxr_wbitstream_uint4(str, image->bands_present);

    switch (image->use_clr_fmt) {
        case 0: /* YONLY */
            image->num_channels = 1;
            break;
        case 1: /* YUV420 */
            image->num_channels = 3;
            _jxr_wbitstream_uint4(str, 0); /* CHROMA_CENTERING */
            _jxr_wbitstream_uint4(str, 0); /* COLOR_INTERPRETATION==0 */
            break;
        case 2: /* YUV422 */
            image->num_channels = 3;
            _jxr_wbitstream_uint4(str, 0); /* CHROMA_CENTERING */
            _jxr_wbitstream_uint4(str, 0); /* COLOR_INTERPRETATION==0 */
            break;
        case 3: /* YUV444 */
            image->num_channels = 3;
            _jxr_wbitstream_uint4(str, 0); /* CHROMA_CENTERING */
            _jxr_wbitstream_uint4(str, 0); /* COLOR_INTERPRETATION==0 */
            break;
        case 4: /* YUVK */
            image->num_channels = 4;
            break;
        case 6: /* NCOMPONENT */
            _jxr_wbitstream_uint4(str, image->num_channels-1);
            _jxr_wbitstream_uint4(str, 0); /* COLOR_INTERPRETATION==0 */
            break;
        case 5: /* RESERVED */
        case 7: /* RESERVED */
            break;
    }

    switch (SOURCE_BITDEPTH(image)) {
        case 0: /* BD1WHITE1 */
        case 1: /* BD8 */
        case 4: /* BD16F */
        case 8: /* BD5 */
        case 9: /* BD10 */
        case 10: /* BD565 */
        case 15: /* BD1BLACK1 */
            break;
        case 2: /* BD16 */
        case 3: /* BD16S */
        case 6: /* BD32S */
            _jxr_wbitstream_uint8(str, image->shift_bits); /* SHIFT_BITS */
            break;
        case 7: /* BD32F */
            _jxr_wbitstream_uint8(str, image->len_mantissa); /* LEN_MANTISSA */
            _jxr_wbitstream_uint8(str, image->exp_bias); /* EXP_BIAS */
            break;
        default: /* RESERVED */
            break;
    }

    /* Emit QP information for DC pass. */
    _jxr_wbitstream_uint1(str, image->dc_frame_uniform); /* DC_FRAME_UNIFORM */
    if (image->dc_frame_uniform) {
        DEBUG(" DC_FRAME_UNIFORM = %s\n", image->dc_frame_uniform?"true":"false");
        _jxr_w_DC_QP(image, str);
    }

    if (image->bands_present != 3 /*DCONLY*/) {
        /* Emit QP information for LP pass. */
        _jxr_wbitstream_uint1(str, 0); /* RESERVED_I_BIT */
        _jxr_wbitstream_uint1(str, image->lp_frame_uniform);
        DEBUG(" LP_FRAME_UNIFORM = %s\n", image->lp_frame_uniform?"true":"false");
        if (image->lp_frame_uniform) {
            assert(image->num_lp_qps > 0);
            /* _jxr_wbitstream_uint4(str, image->num_lp_qps-1); */
            _jxr_w_LP_QP(image, str);
        }

        if (image->bands_present != 2 /*NOHIGHPASS*/) {
            /* Emit QP information for HP pass. */
            _jxr_wbitstream_uint1(str, 0); /* RESERVED_J_BIT */
            _jxr_wbitstream_uint1(str, image->hp_frame_uniform);
            DEBUG(" HP_FRAME_UNIFORM = %s\n", image->hp_frame_uniform?"true":"false");
            if (image->hp_frame_uniform) {
                _jxr_w_HP_QP(image, str);
            }
        }
    }

    _jxr_wbitstream_syncbyte(str);
    DEBUG("END IMAGE_PLANE_HEADER (bitpos=%zu)\n", _jxr_wbitstream_bitpos(str));

    return 0;
}

static int put_ch_mode(jxr_image_t image, struct wbitstream*str)
{
    /* If there is only 1 channel, then CH_MODE==0 is implicit. */
    if (image->num_channels == 1) {
        assert(image->dc_component_mode == JXR_CM_UNIFORM);
        return 0;
    }

    int use_mode = image->dc_component_mode;
    _jxr_wbitstream_uint2(str, use_mode);
    return use_mode;
}

void _jxr_w_DC_QP(jxr_image_t image, struct wbitstream*str)
{
    int idx;
    int ch_mode = put_ch_mode(image, str);
    DEBUG(" DC_QP CH_MODE=%d ", ch_mode);

    switch (ch_mode) {
        case 0: /* UNIFORM */
            DEBUG(" DC_QUANT UNIFORM =%u", image->dc_quant_ch[0]);
            _jxr_wbitstream_uint8(str, image->dc_quant_ch[0]);
            break;
        case 1: /* SEPARATE */
            DEBUG(" DC_QUANT SEPARATE Y=%u, Chr=%u", image->dc_quant_ch[0],image->dc_quant_ch[1]);
            _jxr_wbitstream_uint8(str, image->dc_quant_ch[0]);
            _jxr_wbitstream_uint8(str, image->dc_quant_ch[1]);
            break;
        case 2: /* INDEPENDENT */
            DEBUG(" DC_QUANT INDEPENDENT =");
            for (idx = 0 ; idx < image->num_channels ; idx +=1) {
                DEBUG(" %u", image->dc_quant_ch[idx]);
                _jxr_wbitstream_uint8(str, image->dc_quant_ch[idx]);
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

void _jxr_w_LP_QP(jxr_image_t image, struct wbitstream*str)
{
    unsigned idx;
    for (idx = 0 ; idx < image->num_lp_qps ; idx += 1) {
        int ch_mode = put_ch_mode(image, str);
        DEBUG(" LP_QP[%d] CH_MODE=%d ", idx, ch_mode);

        switch (ch_mode) {
            case 0: /* UNIFORM */
                DEBUG(" LP_QUANT UNIFORM =%u", image->lp_quant_ch[0][idx]);
                _jxr_wbitstream_uint8(str, image->lp_quant_ch[0][idx]);
                break;
            case 1: /* SEPARATE */
                DEBUG(" LP_QUANT SEPARATE Y=%u, Chr=%u", image->lp_quant_ch[0][idx],image->lp_quant_ch[1][idx]);
                _jxr_wbitstream_uint8(str, image->lp_quant_ch[0][idx]);
                _jxr_wbitstream_uint8(str, image->lp_quant_ch[1][idx]);
                break;
            case 2: /* INDEPENDENT */
                DEBUG(" LP_QUANT INDEPENDENT =");
                int ch;
                for (ch = 0 ; ch < image->num_channels ; ch +=1) {
                    DEBUG(" %u", image->lp_quant_ch[ch][idx]);
                    _jxr_wbitstream_uint8(str, image->lp_quant_ch[ch][idx]);
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
}

void _jxr_w_HP_QP(jxr_image_t image, struct wbitstream*str)
{
    unsigned idx;
    for (idx = 0 ; idx < image->num_hp_qps ; idx += 1) {
        int ch_mode = put_ch_mode(image, str);
        DEBUG(" HP_QP[%d] CH_MODE=%d ", idx, ch_mode);

        switch (ch_mode) {
            case 0: /* UNIFORM */
                _jxr_wbitstream_uint8(str, image->hp_quant_ch[0][idx]);
                DEBUG("UNIFORM %d", image->hp_quant_ch[0][idx]);
                break;
            case 1: /* SEPARATE */
                DEBUG("SEPARATE Y=%u, Chr=%u", image->hp_quant_ch[0][idx],image->hp_quant_ch[1][idx]);
                _jxr_wbitstream_uint8(str, image->hp_quant_ch[0][idx]);
                _jxr_wbitstream_uint8(str, image->hp_quant_ch[1][idx]);
                break;
            case 2: /* INDEPENDENT */
                DEBUG("INDEPENDENT =");
                int ch;
                for (ch = 0 ; ch < image->num_channels ; ch +=1) {
                    DEBUG(" %u", image->hp_quant_ch[ch][idx]);
                    _jxr_wbitstream_uint8(str, image->hp_quant_ch[ch][idx]);
                }
                break;
            case 3: /* Reserved */
                break;
            default:
                assert(0);
        }
        DEBUG(" bitpos=%zu\n", _jxr_wbitstream_bitpos(str));
    }
}

static void w_INDEX_TABLE(jxr_image_t image, struct wbitstream*str)
{
    DEBUG("START INDEX_TABLE at bitpos=%zu\n", _jxr_wbitstream_bitpos(str));

    if (INDEXTABLE_PRESENT_FLAG(image)) {
        /* INDEX_TABLE_STARTCODE == 0x0001 */
        DEBUG(" INDEX_TAB:E_STARTCODE at bitpos=%zu\n", _jxr_wbitstream_bitpos(str));
        _jxr_wbitstream_uint8(str, 0x00);
        _jxr_wbitstream_uint8(str, 0x01);

        int idx;
        for (idx = 0 ; idx < image->tile_index_table_length ; idx++) {
            _jxr_wbitstream_intVLW(str, image->tile_index_table[idx]);
        }

        DEBUG(" INDEX_TABLE has %d table entries\n", image->tile_index_table_length);

    }

    DEBUG("INTEX_TABLE DONE bitpos=%zu\n", _jxr_wbitstream_bitpos(str));
}

static uint64_t w_PROFILE_LEVEL_INFO(jxr_image_t image, struct wbitstream*str, uint64_t bytes) 
{
    uint64_t additional_bytes;
    for (additional_bytes = bytes; additional_bytes > 3 ; additional_bytes -= 4) {
        /* These profile and level values are default.  More logic needed */
        _jxr_wbitstream_uint8(str, image->profile_idc); /* PROFILE_IDC */
        _jxr_wbitstream_uint8(str, image->level_idc); /* LEVEL_IDC */
        _jxr_wbitstream_uint15(str, 0); /* RESERVED_L */
        if (additional_bytes > 7)
            _jxr_wbitstream_uint1(str, 0); /* LAST_FLAG */
        else
            _jxr_wbitstream_uint1(str, 1); /* LAST_FLAG */
    }

    return additional_bytes;
}

static void w_TILE(jxr_image_t image, struct wbitstream*str)
{
    unsigned tile_idx = 0;

    if (FREQUENCY_MODE_CODESTREAM_FLAG(image) == 0 /* SPATIALMODE */) {

        if (TILING_FLAG(image)) {
            unsigned tx, ty;
            for (ty = 0 ; ty < image->tile_rows ; ty += 1)
                for (tx = 0 ; tx < image->tile_columns ; tx += 1) {
                    _jxr_w_TILE_SPATIAL(image, str, tx, ty);
                    image->tile_index_table[tile_idx] = str->write_count;
                    tile_idx++;
                }
        } else {
            _jxr_w_TILE_SPATIAL(image, str, 0, 0);
        }

    } else { /* FREQUENCYMODE */
        /* Temporarily declare these bitstreams til I figure out the tmp file vs in memory issue*/
        unsigned tx, ty;
        uint8_t bands_present = image->bands_present_of_primary;

        for (ty = 0 ; ty < image->tile_rows ; ty += 1) {
            for (tx = 0 ; tx < image->tile_columns ; tx += 1) {
                _jxr_w_TILE_DC(image, str, tx, ty);
                image->tile_index_table[tile_idx * (4 - bands_present) + 0] = str->write_count;
                if (bands_present != 3) {
                    _jxr_w_TILE_LP(image, str, tx, ty);
                    image->tile_index_table[tile_idx * (4 - bands_present) + 1] = str->write_count;
                    if (bands_present != 2) {
                        _jxr_w_TILE_HP_FLEX(image, str, tx, ty);
                    }
                }


                tile_idx++;
            }
        }
    }

    if (INDEXTABLE_PRESENT_FLAG(image)) {
        for (tile_idx = image->tile_index_table_length - 1 ; tile_idx > 0  ; tile_idx--) {
            image->tile_index_table[tile_idx] = image->tile_index_table[tile_idx - 1];
        }
        image->tile_index_table[0] = 0;
    }

}


static int short_header_ok(jxr_image_t image)
{
    /* If the image width or height is too big for short header,
    then we need a long header. */
    if (image->width1 >= 0x10000 || image->height1 >= 0x10000)
        return 0;

    /* If the tile width/height is too big for a short header,
    then we need to use a long header. */
    if (jxr_get_TILING_FLAG(image)) {
        unsigned idx;

        for (idx = 0 ; idx < image->tile_columns ; idx += 1)
            if (jxr_get_TILE_WIDTH(image,idx)/16 > 0x100)
                return 0;
        for (idx = 0 ; idx < image->tile_rows ; idx += 1)
            if (jxr_get_TILE_HEIGHT(image,idx)/16 > 0x100)
                return 0;
    }

    /* If nothing else forces us to use a long header, then we can
    use a short header. */
    return 1;
}

static int need_windowing_flag(jxr_image_t image)
{
    if (image->window_extra_top > 0)
        return 1;
    if (image->window_extra_left > 0)
        return 1;

    return 0;
}

static int need_trim_flexbits_flag(jxr_image_t image)
{
    /* If no FLEXBTS data, then no TRIM_FLEXBITS flag */
    if (image->bands_present != JXR_BP_ALL)
        return 0;
    if (image->trim_flexbits == 0)
        return 0;

    return 1;
}

void _jxr_w_TILE_HEADER_DC(jxr_image_t image, struct wbitstream*str,
                             int /*alpha_flag*/, unsigned tx, unsigned ty)
{
    if (image->dc_frame_uniform == 0) {
        int ch;

        /* per-tile configuration, so get the QP settings from the
        per-tile store and make it current. */
        assert(image->tile_quant);
        struct jxr_tile_qp*cur = GET_TILE_QUANT(image, tx, ty);
        image->dc_component_mode = cur->component_mode;
        switch (image->dc_component_mode) {
            case JXR_CM_UNIFORM:
                for (ch = 0 ; ch < image->num_channels ; ch += 1)
                    image->dc_quant_ch[ch] = cur->channel[0].dc_qp;
                break;
            case JXR_CM_SEPARATE:
                image->dc_quant_ch[0] = cur->channel[0].dc_qp;
                for (ch = 1 ; ch < image->num_channels ; ch += 1)
                    image->dc_quant_ch[ch] = cur->channel[1].dc_qp;
                break;
            case JXR_CM_INDEPENDENT:
                for (ch = 0 ; ch < image->num_channels ; ch += 1)
                    image->dc_quant_ch[ch] = cur->channel[ch].dc_qp;
                break;
            case JXR_CM_Reserved:
                assert(0);
                break;
        }

        _jxr_w_DC_QP(image, str);
    }
}

void _jxr_w_TILE_HEADER_LOWPASS(jxr_image_t image, struct wbitstream*str,
                                  int /*alpha_flag*/, unsigned tx, unsigned ty)
{
    if (image->lp_frame_uniform == 0) {
        int ch;
        unsigned idx;

        /* per-tile configuration, so get the QP settings from the
        per-tile store and make it current. */
        assert(image->tile_quant);
        struct jxr_tile_qp*cur = GET_TILE_QUANT(image, tx, ty);
        image->lp_component_mode = cur->component_mode;
        image->num_lp_qps = cur->channel[0].num_lp;

        switch (image->lp_component_mode) {
            case JXR_CM_UNIFORM:
                for (ch = 0 ; ch < image->num_channels ; ch += 1) {
                    for (idx = 0 ; idx < image->num_lp_qps ; idx += 1)
                        image->lp_quant_ch[ch][idx] = cur->channel[0].lp_qp[idx];
                }
                break;
            case JXR_CM_SEPARATE:
                for (idx = 0 ; idx < image->num_lp_qps ; idx += 1)
                    image->lp_quant_ch[0][idx] = cur->channel[0].lp_qp[idx];
                for (ch = 1 ; ch < image->num_channels ; ch += 1) {
                    for (idx = 0 ; idx < image->num_lp_qps ; idx += 1)
                        image->lp_quant_ch[ch][idx] = cur->channel[1].lp_qp[idx];
                }
                break;
            case JXR_CM_INDEPENDENT:
                for (ch = 0 ; ch < image->num_channels ; ch += 1) {
                    for (idx = 0 ; idx < image->num_lp_qps ; idx += 1)
                        image->lp_quant_ch[ch][idx] = cur->channel[ch].lp_qp[idx];
                }
                break;
            case JXR_CM_Reserved:
                assert(0);
                break;
        }

        _jxr_wbitstream_uint1(str, 0); /* XXXX USE_DC_QP == FALSE */
        assert(image->num_lp_qps > 0);
        DEBUG(" TILE_HEADER_LP: NUM_LP_QPS = %d\n", image->num_lp_qps);
        _jxr_wbitstream_uint4(str, image->num_lp_qps-1);
        _jxr_w_LP_QP(image, str);
    }
}

void _jxr_w_TILE_HEADER_HIGHPASS(jxr_image_t image, struct wbitstream*str,
                                   int /*alpha_flag*/, unsigned tx, unsigned ty)
{
    if (image->hp_frame_uniform == 0) {
        int ch;
        unsigned idx;

        /* per-tile configuration, so get the QP settings from the
        per-tile store and make it current. */
        assert(image->tile_quant);
        struct jxr_tile_qp*cur = GET_TILE_QUANT(image, tx, ty);
        image->hp_component_mode = cur->component_mode;
        image->num_hp_qps = cur->channel[0].num_hp;

        switch (image->hp_component_mode) {
            case JXR_CM_UNIFORM:
                for (ch = 0 ; ch < image->num_channels ; ch += 1) {
                    for (idx = 0 ; idx < image->num_hp_qps ; idx += 1)
                        image->hp_quant_ch[ch][idx] = cur->channel[0].hp_qp[idx];
                }
                break;
            case JXR_CM_SEPARATE:
                for (idx = 0 ; idx < image->num_hp_qps ; idx += 1)
                    image->hp_quant_ch[0][idx] = cur->channel[0].hp_qp[idx];
                for (ch = 1 ; ch < image->num_channels ; ch += 1) {
                    for (idx = 0 ; idx < image->num_hp_qps ; idx += 1)
                        image->hp_quant_ch[ch][idx] = cur->channel[1].hp_qp[idx];
                }
                break;
            case JXR_CM_INDEPENDENT:
                for (ch = 0 ; ch < image->num_channels ; ch += 1) {
                    for (idx = 0 ; idx < image->num_hp_qps ; idx += 1)
                        image->hp_quant_ch[ch][idx] = cur->channel[ch].hp_qp[idx];
                }
                break;
            case JXR_CM_Reserved:
                assert(0);
                break;
        }

        _jxr_wbitstream_uint1(str, 0); /* XXXX USE_LP_QP == FALSE */
        assert(image->num_hp_qps > 0);
        _jxr_wbitstream_uint4(str, image->num_hp_qps-1);
        _jxr_w_HP_QP(image, str);
    }
}

void _jxr_w_ENCODE_QP_INDEX(jxr_image_t /*image*/, struct wbitstream*str,
                              unsigned /*tx*/, unsigned /*ty*/, unsigned /*mx*/, unsigned /*my*/,
                              unsigned num_qps, unsigned qp_index)
{
    static const unsigned bits_qp_index[17] = {0, 0,1,1,2, 2,3,3,3, 3,4,4,4, 4,4,4,4};

    assert(num_qps > 1 && num_qps <= 16);

    if (qp_index == 0) {
        /* IS_QPINDEX_NONZERO_FLAG == false */
        _jxr_wbitstream_uint1(str, 0);
        return;
    }

    /* IS_QPINDEX_NONZERO_FLAG == true */
    _jxr_wbitstream_uint1(str, 1);

    int bits_count = bits_qp_index[num_qps];
    assert(bits_count > 0); /* num_qps must be >1 here. */

    _jxr_wbitstream_uintN(str, qp_index-1, bits_count);
}

static void encode_val_dc_yuv(jxr_image_t /*image*/, struct wbitstream*str, int val)
{
    assert(val >= 0 && val <= 7);

    switch (val) {
        case 0: /* 10 */
            _jxr_wbitstream_uint2(str, 2);
            break;
        case 1: /* 001 */
            _jxr_wbitstream_uint2(str, 0);
            _jxr_wbitstream_uint1(str, 1);
            break;
        case 2: /* 0000 1 */
            _jxr_wbitstream_uint4(str, 0);
            _jxr_wbitstream_uint1(str, 1);
            break;
        case 3: /* 0001 */
            _jxr_wbitstream_uint4(str, 1);
            break;
        case 4: /* 11 */
            _jxr_wbitstream_uint2(str, 3);
            break;
        case 5: /* 010 */
            _jxr_wbitstream_uint2(str, 1);
            _jxr_wbitstream_uint1(str, 0);
            break;
        case 6: /* 0000 0 */
            _jxr_wbitstream_uint4(str, 0);
            _jxr_wbitstream_uint1(str, 0);
            break;
        case 7: /* 011 */
            _jxr_wbitstream_uint2(str, 1);
            _jxr_wbitstream_uint1(str, 1);
            break;
    }
}

void _jxr_w_MB_DC(jxr_image_t image, struct wbitstream*str,
                    int /*alpha_flag*/,
                    unsigned tx, unsigned ty,
                    unsigned mx, unsigned my)
{
    int lap_mean[2];
    lap_mean[0] = 0;
    lap_mean[1] = 0;

    DEBUG(" MB_DC tile=[%u %u] mb=[%u %u] bitpos=%zu\n",
        tx, ty, mx, my, _jxr_wbitstream_bitpos(str));

    if (_jxr_InitContext(image, tx, ty, mx, my)) {
        DEBUG(" MB_DC: Initialize Context\n");
        _jxr_InitVLCTable(image, AbsLevelIndDCLum);
        _jxr_InitVLCTable(image, AbsLevelIndDCChr);
        _jxr_InitializeModelMB(&image->model_dc, 0/*DC*/);
    }

    if (image->use_clr_fmt==0 || image->use_clr_fmt==4 || image->use_clr_fmt==6) {
        int32_t dc_val = 0;

        /* clr_fmt == YONLY, YUVK or NCOMPONENT */
        unsigned idx;
        for (idx = 0 ; idx < image->num_channels ; idx += 1) {
            dc_val = MACROBLK_CUR_DC(image,idx,tx,mx);
            int m = (idx == 0)? 0 : 1;
            int model_bits = image->model_dc.bits[m];

            unsigned is_dc_ch = (labs(dc_val)>>model_bits) != 0 ? 1 : 0;
            _jxr_wbitstream_uint1(str, is_dc_ch);
            DEBUG(" MB_DC: IS_DC_CH=%u, model_bits=%d\n",
                is_dc_ch, model_bits);
            if (is_dc_ch) {
                lap_mean[m] += 1;
            }
            DEBUG(" dc_val at t=[%u %u], m=[%u %u] == %d (0x%08x)\n",
                tx, ty, mx, my, dc_val, dc_val);
            w_DEC_DC(image, str, model_bits, 0/*chroma*/, is_dc_ch, dc_val);
        }
    } else {
        int32_t dc_val_Y = MACROBLK_CUR_DC(image,0,tx,mx);
        int32_t dc_val_U = MACROBLK_CUR_DC(image,1,tx,mx);
        int32_t dc_val_V = MACROBLK_CUR_DC(image,2,tx,mx);
        int val_dc_yuv = 0;

        if ((labs(dc_val_Y) >> image->model_dc.bits[0]) != 0) {
            val_dc_yuv |= 0x4;
            lap_mean[0] += 1;
        }
        if ((labs(dc_val_U) >> image->model_dc.bits[1]) != 0) {
            val_dc_yuv |= 0x2;
            lap_mean[1] += 1;
        }
        if ((labs(dc_val_V) >> image->model_dc.bits[1]) != 0) {
            val_dc_yuv |= 0x1;
            lap_mean[1] += 1;
        }

        DEBUG(" VAL_DC_YUV = %x\n", val_dc_yuv);
        encode_val_dc_yuv(image, str, val_dc_yuv);

        DEBUG(" dc_val_Y at t=[%u %u], m=[%u %u] == %d (0x%08x)\n",
            tx, ty, mx, my, dc_val_Y, dc_val_Y);
        int model_bits = image->model_dc.bits[0];
        int is_dc_ch = val_dc_yuv&0x4 ? 1 : 0;
        w_DEC_DC(image, str, model_bits, 0/*chroma*/, is_dc_ch, dc_val_Y);

        DEBUG(" dc_val_U at t=[%u %u], m=[%u %u] == %d (0x%08x)\n",
            tx, ty, mx, my, dc_val_U, dc_val_U);
        model_bits = image->model_dc.bits[1];
        is_dc_ch = val_dc_yuv&0x2 ? 1 : 0;
        w_DEC_DC(image, str, model_bits, 1/*chroma*/, is_dc_ch, dc_val_U);

        DEBUG(" dc_val_V at t=[%u %u], m=[%u %u] == %d (0x%08x)\n",
            tx, ty, mx, my, dc_val_V, dc_val_V);
        model_bits = image->model_dc.bits[1];
        is_dc_ch = val_dc_yuv&0x1 ? 1 : 0;
        w_DEC_DC(image, str, model_bits, 1/*chroma*/, is_dc_ch, dc_val_V);
    }

    /* */
    DEBUG(" MB_DC: UpdateModelMB: lap_mean={%u %u}\n", lap_mean[0], lap_mean[1]);
    _jxr_UpdateModelMB(image, lap_mean, &image->model_dc, 0/*DC*/);
    if (_jxr_ResetContext(image, tx, mx)) {
        DEBUG(" MB_DC: Reset Context\n");
        _jxr_AdaptVLCTable(image, AbsLevelIndDCLum);
        _jxr_AdaptVLCTable(image, AbsLevelIndDCChr);
    }

    DEBUG(" MB_DC DONE tile=[%u %u] mb=[%u %u]\n", tx, ty, mx, my);
}

static void w_REFINE_LP(struct wbitstream*str, int coeff, int model_bits)
{
    if (coeff > 0) {
        _jxr_wbitstream_uintN(str, coeff, model_bits);
        coeff >>= model_bits;
        if (coeff == 0)
            _jxr_wbitstream_uint1(str, 0);
    } else if (coeff < 0) {
        coeff = -coeff;
        _jxr_wbitstream_uintN(str, coeff, model_bits);
        coeff >>= model_bits;
        if (coeff == 0)
            _jxr_wbitstream_uint1(str, 1);
    } else {
        _jxr_wbitstream_uintN(str, 0, model_bits);
        coeff >>= model_bits;
        /* No sign bit is needed for zero values. */
    }
}

/*
* This maps the values in src[1-15] into dst[1-15], and adapts the
* map as it goes.
*/
static void AdaptiveLPPermute(jxr_image_t image, int dst[16], const int src[16])
{
    int idx;
    for (idx = 1 ; idx < 16 ; idx += 1) {

        int map = image->lopass_scanorder[idx-1];
        dst[idx] = src[map];

        if (dst[idx] == 0)
            continue;

        image->lopass_scantotals[idx-1] += 1;

        if (idx > 1 && image->lopass_scantotals[idx-1] > image->lopass_scantotals[idx-2]) {
            SWAP(image->lopass_scantotals[idx-1], image->lopass_scantotals[idx-2]);
            SWAP(image->lopass_scanorder[idx-1], image->lopass_scanorder[idx-2]);
        }
    }
}

static void FixedLPPermuteUV(jxr_image_t image, int dst[16], int src[8][16])
{
    assert(image->use_clr_fmt==1/*YUV420*/ || image->use_clr_fmt==2/*YUV422*/);

    int k;
    int count_chr = 14;
    if (image->use_clr_fmt==1/*YUV420*/)
        count_chr = 6;

    static const int remap_arr422[] = {4, 1, 2, 3, 5, 6, 7};

    /* Shuffle the UV planes into a single dst */
    for (k = 0; k < count_chr; k += 1) {
        int remap = k>>1;
        int plane = (k&1) + 1; /* 1 or 2 (U or V) */

        if (image->use_clr_fmt==1/*YUV420*/)
            remap += 1;
        if (image->use_clr_fmt==2/*YUV422*/)
            remap = remap_arr422[remap];

        if (image->use_clr_fmt==1/*YUV420*/)
            remap = transpose420[remap];
        if (image->use_clr_fmt==2/*YUV422*/)
            remap = transpose422[remap];
        dst[k+1] = src[plane][remap];
    }

    for (k = count_chr; k < 15; k += 1)
        dst[k+1] = 0;
}

static void AdaptiveHPPermute(jxr_image_t image, int dst[16], const int src[16], int mbhp_pred_mode)
{
    if (mbhp_pred_mode == 1) {
        int idx;
        for (idx = 1 ; idx < 16 ; idx += 1) {
            int map = image->hipass_ver_scanorder[idx-1];
            dst[idx] = src[map];

            if (dst[idx] == 0)
                continue;

            image->hipass_ver_scantotals[idx-1] += 1;


            if (idx > 1 && image->hipass_ver_scantotals[idx-1] > image->hipass_ver_scantotals[idx-2]) {
                SWAP(image->hipass_ver_scantotals[idx-1], image->hipass_ver_scantotals[idx-2]);
                SWAP(image->hipass_ver_scanorder[idx-1], image->hipass_ver_scanorder[idx-2]);
            }
        }
    } else {
        int idx;
        for (idx = 1 ; idx < 16 ; idx += 1) {
            int map = image->hipass_hor_scanorder[idx-1];
            dst[idx] = src[map];

            if (dst[idx] == 0)
                continue;

            image->hipass_hor_scantotals[idx-1] += 1;


            if (idx > 1 && image->hipass_hor_scantotals[idx-1] > image->hipass_hor_scantotals[idx-2]) {
                SWAP(image->hipass_hor_scantotals[idx-1], image->hipass_hor_scantotals[idx-2]);
                SWAP(image->hipass_hor_scanorder[idx-1], image->hipass_hor_scanorder[idx-2]);
            }
        }
    }
}

/*
* Convert the string of LP values to a compact list of non-zero LP
* values interspersed with the run of zero values. This is what will
* ultimately be encoded. The idea is that an LP set is generally
* sparse and it is more compact to encode the zero-runs then the
* zeros themselves.
*/
static int RunLengthEncode(jxr_image_t /*image*/, int RLCoeffs[32], const int LPInput[16])
{
    int lp_index = 1;
    int rl_index = 0;
    int run_count = 0;

    DEBUG(" RLCoeffs:");
    while (lp_index < 16) {
        if (LPInput[lp_index] == 0) {
            lp_index += 1;
            run_count += 1;
            continue;
        }

        assert(rl_index <= 30);
        RLCoeffs[rl_index+0] = run_count;
        RLCoeffs[rl_index+1] = LPInput[lp_index];

        DEBUG(" %d--0x%x", RLCoeffs[rl_index+0], RLCoeffs[rl_index+1]);

        rl_index += 2;
        run_count = 0;
        lp_index += 1;
    }

    DEBUG(" num_non_zero=%d\n", rl_index/2);

    /* Return the number of non-zero values. */
    return rl_index/2;
}

static int collect_lp_input(jxr_image_t image, int ch, int tx, int mx, int LPInput[8][16], int model_bits)
{
    int num_non_zero = 0;

    int count = 16;
    if (ch > 0 && image->use_clr_fmt==2/*YUV422*/)
        count = 8;
    if (ch > 0 && image->use_clr_fmt==1/*YUV420*/)
        count = 4;

    /* Collect into LPInput the coefficient values with the
    model_bits stripped out. Be careful to shift only POSITIVE
    numbers because the encoding later on uses magnitude-sign
    to encode these values. The values can come out differently. */
    int k;
    for (k = 1 ; k < count ; k += 1) {
        int sign_flag = 0;
        LPInput[ch][k] = MACROBLK_CUR_LP(image,ch,tx,mx,k-1);
        if (LPInput[ch][k] < 0) {
            sign_flag = 1;
            LPInput[ch][k] = -LPInput[ch][k];
        }
        LPInput[ch][k] >>= model_bits;
        if (LPInput[ch][k] != 0) {
            num_non_zero += 1;
            if (sign_flag)
                LPInput[ch][k] = -LPInput[ch][k];
        }
    }

    return num_non_zero;
}

void _jxr_w_MB_LP(jxr_image_t image, struct wbitstream*str,
                    int /*alpha_flag*/,
                    unsigned tx, unsigned ty,
                    unsigned mx, unsigned my)
{
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
        tx, ty, mx, my, _jxr_wbitstream_bitpos(str));

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
    if (image->use_clr_fmt==1/*YUV420*/ || image->use_clr_fmt==2/*YUV422*/)
        full_planes = 2;

    /* The CBPLP signals whether any non-zero coefficients are
    present in the LP band for this macroblock. It is a bitmask
    with a bit for each channel. So for example, YONLY, which
    has 1 channel, has a 1-bit cbplp. */

    int cbplp = 0;
    /* if CLR_FMT is YUV420, YUV422 or YUV444... */
    if (image->use_clr_fmt==1 || image->use_clr_fmt==2 || image->use_clr_fmt==3) {
        assert(full_planes == 2 || full_planes == 3);
        int max = full_planes * 4 - 5;
        int model_bits0 = image->model_lp.bits[0];
        int model_bits1 = image->model_lp.bits[1];

        int num_non_zero = collect_lp_input(image, 0, tx, mx, LPInput, model_bits0);
        if (num_non_zero > 0)
            cbplp |= 1;

        switch (image->use_clr_fmt) {
            case 1:/*YUV420*/
            case 2:/*YUV422*/
                /* YUV422 has a 2-bit CBPLP, the low bit is the
                CBP for Y, and the high bit for UV. */
                num_non_zero = collect_lp_input(image, 1, tx, mx, LPInput, model_bits1);
                if (num_non_zero > 0)
                    cbplp |= 2;
                num_non_zero = collect_lp_input(image, 2, tx, mx, LPInput, model_bits1);
                if (num_non_zero > 0)
                    cbplp |= 2;
                break;

            case 3:/*YUV444*/
                num_non_zero = collect_lp_input(image, 1, tx, mx, LPInput, model_bits1);
                if (num_non_zero > 0)
                    cbplp |= 2;
                num_non_zero = collect_lp_input(image, 2, tx, mx, LPInput, model_bits1);
                if (num_non_zero > 0)
                    cbplp |= 4;
                break;
        }

        DEBUG(" MB_LP: count_zero_CBPLP=%d, count_max_CBPLP=%d, bitpos=%zu\n",
            image->count_zero_CBPLP, image->count_max_CBPLP, _jxr_wbitstream_bitpos(str));
        if (image->count_zero_CBPLP <= 0 || image->count_max_CBPLP < 0) {
            int cbplp_yuv;
            if (image->count_max_CBPLP < image->count_zero_CBPLP) {
                assert(max >= cbplp);
                cbplp_yuv = max - cbplp;
            } else {
                cbplp_yuv = cbplp;
            }
            switch (image->use_clr_fmt) {
                case 1:/*YUV420*/
                case 2:/*YUV422*/
                    if (cbplp_yuv == 0) {
                        _jxr_wbitstream_uint1(str, 0); /* 0 */
                    } else if (cbplp_yuv == 1) {
                        _jxr_wbitstream_uint2(str, 2); /* 10 */
                    } else {
                        _jxr_wbitstream_uint2(str, 3);
                        if (cbplp_yuv == 2)
                            _jxr_wbitstream_uint1(str, 0); /* 110 */
                        else
                            _jxr_wbitstream_uint1(str, 1); /* 111 */
                    }
                    break;

                case 3:/*YUV444*/
                    switch (cbplp_yuv) { /* YUV444 encoding */
                case 0: /* 0 */
                    _jxr_wbitstream_uint1(str, 0);
                    break;
                case 1: /* 100 */
                    _jxr_wbitstream_uint2(str, 2);
                    _jxr_wbitstream_uint1(str, 0);
                    break;
                default:
                    _jxr_wbitstream_uint4(str, 0xaa + cbplp_yuv-2);
                    break;
                    }
                    break;
                default:
                    assert(0);
            }
        } else {
            int ch;
            for (ch = 0 ; ch < full_planes ; ch += 1) {
                if (cbplp & (1<<(full_planes-1-ch)))
                    _jxr_wbitstream_uint1(str, 1);
                else
                    _jxr_wbitstream_uint1(str, 0);
            }
        }
        _jxr_UpdateCountCBPLP(image, cbplp, max);

    } else {
        int ch;
        cbplp = 0;
        for (ch = 0 ; ch < image->num_channels ; ch += 1) {
            int chroma_flag = ch > 0? 1 : 0;
            int model_bits = image->model_lp.bits[chroma_flag];

            DEBUG(" MB_LP: ch=%d, model_bits=%d\n", ch, model_bits);

            int num_non_zero = collect_lp_input(image, ch, tx, mx, LPInput, model_bits);

            /* If there are non-zero coefficients, then CBPLP
            for this plane is 1. Also, the CBPLP in the
            stream is a single bit for each plane. */
            if (num_non_zero > 0) {
                _jxr_wbitstream_uint1(str, 1);
                cbplp |= 1 << ch;
            } else {
                _jxr_wbitstream_uint1(str, 0);
            }
        }
    }
    DEBUG(" MB_LP: cbplp = 0x%x (full_planes=%u)\n", cbplp, full_planes);

    int ndx;
    for (ndx = 0 ; ndx < full_planes ; ndx += 1) {
        const int chroma_flag = ndx>0? 1 : 0;

        DEBUG(" MB_LP: process full_plane %u, CBPLP for plane=%d, bitpos=%zu\n",
            ndx, (cbplp>>ndx)&1, _jxr_wbitstream_bitpos(str));
#if defined(DETAILED_DEBUG)
        if (chroma_flag && image->use_clr_fmt == 2/*YUV422*/) {
            int k;
            DEBUG(" lp val[ndx=%d] refined data ==", ndx);
            for (k = 1 ; k<8; k+=1) {
                DEBUG(" 0x%x/0x%x", MACROBLK_CUR_LP(image,1,tx,mx,k-1), MACROBLK_CUR_LP(image,2,tx,mx,k-1));
            }
            DEBUG("\n");
            DEBUG(" lp val[ndx=%d] before LPPermute ==", ndx);
            for (k = 1 ; k<8; k+=1) {
                DEBUG(" 0x%x/0x%x", LPInput[1][k], LPInput[2][k]);
            }
            DEBUG("\n");

        } else if (chroma_flag && image->use_clr_fmt == 1/*YUV420*/) {
            int k;
            DEBUG(" lp val[ndx=%d] before LPPermute ==", ndx);
            for (k = 1 ; k<4; k+=1) {
                DEBUG(" 0x%x/0x%x", LPInput[1][k], LPInput[2][k]);
            }
            DEBUG("\n");

        } else {
            int k;
            DEBUG(" lp val[ndx=%d] refined data ==", ndx);
            for (k = 1 ; k<16; k+=1) {
                DEBUG(" 0x%x", MACROBLK_CUR_LP(image,ndx,tx,mx,k-1));
            }
            DEBUG("\n");
            DEBUG(" lp val[ndx=%d] before LPPermute ==", ndx);
            for (k = 1 ; k<16; k+=1) {
                DEBUG(" 0x%x", LPInput[ndx][k]);
            }
            DEBUG("\n");
            DEBUG(" scan order ==");
            for (k = 0 ; k<15; k+=1) {
                DEBUG(" %2d", image->lopass_scanorder[k]);
            }
            DEBUG("\n");
            DEBUG(" scan totals ==");
            for (k = 0 ; k<15; k+=1) {
                DEBUG(" %2d", image->lopass_scantotals[k]);
            }
            DEBUG("\n");
        }
#endif
        /* If there are coeff bits, encode them. */
        if ((cbplp>>ndx) & 1) {
            int LPInput_n[16];

            if (chroma_flag && (image->use_clr_fmt==1 || image->use_clr_fmt==2)) {
                /* LP for YUV42X components are shuffled by
                a fixed permute. */
                FixedLPPermuteUV(image, LPInput_n, LPInput);
            } else {
                /* LP for YUV444 components (and the Y of
                all YUV) are adaptively ordered. */
                AdaptiveLPPermute(image, LPInput_n, LPInput[ndx]);
            }

            int RLCoeffs[32] = {0};
            int num_non_zero = RunLengthEncode(image, RLCoeffs, LPInput_n);
            w_DECODE_BLOCK(image, str, 1 /* LP */, chroma_flag, RLCoeffs, num_non_zero);
            lap_mean[chroma_flag] += num_non_zero;
        }

        /* Emit REFINEMENT bits after the coeff bits are done. */

        int model_bits = image->model_lp.bits[chroma_flag];
        if (model_bits) {
            DEBUG(" MB_LP: Start refine ndx=%d, model_bits=%d, bitpos=%zu\n",
                ndx, model_bits, _jxr_wbitstream_bitpos(str));
            static const int transpose444[16] = { 0, 4, 8,12,
                1, 5, 9,13,
                2, 6,10,14,
                3, 7,11,15 };
            if (chroma_flag == 0) {
                int k;
                for (k=1 ;k<16; k+=1) {
                    int k_ptr = transpose444[k];
                    w_REFINE_LP(str, MACROBLK_CUR_LP(image,ndx,tx,mx,k_ptr-1), model_bits);
                }
            } else {
                int k;
                switch (image->use_clr_fmt) {
                    case 1: /* YUV420 */
                        for (k=1 ; k<4; k+=1) {
                            int k_ptr = transpose420[k];
                            DEBUG(" MP_LP: Refine LP_Input[1/2][%d] = 0x%x/0x%x bitpos=%zu\n",
                                k_ptr, LPInput[1][k_ptr], LPInput[2][k_ptr],
                                _jxr_wbitstream_bitpos(str));
                            w_REFINE_LP(str, MACROBLK_CUR_LP(image,1,tx,mx,k_ptr-1), model_bits);
                            w_REFINE_LP(str, MACROBLK_CUR_LP(image,2,tx,mx,k_ptr-1), model_bits);
                        }
                        break;
                    case 2: /* YUV422 */
                        for (k=1 ; k<8; k+=1) {
                            int k_ptr = transpose422[k];
                            DEBUG(" MP_LP: Refine LP_Input[1/2][%d] = 0x%x/0x%x bitpos=%zu\n",
                                k_ptr, LPInput[1][k_ptr], LPInput[2][k_ptr],
                                _jxr_wbitstream_bitpos(str));
                            w_REFINE_LP(str, MACROBLK_CUR_LP(image,1,tx,mx,k_ptr-1), model_bits);
                            w_REFINE_LP(str, MACROBLK_CUR_LP(image,2,tx,mx,k_ptr-1), model_bits);
                        }
                        break;
                    default: /* All others */
                        for (k=1 ;k<16; k+=1) {
                            int k_ptr = transpose444[k];
                            w_REFINE_LP(str, MACROBLK_CUR_LP(image,ndx,tx,mx,k_ptr-1), model_bits);
                        }
                        break;
                }
            }
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

static void encode_val_1(jxr_image_t image, struct wbitstream*str,
struct adaptive_vlc_s*vlc, int val1, int chr_cbp)
{
    int tmp;

    switch (image->use_clr_fmt) {
        case 0: /* YONLY */
        case 4: /* YUVK */
        case 6: /* NCOMPONENT */
            assert(val1 < 5);
            if (vlc->table == 0) {
                switch (val1) {
                    case 0:
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 1:
                        _jxr_wbitstream_uint1(str, 0);
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 2:
                        _jxr_wbitstream_uint1(str, 0);
                        _jxr_wbitstream_uint1(str, 0);
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 3:
                        _jxr_wbitstream_uint4(str, 0);
                        break;
                    case 4:
                        _jxr_wbitstream_uint4(str, 1);
                        break;
                }
            } 
            else {
                switch (val1) {
                    case 0:
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 1:
                        _jxr_wbitstream_uint1(str, 0);
                        _jxr_wbitstream_uint2(str, 0);
                        break;
                    case 2:
                        _jxr_wbitstream_uint1(str, 0);
                        _jxr_wbitstream_uint2(str, 1);
                        break;
                    case 3:
                        _jxr_wbitstream_uint1(str, 0);
                        _jxr_wbitstream_uint2(str, 2);
                        break;
                    case 4:
                        _jxr_wbitstream_uint1(str, 0);
                        _jxr_wbitstream_uint2(str, 3);
                        break;
                }
            }
            break;

        case 1: /* YUV420 */
        case 2: /* YUV422 */
        case 3: /* YUV444 */
            tmp = val1;
            if (tmp > 8)
                tmp = 8;

            if (vlc->table == 0) {
                switch (tmp) {
                    case 0: /* 010 */
                        _jxr_wbitstream_uint1(str, 0);
                        _jxr_wbitstream_uint2(str, 2);
                        break;
                    case 1: /* 00000 */
                        _jxr_wbitstream_uint1(str, 0);
                        _jxr_wbitstream_uint4(str, 0);
                        break;
                    case 2: /* 0010 */
                        _jxr_wbitstream_uint4(str, 2);
                        break;
                    case 3: /* 00001 */
                        _jxr_wbitstream_uint4(str, 0);
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 4: /* 00010 */
                        _jxr_wbitstream_uint4(str, 1);
                        _jxr_wbitstream_uint1(str, 0);
                        break;
                    case 5: /* 1 */
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 6: /* 011 */
                        _jxr_wbitstream_uint1(str, 0);
                        _jxr_wbitstream_uint2(str, 3);
                        break;
                    case 7: /* 00011 */
                        _jxr_wbitstream_uint4(str, 1);
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 8: /* 0011 */
                        _jxr_wbitstream_uint4(str, 3);
                        break;
                }
            } 
            else {
                switch (tmp) {
                    case 0: /* 1 */
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 1: /* 001 */
                        _jxr_wbitstream_uint2(str, 0);
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 2: /* 010 */
                        _jxr_wbitstream_uint2(str, 1);
                        _jxr_wbitstream_uint1(str, 0);
                        break;
                    case 3: /* 0001 */
                        _jxr_wbitstream_uint4(str, 1);
                        break;
                    case 4: /* 0000 01 */
                        _jxr_wbitstream_uint4(str, 0);
                        _jxr_wbitstream_uint2(str, 1);
                        break;
                    case 5: /* 011 */
                        _jxr_wbitstream_uint1(str, 0);
                        _jxr_wbitstream_uint2(str, 3);
                        break;
                    case 6: /* 0000 1 */
                        _jxr_wbitstream_uint4(str, 0);
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 7: /* 0000 000 */
                        _jxr_wbitstream_uint4(str, 0);
                        _jxr_wbitstream_uint2(str, 0);
                        _jxr_wbitstream_uint1(str, 0);
                        break;
                    case 8: /* 0000 001 */
                        _jxr_wbitstream_uint4(str, 0);
                        _jxr_wbitstream_uint2(str, 0);
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                }
            }

            if (tmp >= 5) {
                DEBUG(" MB_CBP: CHR_CBP=%x\n", chr_cbp);
                assert(chr_cbp < 3);
                switch (chr_cbp) {
                    case 0: /* 1 */
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 1: /* 01 */
                        _jxr_wbitstream_uint2(str, 1);
                        break;
                    case 2: /* 00 */
                        _jxr_wbitstream_uint2(str, 0);
                        break;
                }
            }

            if (tmp == 8) {
                assert((val1 - tmp) <= 2);
                int val_inc = val1 - tmp;
                DEBUG(" MB_CBP: VAL_INC=%d\n", val_inc);
                switch (val_inc) {
                    case 0: /* 1 */
                        _jxr_wbitstream_uint1(str, 1);
                        break;
                    case 1: /* 01 */
                        _jxr_wbitstream_uint2(str, 1);
                        break;
                    case 2: /* 00 */
                        _jxr_wbitstream_uint2(str, 0);
                        break;
                }
            }
            break;
        default:
            assert(0);
    }
}

void _jxr_w_MB_CBP(jxr_image_t image, struct wbitstream*str,
                     int /*alpha_flag*/,
                     unsigned tx, unsigned ty,
                     unsigned mx, unsigned my)
{
    DEBUG(" MB_CBP tile=[%u %u] mb=[%u %u] bitpos=%zu\n",
        tx, ty, mx, my, _jxr_wbitstream_bitpos(str));

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

    /* Get the diff_cbp values for all the channels. */
    int diff_cbp[MAX_CHANNELS];
    int ch;
    for (ch = 0 ; ch < image->num_channels ; ch += 1)
        diff_cbp[ch] = MACROBLK_CUR(image,ch,tx,mx).hp_diff_cbp;

    /* Now process the diff_cbp values for the channels. Note that
    if this is a YUV image, then channels is 1 and the loop
    knows to process three planes at once. */
    for (ch = 0 ; ch < channels ; ch += 1) {

        /* The CBP Block mask is a 4-bit mask that indicates
        which bit-blocks of the 16bit diff_cbp has non-zero
        bits in them. The cbp_X_block_masks are 4bit values
        that indicate which nibbles of the 16bit diff_cbp
        values have bits set. */
        int cbp_Y_block_mask = 0;
        int cbp_U_block_mask = 0;
        int cbp_V_block_mask = 0;
        if (diff_cbp[ch] & 0x000f)
            cbp_Y_block_mask |= 0x001;
        if (diff_cbp[ch] & 0x00f0)
            cbp_Y_block_mask |= 0x002;
        if (diff_cbp[ch] & 0x0f00)
            cbp_Y_block_mask |= 0x004;
        if (diff_cbp[ch] & 0xf000)
            cbp_Y_block_mask |= 0x008;


        switch (image->use_clr_fmt) {
            /* If thie is a YUV420 image, then the block mask
            includes masks for all the 3 planes. The UV
            planes' have only 1 CBP bit per block, so this
            is simple. */
            case 1: /* YUV420 */
                if (diff_cbp[1] & 0x01)
                    cbp_U_block_mask |= 0x01;
                if (diff_cbp[1] & 0x02)
                    cbp_U_block_mask |= 0x02;
                if (diff_cbp[1] & 0x04)
                    cbp_U_block_mask |= 0x04;
                if (diff_cbp[1] & 0x08)
                    cbp_U_block_mask |= 0x08;
                if (diff_cbp[2] & 0x01)
                    cbp_V_block_mask |= 0x01;
                if (diff_cbp[2] & 0x02)
                    cbp_V_block_mask |= 0x02;
                if (diff_cbp[2] & 0x04)
                    cbp_V_block_mask |= 0x04;
                if (diff_cbp[2] & 0x08)
                    cbp_V_block_mask |= 0x08;
                break;
                /* If this is a YUV422 iamge, then the block mask
                includes masks for all the 3 planes, with the
                UV diff_cbp bits encoded funny. */
            case 2: /* YUV422*/
                if (diff_cbp[1] & 0x05)
                    cbp_U_block_mask |= 0x01;
                if (diff_cbp[1] & 0x0a)
                    cbp_U_block_mask |= 0x02;
                if (diff_cbp[1] & 0x50)
                    cbp_U_block_mask |= 0x04;
                if (diff_cbp[1] & 0xa0)
                    cbp_U_block_mask |= 0x08;
                if (diff_cbp[2] & 0x05)
                    cbp_V_block_mask |= 0x01;
                if (diff_cbp[2] & 0x0a)
                    cbp_V_block_mask |= 0x02;
                if (diff_cbp[2] & 0x50)
                    cbp_V_block_mask |= 0x04;
                if (diff_cbp[2] & 0xa0)
                    cbp_V_block_mask |= 0x08;
                break;
                /* If this is a YUV444 image, then the block mask
                includes masks for all 3 planes. */
            case 3: /*YUV444*/
                assert(ch == 0);
                if (diff_cbp[1] & 0x000f)
                    cbp_U_block_mask |= 0x01;
                if (diff_cbp[1] & 0x00f0)
                    cbp_U_block_mask |= 0x02;
                if (diff_cbp[1] & 0x0f00)
                    cbp_U_block_mask |= 0x04;
                if (diff_cbp[1] & 0xf000)
                    cbp_U_block_mask |= 0x08;
                if (diff_cbp[2] & 0x000f)
                    cbp_V_block_mask |= 0x01;
                if (diff_cbp[2] & 0x00f0)
                    cbp_V_block_mask |= 0x02;
                if (diff_cbp[2] & 0x0f00)
                    cbp_V_block_mask |= 0x04;
                if (diff_cbp[2] & 0xf000)
                    cbp_V_block_mask |= 0x08;
                break;
            default:
                break;
        }

        DEBUG(" MB_CBP: diff_cbp[%d]=0x%x (HP_CBP=0x%x) cbp_block_mask=0x%x:%x:%x\n",
            ch, diff_cbp[ch], MACROBLK_CUR_HPCBP(image,ch,tx,mx),
            cbp_Y_block_mask, cbp_U_block_mask, cbp_V_block_mask);

        /* A bit in the CBP (up to 4 bits) is true if the
        corresponding bit of any of hte YUV planes' cbp is
        set. That tells the decoder to look for CBP values
        in any of the planes. */
        int cbp = cbp_Y_block_mask | cbp_U_block_mask | cbp_V_block_mask;
        DEBUG(" MB_CBP: Refined CBP=0x%x\n", cbp);
        w_REFINE_CBP(image, str, cbp);

        int block;
        for (block = 0 ; block < 4 ; block += 1) {
            /* If there are no diff_cbp bits in this nibble of
            any of the YUV planes, then skip. */
            if ((cbp & (1<<block)) == 0)
                continue;

            struct adaptive_vlc_s*vlc = image->vlc_table + DecNumBlkCBP;
            assert(vlc->deltatable == 0);

            DEBUG(" MB_CBP: block=%d Use DecNumBlkCBP table=%d, discriminant=%d, bitpos=%zu\n",
                block, vlc->table, vlc->discriminant, _jxr_wbitstream_bitpos(str));

            /* This is the block of CBP bits to encode. The
            blkcbp is the nibble (indexed by "block") of
            the diff_cbp for the Y plane, also also bits
            set if the UV diff_cbp values have bits
            set. The cbp_chr_X masks have the nibbles of
            the UV diff_cbp. */
            int blkcbp = (diff_cbp[ch] >> 4*block) & 0x000f;

            if (cbp_U_block_mask & (1<<block))
                blkcbp |= 0x10;
            if (cbp_V_block_mask & (1<<block))
                blkcbp |= 0x20;

            /* Map the CBP bit block to a more encodable
            code. Note that this map doesn't look at the
            chroma mask bits because this code is used to
            generate only the low 4 bits by the receiver. */
            static const int code_from_blkcbp[16] = {
                0, 4, 5, 2,
                6, 8, 9,12,
                7,10,11,13,
                3,14,15, 1 };
                int code = code_from_blkcbp[blkcbp&0x0f];
                /* Break the code down to a further encoded val
                and refinement bit count. */
                static const int val_from_code[16] = {
                    0, 5, 2, 2,
                    1, 1, 1, 1,
                    3, 3, 3, 3,
                    4, 4, 4, 4 };
                    int val = val_from_code[code];
                    int chr_cbp = (blkcbp >> 4) & 0x3;
                    if (chr_cbp > 0) {
                        chr_cbp -= 1;
                        val += 6;
                    }

                    assert(val > 0);
                    int num_blkcbp = val-1;

                    DEBUG(" MB_CBP: NUM_BLKCBP=%d, iVal=%d, iCode=%d\n", num_blkcbp, val, code);
                    DEBUG(" MB_CBP: blkcbp=0x%x, code=%d for chunk blk=%d\n", blkcbp, code, block);

                    /* Adapt DecNumBlkCBP based on the num_blkcbp value. */
                    if (image->use_clr_fmt==0 || image->use_clr_fmt==4 || image->use_clr_fmt==6) {
                        assert(num_blkcbp < 5);
                        static const int Num_BLKCBP_Delta5[5] = {0, -1, 0, 1, 1};
                        vlc->discriminant += Num_BLKCBP_Delta5[num_blkcbp];
                    } else {
                        int tmp = val-1;
                        if (tmp > 8) {
                            assert(tmp < 11);
                            tmp = 8;
                        }
                        assert(tmp < 9);
                        static const int Num_BLKCBP_Delta9[9] = {2, 2, 1, 1, -1, -2, -2, -2, -3};
                        vlc->discriminant += Num_BLKCBP_Delta9[tmp];
                    }

                    DEBUG(" MB_CBP: NUM_BLKCBP=%d, discriminant becomes=%d, \n",
                        num_blkcbp, vlc->discriminant);

                    /* Encode VAL-1, and CHR_CBP if present. */
                    encode_val_1(image, str, vlc, val-1, chr_cbp);

                    /* Encode CODE_INC */
                    static const int code_inc_from_code[16] = {
                        0, 0, 0, 1,
                        0, 1, 2, 3,
                        0, 1, 2, 3,
                        0, 1, 2, 3 };
                        static const int code_inc_bits_from_code[16] = {
                            0, 0, 1, 1,
                            2, 2, 2, 2,
                            2, 2, 2, 2,
                            2, 2, 2, 2 };
                            assert(code < 16);
                            switch (code_inc_bits_from_code[code]) {
                                case 0:
                                    break;
                                case 1:
                                    _jxr_wbitstream_uint1(str, code_inc_from_code[code]);
                                    break;
                                case 2:
                                    _jxr_wbitstream_uint2(str, code_inc_from_code[code]);
                                    break;
                                default:
                                    assert(0);
                            }

                            int cbp_chr_U = 0;
                            int cbp_chr_V = 0;
                            switch (image->use_clr_fmt) {
                                case 0: /* YONLY */
                                case 4: /* YUVK */
                                case 6: /* NCOMPONENT */
                                    break;
                                case 1:/* YUV420 */
                                    /* Nothing to encode. The CHR_CBP bits are
                                    sufficient to carry all the UV CBP
                                    informtation for YUV420 UV planes. */
                                    break;
                                case 2: /* YUV422 */
                                    if (blkcbp & 0x10)
                                        refine_cbp_chr422(image, str, diff_cbp[1], block);
                                    if (blkcbp & 0x20)
                                        refine_cbp_chr422(image, str, diff_cbp[2], block);
                                    break;
                                case 3: /* YUV444 */
                                    cbp_chr_U = (diff_cbp[1] >> 4*block) & 0x000f;
                                    cbp_chr_V = (diff_cbp[2] >> 4*block) & 0x000f;
                                    if (blkcbp & 0x10) {
                                        DEBUG(" MB_CBP: Refined CBP_U=0x%x\n", cbp_chr_U);
                                        w_REFINE_CBP_CHR(image, str, cbp_chr_U);
                                    }
                                    if (blkcbp & 0x20) {
                                        DEBUG(" MB_CBP: Refined CBP_V=0x%x\n", cbp_chr_V);
                                        w_REFINE_CBP_CHR(image, str, cbp_chr_V);
                                    }
                                    break;
                                default:
                                    assert(0);
                                    break;
                            }
        }
    }

    DEBUG(" MB_CBP done tile=[%u %u] mb=[%u %u]\n", tx, ty, mx, my);
}

static void refine_cbp_chr422(jxr_image_t /*image*/, struct wbitstream*str, int diff_cbp, int block)
{
    static const int shift_mask[4] = {0, 1, 4, 5};
    int bits = diff_cbp >> shift_mask[block];
    switch (bits & 5) {
        case 0:
            assert(0);
            break;
        case 1: /* 1 */
            _jxr_wbitstream_uint1(str, 1);
            break;
        case 4: /* 01 */
            _jxr_wbitstream_uint2(str, 1);
            break;
        case 5: /* 00 */
            _jxr_wbitstream_uint2(str, 0);
            break;
    }
}

static void refine_cbp_core(jxr_image_t /*image*/, struct wbitstream*str, int cbp_mask)
{
    switch (cbp_mask) {
        case 0x0:
            /* If there are no CBP bits, then encode nothing. */
        case 0xf:
            break;
            /* REF_CBP (num_cbp==1) */
        case 0x1:
            _jxr_wbitstream_uint2(str, 0);
            break;
        case 0x2:
            _jxr_wbitstream_uint2(str, 1);
            break;
        case 0x4:
            _jxr_wbitstream_uint2(str, 2);
            break;
        case 0x8:
            _jxr_wbitstream_uint2(str, 3);
            break;
            /* REF_CBP1 (num_cbp==2) */
        case 0x3:
            _jxr_wbitstream_uint2(str, 0);
            break;
        case 0x5:
            _jxr_wbitstream_uint2(str, 1);
            break;
        case 0x6:
            _jxr_wbitstream_uint1(str, 1);
            _jxr_wbitstream_uint2(str, 0);
            break;
        case 0x9:
            _jxr_wbitstream_uint1(str, 1);
            _jxr_wbitstream_uint2(str, 1);
            break;
        case 0xa:
            _jxr_wbitstream_uint1(str, 1);
            _jxr_wbitstream_uint2(str, 2);
            break;
        case 0xc:
            _jxr_wbitstream_uint1(str, 1);
            _jxr_wbitstream_uint2(str, 3);
            break;
            /* REF_CBP (num_cbp==3) */
        case 0xe:
            _jxr_wbitstream_uint2(str, 0);
            break;
        case 0xd:
            _jxr_wbitstream_uint2(str, 1);
            break;
        case 0xb:
            _jxr_wbitstream_uint2(str, 2);
            break;
        case 0x7:
            _jxr_wbitstream_uint2(str, 3);
            break;
    }
}

static void w_REFINE_CBP(jxr_image_t image, struct wbitstream*str, int cbp_block_mask)
{
    DEBUG(" REFINE_CBP: input CBP=%d (0x%x)\n", cbp_block_mask, cbp_block_mask);

    int num_cbp = 0;
    int idx;
    for (idx = 0 ; idx < 4 ; idx += 1) {
        if (cbp_block_mask & (1<<idx))
            num_cbp += 1;
    }

    struct adaptive_vlc_s*vlc = image->vlc_table + DecNumCBP;

    assert(vlc->deltatable == 0 && num_cbp < 5);
    static const int Num_CBP_Delta[5] = {0, -1, 0, 1, 1};
    vlc->discriminant += Num_CBP_Delta[num_cbp];


    /* First encode the NUM_CBP. (The REFINE_CBP in the spec does
    not include this step, but we include it here because it is
    part of the cbp_block_mask encoding.) */
    int vlc_table = vlc->table;
    assert(vlc_table < 2);

    DEBUG(" REFINE_CBP: num_cbp=%d, vlc table=%d\n", num_cbp, vlc_table);

    if (vlc_table == 0) { 
        switch (num_cbp) {
            case 4:
                _jxr_wbitstream_uint1(str, 0);
				// falls through
            case 2:
                _jxr_wbitstream_uint1(str, 0);
				// falls through
            case 1:
                _jxr_wbitstream_uint1(str, 0);
				// falls through
            case 0:
                _jxr_wbitstream_uint1(str, 1);
                break;
            case 3:
                _jxr_wbitstream_uint1(str, 0);
                _jxr_wbitstream_uint1(str, 0);
                _jxr_wbitstream_uint1(str, 0);
                _jxr_wbitstream_uint1(str, 0);
                break;
        } 
    }
    else {
        switch (num_cbp) {
            case 0:
                _jxr_wbitstream_uint1(str, 1);
                break;
            default:
                _jxr_wbitstream_uint1(str, 0);
                _jxr_wbitstream_uint2(str, num_cbp-1);
                break;
        }
    }

    /* Now encode the CBP itself. Note that many encodings look
    the same. The decoder uses the NUM_CBP encoded above to
    distinguish between the different possible values. */
    refine_cbp_core(image, str, cbp_block_mask);
}

static void w_REFINE_CBP_CHR(jxr_image_t image, struct wbitstream*str, int cbp_block_mask)
{
    DEBUG(" REFINE_CBP: input CBP(CHR)= 0x%x)\n", cbp_block_mask);

    int num_cbp = 0;
    int idx;
    for (idx = 0 ; idx < 4 ; idx += 1) {
        if (cbp_block_mask & (1<<idx))
            num_cbp += 1;
    }

    /* If refining a CBP block mask for a chroma plane, then we
    know that there must be at least one block bit set, so we
    can encode num_cbp-1 instead, and possibly save space. */
    assert(num_cbp > 0);
    num_cbp -= 1;

    /* First encode the NUM_CBP. (The REFINE_CBP in the spec does
    not include this step, but we include it here because it is
    part of the cbp_block_mask encoding.) */
    DEBUG(" REFINE_CBP(CHR): num_ch_blk=%d\n", num_cbp);

    assert(num_cbp < 4);
    switch (num_cbp) {
        case 0: /* 1 */
            _jxr_wbitstream_uint1(str,1);
            break;
        case 1: /* 01 */
            _jxr_wbitstream_uint2(str, 1);
            break;
        case 2: /* 000 */
            _jxr_wbitstream_uint2(str, 0);
            _jxr_wbitstream_uint1(str, 0);
            break;
        case 3: /* 001 */
            _jxr_wbitstream_uint2(str, 0);
            _jxr_wbitstream_uint1(str, 1);
            break;
    }

    /* Now encode the CBP itself. Note that many encodings look
    the same. The decoder uses the NUM_CBP encoded above to
    distinguish between the different possible values. */
    refine_cbp_core(image, str, cbp_block_mask);
}


int _jxr_w_MB_HP(jxr_image_t image, struct wbitstream*str,
                   int /*alpha_flag*/,
                   unsigned tx, unsigned ty,
                   unsigned mx, unsigned my,
                   struct wbitstream*strFB)
{
    /* This function can act either as MB_HP() or MB_HP_FLEX() */
    DEBUG(" MB_HP tile=[%u %u] mb=[%u %u] bitpos=%zu\n",
        tx, ty, mx, my, _jxr_wbitstream_bitpos(str));

    if (_jxr_InitContext(image, tx, ty, mx, my)) {
        DEBUG(" MB_HP: InitContext\n");
        /* This happens only at the top left edge of the tile. */
        _jxr_InitHPVLC(image);
        _jxr_InitializeAdaptiveScanHP(image);
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

    int mbhp_pred_mode = MACROBLK_CUR(image,0,tx,mx).mbhp_pred_mode;

    int idx;
    for (idx = 0 ; idx < image->num_channels; idx += 1) {
        int chroma_flag = idx>0? 1 : 0;
        int nblocks = 4;
        if (chroma_flag && image->use_clr_fmt==1/*YUV420*/)
            nblocks = 1;
        else if (chroma_flag && image->use_clr_fmt==2/*YUV422*/)
            nblocks = 2;

        unsigned model_bits = MACROBLK_CUR(image,0,tx,mx).hp_model_bits[chroma_flag];
        int cbp = MACROBLK_CUR_HPCBP(image, idx, tx, mx);
        int block;
        DEBUG(" MB_HP channel=%d, cbp=0x%x, model_bits=%u MBHPMode=%d\n",
            idx, cbp, model_bits, mbhp_pred_mode);
        for (block=0 ; block<(nblocks*4) ; block += 1, cbp >>= 1) {
            int bpos = block;
            /* Only remap the Y plane of YUV42X data. */
            if (nblocks == 4)
                bpos = _jxr_hp_scan_map[block];
            int num_nonzero = w_DECODE_BLOCK_ADAPTIVE(image, str, tx, mx,
                cbp&1, chroma_flag,
                idx, bpos, mbhp_pred_mode,
                model_bits);
            if (num_nonzero < 0) {
                DEBUG("ERROR: r_DECODE_BLOCK_ADAPTIVE returned rc=%d\n", num_nonzero);
                return JXR_EC_ERROR;
            }
            if (strFB)
                w_BLOCK_FLEXBITS(image, strFB, tx, ty, mx, my,
                idx, bpos, model_bits);
            else if (flex_flag)
                w_BLOCK_FLEXBITS(image, str, tx, ty, mx, my,
                idx, bpos, model_bits);
        }

    }

    if (_jxr_ResetContext(image, tx, mx)) {
        DEBUG(" MB_HP: Run AdaptHP\n");
        _jxr_AdaptHP(image);
    }

    DEBUG(" MB_HP DONE tile=[%u %u] mb=[%u %u]\n", tx, ty, mx, my);
    return 0;
}

static void w_DECODE_FLEX(jxr_image_t image, struct wbitstream*str,
                          unsigned tx, unsigned mx,
                          int ch, unsigned block, unsigned k,
                          unsigned flexbits)
{
    int coeff = MACROBLK_CUR_HP(image, ch, tx, mx, block, k);
    int sign = 0;
    if (coeff < 0) {
        sign = 1;
        coeff = -coeff;
    }

    coeff >>= image->trim_flexbits;
    int mask = (1 << flexbits) - 1;

    int flex_ref = coeff & mask;
    coeff &= ~mask;
    DEBUG(" DECODE_FLEX: coeff=0x%08x, flex_ref=0x%08x\n", coeff, flex_ref);

    _jxr_wbitstream_uintN(str, flex_ref, flexbits);

    if (coeff == 0 && flex_ref != 0)
        _jxr_wbitstream_uint1(str, sign);
}

static void w_BLOCK_FLEXBITS(jxr_image_t image, struct wbitstream*str,
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
        flexbits_left, model_bits, image->trim_flexbits, bl, _jxr_wbitstream_bitpos(str));

    if (flexbits_left > 0) {
        int idx;
        for (idx = 1 ; idx < 16 ; idx += 1) {
            int idx_trans = transpose444[idx];
            w_DECODE_FLEX(image, str, tx, mx, ch, bl, idx_trans-1, flexbits_left);
        }
    }

    DEBUG(" BLOCK_FLEXBITS done\n");
}

static void w_DEC_DC(jxr_image_t image, struct wbitstream*str,
                     int model_bits, int chroma_flag, int 
#ifndef JPEGXR_ADOBE_EXT
						is_dc_ch
#endif //#ifndef JPEGXR_ADOBE_EXT
					 , int32_t dc_val)
{
    int idx;
    int sign_flag = 0;
    int zero_flag = dc_val==0;

    DEBUG(" DEC_DC: DC value is %d (0x%08x)\n", dc_val, dc_val);

    /* Values are encoded as magnitude/sign, and *not* the 2s
    complement value. So here we get the sign of the value and
    convert it to a magnitude for encoding. */
    if (dc_val < 0) {
        sign_flag = 1;
        dc_val = -dc_val;
    }

    /* Pull the LSB bits from the value and save them in a stack
    of DC_REF values. This reduces the number of bits in the dc
    val that will be encoded. */
    uint32_t dc_ref_stack = 0;
    for (idx = 0 ; idx < model_bits ; idx += 1) {
        dc_ref_stack <<= 1;
        dc_ref_stack |= dc_val&1;
        dc_val >>= 1;
    }

    /* The is_dc_ch flag only gates the encoding of the high bits
    of the absolute level. If the level is non-zero, then
    presumably the is_dc_ch flag is true and we go ahead and
    encode the level. The model_bits are encoded in any case. */
    if (dc_val != 0) {
#ifndef JPEGXR_ADOBE_EXT
        assert( is_dc_ch );
#endif //#ifndef JPEGXR_ADOBE_EXT
        DEBUG(" DEC_DC: DECODE_ABS_LEVEL = %d (0x%08x)\n", dc_val, dc_val);
        w_DECODE_ABS_LEVEL(image, str, 0/*DC*/, chroma_flag, dc_val + 1);
    }

    /* Play back the bits in the dc_ref stack. This causes them to
    appear MSB first in the DC_REF field. These are the
    model_bits, the LSB of the level. */
    for (idx = 0 ; idx < model_bits ; idx += 1) {
        _jxr_wbitstream_uint1(str, dc_ref_stack&1);
        dc_ref_stack >>= 1;
    }

    /* The sign bit is last. Include it only if the value is nonzero*/
    if (!zero_flag) {
        DEBUG(" DEC_DC: sign_flag=%s\n", sign_flag? "true":"false");
        _jxr_wbitstream_uint1(str, sign_flag);
    }
}

static void encode_abslevel_index(jxr_image_t image, struct wbitstream*str,
                                  int abslevel_index, int vlc_select);

/*
* This function actually *ENCODES* the ABS_LEVEL.
*/
static void w_DECODE_ABS_LEVEL(jxr_image_t image, struct wbitstream*str,
                               int band, int chroma_flag, uint32_t level)
{
    int vlc_select = _jxr_vlc_select(band, chroma_flag);
    DEBUG(" Use vlc_select = %s (table=%d) to encode level index, bitpos=%zu\n",
        _jxr_vlc_index_name(vlc_select), image->vlc_table[vlc_select].table,
        _jxr_wbitstream_bitpos(str));

    const uint32_t abslevel_limit[6] = { 2, 3, 5, 9, 13, 17 };
    const uint32_t abslevel_remap[6] = {2, 3, 4, 6, 10, 14};
    const int abslevel_fixedlen[6] = {0, 0, 1, 2, 2, 2};

    /* choose the smallest level index that can carry "level". the
    abslevel_limit array holds the maximim value that each
    index can encode. */
    int abslevel_index = 0;
    while (abslevel_index < 6 && abslevel_limit[abslevel_index] < level) {
        abslevel_index += 1;
    }

    DEBUG(" ABSLEVEL_INDEX = %d\n", abslevel_index);
    encode_abslevel_index(image, str, abslevel_index, vlc_select);

    image->vlc_table[vlc_select].discriminant += _jxr_abslevel_index_delta[abslevel_index];

    if (abslevel_index < 6) {
        /* The level index encodes most of the level value. The
        abslevel_remap is the actual value that the index
        encodes. The fixedlen array then gives the number of
        extra bits available to encode the last bit of
        value. This *must* be enough. */

        int idx;
        int fixedlen = abslevel_fixedlen[abslevel_index];
        uint32_t level_ref = level - abslevel_remap[abslevel_index];

        DEBUG(" ABS_LEVEL = 0x%x (fixed = %d, level_ref = %d)\n",
            level, fixedlen, level_ref);

        /* Stack the residual bits... */
        uint32_t level_ref_stack = 0;
        for (idx = 0 ; idx < fixedlen ; idx += 1) {
            level_ref_stack <<= 1;
            level_ref_stack |= level_ref & 1;
            level_ref >>= 1;
        }

        assert(level_ref == 0);

        /* Emit the residual bits in MSB order. */
        for (idx = 0 ; idx < fixedlen ; idx += 1) {
            _jxr_wbitstream_uint1(str, level_ref_stack&1);
            level_ref_stack >>= 1;
        }

    } else {
        uint32_t level_ref = level - 2;
        assert(level_ref > 1);
        unsigned fixed = 0;
        while (level_ref > 1) {
            level_ref >>= 1;
            fixed += 1;
        }

        assert(level >= ((1U<<fixed) + 2));
        level_ref = level - (1<<fixed) - 2;
        DEBUG(" ABS_LEVEL = 0x%x (fixed = %d, level_ref = %u\n", level, fixed, level_ref);

        unsigned fixed_tmp = fixed - 4;
        if (fixed_tmp < 15) {
            _jxr_wbitstream_uint4(str, fixed_tmp);

        } else {
            _jxr_wbitstream_uint4(str, 0xf);
            fixed_tmp -= 15;
            if (fixed_tmp < 3) {
                _jxr_wbitstream_uint2(str, fixed_tmp);
            } else {
                _jxr_wbitstream_uint2(str, 0x3);
                fixed_tmp -= 3;
                assert(fixed_tmp < 8);
                _jxr_wbitstream_uint3(str, fixed_tmp);
            }
        }
        _jxr_wbitstream_uintN(str, level_ref, fixed);
    }
}

static void encode_abslevel_index(jxr_image_t image, struct wbitstream*str,
                                  int abslevel_index, int vlc_select)
{
    int table = image->vlc_table[vlc_select].table;
    assert(table==0 || table==1);

    if (table==0) {
        switch (abslevel_index) {
            case 0:
                _jxr_wbitstream_uint2(str, 0x1);
                break;
            case 1:
                _jxr_wbitstream_uint2(str, 0x2);
                break;
            case 2:
                _jxr_wbitstream_uint2(str, 0x3);
                break;
            case 3:
                _jxr_wbitstream_uint2(str, 0x0);
                _jxr_wbitstream_uint1(str, 0x1);
                break;
            case 4:
                _jxr_wbitstream_uint4(str, 0x1);
                break;
            case 5:
                _jxr_wbitstream_uint4(str, 0x0);
                _jxr_wbitstream_uint1(str, 0x0);
                break;
            case 6:
                _jxr_wbitstream_uint4(str, 0x0);
                _jxr_wbitstream_uint1(str, 0x1);
                break;
        }
    } 
    else {
        switch (abslevel_index) {
            case 0:
                _jxr_wbitstream_uint1(str, 0x1);
                break;
            case 1:
                _jxr_wbitstream_uint2(str, 0x1);
                break;
            case 2:
                _jxr_wbitstream_uint2(str, 0x0);
                _jxr_wbitstream_uint1(str, 0x1);
                break;
            case 3:
                _jxr_wbitstream_uint4(str, 0x1);
                break;
            case 4:
                _jxr_wbitstream_uint4(str, 0x0);
                _jxr_wbitstream_uint1(str, 0x1);
                break;
            case 5:
                _jxr_wbitstream_uint4(str, 0x0);
                _jxr_wbitstream_uint2(str, 0x0);
                break;
            case 6:
                _jxr_wbitstream_uint4(str, 0x0);
                _jxr_wbitstream_uint2(str, 0x1);
                break;
        }
    }
}

static void w_DECODE_BLOCK(jxr_image_t image, struct wbitstream*str, int band, int chroma_flag,
                           const int RLCoeffs[32], int num_non_zero)
{
    int location = 1;
    /* if CLR_FMT is YUV420 or YUV422 */
    if (image->use_clr_fmt==1/*YUV420*/ && chroma_flag && band==1)
        location = 10;
    if (image->use_clr_fmt==2/*YUV422*/ && chroma_flag && band==1)
        location = 2;
    int index_code = 0;
    int sign_flag = 0;
    int value = RLCoeffs[1];

    if (value < 0) {
        sign_flag = 1;
        value = -value;
    }

    /* FIRST_INDEX */
    if (RLCoeffs[0] == 0)
        index_code |= 1;
    if (value != 1)
        index_code |= 2;

    if (num_non_zero == 1)
        index_code |= 0<<2;
    else if (RLCoeffs[2] == 0)
        index_code |= 1<<2;
    else
        index_code |= 2<<2;

    DEBUG(" DECODE_BLOCK: chroma_flag=%d, band=%d value=%s%d num_non_zero=%d index_code=0x%x bitpos=%zu\n",
        chroma_flag, band, sign_flag?"-":"+", value, num_non_zero, index_code, _jxr_wbitstream_bitpos(str));

    DEBUG(" coeff[0] = %d (run)\n", RLCoeffs[0]);
    DEBUG(" coeff[1] = 0x%x\n", RLCoeffs[1]);
    DEBUG(" coeff[1*2+0] = %d (run)\n", RLCoeffs[2]);

    int context = (index_code&1) & (index_code>>2);

    /* Encode FIRST_INDEX */
    w_DECODE_FIRST_INDEX(image, str, chroma_flag, band, index_code);
    /* SIGN_FLAG */
    _jxr_wbitstream_uint1(str, sign_flag);
    if (index_code&2) {
        DEBUG(" DECODE_BLOCK: DECODE_ABS_LEVEL = %d (0x%08x)\n", value, value);
        w_DECODE_ABS_LEVEL(image, str, band, context, value);
    }
    if ((index_code&1) == 0) {
        w_DECODE_RUN(image, str, 15-location, RLCoeffs[0]);
    }
    location += 1 + RLCoeffs[0];

    int nz_index = 1;
    while (nz_index < num_non_zero) {
        /* If the previous index didn't already flag this as a
        zero run, then encode the run now. */
        if (RLCoeffs[2*nz_index+0] > 0)
            w_DECODE_RUN(image, str, 15-location, RLCoeffs[2*nz_index+0]);

        location += 1 + RLCoeffs[2*nz_index+0];

        value = RLCoeffs[2*nz_index+1];
        sign_flag = 0;
        if (value < 0) {
            sign_flag = 1;
            value = -value;
        }

        /* index_code */
        index_code = 0;
        if (value != 1)
            index_code |= 1;
        if (nz_index+1 == num_non_zero)
            index_code |= 0<<1;
        else if (RLCoeffs[2*nz_index+2] == 0)
            index_code |= 1<<1;
        else
            index_code |= 2<<1;

        DEBUG(" DECODE_BLOCK: nz_index=%u, index_code=%x\n", nz_index, index_code);
        w_DECODE_INDEX(image, str, location, chroma_flag, band, context, index_code);
        context &= index_code >> 1;

        _jxr_wbitstream_uint1(str, sign_flag);
        if (value != 1) {
            w_DECODE_ABS_LEVEL(image, str, band, context, value);
        }

        nz_index += 1;
    }
}

static void w_DECODE_FIRST_INDEX(jxr_image_t image, struct wbitstream*str,
                                 int chroma_flag, int band, int first_index)
{
    struct encode_table_s{
        unsigned char bits;
        unsigned char len;
    };

    typedef struct encode_table_s first_index_table_t[12];
    static const first_index_table_t first_index_vlc[5] = {
        { /* VLC0 */
            { 0x08, 5 }, /* 0 == 0000 1... */
            { 0x04, 6 }, /* 1 == 0000 01.. */
            { 0x00, 7 }, /* 2 == 0000 000. */
            { 0x02, 7 }, /* 3 == 0000 001. */
            { 0x20, 5 }, /* 4 == 0010 0... */
            { 0x40, 3 }, /* 5 == 010. .... */
            { 0x28, 5 }, /* 6 == 0010 1... */
            { 0x80, 1 }, /* 7 == 1... .... */
            { 0x30, 5 }, /* 8 == 0011 0... */
            { 0x10, 4 }, /* 9 == 0001 .... */
            { 0x38, 5 }, /* 10 == 0011 1... */
            { 0x60, 3 } /* 11 == 011. .... */
        },
        { /* VLC1 */
            { 0x20, 4 }, /* 0 == 0010 .... */
            { 0x10, 5 }, /* 1 == 0001 0... */
            { 0x00, 6 }, /* 2 == 0000 00.. */
            { 0x04, 6 }, /* 3 == 0000 01.. */
            { 0x30, 4 }, /* 4 == 0011 .... */
            { 0x40, 3 }, /* 5 == 010. .... */
            { 0x18, 5 }, /* 6 == 0001 1... */
            { 0xc0, 2 }, /* 7 == 11.. .... */
            { 0x60, 3 }, /* 8 == 011. .... */
            { 0x80, 3 }, /* 9 == 100. .... */
            { 0x08, 5 }, /* 10 == 0000 1... */
            { 0xa0, 3 } /* 11 == 101. .... */
        },
        { /* VLC2 */
            { 0xc0, 2 }, /* 0 == 11.. .... */
            { 0x20, 3 }, /* 1 == 001. .... */
            { 0x00, 7 }, /* 2 == 0000 000. */
            { 0x02, 7 }, /* 3 == 0000 001. */
            { 0x08, 5 }, /* 4 == 0000 1... */
            { 0x40, 3 }, /* 5 == 010. .... */
            { 0x04, 7 }, /* 6 == 0000 010. */
            { 0x60, 3 }, /* 7 == 011. .... */
            { 0x80, 3 }, /* 8 == 100. .... */
            { 0xa0, 3 }, /* 9 == 101. .... */
            { 0x06, 7 }, /* 10 == 0000 011. */
            { 0x10, 4 }, /* 11 == 0001 .... */
        },
        { /* VLC3 */
            { 0x20, 3 }, /* 0 == 001. .... */
            { 0xc0, 2 }, /* 1 == 11.. .... */
            { 0x00, 7 }, /* 2 == 0000 000. */
            { 0x08, 5 }, /* 3 == 0000 1... */
            { 0x10, 5 }, /* 4 == 0001 0... */
            { 0x40, 3 }, /* 5 == 010. .... */
            { 0x02, 7 }, /* 6 == 0000 001. */
            { 0x60, 3 }, /* 7 == 011. .... */
            { 0x18, 5 }, /* 8 == 0001 1... */
            { 0x80, 3 }, /* 9 == 100. .... */
            { 0x04, 6 }, /* 10 == 0000 01.. */
            { 0xa0, 3 } /* 11 == 101. .... */
            },
            { /* VLC4 */
                { 0x40, 3 }, /* 0 == 010. .... */
                { 0x80, 1 }, /* 1 == 1... .... */
                { 0x02, 7 }, /* 2 == 0000 001. */
                { 0x10, 4 }, /* 3 == 0001 .... */
                { 0x04, 7 }, /* 4 == 0000 010. */
                { 0x60, 3 }, /* 5 == 011. .... */
                { 0x00, 8 }, /* 6 == 0000 0000 */
                { 0x20, 4 }, /* 7 == 0010 .... */
                { 0x06, 7 }, /* 8 == 0000 011. */
                { 0x30, 4 }, /* 9 == 0011 .... */
                { 0x01, 8 }, /* 10 == 0000 0001 */
                { 0x08, 5 } /* 11 == 0000 1... */
            }
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
    DEBUG(" DECODE_FIRST_INDEX: vlc_select = %s, vlc_table = %d, encode 0x%x\n",
        _jxr_vlc_index_name(vlc_select), vlc_table, first_index);

    unsigned char bits = first_index_vlc[vlc_table][first_index].bits;
    unsigned char len = first_index_vlc[vlc_table][first_index].len;

    DEBUG(" bits/len = 0x%02x/%u\n", bits, len);

    while (len > 0) {
        _jxr_wbitstream_uint1(str, (bits&0x80)? 1 : 0);
        bits <<= 1;
        len -= 1;
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
}

static void w_DECODE_INDEX(jxr_image_t image, struct wbitstream*str,
                           int location, int chroma_flag, int band, int context,
                           int index_code)
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

    /* If location>15, then there can't possibly be coefficients
    after the next, so only the low bit need be encoded. */
    if (location > 15) {
        DEBUG(" DECODE_INDEX: location=%d, index_code=%d\n", location, index_code);
        assert(index_code <= 1);
        _jxr_wbitstream_uint1(str, index_code);
        return;
    }

    if (location == 15) {
        DEBUG(" DECODE_INDEX: location=%d, index_code=%d\n", location, index_code);
        /* Table 61
        * INDEX2 table
        * 0 0
        * 2 10
        * 1 110
        * 3 111
        */
        switch (index_code) {
            case 0:
                _jxr_wbitstream_uint1(str, 0);
                break;
            case 2:
                _jxr_wbitstream_uint2(str, 2);
                break;
            case 1:
                _jxr_wbitstream_uint1(str, 1);
                _jxr_wbitstream_uint2(str, 2);
                break;
            case 3:
                _jxr_wbitstream_uint1(str, 1);
                _jxr_wbitstream_uint2(str, 3);
                break;
            default:
                assert(0);
        }
        return;
    }

    int vlc_table = image->vlc_table[vlc_select].table;
    DEBUG(" DECODE_INDEX: vlc_select = %s, vlc_table = %d chroma_flag=%d, index_code=%d\n",
        _jxr_vlc_index_name(vlc_select), vlc_table, chroma_flag, index_code);

    struct encode_table_s{
        unsigned char bits;
        unsigned char len;
    };

    typedef struct encode_table_s index1_table_t[6];
    static const index1_table_t index1_vlc[5] = {
        { /* VLC0 */
            { 0x80, 1 }, /* 0 == 1... .... */
            { 0x00, 5 }, /* 1 == 0000 0... */
            { 0x20, 3 }, /* 2 == 001. .... */
            { 0x08, 5 }, /* 3 == 0000 1... */
            { 0x40, 2 }, /* 4 == 01.. .... */
            { 0x10, 4 } /* 5 == 0001 .... */
        },
        { /* VLC1 */
            { 0x40, 2 }, /* 0 == 01.. .... */
            { 0x00, 4 }, /* 1 == 0000 .... */
            { 0x80, 2 }, /* 2 == 10.. .... */
            { 0x10, 4 }, /* 3 == 0001 .... */
            { 0xc0, 2 }, /* 4 == 11.. .... */
            { 0x20, 3 } /* 5 == 001. .... */
        },
        { /* VLC2 */
            { 0x00, 4 }, /* 0 == 0000 .... */
            { 0x10, 4 }, /* 1 == 0001 .... */
            { 0x40, 2 }, /* 2 == 01.. .... */
            { 0x80, 2 }, /* 3 == 10.. .... */
            { 0xc0, 2 }, /* 4 == 11.. .... */
            { 0x20, 3 } /* 5 == 001. .... */
        },
        { /* VLC3 */
            { 0x00, 5 }, /* 0 == 0000 0... */
            { 0x08, 5 }, /* 1 == 0000 1... */
            { 0x40, 2 }, /* 2 == 01.. .... */
            { 0x80, 1 }, /* 3 == 1... .... */
            { 0x10, 4 }, /* 4 == 0001 .... */
            { 0x20, 3 } /* 5 == 001. .... */
        }
    };

    unsigned char bits = index1_vlc[vlc_table][index_code].bits;
    unsigned char len = index1_vlc[vlc_table][index_code].len;

    DEBUG(" bits/len = 0x%02x/%u\n", bits, len);

    while (len > 0) {
        _jxr_wbitstream_uint1(str, (bits&0x80)? 1 : 0);
        bits <<= 1;
        len -= 1;
    }

    int vlc_delta = image->vlc_table[vlc_select].deltatable;
    int vlc_delta2 = image->vlc_table[vlc_select].delta2table;

    typedef int deltatable_t[6];
    const deltatable_t Index1Delta[3] = {
        {-1, 1, 1, 1, 0, 1 },
        {-2, 0, 0, 2, 0, 0 },
        {-1,-1, 0, 1,-2, 0 }
    };
    image->vlc_table[vlc_select].discriminant += Index1Delta[vlc_delta][index_code];
    image->vlc_table[vlc_select].discriminant2+= Index1Delta[vlc_delta2][index_code];

    DEBUG(" DECODE_INDEX: deltatable/2 = %d/%d, discriminant/2 becomes %d/%d\n",
        vlc_delta, vlc_delta2,
        image->vlc_table[vlc_select].discriminant,
        image->vlc_table[vlc_select].discriminant2);

}

static void w_DECODE_RUN(jxr_image_t /*image*/, struct wbitstream*str, int max_run, int run)
{
    assert(run > 0);
    if (max_run < 5) {
        DEBUG(" DECODE_RUN max_run=%d (<5) run=%d, bitpos=%zu\n",
            max_run, run, _jxr_wbitstream_bitpos(str));

        switch (max_run) {
            case 1:
                assert(run == 1);
                break;
            case 2:
                assert(run <= 2);
                if (run == 2)
                    _jxr_wbitstream_uint1(str, 0);
                else
                    _jxr_wbitstream_uint1(str, 1);
                break;
            case 3:
                if (run == 1)
                    _jxr_wbitstream_uint1(str, 1);
                else if (run == 2) {
                    _jxr_wbitstream_uint1(str, 0);
                    _jxr_wbitstream_uint1(str, 1);
                } else {
                    assert(run == 3);
                    _jxr_wbitstream_uint1(str, 0);
                    _jxr_wbitstream_uint1(str, 0);
                }
                break;
            case 4:
                if (run == 1) {
                    _jxr_wbitstream_uint1(str, 1);
                } else if (run == 2) {
                    _jxr_wbitstream_uint1(str, 0);
                    _jxr_wbitstream_uint1(str, 1);
                } else if (run == 3) {
                    _jxr_wbitstream_uint1(str, 0);
                    _jxr_wbitstream_uint1(str, 0);
                    _jxr_wbitstream_uint1(str, 1);
                } else {
                    assert(run == 4);
                    _jxr_wbitstream_uint1(str, 0);
                    _jxr_wbitstream_uint1(str, 0);
                    _jxr_wbitstream_uint1(str, 0);
                }
        }
        return;
    }

    static const int remap[15] = {1,2,3,5,7,1,2,3,5,7,1,2,3,4,5};
    static const int run_bin[15] = {-1,-1,-1,-1,2,2,2,1,1,1,1,0,0,0,0};
    static const int run_fixed_len[15] = {0,0,1,1,3,0,0,1,1,2,0,0,0,0,1};

    assert(max_run < 15);
    int index_run_bin = 5*run_bin[max_run];

    int run_index = 0;
    for (run_index = 0; run_index < 5; run_index += 1) {
        int index = run_index + index_run_bin;
        if (index < 0)
            continue;

        int use_run = remap[index];
        if (use_run > run)
            continue;

        int fixed = run_fixed_len[index];
        int range = 1 << fixed;
        if (run >= (use_run + range))
            continue;

        /* RUN_INDEX */
        switch(run_index) {
            case 0:
                _jxr_wbitstream_uint1(str, 1);
                break;
            case 1:
                _jxr_wbitstream_uint2(str, 1);
                break;
            case 2:
                _jxr_wbitstream_uint1(str, 0);
                _jxr_wbitstream_uint2(str, 1);
                break;
            case 3:
                _jxr_wbitstream_uint4(str, 0);
                break;
            case 4:
                _jxr_wbitstream_uint4(str, 1);
                break;
        }

        int run_fixed = 0;
        if (fixed > 0) {
            run_fixed = run - use_run;
            assert(run_fixed >= 0 && run_fixed < (1<<fixed));
            _jxr_wbitstream_uintN(str, run_fixed, fixed);
        }

        DEBUG(" DECODE_RUN max_run=%d run=%d+%d = %d, bitpos=%zu\n",
            max_run, use_run, run_fixed, run, _jxr_wbitstream_bitpos(str));

        return;
    }
    assert(0);
}

static int w_DECODE_BLOCK_ADAPTIVE(jxr_image_t image, struct wbitstream*str,
                                   unsigned tx, unsigned mx,
                                   int cbp_flag, int chroma_flag,
                                   int channel, int block, int mbhp_pred_mode,
                                   unsigned model_bits)
{
    /* If the CBP flag is off, then there isn't really anything to
    encode here. */
    if (cbp_flag == 0)
        return 0;

    int values[16];
    values[0] = 0;
    int idx;
    DEBUG(" HP val[tx=%d, mx=%d, block=%d] ==", tx, mx, block);
    for (idx = 0 ; idx < 15 ; idx += 1) {
        values[1+idx] = MACROBLK_CUR_HP(image,channel,tx,mx,block,idx);
        /* Shift out the model bits. Be careful to note that the
        value is processed as sign/magnitude, so do the
        shifting to the abs of the value and invert it back
        if nesessary. */
        if (values[1+idx] >= 0)
            values[1+idx] >>= model_bits;
        else
            values[1+idx] = - ( (-values[1+idx]) >> model_bits );
        DEBUG(" 0x%x", values[1+idx]);
    }
    DEBUG("\n");

    int values2[16];
    values2[0] = 0;
    AdaptiveHPPermute(image, values2, values, mbhp_pred_mode);
#if defined(DETAILED_DEBUG)
    {
        int k;
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

    int coeffs[32];
    int num_nonzero = RunLengthEncode(image, coeffs, values2);

    w_DECODE_BLOCK(image, str, 2, chroma_flag, coeffs, num_nonzero);

    return num_nonzero;;
}

/*
* $Log: w_emit.c,v $
* Revision 1.28 2009/09/16 12:00:00 microsoft
* Reference Software v1.8 updates.
*
* Revision 1.27 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.26 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.25 2008/03/24 18:06:56 steve
* Imrpove DEBUG messages around quantization.
*
* Revision 1.24 2008/03/21 18:05:53 steve
* Proper CMYK formatting on input.
*
* Revision 1.23 2008/03/13 21:23:27 steve
* Add pipeline step for YUV420.
*
* Revision 1.22 2008/03/13 00:30:56 steve
* Force SCALING on if using a subsampled color format.
*
* Revision 1.21 2008/03/11 22:12:49 steve
* Encode YUV422 through DC.
*
* Revision 1.20 2008/03/05 06:58:10 gus
* *** empty log message ***
*
* Revision 1.19 2008/03/05 04:04:30 steve
* Clarify constraints on USE_DC_QP in image plane header.
*
* Revision 1.18 2008/03/05 00:31:18 steve
* Handle UNIFORM/IMAGEPLANE_UNIFORM compression.
*
* Revision 1.17 2008/03/02 19:56:27 steve
* Infrastructure to read write BD16 files.
*
* Revision 1.16 2008/02/28 18:50:31 steve
* Portability fixes.
*
* Revision 1.15 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.14 2008/02/22 23:01:33 steve
* Compress macroblock HP CBP packets.
*
* Revision 1.13 2008/02/01 22:49:53 steve
* Handle compress of YUV444 color DCONLY
*
* Revision 1.12 2008/01/08 01:06:20 steve
* Add first pass overlap filtering.
*
* Revision 1.11 2008/01/07 16:19:10 steve
* Properly configure TRIM_FLEXBITS_FLAG bit.
*
* Revision 1.10 2008/01/04 17:07:36 steve
* API interface for setting QP values.
*
* Revision 1.9 2007/12/13 18:01:09 steve
* Stubs for HP encoding.
*
* Revision 1.8 2007/12/07 01:20:34 steve
* Fix adapt not adapting on line ends.
*
* Revision 1.7 2007/12/05 18:14:19 steve
* hard-code LOG_WORD_FLAG true.
*
* Revision 1.6 2007/12/04 22:06:10 steve
* Infrastructure for encoding LP.
*
* Revision 1.5 2007/11/30 01:50:58 steve
* Compression of DCONLY GRAY.
*
* Revision 1.4 2007/11/26 01:47:16 steve
* Add copyright notices per MS request.
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

