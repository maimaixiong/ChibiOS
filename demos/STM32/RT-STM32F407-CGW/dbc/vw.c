/* Generated by DBCC, see <https://github.com/howerj/dbcc> */
#include "vw.h"
#include <inttypes.h>
#include <math.h> /* uses macros NAN, INFINITY, signbit, no need for -lm */
//#include <assert.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#define UNUSED(X) ((void)(X))
#define assert(X) ((void)(X))

static inline uint64_t reverse_byte_order(uint64_t x) {
	x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
	x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
	x = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
	return x;
}

static inline int print_helper(int r, int print_return_value) {
	return ((r >= 0) && (print_return_value >= 0)) ? r + print_return_value : -1;
}

/* unpack754() -- unpack a floating point number from IEEE-754 format */ 
static double unpack754(const uint64_t i, const unsigned bits, const unsigned expbits) {
	if (i == 0) return 0.0;

	const uint64_t expset = ((1uLL << expbits) - 1uLL) << (bits - expbits - 1);
	if ((i & expset) == expset) { /* NaN or +/-Infinity */
		if (i & ((1uLL << (bits - expbits - 1)) - 1uLL)) /* Non zero Mantissa means NaN */
			return NAN;
		return i & (1uLL << (bits - 1)) ? -INFINITY : INFINITY;
	}

	/* pull the significand */
	const unsigned significandbits = bits - expbits - 1; /* - 1 for sign bit */
	double result = (i & ((1LL << significandbits) - 1)); /* mask */
	result /= (1LL << significandbits);  /* convert back to float */
	result += 1.0f;                        /* add the one back on */

	/* deal with the exponent */
	const unsigned bias = (1 << (expbits - 1)) - 1;
	long long shift = ((i >> significandbits) & ((1LL << expbits) - 1)) - bias;
	while (shift > 0) { result *= 2.0; shift--; }
	while (shift < 0) { result /= 2.0; shift++; }
	
	return (i >> (bits - 1)) & 1? -result: result; /* sign it, and return */
}

static inline float    unpack754_32(uint32_t i) { return unpack754(i, 32, 8); }
static inline double   unpack754_64(uint64_t i) { return unpack754(i, 64, 11); }


/* pack754() -- pack a floating point number into IEEE-754 format */ 
static uint64_t pack754(const double f, const unsigned bits, const unsigned expbits) {
	if (f == 0.0) /* get this special case out of the way */
		return signbit(f) ? (1uLL << (bits - 1)) :  0;
	if (f != f) /* NaN, encoded as Exponent == all-bits-set, Mantissa != 0, Signbit == Do not care */
		return (1uLL << (bits - 1)) - 1uLL;
	if (f == INFINITY) /* +INFINITY encoded as Mantissa == 0, Exponent == all-bits-set */
		return ((1uLL << expbits) - 1uLL) << (bits - expbits - 1);
	if (f == -INFINITY) /* -INFINITY encoded as Mantissa == 0, Exponent == all-bits-set, Signbit == 1 */
		return (1uLL << (bits - 1)) | ((1uLL << expbits) - 1uLL) << (bits - expbits - 1);

	long long sign = 0;
	double fnorm = f;
	/* check sign and begin normalization */
	if (f < 0) { sign = 1; fnorm = -f; }

	/* get the normalized form of f and track the exponent */
	int shift = 0;
	while (fnorm >= 2.0) { fnorm /= 2.0; shift++; }
	while (fnorm < 1.0)  { fnorm *= 2.0; shift--; }
	fnorm = fnorm - 1.0;

	const unsigned significandbits = bits - expbits - 1; // -1 for sign bit

	/* calculate the binary form (non-float) of the significand data */
	const long long significand = fnorm * (( 1LL << significandbits) + 0.5f);

	/* get the biased exponent */
	const long long exp = shift + ((1LL << (expbits - 1)) - 1); // shift + bias

	/* return the final answer */
	return (sign << (bits - 1)) | (exp << (bits - expbits - 1)) | significand;
}

static inline uint32_t   pack754_32(const float  f)   { return   pack754(f, 32, 8); }
static inline uint64_t   pack754_64(const double f)   { return   pack754(f, 64, 11); }


