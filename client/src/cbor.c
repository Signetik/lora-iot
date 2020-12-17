
// Copyright 2016 Signetik, LLC.
// All rights reserved.
// Signetik Confidential Proprietary Information -- Disclosure prohibited.
// Author: Bruce Carlson
// www.signetik.com


// CBOR library
// refer to the CBOR RFC at  http://tools.ietf.org/html/rfc7049

#include "cbor.h"

#include <stdlib.h>
#include <string.h>
#if defined(CBOR_FLOAT_SUPPORT)
#include "arm_math.h"	// NAN
#endif

// This module requires the use of C99 long long integer constants.
//#pragma GCC diagnostic ignored "-Wlong-long"


/// CBOR Major Types per RFC 7049 section 2.1
typedef enum {
	CBOR_POS_INT = 0,
	CBOR_NEG_INT = 1,
	CBOR_RAW	 = 2,	// rfc calls this "byte string"
	CBOR_TEXT	 = 3,
	CBOR_ARRAY	 = 4,
	CBOR_MAP	 = 5,	// TABLE or DICTIONARY
	CBOR_TAG	 = 6,
	CBOR_OTHER	 = 7,	// most notably FLOAT
	CBOR_INVALID = 255	// 3-bit field can never be >7
} cbor_type_t;

#define CBOR_FLOAT CBOR_OTHER	// floats are a sub-type of CBOR_OTHER



#define CBOR_LENGTH_MASK	0x1f

/// CBOR LENGTH Field constants for cbor major type OTHER
#define CBOR_FALSE		20
#define CBOR_TRUE		21
#define CBOR_NULL		22
#define CBOR_UNDEFINED	23
#define CBOR_HALF		25		// half-precision floating point
#define CBOR_SINGLE		26
#define CBOR_DOUBLE		27
#define CBOR_BREAK		31

/// CBOR LENGTH Field constant for cbor major types RAW, TEXT, ARRAY, MAP:
#define CBOR_LENGTH_INDEFINITE 31		// indefinite-length items have this value for first byte length field



static int smaller(int a, int b);
static int64_t smaller_int64(int64_t a, int64_t b);
static void put_byte(cbor_data_t *d, uint8_t x);
static void put_bytes(cbor_data_t *d, uint8_t *x, int x_len);
static uint8_t get_byte_or_zero(cbor_data_t *d);
static void skip_bytes(cbor_data_t *d, int64_t bytes_to_skip);


/** Init a cbor_data_t handle to cbor data
	Caller must allocate len bytes to data before calling.
 */
cbor_data_t cbor_cursor(uint8_t *data, int len)
{
	return (cbor_data_t) { data, len };
}

bool cbor_empty(cbor_data_t d)
{
	return ((d.b == NULL) || (d.len <= 0));
}

/** Make a cursor to read the data just written with cbor_put_ functions
	This is a bit makeshift since cbor_data_t doesn't store the original pointer.
 */
cbor_data_t cbor_get_read_cursor(cbor_data_t current_cbor_cursor, uint8_t *original_buffer_pointer)
{
	int len = (int) (current_cbor_cursor.b - original_buffer_pointer);
	if (len < 0) { len = 0; }
	return (cbor_data_t) { original_buffer_pointer, len };
}


//////////////////////////////////////////////////////////////////////////////////////////
// Writing CBOR


// Each of these functions accepts cbor_data_t, which is a cursor (a pointer
//  to the cbor buffer that moves as the buffer is consumed).

static void cbor_put_type_no_num( cbor_data_t *d, cbor_type_t item_type, uint8_t subtype);
static void cbor_put_type_and_num(cbor_data_t *d, cbor_type_t item_type, uint64_t num);


/** Write a CBOR integer to buffer
 */
void cbor_put_int(cbor_data_t *d, int64_t x)
{
	if (x >= 0) {
		cbor_put_type_and_num(d, CBOR_POS_INT, (uint64_t) x);
	} else {
		cbor_put_type_and_num(d, CBOR_NEG_INT, (uint64_t) 0 - (x + 1) );
	}
}

/** Write CBOR raw binary data to buffer
 */
void cbor_put_raw(cbor_data_t *d, uint8_t *data, unsigned int data_len)
{
	cbor_put_type_and_num(d, CBOR_RAW, data_len);
	put_bytes(d, data, data_len);
}
/** Write the start of a CBOR indefinite length raw block to the buffer.
	After this call, caller is responsible to cbor_put_raw one or more blocks followed by cbor_put_end_indefinite.
 */
void cbor_put_raw_start_indefinite(cbor_data_t *d)
{
	cbor_put_type_no_num(d, CBOR_RAW, CBOR_LENGTH_INDEFINITE);
}

/** Write CBOR text to buffer
	@param text is null-terminated string
 */
void cbor_put_text(cbor_data_t *d, const char *text)
{
	int data_len = strlen(text);
	cbor_put_type_and_num(d, CBOR_TEXT, data_len);
	put_bytes(d, (uint8_t *) text, data_len);
}
/** Write the start of a CBOR indefinite length text block to the buffer.
	After this call, caller is responsible to cbor_put_text one or more blocks followed by cbor_put_end_indefinite.
 */
void cbor_put_text_start_indefinite(cbor_data_t *d)
{
	cbor_put_type_no_num(d, CBOR_TEXT, CBOR_LENGTH_INDEFINITE);
}

/** Write the start of a CBOR array to the buffer.
	After this call, caller is responsible to write array_element_count more elements to compose the array.
 */
void cbor_put_array_start(cbor_data_t *d, unsigned int array_element_count)
{
	cbor_put_type_and_num(d, CBOR_ARRAY, array_element_count);
}
/** Write the start of a CBOR array to the buffer.
	After this call, caller is responsible to write the array elements followed by cbor_put_end_indefinite()
 */
void cbor_put_array_start_indefinite(cbor_data_t *d)
{
	cbor_put_type_no_num(d, CBOR_ARRAY, CBOR_LENGTH_INDEFINITE);
}

/** Write the start of a CBOR map to the buffer
	After this call, caller is responsible to write map_element_count (key, value) pairs to compose the map.
 */
void cbor_put_map_start(cbor_data_t *d, unsigned int map_element_count)
{
	cbor_put_type_and_num(d, CBOR_MAP, map_element_count);
}
/** Write the start of a CBOR map to the buffer
	After this call, caller is responsible to write  the map key, value pairs followed by cbor_put_end_indefinite().
 */
