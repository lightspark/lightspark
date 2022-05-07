#ifndef __jxr_priv_H
#define __jxr_priv_H
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
#pragma comment (user,"$Id: jxr_priv.h,v 1.3 2008-05-13 13:47:11 thor Exp $")
#else
#ident "$Id: jxr_priv.h,v 1.3 2008-05-13 13:47:11 thor Exp $"
#endif

# include "jpegxr.h"

#ifndef _MSC_VER
# include <stdint.h>
#else
/* MSVC (as of 2008) does not support C99 or the stdint.h header
file. So include a private little header file here that does the
minimal typedefs that we need. */
#ifdef JPEGXR_ADOBE_EXT
# include "jpegxr_stdint_minimal.h"
# pragma warning(disable:4018)
# pragma warning(disable:4389)
# pragma warning(disable:4710)
#else //#ifdef JPEGXR_ADOBE_EXT
# include "stdint_minimal.h"
#endif //#ifdef JPEGXR_ADOBE_EXT
#endif

#include <string.h>
#include <memory.h>
#include <stdlib.h>

#if 0 // def JPEGXR_ADOBE_EXT
#include "../../core/mmfx-external.h"
#endif //#ifdef JPEGXR_ADOBE_EXT

#if 0 // def JPEGXR_ADOBE_EXT

#define jpegxr_calloc(count, size) \
	((void*)mmfx_alloc_opt((count)*(size),MMgc::kZero))

#define jpegxr_malloc(size) \
	((void*)mmfx_alloc((size)))

#define jpegxr_free(ptr) \
	mmfx_free((uint8_t*)ptr)
	
#else //#ifdef JPEGXR_ADOBE_EXT

#define jpegxr_calloc(count, size) \
	((void*)calloc((count),(size)))

#define jpegxr_malloc(size) \
	((void*)malloc((size)))

#define jpegxr_free(ptr) \
	free((uint8_t*)ptr)

#endif //#ifdef JPEGXR_ADOBE_EXT

#ifdef JPEGXR_ADOBE_EXT

struct mbitstream {
public:

	mbitstream() { 
		m_cptr = 0; 
		m_dptr = 0; 
		m_len = 0; 
		m_pos = 0; 
		m_size = 0;
	}
	
	mbitstream(const uint8_t *data, int32_t len) { 
		m_cptr = data; 
		m_len = len; 
		m_dptr = 0; 
		m_pos = 0; 
		m_size = 0;
	}
	
	~mbitstream() {
		jpegxr_free(m_dptr);
	}

	int32_t seek(int32_t pos, int32_t mode) {
		switch ( mode ) {
			default:
			case	SEEK_SET:
					m_pos = pos;
					break;
			case	SEEK_CUR:
					m_pos += pos;
					break;
			case	SEEK_END:
					m_pos = m_len - pos;
					break;
		}
		
		if ( m_pos < 0 ) {
			m_pos = 0;
		}
		
		if ( m_pos >= m_len ) {
			if ( m_dptr ) {
				resize(m_len = (m_pos+1));
			} else {
				m_pos = (m_len-1);
			}
		}
		
		return m_pos;
	}

	inline void putbyte(uint8_t c) {
		if ( !m_dptr ) {
			m_dptr = (uint8_t *)jpegxr_malloc(65536);
			m_size = 65536;
		}

		if ( m_pos >= m_len ) {
			m_len = (m_pos+1);
		}
		
		resize(m_len);

		m_dptr[m_pos++] = c;
	}

	inline uint8_t getc() {
		if ( m_pos < m_len ) {
			if ( m_cptr ) {
				return m_cptr[m_pos++];
			}
			if ( m_dptr ) {
				return m_dptr[m_pos++];
			}
		}
		return 0;
	}

	int32_t write(const uint8_t *data, int32_t len) {
		int32_t wb = 0;
		if ( m_cptr ) {
			return 0;
		}
		for ( ; len > 0 ; len-- ) {
			putbyte(*data++);
			wb++;
		}
		return wb;
	}
	
	int32_t read(uint8_t *data, int32_t len) {
		int32_t rb = 0;
		for ( ; len > 0 && m_pos < m_len ; len-- ) {
			*data++ = getc();
			rb++;
		}
		return rb;
	}

	inline int32_t tell() const { 
		return m_pos; 
	}
	
	inline const uint8_t *buffer() const { 
		return m_dptr; 
	}
	
	inline int32_t len() const { 
		return m_len; 
	}

private:

	void resize(int32_t newSize) {
		if ( newSize >= m_size ) {
			uint8_t *nptr = (uint8_t *)jpegxr_malloc(m_size*2);
			memcpy(nptr,m_dptr,m_size);
			jpegxr_free(m_dptr);
			m_size *= 2;
			m_dptr = nptr;
		}
	}

	const uint8_t *	m_cptr;
	uint8_t	*		m_dptr;
	int32_t			m_len;
	int32_t			m_pos;
	int32_t			m_size;
	
};
#endif //#ifdef JPEGXR_ADOBE_EXT

/* define this to check if range of values exceeds signed 16-bit */
#define VERIFY_16BIT