static int pack_can_0x09f_LH_EPS_03(can_obj_vw_h_t *o, uint64_t *data) {
	assert(o);
	assert(data);
	register uint64_t x;
	register uint64_t i = 0;
	/* EPS_Berechneter_LW: start-bit 16, length 12, endianess intel, scaling 0.15, offset 0 */
	x = ((uint16_t)(o->can_0x09f_LH_EPS_03.EPS_Berechneter_LW)) & 0xfff;
	x <<= 16; 
	i |= x;
	/* EPS_Lenkmoment: start-bit 40, length 10, endianess intel, scaling 1, offset 0 */
	x = ((uint16_t)(o->can_0x09f_LH_EPS_03.EPS_Lenkmoment)) & 0x3ff;
	x <<= 40; 
	i |= x;
	/* CHECKSUM: start-bit 0, length 8, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x09f_LH_EPS_03.CHECKSUM)) & 0xff;
	i |= x;
	/* EPS_HCA_Status: start-bit 32, length 4, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x09f_LH_EPS_03.EPS_HCA_Status)) & 0xf;
	x <<= 32; 
	i |= x;
	/* EPS_Lenkungstyp: start-bit 60, length 4, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x09f_LH_EPS_03.EPS_Lenkungstyp)) & 0xf;
	x <<= 60; 
	i |= x;
	/* COUNTER: start-bit 8, length 4, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x09f_LH_EPS_03.COUNTER)) & 0xf;
	x <<= 8; 
	i |= x;
	/* EPS_DSR_Status: start-bit 12, length 4, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x09f_LH_EPS_03.EPS_DSR_Status)) & 0xf;
	x <<= 12; 
	i |= x;
	/* EPS_VZ_BLW: start-bit 31, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x09f_LH_EPS_03.EPS_VZ_BLW)) & 0x1;
	x <<= 31; 
	i |= x;
	/* EPS_Lenkmoment_QBit: start-bit 54, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x09f_LH_EPS_03.EPS_Lenkmoment_QBit)) & 0x1;
	x <<= 54; 
	i |= x;
	/* EPS_VZ_Lenkmoment: start-bit 55, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x09f_LH_EPS_03.EPS_VZ_Lenkmoment)) & 0x1;
	x <<= 55; 
	i |= x;
	/* EPS_BLW_QBit: start-bit 30, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x09f_LH_EPS_03.EPS_BLW_QBit)) & 0x1;
	x <<= 30; 
	i |= x;
	*data = (i);
	o->can_0x09f_LH_EPS_03_tx = 1;
	return 0;
}

static int unpack_can_0x09f_LH_EPS_03(can_obj_vw_h_t *o, uint64_t data, uint8_t dlc, dbcc_time_stamp_t time_stamp) {
	assert(o);
	assert(dlc <= 8);
	register uint64_t x;
	register uint64_t i = (data);
	if (dlc < 8)
		return -1;
	/* EPS_Berechneter_LW: start-bit 16, length 12, endianess intel, scaling 0.15, offset 0 */
	x = (i >> 16) & 0xfff;
	o->can_0x09f_LH_EPS_03.EPS_Berechneter_LW = x;
	/* EPS_Lenkmoment: start-bit 40, length 10, endianess intel, scaling 1, offset 0 */
	x = (i >> 40) & 0x3ff;
	o->can_0x09f_LH_EPS_03.EPS_Lenkmoment = x;
	/* CHECKSUM: start-bit 0, length 8, endianess intel, scaling 1, offset 0 */
	x = i & 0xff;
	o->can_0x09f_LH_EPS_03.CHECKSUM = x;
	/* EPS_HCA_Status: start-bit 32, length 4, endianess intel, scaling 1, offset 0 */
	x = (i >> 32) & 0xf;
	o->can_0x09f_LH_EPS_03.EPS_HCA_Status = x;
	/* EPS_Lenkungstyp: start-bit 60, length 4, endianess intel, scaling 1, offset 0 */
	x = (i >> 60) & 0xf;
	o->can_0x09f_LH_EPS_03.EPS_Lenkungstyp = x;
	/* COUNTER: start-bit 8, length 4, endianess intel, scaling 1, offset 0 */
	x = (i >> 8) & 0xf;
	o->can_0x09f_LH_EPS_03.COUNTER = x;
	/* EPS_DSR_Status: start-bit 12, length 4, endianess intel, scaling 1, offset 0 */
	x = (i >> 12) & 0xf;
	o->can_0x09f_LH_EPS_03.EPS_DSR_Status = x;
	/* EPS_VZ_BLW: start-bit 31, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 31) & 0x1;
	o->can_0x09f_LH_EPS_03.EPS_VZ_BLW = x;
	/* EPS_Lenkmoment_QBit: start-bit 54, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 54) & 0x1;
	o->can_0x09f_LH_EPS_03.EPS_Lenkmoment_QBit = x;
	/* EPS_VZ_Lenkmoment: start-bit 55, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 55) & 0x1;
	o->can_0x09f_LH_EPS_03.EPS_VZ_Lenkmoment = x;
	/* EPS_BLW_QBit: start-bit 30, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 30) & 0x1;
	o->can_0x09f_LH_EPS_03.EPS_BLW_QBit = x;
	o->can_0x09f_LH_EPS_03_rx = 1;
	o->can_0x09f_LH_EPS_03_time_stamp_rx = time_stamp;
	return 0;
}

int decode_can_0x09f_EPS_Berechneter_LW(const can_obj_vw_h_t *o, double *out) {
	assert(o);
	assert(out);
	double rval = (double)(o->can_0x09f_LH_EPS_03.EPS_Berechneter_LW);
	rval *= 0.15;
	if (rval <= 613.95) {
		*out = rval;
		return 0;
	} else {
		*out = (double)0;
		return -1;
	}
}

int encode_can_0x09f_EPS_Berechneter_LW(can_obj_vw_h_t *o, double in) {
	assert(o);
	o->can_0x09f_LH_EPS_03.EPS_Berechneter_LW = 0;
	if (in > 613.95)
		return -1;
	in *= 6.66667;
	o->can_0x09f_LH_EPS_03.EPS_Berechneter_LW = in;
	return 0;
}

int decode_can_0x09f_EPS_Lenkmoment(const can_obj_vw_h_t *o, uint16_t *out) {
	assert(o);
	assert(out);
	uint16_t rval = (uint16_t)(o->can_0x09f_LH_EPS_03.EPS_Lenkmoment);
	if (rval <= 8) {
		*out = rval;
		return 0;
	} else {
		*out = (uint16_t)0;
		return -1;
	}
}

int encode_can_0x09f_EPS_Lenkmoment(can_obj_vw_h_t *o, uint16_t in) {
	assert(o);
	o->can_0x09f_LH_EPS_03.EPS_Lenkmoment = 0;
	if (in > 8)
		return -1;
	o->can_0x09f_LH_EPS_03.EPS_Lenkmoment = in;
	return 0;
}

int decode_can_0x09f_CHECKSUM(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x09f_LH_EPS_03.CHECKSUM);
	*out = rval;
	return 0;
}

int encode_can_0x09f_CHECKSUM(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x09f_LH_EPS_03.CHECKSUM = in;
	return 0;
}

int decode_can_0x09f_EPS_HCA_Status(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x09f_LH_EPS_03.EPS_HCA_Status);
	*out = rval;
	return 0;
}

int encode_can_0x09f_EPS_HCA_Status(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x09f_LH_EPS_03.EPS_HCA_Status = in;
	return 0;
}

int decode_can_0x09f_EPS_Lenkungstyp(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x09f_LH_EPS_03.EPS_Lenkungstyp);
	*out = rval;
	return 0;
}

int encode_can_0x09f_EPS_Lenkungstyp(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x09f_LH_EPS_03.EPS_Lenkungstyp = in;
	return 0;
}

int decode_can_0x09f_COUNTER(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x09f_LH_EPS_03.COUNTER);
	*out = rval;
	return 0;
}

int encode_can_0x09f_COUNTER(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x09f_LH_EPS_03.COUNTER = in;
	return 0;
}

int decode_can_0x09f_EPS_DSR_Status(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x09f_LH_EPS_03.EPS_DSR_Status);
	*out = rval;
	return 0;
}

int encode_can_0x09f_EPS_DSR_Status(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x09f_LH_EPS_03.EPS_DSR_Status = in;
	return 0;
}

int decode_can_0x09f_EPS_VZ_BLW(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x09f_LH_EPS_03.EPS_VZ_BLW);
	*out = rval;
	return 0;
}

int encode_can_0x09f_EPS_VZ_BLW(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x09f_LH_EPS_03.EPS_VZ_BLW = in;
	return 0;
}

int decode_can_0x09f_EPS_Lenkmoment_QBit(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x09f_LH_EPS_03.EPS_Lenkmoment_QBit);
	*out = rval;
	return 0;
}

int encode_can_0x09f_EPS_Lenkmoment_QBit(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x09f_LH_EPS_03.EPS_Lenkmoment_QBit = in;
	return 0;
}

int decode_can_0x09f_EPS_VZ_Lenkmoment(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x09f_LH_EPS_03.EPS_VZ_Lenkmoment);
	*out = rval;
	return 0;
}

int encode_can_0x09f_EPS_VZ_Lenkmoment(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x09f_LH_EPS_03.EPS_VZ_Lenkmoment = in;
	return 0;
}

int decode_can_0x09f_EPS_BLW_QBit(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x09f_LH_EPS_03.EPS_BLW_QBit);
	*out = rval;
	return 0;
}

int encode_can_0x09f_EPS_BLW_QBit(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x09f_LH_EPS_03.EPS_BLW_QBit = in;
	return 0;
}

int print_can_0x09f_LH_EPS_03(const can_obj_vw_h_t *o, BaseSequentialStream *output) {
	assert(o);
	assert(output);
	int r = 0;
	r = print_helper(r, chprintf(output, "EPS_Berechneter_LW = (wire: %.0f)\r\n", (double)(o->can_0x09f_LH_EPS_03.EPS_Berechneter_LW)));
	r = print_helper(r, chprintf(output, "EPS_Lenkmoment = (wire: %.0f)\r\n", (double)(o->can_0x09f_LH_EPS_03.EPS_Lenkmoment)));
	r = print_helper(r, chprintf(output, "CHECKSUM = (wire: %.0f)\r\n", (double)(o->can_0x09f_LH_EPS_03.CHECKSUM)));
	r = print_helper(r, chprintf(output, "EPS_HCA_Status = (wire: %.0f)\r\n", (double)(o->can_0x09f_LH_EPS_03.EPS_HCA_Status)));
	r = print_helper(r, chprintf(output, "EPS_Lenkungstyp = (wire: %.0f)\r\n", (double)(o->can_0x09f_LH_EPS_03.EPS_Lenkungstyp)));
	r = print_helper(r, chprintf(output, "COUNTER = (wire: %.0f)\r\n", (double)(o->can_0x09f_LH_EPS_03.COUNTER)));
	r = print_helper(r, chprintf(output, "EPS_DSR_Status = (wire: %.0f)\r\n", (double)(o->can_0x09f_LH_EPS_03.EPS_DSR_Status)));
	r = print_helper(r, chprintf(output, "EPS_VZ_BLW = (wire: %.0f)\r\n", (double)(o->can_0x09f_LH_EPS_03.EPS_VZ_BLW)));
	r = print_helper(r, chprintf(output, "EPS_Lenkmoment_QBit = (wire: %.0f)\r\n", (double)(o->can_0x09f_LH_EPS_03.EPS_Lenkmoment_QBit)));
	r = print_helper(r, chprintf(output, "EPS_VZ_Lenkmoment = (wire: %.0f)\r\n", (double)(o->can_0x09f_LH_EPS_03.EPS_VZ_Lenkmoment)));
	r = print_helper(r, chprintf(output, "EPS_BLW_QBit = (wire: %.0f)\r\n", (double)(o->can_0x09f_LH_EPS_03.EPS_BLW_QBit)));
	return r;
}

static int pack_can_0x0b2_ESP_19(can_obj_vw_h_t *o, uint64_t *data) {
	assert(o);
	assert(data);
	register uint64_t x;
	register uint64_t i = 0;
	/* ESP_HL_Radgeschw_02: start-bit 0, length 16, endianess intel, scaling 0.0075, offset 0 */
	x = ((uint16_t)(o->can_0x0b2_ESP_19.ESP_HL_Radgeschw_02)) & 0xffff;
	i |= x;
	/* ESP_HR_Radgeschw_02: start-bit 16, length 16, endianess intel, scaling 0.0075, offset 0 */
	x = ((uint16_t)(o->can_0x0b2_ESP_19.ESP_HR_Radgeschw_02)) & 0xffff;
	x <<= 16; 
	i |= x;
	/* ESP_VL_Radgeschw_02: start-bit 32, length 16, endianess intel, scaling 0.0075, offset 0 */
	x = ((uint16_t)(o->can_0x0b2_ESP_19.ESP_VL_Radgeschw_02)) & 0xffff;
	x <<= 32; 
	i |= x;
	/* ESP_VR_Radgeschw_02: start-bit 48, length 16, endianess intel, scaling 0.0075, offset 0 */
	x = ((uint16_t)(o->can_0x0b2_ESP_19.ESP_VR_Radgeschw_02)) & 0xffff;
	x <<= 48; 
	i |= x;
	*data = (i);
	o->can_0x0b2_ESP_19_tx = 1;
	return 0;
}