void cbor_put_map_start_indefinite(cbor_data_t *d)
{
	cbor_put_type_no_num(d, CBOR_MAP, CBOR_LENGTH_INDEFINITE);
}

/** Write CBOR tag to buffer
 */
void cbor_put_tag(cbor_data_t *d, cbor_tag_t tag)
{
	cbor_put_type_and_num(d, CBOR_TAG, (uint64_t) tag);
}

#if defined(CBOR_FLOAT_SUPPORT)
/** Write CBOR half-precision (16-bit) floating point number to buffer
 */
void cbor_put_half(cbor_data_t *d, float32_t x)
{
	cbor_put_type_no_num(d, CBOR_OTHER, CBOR_HALF);
	put_byte(d, 0);					// TODO
	put_byte(d, 0);					// TODO
}

/** Write CBOR single-precision floating point number to buffer
 */
void cbor_put_single(cbor_data_t *d, float32_t x)
{
	cbor_put_type_no_num(d, CBOR_OTHER, CBOR_SINGLE);
	put_bytes(d, (uint8_t *) &x, 4);	// todo: byte order might be wrong! Rewrite it as machine-independent.
}

void cbor_put_single_reverse(cbor_data_t *d, float32_t x)
{
	char temp[4];
	char t;
	cbor_put_type_no_num(d, CBOR_OTHER, CBOR_SINGLE);
	memcpy(temp, &x, 4);
	t = temp[0];
	temp[0] = temp[3];
	temp[3] = t;
	t = temp[1];
	temp[1] = temp[2];
	temp[2] = t;
	put_bytes(d, (uint8_t *) (float32_t*)temp, 4);	// todo: byte order might be wrong! Rewrite it as machine-independent.

}

/** Write CBOR double-precision floating point number to buffer
 */
void cbor_put_double(cbor_data_t *d, float64_t x)
{
	cbor_put_type_no_num(d, CBOR_OTHER, CBOR_DOUBLE);
	put_bytes(d, (uint8_t *) &x, 8);	// todo: byte order might be wrong! Rewrite it as machine-independent.
}
#endif


/** Write CBOR break to buffer to signal the end of indefinite-length arrays, maps, raw binary data, or text.
 */
void cbor_put_end_indefinite(cbor_data_t *d)
{
	cbor_put_type_no_num(d, CBOR_OTHER, CBOR_BREAK);
}

/** Write CBOR boolean value to buffer
 */
void cbor_put_bool(cbor_data_t *d, bool x)
{
	if (x) {
		cbor_put_type_no_num(d, CBOR_OTHER, CBOR_TRUE);
	} else {
		cbor_put_type_no_num(d, CBOR_OTHER, CBOR_FALSE);
	}
}

/** Write CBOR null to buffer
 */
void cbor_put_null(cbor_data_t *d)
{
	cbor_put_type_no_num(d, CBOR_OTHER, CBOR_NULL);
}
/** Write CBOR undefined to buffer
 */
void cbor_put_undefined(cbor_data_t *d)
{
	cbor_put_type_no_num(d, CBOR_OTHER, CBOR_UNDEFINED);
}


/** Write an item type - use this for special item types like indefinite length which must be only a single byte long.
 */
static void cbor_put_type_no_num( cbor_data_t *d, cbor_type_t item_type, uint8_t subtype)
{
	uint8_t typ = item_type << 5;
	put_byte(d, typ | (subtype & CBOR_LENGTH_MASK));
}

/** Write an item type and number to buffer.
	@param num has different significance depending on itype
	If item type is CBOR_POS_INT then num is the actual value of the integer (no further data)
	If item type is CBOR_NEG_INT then the actual value is 1-num (no further data)
	If item type is CBOR_RAW or CBOR_TEXT then num indicates how many bytes of data follow
	If item type is CBOR_ARRAY then num indicates how many array elements follow
	If item type is CBOR_MAP then num indicates how many (key, value) pairs follow
	If item type is CBOR_TAG then num indicates tag number
	If item type is CBOR_OTHER then don't use this function.
 */
static void cbor_put_type_and_num(cbor_data_t *d, cbor_type_t item_type, uint64_t num)
{
	uint8_t typ = item_type << 5;
	if (num <= 23) {
		put_byte(d, typ | num);
	} else if (num < 0x100) {
		put_byte(d, typ | 0x18);
		put_byte(d, num);
	} else if (num < 0x10000) {
		put_byte(d, typ | 0x19);
		put_byte(d, (num >> 8) & 0x00ff);
		put_byte(d, (num >> 0) & 0x00ff);
	} else if (num < 0x100000000ULL) {
		put_byte(d, typ | 0x1a);
		put_byte(d, (num >> 24) & 0x000000ff);
		put_byte(d, (num >> 16) & 0x000000ff);
		put_byte(d, (num >>  8) & 0x000000ff);
		put_byte(d, (num >>  0) & 0x000000ff);
	} else {
		put_byte(d, typ | 0x1b);
		put_byte(d, (num >> 56) & 0x00000000000000ff);
		put_byte(d, (num >> 48) & 0x00000000000000ff);
		put_byte(d, (num >> 40) & 0x00000000000000ff);
		put_byte(d, (num >> 32) & 0x00000000000000ff);
		put_byte(d, (num >> 24) & 0x00000000000000ff);
		put_byte(d, (num >> 16) & 0x00000000000000ff);
		put_byte(d, (num >>  8) & 0x00000000000000ff);
		put_byte(d, (num >>  0) & 0x00000000000000ff);
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Reading CBOR


static cbor_type_t cbor_type(cbor_data_t d);
static uint8_t cbor_length_field(cbor_data_t d);
static uint64_t cbor_parse_item_num(cbor_data_t *d);

static void cbor_skip_n(cbor_data_t *d, int max_nesting_levels);
static void cbor_skip_tags(cbor_data_t *d);



/** Is the next cbor item an integer?
 */
bool cbor_is_int(cbor_data_t d)
{
	cbor_skip_tags(&d);
	cbor_type_t t = cbor_type(d);
	return (t == CBOR_POS_INT) || (t == CBOR_NEG_INT);
}

// These cbor_is_ functions tell about the next item without moving the cursor.  Tags are ignored.
bool cbor_is_raw(		cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_RAW);		}
bool cbor_is_text(		cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_TEXT);	}
bool cbor_is_array(		cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_ARRAY);	}
bool cbor_is_map(		cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_MAP);		}
bool cbor_has_tag(		cbor_data_t d) {					 return (cbor_type(d) == CBOR_TAG);		}
bool cbor_is_half(		cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_OTHER) && (cbor_length_field(d) == CBOR_HALF); }
bool cbor_is_single(	cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_OTHER) && (cbor_length_field(d) == CBOR_SINGLE); }
bool cbor_is_double(	cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_OTHER) && (cbor_length_field(d) == CBOR_DOUBLE); }
bool cbor_is_null(		cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_OTHER) && (cbor_length_field(d) == CBOR_NULL); }
bool cbor_is_undefined(	cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_OTHER) && (cbor_length_field(d) == CBOR_UNDEFINED); }
bool cbor_is_true(		cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_OTHER) && (cbor_length_field(d) == CBOR_TRUE); }
bool cbor_is_false(		cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_OTHER) && (cbor_length_field(d) == CBOR_FALSE); }
bool cbor_is_break(		cbor_data_t d) { cbor_skip_tags(&d); return (cbor_type(d) == CBOR_OTHER) && (cbor_length_field(d) == CBOR_BREAK); }
bool cbor_is_bool(		cbor_data_t d) { return cbor_is_true(d) || cbor_is_false(d); }
bool cbor_is_floating(	cbor_data_t d) { return cbor_is_half(d) || cbor_is_single(d) || cbor_is_double(d); }
bool cbor_is_number(	cbor_data_t d) { return cbor_is_floating(d) || cbor_is_int(d); }