#ifdef VERIFY_16BIT
#define CHECK1(flag, a) if(((a) < -0x8000) || ((a) >= 0x8000)) flag = 1
#else
#define CHECK1(flag, a) do { } while(0)
#endif

#define CHECK2(flag, a, b) CHECK1(flag, a); CHECK1(flag, b)
#define CHECK3(flag, a, b, c) CHECK2(flag, a, b); CHECK1(flag, c)
#define CHECK4(flag, a, b, c, d) CHECK3(flag, a, b, c); CHECK1(flag, d)
#define CHECK5(flag, a, b, c, d, e) CHECK4(flag, a, b, c, d); CHECK1(flag, e)
#define CHECK6(flag, a, b, c, d, e, f) CHECK5(flag, a, b, c, d, e); CHECK1(flag, f)

struct macroblock_s{
    int*data;
    /* This is used to temporarily hold Predicted values. */
    int*pred_dclp;
    /* */
    unsigned lp_quant : 8;
    unsigned hp_quant : 8;
    int mbhp_pred_mode : 3;
    int have_qnt : 1;/* THOR: If set, the quant values are valid */
    /* Stash HP CBP values for the macroblock. */
    int hp_cbp, hp_diff_cbp;
    /* model_bits for current HP. HP uses this to pass model_bits
    to FLEXBITS parsing. */
    unsigned hp_model_bits[2];
};

struct adaptive_vlc_s{
    int discriminant;
    int discriminant2;
    int table;
    int deltatable;
    int delta2table;
};

struct cbp_model_s{
    int state[2];
    int count0[2];
    int count1[2];
};

typedef enum abs_level_vlc_index_e{
    AbsLevelIndDCLum,
    AbsLevelIndDCChr,
    DecFirstIndLPLum,
    AbsLevelIndLP0,
    AbsLevelIndLP1,
    AbsLevelIndHP0,
    AbsLevelIndHP1,
    DecIndLPLum0,
    DecIndLPLum1,
    DecFirstIndLPChr,
    DecIndLPChr0,
    DecIndLPChr1,
    DecNumCBP,
    DecNumBlkCBP,
    DecIndHPLum0,
    DecIndHPLum1,
    DecFirstIndHPLum,
    DecFirstIndHPChr,
    DecIndHPChr0,
    DecIndHPChr1,
    AbsLevelInd_COUNT
}abs_level_vlc_index_t;

struct model_s{
    int bits[2];
    int state[2];
};

struct jxr_image{
    int user_flags;

    /* These store are the width/height minus 1 (so that 4Gwidth
    is OK but 0 width is not.) This, by the way, is how the
    width/height is stored in the HPPhoto stream too. */
    uint32_t width1;
    uint32_t height1;
    uint32_t extended_width;
    uint32_t extended_height;

    uint8_t header_flags1;
    uint8_t header_flags2;
    /* Color format of the source image data */
    uint8_t header_flags_fmt;
    jxr_output_clr_fmt_t output_clr_fmt;
    /* Color format to use internally. */
    /* 0=YONLY, 1=YUV420, 2=YUV422, 3=YUV444, 4=CMYK, 5=CMYKDIRECT, 6=NCOMPONENT */
    uint8_t use_clr_fmt;

    /* pixel format */
    jxrc_t_pixelFormat ePixelFormat;
    
    /* If TRIM_FLEXBITS_FLAG, then this is the bits to trim. */
    unsigned trim_flexbits : 4;

    /* 0==ALL, 1==NOFLEXBITS, 2==NOHIGHPASS, 3==DCONLY, 4==ISOLATED */
    uint8_t bands_present;
    uint8_t bands_present_of_primary; /* stores value of primary image plane to apply restriction on value of alpha image plane */

    uint8_t chroma_centering_x;
    uint8_t chroma_centering_y;

    uint8_t num_channels;

    /* Disable overlap flag, true for hard tiles, false for soft tiles*/
    unsigned disableTileOverlapFlag;

    unsigned tile_rows;
    unsigned tile_columns;
    unsigned*tile_row_height;
    unsigned*tile_column_width;
    /* This is the position in image macroblocks. */
    unsigned*tile_column_position;
    unsigned*tile_row_position;
    /* The numbers collected by INDEX_TABLE */
    int64_t*tile_index_table;
    int64_t tile_index_table_length;

    uint16_t window_extra_top;
    uint16_t window_extra_left;
    uint16_t window_extra_bottom;
    uint16_t window_extra_right;

    /* Information from the plane header. */

    unsigned scaled_flag : 1;
    unsigned dc_frame_uniform : 1;
    unsigned lp_use_dc_qp : 1;
    unsigned lp_frame_uniform : 1;
    unsigned hp_use_lp_qp : 1;
    unsigned hp_frame_uniform : 1;

    unsigned char shift_bits;
    unsigned char len_mantissa;
    char exp_bias;

    /* Per-tile information (changes as tiles are processed) */
    unsigned num_lp_qps;
    unsigned num_hp_qps;