static int unpack_can_0x0b2_ESP_19(can_obj_vw_h_t *o, uint64_t data, uint8_t dlc, dbcc_time_stamp_t time_stamp) {
	assert(o);
	assert(dlc <= 8);
	register uint64_t x;
	register uint64_t i = (data);
	if (dlc < 8)
		return -1;
	/* ESP_HL_Radgeschw_02: start-bit 0, length 16, endianess intel, scaling 0.0075, offset 0 */
	x = i & 0xffff;
	o->can_0x0b2_ESP_19.ESP_HL_Radgeschw_02 = x;
	/* ESP_HR_Radgeschw_02: start-bit 16, length 16, endianess intel, scaling 0.0075, offset 0 */
	x = (i >> 16) & 0xffff;
	o->can_0x0b2_ESP_19.ESP_HR_Radgeschw_02 = x;
	/* ESP_VL_Radgeschw_02: start-bit 32, length 16, endianess intel, scaling 0.0075, offset 0 */
	x = (i >> 32) & 0xffff;
	o->can_0x0b2_ESP_19.ESP_VL_Radgeschw_02 = x;
	/* ESP_VR_Radgeschw_02: start-bit 48, length 16, endianess intel, scaling 0.0075, offset 0 */
	x = (i >> 48) & 0xffff;
	o->can_0x0b2_ESP_19.ESP_VR_Radgeschw_02 = x;
	o->can_0x0b2_ESP_19_rx = 1;
	o->can_0x0b2_ESP_19_time_stamp_rx = time_stamp;
	return 0;
}