// These cbor_get_ functions parse cbor data, return results and move the buffer pointer past the item,
// the cursor is moved past the item whether or not the item matches the expected type.


/** Get a number (any format) from cbor data - return it as a float and move cursor ahead
	If the next data item does not represent a number, then return default value and do not move cbor cursor.
 */
#if defined(CBOR_FLOAT_SUPPORT)
float64_t cbor_get_number_float(cbor_data_t *source, float64_t default_val)
{
	cbor_data_t local_cursor = *source;
	cbor_data_t *d = &local_cursor;
	cbor_skip(source);

	if (cbor_is_int(*d)) {
		cbor_data_t t2 = *d;
		int64_t x = cbor_get_int(d, -1);
		if (x == -1) {
			x = cbor_get_int(&t2, -2);
			if (x == -2) { return default_val; }
		}
		return (float64_t) x;
	}
	if (cbor_is_floating(*d)) {
		float64_t x = cbor_get_floating(d);
		if (isnan(x)) { return default_val; }	// if you want actual NANs in the cbor data to pass properly then pass NAN for default_val.
		return x;
	}
	if (cbor_is_bool(*d)) {
		int x = cbor_get_bool(d);
		if (x < 0) { return default_val; }
		return (float64_t) x;
	}
	// future enhancement - convert text to number?
	return default_val;
}
#endif

/** Get a number (any format) from cbor data - return it as an integer and move cursor ahead
	If the next data item does not represent an integer, then return default value and do not move cbor cursor.
	Limitation: this function does not support integers outside of -2^63 to 2^63 - 1  because they
				don't fit in signed int64_t.  It instead returns default.
 */
int64_t cbor_get_number(cbor_data_t *source, int64_t default_val)
{
	cbor_data_t local_cursor = *source;
	cbor_data_t *d = &local_cursor;
	cbor_skip(source);

#if defined(CBOR_FLOAT_SUPPORT)
	if (cbor_is_floating(*d)) {
		float64_t x = cbor_get_floating(d);
		if (isnan(x)) { return default_val; }
		x = roundl(x);
		return (int64_t) x;
	}
#endif
	if (cbor_is_int(*d)) {
		return cbor_get_int(d, default_val);
	}
	if (cbor_is_bool(*d)) {
		int x = cbor_get_bool(d);
		if (x < 0) { return default_val; }
		return (int64_t) x;
	}
	// future enhancement - convert text to number?
	return default_val;
}


/** Get an integer from cbor data and move cursor ahead
	If the next data item does not represent an integer, then return default value and do not move cbor cursor.
	Limitation: this function does not support integers outside of -2^63 to 2^63 - 1  because they
				don't fit in signed int64_t.  It instead returns default.
 */
int64_t cbor_get_int(cbor_data_t *source, int64_t default_val)
{
	cbor_data_t local_cursor = *source;
	cbor_data_t *d = &local_cursor;
	cbor_skip(source);

	cbor_skip_tags(d);
	if (cbor_empty(*d))  { return default_val; }
	cbor_type_t typ = cbor_type(*d);
	if (typ == CBOR_POS_INT) {
		uint64_t x = cbor_parse_item_num(d);
		if (x <= INT64_MAX) {
			return x;
		}
	}
	if (typ == CBOR_NEG_INT) {
		uint64_t p = cbor_parse_item_num(d);
		int64_t x = -1 - p;
		if (x < 0) {				// if not underflow.
			return x;
		}
	}
	return default_val;
}

/** Get tag value from cbor data and move cursor ahead, but only if the item is a tag.
	Note that this may return values that are not enumerated in cbor_tag_t.  Caller should validate.
	Multiple tags may be read with successive calls
 */
cbor_tag_t cbor_get_tag(cbor_data_t *d)
{
	if (d == NULL) { return CBOR_TAG_INVALID; }
	if (cbor_has_tag(*d)) {
		uint64_t x = cbor_parse_item_num(d);
		if (x <= INT32_MAX) {
			return (cbor_tag_t) x;
		}
	}
	return CBOR_TAG_INVALID;
}


/** Get CBOR raw binary data to buffer and move cursor ahead
	@param dest - this function reads data into dest.  caller must allocate dest
	@return count of bytes that were available for reading, or <0 on error (next item is not raw).
			If return value > dest_len then some data were discarded.
 */