    /* State variables used by encoder/decoder. */
    int cur_my; /* Address of strip_cur */
    struct{
        struct macroblock_s*up4;
        struct macroblock_s*up3;
        struct macroblock_s*up2;
        struct macroblock_s*up1;
        struct macroblock_s*cur;
        int *upsample_memory_y; /* Number of elements is dependent on number of MBs in each MB row */
        int *upsample_memory_x; /* Always contains 16 elements */
    }strip[MAX_CHANNELS];

    /* SPATIAL: Hold previous strips of current tile */
    /* FREQUENCY: mbs for the entire image. */
    struct macroblock_s*mb_row_buffer[MAX_CHANNELS];
    /* Hold final 4 strips of previous tile */
    struct macroblock_s*mb_row_context[MAX_CHANNELS];

    struct adaptive_vlc_s vlc_table[AbsLevelInd_COUNT];
    int count_max_CBPLP;
    int count_zero_CBPLP;

    struct cbp_model_s hp_cbp_model;

    unsigned lopass_scanorder[15];
    unsigned lopass_scantotals[15];

    unsigned hipass_hor_scanorder[15];
    unsigned hipass_hor_scantotals[15];
    unsigned hipass_ver_scanorder[15];
    unsigned hipass_ver_scantotals[15];

    jxr_component_mode_t dc_component_mode, lp_component_mode, hp_component_mode;

    /* Quantization parameters for up to 16 channels. */
    unsigned char dc_quant_ch[MAX_CHANNELS];
    unsigned char lp_quant_ch[MAX_CHANNELS][MAX_LP_QPS];
    unsigned char hp_quant_ch[MAX_CHANNELS][MAX_HP_QPS];
# define HP_QUANT_Y hp_quant_ch[0]

    /* Per-tile quantization data (used during encode) */
    struct jxr_tile_qp*tile_quant;
# define GET_TILE_QUANT(image,tx,ty) ((image)->tile_quant + (ty)*((image)->tile_rows+1) + (tx))

    struct model_s model_dc, model_lp, model_hp;

    struct model_s*model_hp_buffer;
    struct cbp_model_s*hp_cbp_model_buffer;

    block_fun_t out_fun;
    block_fun_t inp_fun;
    void*user_data;

    struct jxr_image * alpha;  /* interleaved alpha image plane */
    int primary;               /* primary channel or alpha channel */

    uint8_t profile_idc;
    uint8_t level_idc;

    uint8_t lwf_test; /* flag to track whether long_word_flag needs to be TRUE */

    /* store container values */
    uint32_t container_width;
    uint32_t container_height;
    uint8_t container_nc;
    uint8_t container_alpha;
    uint8_t container_separate_alpha;
    jxr_bitdepth_t container_bpc;
    jxr_output_clr_fmt_t container_color;
    uint8_t container_image_band_presence;
    uint8_t container_alpha_band_presence;
    uint8_t container_current_separate_alpha;    
};

extern unsigned char _jxr_select_lp_index(jxr_image_t image, unsigned tx, unsigned ty,
                                          unsigned mx, unsigned my);
extern unsigned char _jxr_select_hp_index(jxr_image_t image, unsigned tx, unsigned ty,
                                          unsigned mx, unsigned my);

/* User flags for controlling encode/decode */
# define SKIP_HP_DATA(image) ((image)->user_flags & 0x0001)
# define SKIP_FLEX_DATA(image) ((image)->user_flags & 0x0002)

/* Get the width of the image in macroblocks. Round up. */
# define WIDTH_BLOCKS(image) (((image)->width1 + 16) >> 4)
# define HEIGHT_BLOCKS(image) (((image)->height1 + 16) >> 4)

# define EXTENDED_WIDTH_BLOCKS(image) (((image)->extended_width) >> 4)
# define EXTENDED_HEIGHT_BLOCKS(image) (((image)->extended_height) >> 4)

/* TRUE if tiling is supported in this image. */
# define TILING_FLAG(image) ((image)->header_flags1 & 0x80)
/* TRUE if the bitstream format is frequency mode (FREQUENCY_MODE_CODESTREAM_FLAG is true), FALSE for spatial mode */
# define FREQUENCY_MODE_CODESTREAM_FLAG(image) ((image)->header_flags1 & 0x40)
/* TRUE if the INDEXTABLE_PRESENT_FLAG is set for the image. */
# define INDEXTABLE_PRESENT_FLAG(image) ((image)->header_flags1 & 0x04)
/* OVERLAP value: 0, 1, 2 or 3 */
# define OVERLAP_INFO(image) ((image)->header_flags1 & 0x03)
/* LONG_WORD_FLAG */
# define LONG_WORD_FLAG(image) ((image)->header_flags2 & 0x40) 

/* TRUE if the SHORT_HEADER flag is set. */
# define SHORT_HEADER_FLAG(image) ((image)->header_flags2 & 0x80)
/* TRUE if windowing is allowed, 0 otherwise. */
# define WINDOWING_FLAG(image) ((image)->header_flags2 & 0x20)
# define TRIM_FLEXBITS_FLAG(image) ((image)->header_flags2 & 0x10)
# define ALPHACHANNEL_FLAG(image) ((image)->header_flags2 & 0x01)

# define SOURCE_CLR_FMT(image) (((image)->header_flags_fmt >> 4) & 0x0f)