int decode_can_0x0b2_ESP_HL_Radgeschw_02(const can_obj_vw_h_t *o, double *out) {
	assert(o);
	assert(out);
	double rval = (double)(o->can_0x0b2_ESP_19.ESP_HL_Radgeschw_02);
	rval *= 0.0075;
	if (rval <= 491.49) {
		*out = rval;
		return 0;
	} else {
		*out = (double)0;
		return -1;
	}
}

int encode_can_0x0b2_ESP_HL_Radgeschw_02(can_obj_vw_h_t *o, double in) {
	assert(o);
	o->can_0x0b2_ESP_19.ESP_HL_Radgeschw_02 = 0;
	if (in > 491.49)
		return -1;
	in *= 133.333;
	o->can_0x0b2_ESP_19.ESP_HL_Radgeschw_02 = in;
	return 0;
}

int decode_can_0x0b2_ESP_HR_Radgeschw_02(const can_obj_vw_h_t *o, double *out) {
	assert(o);
	assert(out);
	double rval = (double)(o->can_0x0b2_ESP_19.ESP_HR_Radgeschw_02);
	rval *= 0.0075;
	if (rval <= 491.49) {
		*out = rval;
		return 0;
	} else {
		*out = (double)0;
		return -1;
	}
}

int encode_can_0x0b2_ESP_HR_Radgeschw_02(can_obj_vw_h_t *o, double in) {
	assert(o);
	o->can_0x0b2_ESP_19.ESP_HR_Radgeschw_02 = 0;
	if (in > 491.49)
		return -1;
	in *= 133.333;
	o->can_0x0b2_ESP_19.ESP_HR_Radgeschw_02 = in;
	return 0;
}

int decode_can_0x0b2_ESP_VL_Radgeschw_02(const can_obj_vw_h_t *o, double *out) {
	assert(o);
	assert(out);
	double rval = (double)(o->can_0x0b2_ESP_19.ESP_VL_Radgeschw_02);
	rval *= 0.0075;
	if (rval <= 491.49) {
		*out = rval;
		return 0;
	} else {
		*out = (double)0;
		return -1;
	}
}

int encode_can_0x0b2_ESP_VL_Radgeschw_02(can_obj_vw_h_t *o, double in) {
	assert(o);
	o->can_0x0b2_ESP_19.ESP_VL_Radgeschw_02 = 0;
	if (in > 491.49)
		return -1;
	in *= 133.333;
	o->can_0x0b2_ESP_19.ESP_VL_Radgeschw_02 = in;
	return 0;
}

int decode_can_0x0b2_ESP_VR_Radgeschw_02(const can_obj_vw_h_t *o, double *out) {
	assert(o);
	assert(out);
	double rval = (double)(o->can_0x0b2_ESP_19.ESP_VR_Radgeschw_02);
	rval *= 0.0075;
	if (rval <= 491.49) {
		*out = rval;
		return 0;
	} else {
		*out = (double)0;
		return -1;
	}
}

int encode_can_0x0b2_ESP_VR_Radgeschw_02(can_obj_vw_h_t *o, double in) {
	assert(o);
	o->can_0x0b2_ESP_19.ESP_VR_Radgeschw_02 = 0;
	if (in > 491.49)
		return -1;
	in *= 133.333;
	o->can_0x0b2_ESP_19.ESP_VR_Radgeschw_02 = in;
	return 0;
}

int print_can_0x0b2_ESP_19(const can_obj_vw_h_t *o, BaseSequentialStream *output) {
	assert(o);
	assert(output);
	int r = 0;
	r = print_helper(r, chprintf(output, "ESP_HL_Radgeschw_02 = (wire: %.0f)\r\n", (double)(o->can_0x0b2_ESP_19.ESP_HL_Radgeschw_02)));
	r = print_helper(r, chprintf(output, "ESP_HR_Radgeschw_02 = (wire: %.0f)\r\n", (double)(o->can_0x0b2_ESP_19.ESP_HR_Radgeschw_02)));
	r = print_helper(r, chprintf(output, "ESP_VL_Radgeschw_02 = (wire: %.0f)\r\n", (double)(o->can_0x0b2_ESP_19.ESP_VL_Radgeschw_02)));
	r = print_helper(r, chprintf(output, "ESP_VR_Radgeschw_02 = (wire: %.0f)\r\n", (double)(o->can_0x0b2_ESP_19.ESP_VR_Radgeschw_02)));
	return r;
}