int cbor_get_raw(cbor_data_t *source, uint8_t *dest, int dest_len)
{
	if (source == NULL) { return -1; }
	if (cbor_empty(*source)) { return -1; }
	cbor_data_t local_cursor = *source;
	cbor_data_t *d = &local_cursor;
	cbor_skip(source);

	if ((dest == NULL) || (dest_len <= 0)) { return -1; }
	cbor_skip_tags(d);
	if ( ! cbor_is_raw(*d)) { return -1; }
	int64_t data_len = 0;
	if (cbor_is_indefinite_length(*d)) {
		// todo: indefinite_length raw not currently supported
		cbor_skip(d);
	} else {
		data_len = (int64_t) cbor_parse_item_num(d);
		data_len = smaller_int64(data_len, d->len);
		int bytes_to_copy = smaller_int64(data_len, dest_len);
		memcpy(dest, d->b, bytes_to_copy);
		skip_bytes(d, data_len);
	}
	return data_len;
}
/** Get pointer to CBOR raw binary data and move cursor ahead
	On return *ptr is set to point to the raw binary data, which remains in place.
	Caller should not modify the data, especially beyond the returned length.
	This function does not support CBOR indefinite length raw data items.
	@param dest - this function reads data into dest.  caller must allocate dest
	@return count of bytes in raw data, or <0 on error (next item is not raw).
 */
int cbor_get_raw_ptr(cbor_data_t *source, uint8_t **ptr)
{
	if (source == NULL) { return -1; }
	if (cbor_empty(*source)) { return -1; }
	cbor_data_t local_cursor = *source;
	cbor_data_t *d = &local_cursor;
	cbor_skip(source);

	if (ptr == NULL) { return -1; }
	*ptr = NULL;
	cbor_skip_tags(d);
	if ( ! cbor_is_raw(*d)) { return -1; }
	int64_t data_len = 0;
	if (cbor_is_indefinite_length(*d)) {
		return -1;					// indefinite_length raw not supported
	} else {
		data_len = (int64_t) cbor_parse_item_num(d);
		data_len = smaller_int64(data_len, d->len);
		*ptr = d->b;
	}
	return data_len;
}

/** Get CBOR text data to buffer and move cursor ahead
	@param dest - this function reads data into dest.  caller must allocate dest
	@return count of bytes that were available for reading, or <0 on error (next item is not text).
			If return value >= dest_len then some data were discarded.
 */
int cbor_get_text(cbor_data_t *source, char *dest, int dest_len)
{
	if (source == NULL) { return -1; }
	if (cbor_empty(*source)) { return -1; }
	cbor_data_t local_cursor = *source;
	cbor_data_t *d = &local_cursor;
	cbor_skip(source);

	if ((dest == NULL) || (dest_len <= 0)) { return -1; }
	cbor_skip_tags(d);
	if ( ! cbor_is_text(*d)) { return -1; }
	int64_t data_len = 0;
	if (cbor_is_indefinite_length(*d)) {
		// todo: indefinite_length text not currently supported
		cbor_skip(d);
		dest[0] = '\0';
	} else {
		data_len = (int64_t) cbor_parse_item_num(d);
		data_len = smaller_int64(data_len, d->len);
		int bytes_to_copy = smaller_int64(data_len, dest_len - 1);
		memcpy(dest, d->b, bytes_to_copy);
		dest[bytes_to_copy] = '\0';
		skip_bytes(d, data_len);
	}
	return data_len;
}

static int cbor_items_before_break(cbor_data_t d);

/** Count number of cbor items until cbor break encountered
	If no break is encountered then this will return how many items there are.
 */
static int cbor_items_before_break(cbor_data_t d)
{
	int i = 0;
	while (( ! cbor_is_break(d)) && ( ! cbor_empty(d))) {
		cbor_skip(&d);			// local d is modified but this does not affect caller's copy of d
		i++;
	}
	return i;
}

/** Get number of elements in cbor array, or -1 on error, or -2 if array length is indeterminate and move the cursor ahead into the array content
	This must be called before attempting to parse the array elements
 */
int cbor_get_array_count(cbor_data_t *d)
{
	if (d == NULL) { return -1; }
	if ( ! cbor_is_array(*d)) { cbor_skip(d); return -1; }
	cbor_skip_tags(d);
	if ( ! cbor_is_indefinite_length(*d)) {
		uint64_t x = cbor_parse_item_num(d);
		if (x > INT32_MAX) {
			return -1;
		}
		return x;
	} else {	// find length of indefinite array :
		skip_bytes(d, 1);	// skip the indefinite array start item
		return cbor_items_before_break(*d);
	}
}

/** Get number of element pairs in cbor array, or -1 on error, or -2 if length is indeterminate and move the cursor ahead into the map content
	This must be called before attempting to parse the map elements
 */
int cbor_get_map_count(cbor_data_t *d)
{
	if (d == NULL) { return -1; }
	if ( ! cbor_is_map(*d)) { cbor_skip(d); return -1; }
	cbor_skip_tags(d);
	if ( ! cbor_is_indefinite_length(*d)) {
		uint64_t x = cbor_parse_item_num(d);
		if (x > INT32_MAX) {
			return -1;
		}
		return x;
	} else {	// find length of indefinite array :
		skip_bytes(d, 1);	// skip the indefinite array start item
		int cnt = cbor_items_before_break(*d);
		return (cnt + 1) / 2;	// if there is an odd number of items then our map is not well-formed.
	}
}

/** Get a cursor to the contents of a cbor map
	This strips off the map wrapper and returns a cursor to just the map contents.
 */
cbor_data_t cbor_map_contents(cbor_data_t d)
{
	if ( ! cbor_is_map(d) ) { return (cbor_data_t) { NULL, 0 }; }
	int len = cbor_item_len(d);
	if (cbor_is_indefinite_length(d)) { // account for one break byte on the end
		len--;
	}
	cbor_data_t orig = d;
	cbor_skip_tags(&d);
	cbor_parse_item_num(&d);	// discard result - this moves past the opening byte(s) of map
	len -= (orig.len - d.len);	// deduct how many bytes we just passed
	d.len = len;
	return d;
}


/** Get CBOR half-precision floating point number from buffer
 */