/* SOURCE_BITDEPTH (aka OUTPUT_BITDEPTH) can be:
* 0 BD1WHITE1
* 1 BD8
* 2 BD16
* 3 BD16S
* 4 BD16F
* 5 RESERVED
* 6 BD32S
* 7 BD32F
* 8 BD5
* 9 BD10
* 10 BD565
* 11 RESERVED
* 12 RESERVED
* 13 RESERVED
* 14 RESERVED
* 15 BD1BLACK1
*/
# define SOURCE_BITDEPTH(image) (((image)->header_flags_fmt >> 0) & 0x0f)

/*
* Given the tile index and macroblock index within the tile, return
* the l-value macroblock structure.
*/
# define MACROBLK_CUR(image,c,tx,mx) ((image)->strip[c].cur[(image)->tile_column_position[tx]+mx])
# define MACROBLK_UP1(image,c,tx,mx) ((image)->strip[c].up1[(image)->tile_column_position[tx]+mx])
# define MACROBLK_UP2(image,c,tx,mx) ((image)->strip[c].up2[(image)->tile_column_position[tx]+mx])
# define MACROBLK_UP3(image,c,tx,mx) ((image)->strip[c].up3[(image)->tile_column_position[tx]+mx])
# define MACROBLK_UP4(image,c,tx,mx) ((image)->strip[c].up4[(image)->tile_column_position[tx]+mx])

/* Return the l-value of the DC coefficient in the macroblock. */
# define MACROBLK_UP2_DC(image, c, tx, mx) (MACROBLK_UP2(image,c,tx,mx).data[0])
# define MACROBLK_UP_DC(image, c, tx, mx) (MACROBLK_UP1(image,c,tx,mx).data[0])
# define MACROBLK_CUR_DC(image, c, tx, mx) (MACROBLK_CUR(image,c,tx,mx).data[0])
/* Return the l-value of the LP coefficient in the macroblock.
The k value can range from 0-14. (There are 15 LP coefficients.) */
# define MACROBLK_CUR_LP(image,c,tx,mx,k) (MACROBLK_CUR(image,c,tx,mx).data[1+k])
# define MACROBLK_UP_LP(image,c,tx,mx,k) (MACROBLK_UP1(image,c,tx,mx).data[1+k])
# define MACROBLK_UP2_LP(image,c,tx,mx,k) (MACROBLK_UP2(image,c,tx,mx).data[1+k])

# define MACROBLK_CUR_LP_QUANT(image,c,tx,mx) (MACROBLK_CUR(image,c,tx,mx).lp_quant)
# define MACROBLK_UP1_LP_QUANT(image,c,tx,mx) (MACROBLK_UP1(image,c,tx,mx).lp_quant)
# define MACROBLK_UP2_LP_QUANT(image,c,tx,mx) (MACROBLK_UP2(image,c,tx,mx).lp_quant)

/* Return the l-value of the HP coefficient in the macroblk.
The blk value is 0-15 and identifies the block within the macroblk.
The k value is 0-14 and addresses the coefficient within the blk. */
# define MACROBLK_CUR_HP(image,c,tx,mx,blk,k) (MACROBLK_CUR(image,c,tx,mx).data[16+15*(blk)+(k)])
# define MACROBLK_UP1_HP(image,c,tx,mx,blk,k) (MACROBLK_UP1(image,c,tx,mx).data[16+15*(blk)+(k)])
# define MACROBLK_UP2_HP(image,c,tx,mx,blk,k) (MACROBLK_UP2(image,c,tx,mx).data[16+15*(blk)+(k)])

# define MACROBLK_CUR_HP_QUANT(image,c,tx,mx) (MACROBLK_CUR(image,c,tx,mx).hp_quant)
# define MACROBLK_UP1_HP_QUANT(image,c,tx,mx) (MACROBLK_UP1(image,c,tx,mx).hp_quant)
# define MACROBLK_UP2_HP_QUANT(image,c,tx,mx) (MACROBLK_UP2(image,c,tx,mx).hp_quant)

/* Return the l-value of the HP CBP value. */
# define MACROBLK_CUR_HPCBP(image,c,tx,mx) (MACROBLK_CUR(image,c,tx,mx).hp_cbp)
# define MACROBLK_UP1_HPCBP(image,c,tx,mx) (MACROBLK_UP1(image,c,tx,mx).hp_cbp)

extern void _jxr_make_mbstore(jxr_image_t image, int include_up4);
extern void _jxr_fill_strip(jxr_image_t image);
extern void _jxr_rflush_mb_strip(jxr_image_t image, int tx, int ty, int my);
extern void _jxr_wflush_mb_strip(jxr_image_t image, int tx, int ty, int my, int read_new);

/* Common strip handling functions. */
extern void _jxr_clear_strip_cur(jxr_image_t image);
extern void _jxr_r_rotate_mb_strip(jxr_image_t image);

/* Wrap up an image collected in frequency mode. */
extern void _jxr_frequency_mode_render(jxr_image_t image);

/* Application interface functions */
extern void _jxr_send_mb_to_output(jxr_image_t image, int mx, int my, int*data);