static int pack_can_0x120_TSK_06(can_obj_vw_h_t *o, uint64_t *data) {
	assert(o);
	assert(data);
	register uint64_t x;
	register uint64_t i = 0;
	/* TSK_Radbremsmom: start-bit 12, length 12, endianess intel, scaling 8, offset 0 */
	x = ((uint16_t)(o->can_0x120_TSK_06.TSK_Radbremsmom)) & 0xfff;
	x <<= 12; 
	i |= x;
	/* TSK_ax_Getriebe_02: start-bit 48, length 9, endianess intel, scaling 0.024, offset -2.016 */
	x = ((uint16_t)(o->can_0x120_TSK_06.TSK_ax_Getriebe_02)) & 0x1ff;
	x <<= 48; 
	i |= x;
	/* CHECKSUM: start-bit 0, length 8, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x120_TSK_06.CHECKSUM)) & 0xff;
	i |= x;
	/* TSK_zul_Regelabw: start-bit 58, length 6, endianess intel, scaling 0.024, offset 0 */
	x = ((uint8_t)(o->can_0x120_TSK_06.TSK_zul_Regelabw)) & 0x3f;
	x <<= 58; 
	i |= x;
	/* COUNTER: start-bit 8, length 4, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x120_TSK_06.COUNTER)) & 0xf;
	x <<= 8; 
	i |= x;
	/* TSK_Status: start-bit 24, length 3, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x120_TSK_06.TSK_Status)) & 0x7;
	x <<= 24; 
	i |= x;
	/* TSK_v_Begrenzung_aktiv: start-bit 27, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x120_TSK_06.TSK_v_Begrenzung_aktiv)) & 0x1;
	x <<= 27; 
	i |= x;
	/* TSK_Standby_Anf_ESP: start-bit 28, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x120_TSK_06.TSK_Standby_Anf_ESP)) & 0x1;
	x <<= 28; 
	i |= x;
	/* TSK_Freig_Verzoeg_Anf: start-bit 30, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x120_TSK_06.TSK_Freig_Verzoeg_Anf)) & 0x1;
	x <<= 30; 
	i |= x;
	/* TSK_Limiter_ausgewaehlt: start-bit 31, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x120_TSK_06.TSK_Limiter_ausgewaehlt)) & 0x1;
	x <<= 31; 
	i |= x;
	/* TSK_Zwangszusch_ESP: start-bit 57, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x120_TSK_06.TSK_Zwangszusch_ESP)) & 0x1;
	x <<= 57; 
	i |= x;
	*data = (i);
	o->can_0x120_TSK_06_tx = 1;
	return 0;
}

static int unpack_can_0x120_TSK_06(can_obj_vw_h_t *o, uint64_t data, uint8_t dlc, dbcc_time_stamp_t time_stamp) {
	assert(o);
	assert(dlc <= 8);
	register uint64_t x;
	register uint64_t i = (data);
	if (dlc < 8)
		return -1;
	/* TSK_Radbremsmom: start-bit 12, length 12, endianess intel, scaling 8, offset 0 */
	x = (i >> 12) & 0xfff;
	o->can_0x120_TSK_06.TSK_Radbremsmom = x;
	/* TSK_ax_Getriebe_02: start-bit 48, length 9, endianess intel, scaling 0.024, offset -2.016 */
	x = (i >> 48) & 0x1ff;
	o->can_0x120_TSK_06.TSK_ax_Getriebe_02 = x;
	/* CHECKSUM: start-bit 0, length 8, endianess intel, scaling 1, offset 0 */
	x = i & 0xff;
	o->can_0x120_TSK_06.CHECKSUM = x;
	/* TSK_zul_Regelabw: start-bit 58, length 6, endianess intel, scaling 0.024, offset 0 */
	x = (i >> 58) & 0x3f;
	o->can_0x120_TSK_06.TSK_zul_Regelabw = x;
	/* COUNTER: start-bit 8, length 4, endianess intel, scaling 1, offset 0 */
	x = (i >> 8) & 0xf;
	o->can_0x120_TSK_06.COUNTER = x;
	/* TSK_Status: start-bit 24, length 3, endianess intel, scaling 1, offset 0 */
	x = (i >> 24) & 0x7;
	o->can_0x120_TSK_06.TSK_Status = x;
	/* TSK_v_Begrenzung_aktiv: start-bit 27, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 27) & 0x1;
	o->can_0x120_TSK_06.TSK_v_Begrenzung_aktiv = x;
	/* TSK_Standby_Anf_ESP: start-bit 28, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 28) & 0x1;
	o->can_0x120_TSK_06.TSK_Standby_Anf_ESP = x;
	/* TSK_Freig_Verzoeg_Anf: start-bit 30, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 30) & 0x1;
	o->can_0x120_TSK_06.TSK_Freig_Verzoeg_Anf = x;
	/* TSK_Limiter_ausgewaehlt: start-bit 31, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 31) & 0x1;
	o->can_0x120_TSK_06.TSK_Limiter_ausgewaehlt = x;
	/* TSK_Zwangszusch_ESP: start-bit 57, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 57) & 0x1;
	o->can_0x120_TSK_06.TSK_Zwangszusch_ESP = x;
	o->can_0x120_TSK_06_rx = 1;
	o->can_0x120_TSK_06_time_stamp_rx = time_stamp;
	return 0;
}

int decode_can_0x120_TSK_Radbremsmom(const can_obj_vw_h_t *o, double *out) {
	assert(o);
	assert(out);
	double rval = (double)(o->can_0x120_TSK_06.TSK_Radbremsmom);
	rval *= 8;
	*out = rval;
	return 0;
}

int encode_can_0x120_TSK_Radbremsmom(can_obj_vw_h_t *o, double in) {
	assert(o);
	in *= 0.125;
	o->can_0x120_TSK_06.TSK_Radbremsmom = in;
	return 0;
}

int decode_can_0x120_TSK_ax_Getriebe_02(const can_obj_vw_h_t *o, double *out) {
	assert(o);
	assert(out);
	double rval = (double)(o->can_0x120_TSK_06.TSK_ax_Getriebe_02);
	rval *= 0.024;
	rval += -2.016;
	if (rval <= 10.224) {
		*out = rval;
		return 0;
	} else {
		*out = (double)0;
		return -1;
	}
}

int encode_can_0x120_TSK_ax_Getriebe_02(can_obj_vw_h_t *o, double in) {
	assert(o);
	o->can_0x120_TSK_06.TSK_ax_Getriebe_02 = 0;
	if (in > 10.224)
		return -1;
	in += 2.016;
	in *= 41.6667;
	o->can_0x120_TSK_06.TSK_ax_Getriebe_02 = in;
	return 0;
}

int decode_can_0x120_CHECKSUM(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x120_TSK_06.CHECKSUM);
	*out = rval;
	return 0;
}

int encode_can_0x120_CHECKSUM(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x120_TSK_06.CHECKSUM = in;
	return 0;
}

int decode_can_0x120_TSK_zul_Regelabw(const can_obj_vw_h_t *o, double *out) {
	assert(o);
	assert(out);
	double rval = (double)(o->can_0x120_TSK_06.TSK_zul_Regelabw);
	rval *= 0.024;
	if (rval <= 1.512) {
		*out = rval;
		return 0;
	} else {
		*out = (double)0;
		return -1;
	}
}

int encode_can_0x120_TSK_zul_Regelabw(can_obj_vw_h_t *o, double in) {
	assert(o);
	o->can_0x120_TSK_06.TSK_zul_Regelabw = 0;
	if (in > 1.512)
		return -1;
	in *= 41.6667;
	o->can_0x120_TSK_06.TSK_zul_Regelabw = in;
	return 0;
}

int decode_can_0x120_COUNTER(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x120_TSK_06.COUNTER);
	*out = rval;
	return 0;
}

int encode_can_0x120_COUNTER(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x120_TSK_06.COUNTER = in;
	return 0;
}

int decode_can_0x120_TSK_Status(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x120_TSK_06.TSK_Status);
	*out = rval;
	return 0;
}

int encode_can_0x120_TSK_Status(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x120_TSK_06.TSK_Status = in;
	return 0;
}

int decode_can_0x120_TSK_v_Begrenzung_aktiv(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x120_TSK_06.TSK_v_Begrenzung_aktiv);
	*out = rval;
	return 0;
}

int encode_can_0x120_TSK_v_Begrenzung_aktiv(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x120_TSK_06.TSK_v_Begrenzung_aktiv = in;
	return 0;
}

int decode_can_0x120_TSK_Standby_Anf_ESP(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x120_TSK_06.TSK_Standby_Anf_ESP);
	*out = rval;
	return 0;
}

int encode_can_0x120_TSK_Standby_Anf_ESP(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x120_TSK_06.TSK_Standby_Anf_ESP = in;
	return 0;
}

int decode_can_0x120_TSK_Freig_Verzoeg_Anf(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x120_TSK_06.TSK_Freig_Verzoeg_Anf);
	*out = rval;
	return 0;
}

int encode_can_0x120_TSK_Freig_Verzoeg_Anf(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x120_TSK_06.TSK_Freig_Verzoeg_Anf = in;
	return 0;
}

int decode_can_0x120_TSK_Limiter_ausgewaehlt(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x120_TSK_06.TSK_Limiter_ausgewaehlt);
	*out = rval;
	return 0;
}

int encode_can_0x120_TSK_Limiter_ausgewaehlt(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x120_TSK_06.TSK_Limiter_ausgewaehlt = in;
	return 0;
}

int decode_can_0x120_TSK_Zwangszusch_ESP(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x120_TSK_06.TSK_Zwangszusch_ESP);
	*out = rval;
	return 0;
}

int encode_can_0x120_TSK_Zwangszusch_ESP(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x120_TSK_06.TSK_Zwangszusch_ESP = in;
	return 0;
}

int print_can_0x120_TSK_06(const can_obj_vw_h_t *o, BaseSequentialStream *output) {
	assert(o);
	assert(output);
	int r = 0;
	r = print_helper(r, chprintf(output, "TSK_Radbremsmom = (wire: %.0f)\r\n", (double)(o->can_0x120_TSK_06.TSK_Radbremsmom)));
	r = print_helper(r, chprintf(output, "TSK_ax_Getriebe_02 = (wire: %.0f)\r\n", (double)(o->can_0x120_TSK_06.TSK_ax_Getriebe_02)));
	r = print_helper(r, chprintf(output, "CHECKSUM = (wire: %.0f)\r\n", (double)(o->can_0x120_TSK_06.CHECKSUM)));
	r = print_helper(r, chprintf(output, "TSK_zul_Regelabw = (wire: %.0f)\r\n", (double)(o->can_0x120_TSK_06.TSK_zul_Regelabw)));
	r = print_helper(r, chprintf(output, "COUNTER = (wire: %.0f)\r\n", (double)(o->can_0x120_TSK_06.COUNTER)));
	r = print_helper(r, chprintf(output, "TSK_Status = (wire: %.0f)\r\n", (double)(o->can_0x120_TSK_06.TSK_Status)));
	r = print_helper(r, chprintf(output, "TSK_v_Begrenzung_aktiv = (wire: %.0f)\r\n", (double)(o->can_0x120_TSK_06.TSK_v_Begrenzung_aktiv)));
	r = print_helper(r, chprintf(output, "TSK_Standby_Anf_ESP = (wire: %.0f)\r\n", (double)(o->can_0x120_TSK_06.TSK_Standby_Anf_ESP)));
	r = print_helper(r, chprintf(output, "TSK_Freig_Verzoeg_Anf = (wire: %.0f)\r\n", (double)(o->can_0x120_TSK_06.TSK_Freig_Verzoeg_Anf)));
	r = print_helper(r, chprintf(output, "TSK_Limiter_ausgewaehlt = (wire: %.0f)\r\n", (double)(o->can_0x120_TSK_06.TSK_Limiter_ausgewaehlt)));
	r = print_helper(r, chprintf(output, "TSK_Zwangszusch_ESP = (wire: %.0f)\r\n", (double)(o->can_0x120_TSK_06.TSK_Zwangszusch_ESP)));
	return r;
}

static int pack_can_0x126_HCA_01(can_obj_vw_h_t *o, uint64_t *data) {
	assert(o);
	assert(data);
	register uint64_t x;
	register uint64_t i = 0;
	/* Assist_Torque: start-bit 16, length 14, endianess intel, scaling 1, offset 0 */
	x = ((uint16_t)(o->can_0x126_HCA_01.Assist_Torque)) & 0x3fff;
	x <<= 16; 
	i |= x;
	/* CHECKSUM: start-bit 0, length 8, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x126_HCA_01.CHECKSUM)) & 0xff;
	i |= x;
	/* SET_ME_0XFE: start-bit 40, length 8, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x126_HCA_01.SET_ME_0XFE)) & 0xff;
	x <<= 40; 
	i |= x;
	/* SET_ME_0X07: start-bit 48, length 8, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x126_HCA_01.SET_ME_0X07)) & 0xff;
	x <<= 48; 
	i |= x;
	/* SET_ME_0X3: start-bit 12, length 4, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x126_HCA_01.SET_ME_0X3)) & 0xf;
	x <<= 12; 
	i |= x;
	/* COUNTER: start-bit 8, length 4, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x126_HCA_01.COUNTER)) & 0xf;
	x <<= 8; 
	i |= x;
	/* Assist_Requested: start-bit 30, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x126_HCA_01.Assist_Requested)) & 0x1;
	x <<= 30; 
	i |= x;
	/* Assist_VZ: start-bit 31, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x126_HCA_01.Assist_VZ)) & 0x1;
	x <<= 31; 
	i |= x;
	/* HCA_Available: start-bit 32, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x126_HCA_01.HCA_Available)) & 0x1;
	x <<= 32; 
	i |= x;
	/* HCA_Standby: start-bit 33, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x126_HCA_01.HCA_Standby)) & 0x1;
	x <<= 33; 
	i |= x;
	/* HCA_Active: start-bit 34, length 1, endianess intel, scaling 1, offset 0 */
	x = ((uint8_t)(o->can_0x126_HCA_01.HCA_Active)) & 0x1;
	x <<= 34; 
	i |= x;
	*data = (i);
	o->can_0x126_HCA_01_tx = 1;
	return 0;
}