#if defined(CBOR_FLOAT_SUPPORT)
float32_t cbor_get_half(cbor_data_t *source)
{
	if (source == NULL) { return NAN; }
	cbor_data_t local_cursor = *source;
	cbor_data_t *d = &local_cursor;
	cbor_skip(source);

	if ( ! cbor_is_half(*d)) { return NAN; }
	cbor_skip_tags(d);
	if (d->len < 1 + 2) {
		skip_bytes(d, 1 + 2);
		return NAN;
	}
	uint16_t raw = cbor_parse_item_num(d);
	// the following code adapted from RFC7049 appendix D
	int exp = (raw >> 10) & 0x1f;
	int mant = raw & 0x3ff;
	float32_t val;
	if (exp == 0) {			val = ldexp(mant, -24);
	} else if (exp != 31) {	val = ldexp(mant + 1024, exp - 25);
	} else {				val = (mant == 0) ? INFINITY : NAN;
	}
	return (raw & 0x8000) ? -val : val;
	// END adapted code
}

/** Get CBOR single-precision floating point number from buffer
 */
float32_t cbor_get_single(cbor_data_t *source)
{
	if (source == NULL) { return NAN; }
	cbor_data_t local_cursor = *source;
	cbor_data_t *d = &local_cursor;
	cbor_skip(source);

	if ( ! cbor_is_single(*d)) { return NAN; }
	cbor_skip_tags(d);
	if (d->len <      1 + 4) {
		skip_bytes(d, 1 + 4);
		return NAN;
	}
	uint32_t raw = cbor_parse_item_num(d);
	float32_t *p = (float32_t *) &raw;		// todo: byte order might be wrong! Rewrite it as machine-independent?
	return *p;
}

/** Get CBOR double-precision floating point number from buffer
 */
float64_t cbor_get_double(cbor_data_t *source)
{
	if (source == NULL) { return NAN; }
	cbor_data_t local_cursor = *source;
	cbor_data_t *d = &local_cursor;
	cbor_skip(source);

	if ( ! cbor_is_double(*d)) { return NAN; }
	cbor_skip_tags(d);
	if (d->len <      1 + 8) {
		skip_bytes(d, 1 + 8);
		return NAN;
	}
	uint64_t raw = cbor_parse_item_num(d);
	float64_t *p = (float64_t *) &raw;		// todo: byte order might be wrong! Rewrite it as machine-independent?
	return *p;
}

/** Get CBOR floating point value (any size)
 */
float64_t cbor_get_floating(cbor_data_t *d)
{
	if (cbor_is_half(*d)) {
		return cbor_get_half(d);
	}
	if (cbor_is_single(*d)) {
		return cbor_get_single(d);
	}
	return cbor_get_double(d);
}
#endif

/** Get CBOR bool from buffer
	@return 0 for false, 1 for true, or -1 for error (item is not bool)
 */
int cbor_get_bool(cbor_data_t *source)
{
	if (source == NULL) { return -1; }
	cbor_data_t local_cursor = *source;
	cbor_data_t *d = &local_cursor;
	cbor_skip(source);

	if ( ! cbor_is_bool(*d)) { return -1; }
	bool x = cbor_is_true(*d);
	return (int) x;
}

/**	Does the cbor item have an indefinite length?
	If so then the item will later end with a BREAK.
 */
bool cbor_is_indefinite_length(cbor_data_t d)
{
	cbor_skip_tags(&d);			// does not affect caller - only moves local copy
	if (cbor_empty(d)) { return false; }
	cbor_type_t t = cbor_type(d);
	switch (t) {
		case CBOR_RAW	  :
		case CBOR_TEXT	  :
		case CBOR_ARRAY	  :
		case CBOR_MAP	  :
			return (cbor_length_field(d) == CBOR_LENGTH_INDEFINITE);
		default:
			; // do nothing
	}
	return false;
}

/** Get the length (bytes) of the next cbor entity's cbor representation, including any sub-entities.
	@return item length ( > 0), or 0 for error.  Error may be end-of-data, or nesting too deep.
 */
int cbor_item_len(cbor_data_t startp)
{
	cbor_data_t endp = startp;
	bool success = cbor_skip(&endp);
	if ((endp.len > startp.len) || ! success) {
		return 0;
	}
	return startp.len - endp.len;
}


/** Skip the current cbor item.
	This will skip over an entire array or map.
	If the item is indefinite-length, the whole indefinite-length item will be skipped.
	@return true on success / false on error (cbor data contains too many nested structures)
	On error (nesting too deep) b is set to NULL and len is set to zero.
 */
bool cbor_skip(cbor_data_t *d)
{
	if (d == NULL) { return false; }
	if (cbor_empty(*d)) { return false; }
	cbor_skip_n(d, 20);
	return (d->b != NULL);
}

/** Skip the current cbor item.
	This will skip over an entire array or map.
	On error (nesting too deep) b is set to NULL and len is set to zero.
	@param max_nesting_levels is the number of nesting levels allowed in cbor data.
	(This prevents this recursive function from blowing its stack)
	This does not consider a tag as an item in itself, but it is a component of the item that follows it.
 */
static void cbor_skip_n(cbor_data_t *d, int max_nesting_levels)
{
	if (d == NULL) { return; }
	if (max_nesting_levels <= 0) {		// nesting too deep!  stop processing.
		d->b = NULL;
		d->len = 0;
	}
	if (cbor_empty(*d)) {
		return;
	}
	max_nesting_levels--;
	cbor_skip_tags(d);
	bool indefinite_len = cbor_is_indefinite_length(*d);
	if (cbor_empty(*d)) { return; }
	cbor_type_t t = cbor_type(*d);
	uint64_t n = cbor_parse_item_num(d);
	if (indefinite_len) {	// same handling no matter which type:
		while ( ! cbor_is_break(*d) && ! cbor_empty(*d)) {
			cbor_skip_n(d, max_nesting_levels);
		}
		cbor_skip_n(d, max_nesting_levels);		// also skip the break
	} else {
		uint64_t i;
		switch (t) {
			case CBOR_POS_INT :
			case CBOR_NEG_INT :
			case CBOR_TAG	  :		// CBOR_TAG should never happen since we just did cbor_skip_tags
			case CBOR_OTHER	  :
				// we just read the whole entry.
				break;
			case CBOR_RAW	  :
			case CBOR_TEXT	  :
				skip_bytes(d, n);
				break;
			case CBOR_ARRAY	  :
				for (i=0; (i<n) && ! cbor_empty(*d); i++) {
					cbor_skip_n(d, max_nesting_levels);
				}
				break;
			case CBOR_MAP	  :
				for (i=0; (i<n) && ! cbor_empty(*d); i++) {
					cbor_skip_n(d, max_nesting_levels);
					cbor_skip_n(d, max_nesting_levels);
				}
			break;
			default:	// probably never happens
				;		// do nothing
		}
	}
}