/* I/O functions. */

struct rbitstream 
#ifdef JPEGXR_ADOBE_EXT
	: public mbitstream
#endif //#ifdef JPEGXR_ADOBE_EXT
	{
#ifdef JPEGXR_ADOBE_EXT
	rbitstream() { }
	rbitstream(const uint8_t *data, int32_t len):mbitstream(data, len) { }
#endif //#ifdef JPEGXR_ADOBE_EXT
    unsigned char byte;
    int bits_avail;
#ifndef JPEGXR_ADOBE_EXT
    FILE*fd;
#endif //#ifndef JPEGXR_ADOBE_EXT
    size_t read_count;

    long mark_stream_position;
};

/* Get the current *bit* position, for diagnostic use. */
extern void _jxr_rbitstream_initialize(struct rbitstream*str
#ifndef JPEGXR_ADOBE_EXT
	, FILE*fd
#endif //#ifndef JPEGXR_ADOBE_EXT
);
extern size_t _jxr_rbitstream_bitpos(struct rbitstream*str);

extern void _jxr_rbitstream_syncbyte(struct rbitstream*str);
extern int _jxr_rbitstream_uint1(struct rbitstream*str);
extern uint8_t _jxr_rbitstream_uint2(struct rbitstream*str);
extern uint8_t _jxr_rbitstream_uint3(struct rbitstream*str);
extern uint8_t _jxr_rbitstream_uint4(struct rbitstream*str);
extern uint8_t _jxr_rbitstream_uint6(struct rbitstream*str);
extern uint8_t _jxr_rbitstream_uint8(struct rbitstream*str);
extern uint16_t _jxr_rbitstream_uint12(struct rbitstream*str);
extern uint16_t _jxr_rbitstream_uint15(struct rbitstream*str);
extern uint16_t _jxr_rbitstream_uint16(struct rbitstream*str);
extern uint32_t _jxr_rbitstream_uint32(struct rbitstream*str);
extern uint32_t _jxr_rbitstream_uintN(struct rbitstream*str, int N);
/* Return <0 if there is an escape code. */
extern int64_t _jxr_rbitstream_intVLW(struct rbitstream*str);

extern int _jxr_rbitstream_intE(struct rbitstream*str, int code_size,
                                const unsigned char*codeb,
                                const signed char*codev);
extern const char* _jxr_vlc_index_name(int vlc);

/*
* These functions provided limited random access into the file
* stream. The mark() function causes a given point to be marked as
* "zero", then the seek() function can go to a *byte* position
* relative the zeo mark. These functions can only be called when the
* stream is on a byte boundary.
*/
extern void _jxr_rbitstream_mark(struct rbitstream*str);
extern void _jxr_rbitstream_seek(struct rbitstream*str, uint64_t off);

struct wbitstream 
#ifdef JPEGXR_ADOBE_EXT
	: public mbitstream
#endif //#ifdef JPEGXR_ADOBE_EXT
   {
    unsigned char byte;
    int bits_ready;
#ifndef JPEGXR_ADOBE_EXT
    FILE*fd;
#endif //#ifndef JPEGXR_ADOBE_EXT
    size_t write_count;
};

extern void _jxr_wbitstream_initialize(struct wbitstream*str
#ifndef JPEGXR_ADOBE_EXT
	, FILE*fd
#endif //#ifndef JPEGXR_ADOBE_EXT
);
extern size_t _jxr_wbitstream_bitpos(struct wbitstream*str);
extern void _jxr_wbitstream_syncbyte(struct wbitstream*str);
extern void _jxr_wbitstream_uint1(struct wbitstream*str, int val);
extern void _jxr_wbitstream_uint2(struct wbitstream*str, uint8_t val);
extern void _jxr_wbitstream_uint3(struct wbitstream*str, uint8_t val);
extern void _jxr_wbitstream_uint4(struct wbitstream*str, uint8_t val);
extern void _jxr_wbitstream_uint6(struct wbitstream*str, uint8_t val);
extern void _jxr_wbitstream_uint8(struct wbitstream*str, uint8_t val);
extern void _jxr_wbitstream_uint12(struct wbitstream*str, uint16_t val);
extern void _jxr_wbitstream_uint15(struct wbitstream*str, uint16_t val);
extern void _jxr_wbitstream_uint16(struct wbitstream*str, uint16_t val);
extern void _jxr_wbitstream_uint32(struct wbitstream*str, uint32_t val);
extern void _jxr_wbitstream_uintN(struct wbitstream*str, uint32_t val, int N);
extern void _jxr_wbitstream_intVLW(struct wbitstream*str, uint64_t val);
extern void _jxr_wbitstream_flush(struct wbitstream*str);

extern void _jxr_wbitstream_mark(struct wbitstream*str);
extern void _jxr_wbitstream_seek(struct wbitstream*str, uint64_t off);

extern void _jxr_w_TILE_SPATIAL(jxr_image_t image, struct wbitstream*str,
                                unsigned tx, unsigned ty);
extern void _jxr_w_TILE_DC(jxr_image_t image, struct wbitstream*str,
                          unsigned tx, unsigned ty);