static int unpack_can_0x126_HCA_01(can_obj_vw_h_t *o, uint64_t data, uint8_t dlc, dbcc_time_stamp_t time_stamp) {
	assert(o);
	assert(dlc <= 8);
	register uint64_t x;
	register uint64_t i = (data);
	if (dlc < 8)
		return -1;
	/* Assist_Torque: start-bit 16, length 14, endianess intel, scaling 1, offset 0 */
	x = (i >> 16) & 0x3fff;
	o->can_0x126_HCA_01.Assist_Torque = x;
	/* CHECKSUM: start-bit 0, length 8, endianess intel, scaling 1, offset 0 */
	x = i & 0xff;
	o->can_0x126_HCA_01.CHECKSUM = x;
	/* SET_ME_0XFE: start-bit 40, length 8, endianess intel, scaling 1, offset 0 */
	x = (i >> 40) & 0xff;
	o->can_0x126_HCA_01.SET_ME_0XFE = x;
	/* SET_ME_0X07: start-bit 48, length 8, endianess intel, scaling 1, offset 0 */
	x = (i >> 48) & 0xff;
	o->can_0x126_HCA_01.SET_ME_0X07 = x;
	/* SET_ME_0X3: start-bit 12, length 4, endianess intel, scaling 1, offset 0 */
	x = (i >> 12) & 0xf;
	o->can_0x126_HCA_01.SET_ME_0X3 = x;
	/* COUNTER: start-bit 8, length 4, endianess intel, scaling 1, offset 0 */
	x = (i >> 8) & 0xf;
	o->can_0x126_HCA_01.COUNTER = x;
	/* Assist_Requested: start-bit 30, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 30) & 0x1;
	o->can_0x126_HCA_01.Assist_Requested = x;
	/* Assist_VZ: start-bit 31, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 31) & 0x1;
	o->can_0x126_HCA_01.Assist_VZ = x;
	/* HCA_Available: start-bit 32, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 32) & 0x1;
	o->can_0x126_HCA_01.HCA_Available = x;
	/* HCA_Standby: start-bit 33, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 33) & 0x1;
	o->can_0x126_HCA_01.HCA_Standby = x;
	/* HCA_Active: start-bit 34, length 1, endianess intel, scaling 1, offset 0 */
	x = (i >> 34) & 0x1;
	o->can_0x126_HCA_01.HCA_Active = x;
	o->can_0x126_HCA_01_rx = 1;
	o->can_0x126_HCA_01_time_stamp_rx = time_stamp;
	return 0;
}