static void cbor_skip_tags(cbor_data_t *d)
{
	while (cbor_get_tag(d) != CBOR_TAG_INVALID) {
		; // do nothing
	}
}


/** Return the number associated with a CBOR item and move the cursor past the number.
	The meaning of the number depends on the item type, which should have been read before calling this.
	On error this function returns 0.
	This function cannot be used to determine if an item is indefinite-length, or break, etc.
	Does not skip tags!
 */
static uint64_t cbor_parse_item_num(cbor_data_t *d)
{
	if (d == NULL) { return 0; }
	if (cbor_empty(*d)) { return 0; }
	int n = get_byte_or_zero(d);
	n &= CBOR_LENGTH_MASK;
	if (n < 24) {		// 24 == 0x18
		return n;
	}
	if (n == 24) {
		return get_byte_or_zero(d);
	}
	int len = 1 << (n - 0x18);
	if (len > 8) {				// indefinite-length start or break, or malformed CBOR data (or new unsupported CBOR protocol feature).
		return 0;
	}
	if (len > d->len) {
		skip_bytes(d, len);
		return 0;
	}
	int i;
	uint64_t x = 0;
	for (i=0; i<len; i++) {
		x = (x << 8) | get_byte_or_zero(d);
	}
	return x;
}



/** Return the CBOR major type.  (does not distinguish sub-types - bool and float both are "CBOR_OTHER")
	Does not skip tags!
 */
static cbor_type_t cbor_type(cbor_data_t d)
{
	if (cbor_empty(d))  { return CBOR_INVALID; }
	return *(d.b) >> 5;
}

/** Return the 5-bit length field.
	Does not skip tags!
 */
static uint8_t cbor_length_field(cbor_data_t d)
{
	if (cbor_empty(d))  { return CBOR_INVALID; }
	return *(d.b) & CBOR_LENGTH_MASK;
}



////////////////////////////////////////////////////////////////////////////////////
//
// JSONish Hierarchical Map Item Retrieval


static cbor_data_t cbor_retrieve_one(cbor_data_t d, char *item_name);


/** These cbor_retrieve_ functions retrieve the value of the named item from a JSONish hierarchical map.
	@param item_path is a string like "message.header.sender" where periods separate
	 hierarchical map entry names.
	 If anything goes wrong,
		functions that accept a default_val return default_val
		cbor_retrieve_raw, cbor_retrieve_text, and cbor_retrieve_bool return < 0
		cbor_retrieve_floating returns NAN
	The caller's cursor never moves (since it is passed in by value).
 */

int64_t		cbor_retrieve_int(			cbor_data_t d, const char *item_path, int64_t default_val)			{ cbor_data_t t = cbor_retrieve(d, item_path); return cbor_get_int(&t, default_val); }
int			cbor_retrieve_raw(			cbor_data_t d, const char *item_path, uint8_t *dest, int dest_len)	{ cbor_data_t t = cbor_retrieve(d, item_path); return cbor_get_raw(&t, dest, dest_len); }
int			cbor_retrieve_raw_ptr(		cbor_data_t d, const char *item_path, uint8_t **ptr)					{ cbor_data_t t = cbor_retrieve(d, item_path); return cbor_get_raw_ptr(&t, ptr); }
#if defined(CBOR_FLOAT_SUPPORT)
float64_t	cbor_retrieve_floating(		cbor_data_t d, const char *item_path)								{ cbor_data_t t = cbor_retrieve(d, item_path); return cbor_get_floating(&t); }
#endif
int			cbor_retrieve_bool(			cbor_data_t d, const char *item_path)								{ cbor_data_t t = cbor_retrieve(d, item_path); return cbor_get_bool(&t); }
#if defined(CBOR_FLOAT_SUPPORT)
float64_t	cbor_retrieve_number_float(	cbor_data_t d, const char *item_path, float64_t default_val)		{ cbor_data_t t = cbor_retrieve(d, item_path); return cbor_get_number_float(&t, default_val); }
#endif
int64_t		cbor_retrieve_number(		cbor_data_t d, const char *item_path, int64_t default_val)			{ cbor_data_t t = cbor_retrieve(d, item_path); return cbor_get_number(&t, default_val); }

int			cbor_retrieve_text(			cbor_data_t d, const char *item_path, char *dest, int dest_len, const char *default_val)
{
	cbor_data_t t = cbor_retrieve(d, item_path);
	int x = cbor_get_text(&t, dest, dest_len);
	if (x < 0) {
		strncpy(dest, default_val, dest_len);
		dest[dest_len-1] = '\0';
	}
	return x;
}


/**  Find a map member in d that is named item_name.
	 @param d should have cbor map data structured like JSON.
	 This searches only the outer level - it does not search inner levels.
	 This assumes the first item in each map pair is the item name, which is expected to be stored as text.
	 Only the first 40 chars of item_name will be compared.
	 @return a pointer to the second item in the named map pair, or an empty cursor if the item could not be found.
 */
static cbor_data_t cbor_retrieve_one(cbor_data_t d, char *item_name)
{
	if (item_name == NULL) { return (cbor_data_t) { NULL, 0 }; }
	if ( ! cbor_is_map(d)) { return (cbor_data_t) { NULL, 0 }; }
	cbor_data_t cur = d;
	// find the named item in the map:
	int map_pairs = cbor_get_map_count(&cur);
	int i;
	for (i=0; i<map_pairs; i++) {
		if (cbor_is_text(cur)) {
			char name[41];
			cbor_get_text(&cur, name, sizeof(name));				// Possible optimization : could do the text compare with name in place.  This would require a new version of cbor_get_text and an extra skip.
			if (strncmp(name, item_name, sizeof(name) - 1) == 0) {
				return cur;		// return second item
			}
		} else {
			cbor_skip(&cur);	// skip first item not read
		}
		cbor_skip(&cur);		// skip second item
	}
	return (cbor_data_t) { NULL, 0 };
}