extern void _jxr_w_TILE_LP(jxr_image_t image, struct wbitstream*str,
                         unsigned tx, unsigned ty);
extern void _jxr_w_TILE_HP_FLEX(jxr_image_t image, struct wbitstream*str,
                         unsigned tx, unsigned ty);
extern void _jxr_w_TILE_HEADER_DC(jxr_image_t image, struct wbitstream*str,
                             int alpha_flag, unsigned tx, unsigned ty);
extern void _jxr_w_TILE_HEADER_LOWPASS(jxr_image_t image, struct wbitstream*str,
                                  int alpha_flag, unsigned tx, unsigned ty);
extern void _jxr_w_TILE_HEADER_HIGHPASS(jxr_image_t image, struct wbitstream*str,
                                   int alpha_flag, unsigned tx, unsigned ty);
extern void _jxr_w_MB_DC(jxr_image_t image, struct wbitstream*str,
                    int alpha_flag,
                    unsigned tx, unsigned ty,
                    unsigned mx, unsigned my);
extern void _jxr_w_MB_LP(jxr_image_t image, struct wbitstream*str,
                    int alpha_flag,
                    unsigned tx, unsigned ty,
                    unsigned mx, unsigned my);
extern int _jxr_w_MB_HP(jxr_image_t image, struct wbitstream*str,
                   int alpha_flag,
                   unsigned tx, unsigned ty,
                   unsigned mx, unsigned my, struct wbitstream*strFB);
extern void _jxr_w_MB_CBP(jxr_image_t image, struct wbitstream*str,
                     int alpha_flag,
                     unsigned tx, unsigned ty,
                     unsigned mx, unsigned my);
extern void _jxr_w_DC_QP(jxr_image_t image, struct wbitstream*str);
extern void _jxr_w_LP_QP(jxr_image_t image, struct wbitstream*str);
extern void _jxr_w_HP_QP(jxr_image_t image, struct wbitstream*str);
extern void _jxr_w_ENCODE_QP_INDEX(jxr_image_t image, struct wbitstream*str,
                              unsigned tx, unsigned ty, unsigned mx, unsigned my,
                              unsigned num_qps, unsigned qp_index);


extern int _jxr_r_TILE_SPATIAL(jxr_image_t image, struct rbitstream*str,
                               unsigned tx, unsigned ty);
extern int _jxr_r_TILE_DC(jxr_image_t image, struct rbitstream*str,
                          unsigned tx, unsigned ty);
extern int _jxr_r_TILE_LP(jxr_image_t image, struct rbitstream*str,
                          unsigned tx, unsigned ty);
extern int _jxr_r_TILE_HP(jxr_image_t image, struct rbitstream*str,
                          unsigned tx, unsigned ty);
extern int _jxr_r_TILE_FLEXBITS(jxr_image_t image, struct rbitstream*str,
                                unsigned tx, unsigned ty);
extern int _jxr_r_TILE_FLEXBITS_ESCAPE(jxr_image_t image,
                                       unsigned tx, unsigned ty);
extern void _jxr_r_TILE_HEADER_DC(jxr_image_t image, struct rbitstream*str,
                                  int alpha_flag, unsigned tx, unsigned ty);
extern void _jxr_r_TILE_HEADER_LOWPASS(jxr_image_t image, struct rbitstream*str,
                                       int alpha_flag, unsigned tx, unsigned ty);
extern void _jxr_r_TILE_HEADER_HIGHPASS(jxr_image_t image, struct rbitstream*str,
                                        int alpha_flag, unsigned tx, unsigned ty);

extern void _jxr_r_MB_DC(jxr_image_t image, struct rbitstream*str,
                         int alpha_flag,
                         unsigned tx, unsigned ty,
                         unsigned mx, unsigned my);
extern void _jxr_r_MB_LP(jxr_image_t image, struct rbitstream*str,
                         int alpha_flag,
                         unsigned tx, unsigned ty,
                         unsigned mx, unsigned my);
extern int _jxr_r_MB_HP(jxr_image_t image, struct rbitstream*str,
                        int alpha_flag,
                        unsigned tx, unsigned ty,
                        unsigned mx, unsigned my);
extern int _jxr_r_MB_CBP(jxr_image_t image, struct rbitstream*str,
                         int alpha_flag,
                         unsigned tx, unsigned ty,
                         unsigned mx, unsigned my);
int _jxr_r_MB_FLEXBITS(jxr_image_t image, struct rbitstream*str,
                       int alpha_flag,
                       unsigned tx, unsigned ty,
                       unsigned mx, unsigned my);

extern int _jxr_r_DC_QP(jxr_image_t image, struct rbitstream*str);
extern int _jxr_r_LP_QP(jxr_image_t image, struct rbitstream*str);

extern unsigned _jxr_DECODE_QP_INDEX(struct rbitstream*str,
                                     unsigned index_count);
extern int r_DECODE_BLOCK(jxr_image_t image, struct rbitstream*str,
                          int chroma_flag, int coeff[16], int band, int location);

/* algorithm functions */
/*
* In these functions, the mx and my are the position of the MB
* within a tile (i.e. mx=my=0 is the top left MB in the TILE not
* necessarily the image). The mx and my are given it units of MB. The
* pixel position is mx*16, my*16.
*/