int decode_can_0x126_Assist_Torque(const can_obj_vw_h_t *o, uint16_t *out) {
	assert(o);
	assert(out);
	uint16_t rval = (uint16_t)(o->can_0x126_HCA_01.Assist_Torque);
	if (rval <= 300) {
		*out = rval;
		return 0;
	} else {
		*out = (uint16_t)0;
		return -1;
	}
}

int encode_can_0x126_Assist_Torque(can_obj_vw_h_t *o, uint16_t in) {
	assert(o);
	o->can_0x126_HCA_01.Assist_Torque = 0;
	if (in > 300)
		return -1;
	o->can_0x126_HCA_01.Assist_Torque = in;
	return 0;
}

int decode_can_0x126_CHECKSUM(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x126_HCA_01.CHECKSUM);
	*out = rval;
	return 0;
}

int encode_can_0x126_CHECKSUM(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x126_HCA_01.CHECKSUM = in;
	return 0;
}

int decode_can_0x126_SET_ME_0XFE(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x126_HCA_01.SET_ME_0XFE);
	*out = rval;
	return 0;
}

int encode_can_0x126_SET_ME_0XFE(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x126_HCA_01.SET_ME_0XFE = in;
	return 0;
}

int decode_can_0x126_SET_ME_0X07(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x126_HCA_01.SET_ME_0X07);
	*out = rval;
	return 0;
}