/** Find a map hierarchy member in d with path matching the item_path.
	@param item_path is a string like "message.header.sender" where periods separate
	 hierarchical map entry names.
	 This expects d points to cbor data structured like JSON - containing a map hierarchy.
	 This will return an empty handle if ! cbor_is_map(&buf)
	 This assumes the first item in each map pair is the item name, which is
	  expected to be stored as text.   Map entry names containing '.' are not supported.
	 Only the first 40 chars of item_name will be compared.
	 @return a pointer to the named item, or NULL if the item could not be found.
	 Future enhancement - maybe add array indexing?
 */
cbor_data_t cbor_retrieve(cbor_data_t d, const char *item_path)
{
	cbor_data_t cur = d;
	char name[41];
	const char *path;
	path = item_path;

	while (( ! cbor_empty(cur)) && (*path != '\0')) {
		// i = first '.' or NULL
		int i = 0;
		while ((path[i] != '.') && (path[i] != '\0')) {
			i++;
		}
		int name_len = smaller(i, sizeof(name) - 1);
		strncpy(name, path, name_len);
		name[name_len] = '\0';
		path += i;
		if (*path == '.') {
			path++;
		}
		cur = cbor_retrieve_one(cur, name);
	}
	return cur;
}




////////////////////////////////////////////////////////////////////////////////////
// primitives



static int smaller(int a, int b)
{
	return (a < b) ? a : b;
}

static int64_t smaller_int64(int64_t a, int64_t b)
{
	return (a < b) ? a : b;
}

/** write a byte to a buffer, moving the buffer pointer ahead and reducing the number of bytes available.
	To detect buffer overrun, allocate an extra byte.  buffer has overrun when d->len <= 0
 */
static void put_byte(cbor_data_t *d, uint8_t x)
{
	if (d == NULL) { return; }
	if (cbor_empty(*d)) { return; }
	*(d->b) = x;
	(d->b)   += 1;
	(d->len) -= 1;
}

static void put_bytes(cbor_data_t *d, uint8_t *x, int x_len)
{
	if ((d == NULL) || (x == NULL) || (x_len <= 0)) { return; }
	if (cbor_empty(*d)) { return; }
	int bytes_to_copy = smaller(d->len, x_len);
	memcpy(d->b, x, bytes_to_copy);
	(d->b)   += bytes_to_copy;
	(d->len) -= bytes_to_copy;
}


///** read a byte from a buffer, moving the buffer pointer ahead and reducing the number of bytes available.
//	@return the byte value or 0 on error
// */
static uint8_t get_byte_or_zero(cbor_data_t *d)
{
	if (d == NULL)		{ return 0; }
	if (cbor_empty(*d)) { return 0; }
	uint8_t x = *(d->b);
	(d->b)   += 1;
	(d->len) -= 1;
	return x;
}

static void skip_bytes(cbor_data_t *d, int64_t bytes_to_skip)
{
	if (d == NULL)		{ return ; }
	if (cbor_empty(*d)) { return ; }
	if (bytes_to_skip < 0) { return; }
	if (bytes_to_skip > d->len) {
		bytes_to_skip = d->len;
	}
	(d->b)   += bytes_to_skip;
	(d->len) -= bytes_to_skip;
}


#ifdef CBOR_TEST
////////////////////////////////////////////////////////////////////////////////////////////////////
// test

#include <stdio.h>


#define CBOR_TEST_ASSERT(x) cbor_test_assert(x, #x)

static void cbor_test_assert(bool condition, const char *test)
{
	if ( ! condition) {
		printf("CBOR_TEST_ASSERT FAILED %s\r\n", test);
	}
}