extern int _jxr_quant_map(jxr_image_t image, int x, int shift);
extern int _jxr_InitContext(jxr_image_t image, unsigned tx, unsigned ty,
                            unsigned mx, unsigned my);
extern int _jxr_ResetContext(jxr_image_t image, unsigned tx, unsigned mx);
extern int _jxr_ResetTotals(jxr_image_t image, unsigned mx);
extern int _jxr_vlc_select(int band, int chroma_flag);
extern void _jxr_InitVLCTable(jxr_image_t image, int vlc_select);
extern void _jxr_AdaptVLCTable(jxr_image_t image, int vlc_select);
extern void _jxr_AdaptLP(jxr_image_t image);
extern void _jxr_AdaptHP(jxr_image_t image);
extern void _jxr_InitializeModelMB(struct model_s*model, int band);
extern void _jxr_UpdateModelMB(jxr_image_t image, int lap_mean[2],
struct model_s*model, int band);

extern void _jxr_InitializeCountCBPLP(jxr_image_t image);
extern void _jxr_UpdateCountCBPLP(jxr_image_t image, int cbplp, int max);

extern void _jxr_InitLPVLC(jxr_image_t image);
extern void _jxr_InitCBPVLC(jxr_image_t image);
extern void _jxr_InitHPVLC(jxr_image_t image);

extern void _jxr_InitializeCBPModel(jxr_image_t image);
extern void _jxr_w_PredCBP444(jxr_image_t image,
                              int ch, unsigned tx,
                              unsigned mx, int my);
extern void _jxr_w_PredCBP422(jxr_image_t image,
                              int ch, unsigned tx,
                              unsigned mx, int my);
extern void _jxr_w_PredCBP420(jxr_image_t image,
                              int ch, unsigned tx,
                              unsigned mx, int my);
extern int _jxr_PredCBP444(jxr_image_t image, int*diff_cbp,
                           int channel, unsigned tx, unsigned mx, unsigned my);
extern int _jxr_PredCBP422(jxr_image_t image, int*diff_cbp,
                           int channel, unsigned tx, unsigned mx, unsigned my);
extern int _jxr_PredCBP420(jxr_image_t image, int*diff_cbp,
                           int channel, unsigned tx, unsigned mx, unsigned my);
extern void _jxr_UpdateCBPModel(jxr_image_t image);

extern void _jxr_InitializeAdaptiveScanLP(jxr_image_t image);
extern void _jxr_ResetTotalsAdaptiveScanLP(jxr_image_t image);

extern void _jxr_InitializeAdaptiveScanHP(jxr_image_t image);
extern void _jxr_ResetTotalsAdaptiveScanHP(jxr_image_t image);

extern void _jxr_complete_cur_dclp(jxr_image_t image, int tx, int mx, int my);
extern void _jxr_propagate_hp_predictions(jxr_image_t image,
                                          int ch, unsigned tx, unsigned mx,
                                          int mbhp_pred_mode);

extern void _jxr_4OverlapFilter(int*a, int*b, int*c, int*d);
extern void _jxr_4x4OverlapFilter(int*a, int*b, int*c, int*d,
                                  int*e, int*f, int*g, int*h,
                                  int*i, int*j, int*k, int*l,
                                  int*m, int*n, int*o, int*p);
extern void _jxr_2OverlapFilter(int*a, int*b);

extern void _jxr_2x2OverlapFilter(int*a, int*b, int*c, int*d);

extern void _jxr_4PreFilter(int*a, int*b, int*c, int*d);
extern void _jxr_4x4PreFilter(int*a, int*b, int*c, int*d,
                              int*e, int*f, int*g, int*h,
                              int*i, int*j, int*k, int*l,
                              int*m, int*n, int*o, int*p);

extern void _jxr_2PreFilter(int*a, int*b);

extern void _jxr_2x2PreFilter(int*a, int*b, int*c, int*d);

extern const int _jxr_abslevel_index_delta[7];

/*
* All the blocks of HP data (16 blocks, 15 coefficients per block) in
* a macroblock are stored together. The data is written into the
* stream in this order:
*
* 0 1 4 5
* 2 3 6 7
* 8 9 12 13
* 10 11 14 15
*/
extern const int _jxr_hp_scan_map[16];

extern uint8_t _jxr_read_lwf_test_flag();
extern void _jxr_4x4IPCT(int*coeff);
extern void _jxr_2x2IPCT(int*coeff);
extern void _jxr_2ptT(int*a, int*b);
extern void _jxr_2ptFwdT(int*a, int*b);
extern void _jxr_InvPermute2pt(int*a, int*b);
extern void _jxr_4x4PCT(int*coeff);
extern void _jxr_2x2PCT(int*coeff);


extern int _jxr_floor_div2(int x);
extern int _jxr_ceil_div2(int x);

# define SWAP(x,y) do { int tmp = (x); (x) = (y); (y) = tmp; } while(0)


/* ***
* Container types
* ***/

struct ifd_table{
    uint16_t tag;
    uint16_t type;
    uint32_t cnt;