int encode_can_0x126_SET_ME_0X07(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x126_HCA_01.SET_ME_0X07 = in;
	return 0;
}

int decode_can_0x126_SET_ME_0X3(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x126_HCA_01.SET_ME_0X3);
	*out = rval;
	return 0;
}

int encode_can_0x126_SET_ME_0X3(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x126_HCA_01.SET_ME_0X3 = in;
	return 0;
}

int decode_can_0x126_COUNTER(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x126_HCA_01.COUNTER);
	*out = rval;
	return 0;
}

int encode_can_0x126_COUNTER(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x126_HCA_01.COUNTER = in;
	return 0;
}

int decode_can_0x126_Assist_Requested(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x126_HCA_01.Assist_Requested);
	*out = rval;
	return 0;
}

int encode_can_0x126_Assist_Requested(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x126_HCA_01.Assist_Requested = in;
	return 0;
}

int decode_can_0x126_Assist_VZ(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x126_HCA_01.Assist_VZ);
	*out = rval;
	return 0;
}

int encode_can_0x126_Assist_VZ(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x126_HCA_01.Assist_VZ = in;
	return 0;
}

int decode_can_0x126_HCA_Available(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x126_HCA_01.HCA_Available);
	*out = rval;
	return 0;
}

int encode_can_0x126_HCA_Available(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x126_HCA_01.HCA_Available = in;
	return 0;
}

int decode_can_0x126_HCA_Standby(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x126_HCA_01.HCA_Standby);
	*out = rval;
	return 0;
}

int encode_can_0x126_HCA_Standby(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x126_HCA_01.HCA_Standby = in;
	return 0;
}

int decode_can_0x126_HCA_Active(const can_obj_vw_h_t *o, uint8_t *out) {
	assert(o);
	assert(out);
	uint8_t rval = (uint8_t)(o->can_0x126_HCA_01.HCA_Active);
	*out = rval;
	return 0;
}

int encode_can_0x126_HCA_Active(can_obj_vw_h_t *o, uint8_t in) {
	assert(o);
	o->can_0x126_HCA_01.HCA_Active = in;
	return 0;
}

int print_can_0x126_HCA_01(const can_obj_vw_h_t *o, BaseSequentialStream *output) {
	assert(o);
	assert(output);
	int r = 0;
	r = print_helper(r, chprintf(output, "Assist_Torque = (wire: %u)\r\n", (o->can_0x126_HCA_01.Assist_Torque)));
	r = print_helper(r, chprintf(output, "CHECKSUM = (wire: %02x)\r\n", (o->can_0x126_HCA_01.CHECKSUM)));
	r = print_helper(r, chprintf(output, "SET_ME_0XFE = (wire: %02x)\r\n", (o->can_0x126_HCA_01.SET_ME_0XFE)));
	r = print_helper(r, chprintf(output, "SET_ME_0X07 = (wire: %02x)\r\n", (o->can_0x126_HCA_01.SET_ME_0X07)));
	r = print_helper(r, chprintf(output, "SET_ME_0X3 = (wire: %02x)\r\n", (o->can_0x126_HCA_01.SET_ME_0X3)));
	r = print_helper(r, chprintf(output, "COUNTER = (wire: %u)\r\n", (o->can_0x126_HCA_01.COUNTER)));
	r = print_helper(r, chprintf(output, "Assist_Requested = (wire: %u)\r\n", (o->can_0x126_HCA_01.Assist_Requested)));
	r = print_helper(r, chprintf(output, "Assist_VZ = (wire: %u)\r\n", (o->can_0x126_HCA_01.Assist_VZ)));
	r = print_helper(r, chprintf(output, "HCA_Available = (wire: %u)\r\n", (o->can_0x126_HCA_01.HCA_Available)));
	r = print_helper(r, chprintf(output, "HCA_Standby = (wire: %u)\r\n", (o->can_0x126_HCA_01.HCA_Standby)));
	r = print_helper(r, chprintf(output, "HCA_Active = (wire: %u)\r\n", (o->can_0x126_HCA_01.HCA_Active)));
	return r;
}

int unpack_message(can_obj_vw_h_t *o, const unsigned long id, uint64_t data, uint8_t dlc, dbcc_time_stamp_t time_stamp) {
	assert(o);
	assert(id < (1ul << 29)); /* 29-bit CAN ID is largest possible */
	assert(dlc <= 8);         /* Maximum of 8 bytes in a CAN packet */
	switch (id) {
	case 0x09f: return unpack_can_0x09f_LH_EPS_03(o, data, dlc, time_stamp);
	case 0x0b2: return unpack_can_0x0b2_ESP_19(o, data, dlc, time_stamp);
	case 0x120: return unpack_can_0x120_TSK_06(o, data, dlc, time_stamp);
	case 0x126: return unpack_can_0x126_HCA_01(o, data, dlc, time_stamp);
	default: break; 
	}
	return -1; 
}

int pack_message(can_obj_vw_h_t *o, const unsigned long id, uint64_t *data) {
	assert(o);
	assert(id < (1ul << 29)); /* 29-bit CAN ID is largest possible */
	switch (id) {
	case 0x09f: return pack_can_0x09f_LH_EPS_03(o, data);
	case 0x0b2: return pack_can_0x0b2_ESP_19(o, data);
	case 0x120: return pack_can_0x120_TSK_06(o, data);
	case 0x126: return pack_can_0x126_HCA_01(o, data);
	default: break; 
	}
	return -1; 
}

int print_message(const can_obj_vw_h_t *o, const unsigned long id, BaseSequentialStream *output) {
	assert(o);
	assert(id < (1ul << 29)); /* 29-bit CAN ID is largest possible */
	assert(output);
	switch (id) {
	case 0x09f: return print_can_0x09f_LH_EPS_03(o, output);
	case 0x0b2: return print_can_0x0b2_ESP_19(o, output);
	case 0x120: return print_can_0x120_TSK_06(o, output);
	case 0x126: return print_can_0x126_HCA_01(o, output);
	default: break; 
	}
	return -1; 
}