void cbor_test(void)
{
	uint8_t buf[300];
	cbor_data_t d;
	d = cbor_cursor(buf, sizeof(buf));

// does not test indefinites:
//	cbor_put_raw_start_indefinite(	d);
//	cbor_put_text_start_indefinite(	d);
//	cbor_put_array_start_indefinite(d);

	cbor_put_int(			&d, 5);
	cbor_put_int(			&d, 87);
	cbor_put_int(			&d, -2580);
	cbor_put_int(			&d, 1809330);
	cbor_put_int(			&d, -3987459321809330);
	cbor_put_raw(			&d, (uint8_t *)"test bi\0nary data data", 22);
	cbor_put_text(			&d, "hello there!  (text data)");
	cbor_put_array_start(	&d, 3);
	cbor_put_int(			&d, 87);
	cbor_put_int(			&d, -2580);
	cbor_put_int(			&d, 1809330);
	cbor_put_map_start(		&d, 2);
	cbor_put_text(			&d, "name");
	cbor_put_text(			&d, "fluffy");
	cbor_put_text(			&d, "age");
	cbor_put_int(			&d, 6);
	cbor_put_tag(			&d, CBOR_URI);
	cbor_put_single(		&d, 3.1415926);
	cbor_put_double(		&d, 6.28318530717958647692);
	cbor_put_null(			&d);
	cbor_put_undefined(		&d);
	cbor_put_bool(			&d, true);
	cbor_put_bool(			&d, false);


	d = cbor_cursor(buf, sizeof(buf));

	char b2[100];

	CBOR_TEST_ASSERT( false );						// make sure assert works
	CBOR_TEST_ASSERT(   cbor_is_int(		d));
	CBOR_TEST_ASSERT( ! cbor_is_raw(		d));
	CBOR_TEST_ASSERT( ! cbor_is_text(		d));
	CBOR_TEST_ASSERT( ! cbor_is_array(		d));
	CBOR_TEST_ASSERT( ! cbor_is_map(		d));
	CBOR_TEST_ASSERT( ! cbor_is_single(		d));
	CBOR_TEST_ASSERT( ! cbor_is_double(		d));
	CBOR_TEST_ASSERT( ! cbor_is_null(		d));
	CBOR_TEST_ASSERT( ! cbor_is_undefined(	d));
	CBOR_TEST_ASSERT( ! cbor_is_true(		d));
	CBOR_TEST_ASSERT( ! cbor_is_false(		d));
	CBOR_TEST_ASSERT( ! cbor_is_break(		d));
	CBOR_TEST_ASSERT( ! cbor_is_bool(		d));
	CBOR_TEST_ASSERT( ! cbor_is_indefinite_length(d));
	CBOR_TEST_ASSERT( ! cbor_has_tag(		d));

	CBOR_TEST_ASSERT(cbor_is_int(		d)); printf("cbor_get_int	   = %lld\r\n",		cbor_get_int(	&d, -1));
	CBOR_TEST_ASSERT(cbor_is_int(		d)); printf("cbor_get_int	   = %lld\r\n",		cbor_get_int(	&d, -1));
	CBOR_TEST_ASSERT(cbor_is_int(		d)); printf("cbor_get_int	   = %lld\r\n",		cbor_get_int(	&d, -1));
	CBOR_TEST_ASSERT(cbor_is_int(		d)); printf("cbor_get_int	   = %lld\r\n",		cbor_get_int(	&d, -1));
	CBOR_TEST_ASSERT(cbor_is_int(		d)); printf("cbor_get_int	   = %lld\r\n",		cbor_get_int(	&d, -1));
	CBOR_TEST_ASSERT(cbor_is_raw(		d));
	int len = cbor_get_raw(		  &d, (uint8_t *) b2, sizeof(b2));
	b2[len] = '\0';
											 printf("cbor_get_raw	   = %s\r\n",		b2);
	CBOR_TEST_ASSERT(cbor_is_text(		d));
	cbor_get_text(		  &d, b2, sizeof(b2));
											 printf("cbor_get_text	   = %s\r\n",		b2);
	CBOR_TEST_ASSERT(cbor_is_array(		d)); printf("cbor_get_array_count= %d\r\n",		cbor_get_array_count(&d));
	CBOR_TEST_ASSERT(cbor_is_int(		d)); printf("cbor_get_int	   = %lld\r\n",		cbor_get_int(	&d, -1));
	CBOR_TEST_ASSERT(cbor_is_int(		d)); printf("cbor_get_int	   = %lld\r\n",		cbor_get_int(	&d, -1));
	CBOR_TEST_ASSERT(cbor_is_int(		d)); printf("cbor_get_int	   = %lld\r\n",		cbor_get_int(	&d, -1));
	CBOR_TEST_ASSERT(cbor_is_map(		d)); printf("cbor_get_map_count= %d\r\n",		cbor_get_map_count(	&d));
	CBOR_TEST_ASSERT(cbor_is_text(		d));
	cbor_get_text(		  &d, b2, sizeof(b2));
											 printf("cbor_get_text	   = %s\r\n",		b2);
	CBOR_TEST_ASSERT(cbor_is_text(		d));
	cbor_get_text(		  &d, b2, sizeof(b2));
											 printf("cbor_get_text	   = %s\r\n",		b2);
	CBOR_TEST_ASSERT(cbor_is_text(		d));
	cbor_get_text(		  &d, b2, sizeof(b2));
											 printf("cbor_get_text	   = %s\r\n",		b2);
	CBOR_TEST_ASSERT(cbor_is_int(		d)); printf("cbor_get_int	   = %lld\r\n",		cbor_get_int(	&d, -1));
	CBOR_TEST_ASSERT(cbor_has_tag(		d)); printf("cbor_get_tag	   = %d\r\n",		cbor_get_tag(	&d));
#if defined(CBOR_FLOAT_SUPPORT)
	CBOR_TEST_ASSERT(cbor_is_single(	d)); printf("cbor_get_single   = %f\r\n",		cbor_get_single(&d));
	CBOR_TEST_ASSERT(cbor_is_double(	d)); printf("cbor_get_double   = %f\r\n",		cbor_get_double(&d));
#endif
	CBOR_TEST_ASSERT(cbor_is_null(		d)); cbor_skip(&d);
	CBOR_TEST_ASSERT(cbor_is_undefined(	d)); cbor_skip(&d);
	CBOR_TEST_ASSERT(cbor_is_bool(		d)); printf("cbor_get_bool	   = %d\r\n",		cbor_get_bool(	&d));
	CBOR_TEST_ASSERT(cbor_is_bool(		d)); printf("cbor_get_bool	   = %d\r\n",		cbor_get_bool(	&d));

	printf("cbor_test 1 done\r\n");


	// Now test JSONish CBOR and retrieve functions:
	// The cbor below represents the following JSON:
//	{
//		"message": {
//			"date":	"2016-06-23",
//			"subject": "Hi",
//			"body": "Hello there!",
//			"is_good": true,
//			"number": 42
//		}
//	}

	memset(buf, 0, sizeof(buf));
	d = cbor_cursor(buf, sizeof(buf));

	cbor_put_map_start_indefinite(&d);
		cbor_put_text(&d, "message");		cbor_put_map_start(&d, 6);
		cbor_put_text(&d, "date");			cbor_put_text(&d, "2016-06-23");
		cbor_put_text(&d, "subject");		cbor_put_text(&d, "Hi");
		cbor_put_text(&d, "body");			cbor_put_text(&d, "Hello, there.  This text is a bit longer than the subject.");
		cbor_put_text(&d, "number");		cbor_put_int(&d, 42);
		cbor_put_text(&d, "is_good");		cbor_put_bool(&d, true);
		cbor_put_text(&d, "is_bad");		cbor_put_bool(&d, false);
	cbor_put_end_indefinite(&d);


	d = cbor_cursor(buf, sizeof(buf));

	cbor_retrieve_text(d, "message.date", b2, sizeof(b2), "<blank>");		// discard result
	printf("message.date = %s\r\n", b2);

	cbor_retrieve_text(d, "message.subject", b2, sizeof(b2), "<blank>");		// discard result
	printf("message.subject = %s\r\n", b2);

	cbor_retrieve_text(d, "message.body", b2, sizeof(b2), "<blank>");		// discard result
	printf("message.body = %s\r\n", b2);

	cbor_retrieve_text(d, "message.nonesuch", b2, sizeof(b2), "<blank>");		// discard result
	printf("message.nonesuch = %s\r\n", b2);

	int x = cbor_retrieve_number(d, "message.number", -1);
	printf("message.number = %d\r\n", x);

	int g = cbor_retrieve_number(d, "message.is_good", -1);
	printf("message.is_good (num) = %d\r\n", g);

	bool good = cbor_retrieve_bool(d, "message.is_good");			// cbor_retrieve_bool returns int.  careful casting it to bool because -1 (error) becomes true
	printf("message.is_good = %i\r\n", (int) good);

	int b = cbor_retrieve_number(d, "message.is_bad", -1);
	printf("message.is_bad (num) = %d\r\n", b);

	bool bad = cbor_retrieve_bool(d, "message.is_bad");
	printf("message.is_bad = %i\r\n", (int) bad);

	printf("cbor_test 2 done\r\n");
}

#endif