    union{
        uint8_t   v_byte[4];
        uint16_t  v_short[2];
        uint32_t  v_long;
        int8_t    v_sbyte[4];
        int16_t   v_sshort[2];
        int32_t   v_slong;
        float     v_float;

        uint8_t  *p_byte;
        uint16_t *p_short;
        uint32_t *p_long;
        uint64_t *p_rational;
        int8_t   *p_sbyte;
        int16_t  *p_sshort;
        int32_t  *p_slong;
        int64_t  *p_srational;
        float    *p_float;
        double   *p_double;
    }value_;
};

/*
* This is the container itself. It contains pointers to the tables.
*/
struct jxr_container{
    /* Number of images in the container. */
    int image_count;

    /* IFDs for all the images. */
    unsigned*table_cnt;
    struct ifd_table**table;

    /* Members for managing output. */

#ifdef JPEGXR_ADOBE_EXT
	wbitstream wb;
#else //#ifdef JPEGXR_ADOBE_EXT
    FILE*fd; /* File to write to. */
#endif //#ifdef JPEGXR_ADOBE_EXT

    uint32_t file_mark;
    uint32_t next_ifd_mark;
    uint32_t image_count_mark;
    uint32_t image_offset_mark;
    uint32_t alpha_count_mark;
    uint32_t alpha_offset_mark;
    uint32_t alpha_begin_mark; /* Required to calculate bytes used up by the alpha plane */

    uint8_t image_band, alpha_band; /* for bands present */
    uint32_t wid, hei;
    unsigned char pixel_format[16];

    uint8_t separate_alpha_image_plane;
};

extern int jxr_read_image_pixelformat(jxr_container_t c);

extern unsigned int isEqualGUID(unsigned char guid1[16], unsigned char guid2[16]);

#if defined(DETAILED_DEBUG)
# define DEBUG(...) fprintf(stdout, __VA_ARGS__)
#else
# define DEBUG(...) do { } while(0)
#endif

extern const char*_jxr_vld_index_name(int vlc);

/*
* $Log: jxr_priv.h,v $
* Revision 1.67 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.66 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.65 2008-05-13 13:47:11 thor
* Some experiments with a smarter selection for the quantization size,
* does not yet compile.
*
* Revision 1.64 2008-05-09 19:57:48 thor
* Reformatted for unix LF.
*
* Revision 1.63 2008-04-15 14:28:12 thor
* Start of the repository for the jpegxr reference software.
*
* Revision 1.62 2008/03/20 22:38:53 steve
* Use MB HPQP instead of first HPQP in decode.
*
* Revision 1.61 2008/03/14 00:54:08 steve
* Add second prefilter for YUV422 and YUV420 encode.
*
* Revision 1.60 2008/03/13 21:23:27 steve
* Add pipeline step for YUV420.
*
* Revision 1.59 2008/03/13 17:49:31 steve
* Fix problem with YUV422 CBP prediction for UV planes
*
* Add support for YUV420 encoding.
*
* Revision 1.58 2008/03/11 22:12:48 steve
* Encode YUV422 through DC.
*
* Revision 1.57 2008/03/06 02:05:48 steve
* Distributed quantization
*
* Revision 1.56 2008/03/05 00:31:17 steve
* Handle UNIFORM/IMAGEPLANE_UNIFORM compression.
*
* Revision 1.55 2008/03/02 18:35:27 steve
* Add support for BD16
*
* Revision 1.54 2008/03/01 02:46:08 steve
* Add support for JXR container.
*
* Revision 1.53 2008/02/28 18:50:31 steve
* Portability fixes.
*
* Revision 1.52 2008/02/26 23:44:25 steve
* Handle the special case of compiling via MS C.
*
* Revision 1.51 2008/02/01 22:49:52 steve
* Handle compress of YUV444 color DCONLY
*
* Revision 1.50 2008/01/08 01:06:20 steve
* Add first pass overlap filtering.
*
* Revision 1.49 2008/01/06 01:29:28 steve
* Add support for TRIM_FLEXBITS in compression.
*
* Revision 1.48 2008/01/04 17:07:35 steve
* API interface for setting QP values.
*
* Revision 1.47 2008/01/01 01:07:26 steve
* Add missing HP prediction.
*
* Revision 1.46 2007/12/30 00:16:00 steve
* Add encoding of HP values.
*
* Revision 1.45 2007/12/14 17:10:39 steve
* HP CBP Prediction
*
* Revision 1.44 2007/12/13 18:01:09 steve
* Stubs for HP encoding.
*
* Revision 1.43 2007/12/07 01:20:34 steve
* Fix adapt not adapting on line ends.
*
* Revision 1.42 2007/12/06 23:12:41 steve
* Stubs for LP encode operations.
*
* Revision 1.41 2007/12/04 22:06:10 steve
* Infrastructure for encoding LP.
*
* Revision 1.40 2007/11/30 01:50:58 steve
* Compression of DCONLY GRAY.
*
* Revision 1.39 2007/11/26 01:47:15 steve
* Add copyright notices per MS request.
*
* Revision 1.38 2007/11/21 23:26:14 steve
* make all strip buffers store MB data.
*/
#endif
