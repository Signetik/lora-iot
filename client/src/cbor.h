
// Copyright 2016 Signetik, LLC.
// All rights reserved.
// Signetik Confidential Proprietary Information -- Disclosure prohibited.
// Author: Bruce Carlson
// www.signetik.com


#ifndef CBOR_H
#define CBOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compiler/Machine-specific:
#if defined(CBOR_FLOAT_SUPPORT)
#include <arm_math.h>	// defines float32_t and float64_t
#ifndef float32_t			// todo : how to verify float is really 32 bits?
#define float32_t float
#endif
#ifndef float64_t			// todo : how to verify double is really 64 bits?
#define float64_t double
#endif
#endif


// Some functions in this module return cbor_data_t, which is a struct.
//#pragma GCC diagnostic ignored "-Waggregate-return"
// It would be nice if you could disable this warning on a per-function basis.

// CBOR tag types per RFC 7049 Table 3
typedef enum {
	CBOR_STANDARD_DATE_TIME_STRING	=     0,
	CBOR_EPOCH_DATE_TIME			=     1,
	CBOR_POS_BIGNUM					=     2,
	CBOR_NEG_BIGNUM					=     3,
	CBOR_DECIMAL_FRACTION			=     4,
	CBOR_BIGFLOAT					=     5,

	CBOR_CONVERT_TO_BASE64URL		=    21,
	CBOR_CONVERT_TO_BASE64			=    22,
	CBOR_CONVERT_TO_BASE16			=    23,
	CBOR_ENCODED_CBOR				=    24,

	CBOR_URI						=    32,
	CBOR_BASE64URL					=    33,
	CBOR_BASE64						=    34,
	CBOR_REGEX						=    35,
	CBOR_MIME						=    36,

	CBOR_SELF_DESCRIBE_CBOR			= 55799,

	CBOR_TAG_INVALID				=	 -1
} cbor_tag_t;


/** CBOR data handle or "cursor" that moves sequentially through the
	 buffer as data is written or read.
	A cbor_data_t is good for only one pass through the buffer (reading or writing).
	To go back to the beginning of the buffer the cursor must be re-initialized
	 by once again providing it with the buffer start and length.
 */
typedef struct {
	uint8_t *b;		// pointer to the next location to read/write
	int len;		// how much data remains to be read OR how much space is available to be written
} cbor_data_t;

cbor_data_t cbor_cursor(uint8_t *data, int len);
bool cbor_empty(cbor_data_t d);
cbor_data_t cbor_get_read_cursor(cbor_data_t current_cbor_cursor, uint8_t *original_buffer_pointer);


// WRITING CBOR

// cbor_put_ functions write to the cbor buffer and move the cursor ahead.

void cbor_put_int(						cbor_data_t *d, int64_t x);
void cbor_put_raw(						cbor_data_t *d, uint8_t *data, unsigned int data_len);
void cbor_put_raw_start_indefinite(		cbor_data_t *d);
void cbor_put_text(						cbor_data_t *d, const char *text);
void cbor_put_text_start_indefinite(	cbor_data_t *d);
void cbor_put_array_start(				cbor_data_t *d, unsigned int array_element_count);
void cbor_put_array_start_indefinite(	cbor_data_t *d);
void cbor_put_map_start(				cbor_data_t *d, unsigned int map_element_count);
void cbor_put_map_start_indefinite(		cbor_data_t *d);
void cbor_put_end_indefinite(			cbor_data_t *d);
void cbor_put_tag(						cbor_data_t *d, cbor_tag_t tag);
#if defined(CBOR_FLOAT_SUPPORT)
void cbor_put_single(					cbor_data_t *d, float32_t x);
void cbor_put_single_reverse(			cbor_data_t *d, float32_t x);

void cbor_put_double(					cbor_data_t *d, float64_t x);
#endif
void cbor_put_null(						cbor_data_t *d);
void cbor_put_undefined(				cbor_data_t *d);
void cbor_put_bool(						cbor_data_t *d, bool x);

// READING CBOR

// cbor_is_ functions return information about the next cbor item in the buffer without moving the cursor.

bool cbor_is_int(		cbor_data_t d);
bool cbor_is_raw(		cbor_data_t d);
bool cbor_is_text(		cbor_data_t d);
bool cbor_is_array(		cbor_data_t d);
bool cbor_is_map(		cbor_data_t d);
bool cbor_is_half(		cbor_data_t d);
bool cbor_is_single(	cbor_data_t d);
bool cbor_is_double(	cbor_data_t d);
bool cbor_is_floating(	cbor_data_t d);
bool cbor_is_null(		cbor_data_t d);
bool cbor_is_undefined(	cbor_data_t d);
bool cbor_is_true(		cbor_data_t d);
bool cbor_is_false(		cbor_data_t d);
bool cbor_is_break(		cbor_data_t d);
bool cbor_is_bool(		cbor_data_t d);
#define cbor_is_end_indefinite(b) cbor_is_break(b)
bool cbor_is_indefinite_length(cbor_data_t d);
bool cbor_is_number(	cbor_data_t d);

bool cbor_has_tag(		cbor_data_t d);


// cbor_get_ functions read data from the cbor buffer and move the cursor ahead.

int64_t cbor_get_int(		cbor_data_t *d, int64_t default_val);
int		cbor_get_raw(		cbor_data_t *d, uint8_t *dest, int dest_len);
int 	cbor_get_raw_ptr(	cbor_data_t *source, uint8_t **ptr);
int		cbor_get_text(		cbor_data_t *d, char *dest, int dest_len);
int		cbor_get_array_count(cbor_data_t *d);
//void	cbor_enter_array(	cbor_data_t *d);
int		cbor_get_map_count(	cbor_data_t *d);
//void	cbor_enter_map(		cbor_data_t *d);

cbor_data_t cbor_map_contents(cbor_data_t d);

cbor_tag_t	cbor_get_tag(			cbor_data_t *d);
#if defined(CBOR_FLOAT_SUPPORT)
float32_t	cbor_get_half(			cbor_data_t *d);
float32_t	cbor_get_single(		cbor_data_t *d);
float64_t	cbor_get_double(		cbor_data_t *d);
float64_t	cbor_get_floating(		cbor_data_t *d);
#endif
int			cbor_get_bool(			cbor_data_t *d);
int64_t		cbor_get_number(		cbor_data_t *d, int64_t default_val);
#if defined(CBOR_FLOAT_SUPPORT)
float64_t	cbor_get_number_float(	cbor_data_t *d, float64_t default_val);
#endif

bool	cbor_skip(			cbor_data_t *d);
int		cbor_item_len(		cbor_data_t d);



// cbor_retrieve returns a named value from JSONish cbor

// suggest starting with cbor_retrieve_number
int64_t		cbor_retrieve_number(		cbor_data_t d, const char *item_path, int64_t default_val);
#if defined(CBOR_FLOAT_SUPPORT)
float64_t	cbor_retrieve_number_float(	cbor_data_t d, const char *item_path, float64_t default_val);
#endif

int64_t		cbor_retrieve_int(		cbor_data_t d, const char *item_path, int64_t default_val);
int			cbor_retrieve_raw(		cbor_data_t d, const char *item_path, uint8_t *dest, int dest_len);
int			cbor_retrieve_raw_ptr(	cbor_data_t d, const char *item_path, uint8_t **ptr);
int			cbor_retrieve_text(		cbor_data_t d, const char *item_path, char *dest, int dest_len, const char *default_val);
#if defined(CBOR_FLOAT_SUPPORT)
float64_t	cbor_retrieve_floating(	cbor_data_t d, const char *item_path);
#endif
int			cbor_retrieve_bool(		cbor_data_t d, const char *item_path);

// cbor_retrieve returns a new cursor that points to the requested data item
cbor_data_t cbor_retrieve(cbor_data_t d, const char *item_path);


void cbor_test(void);



#ifdef __cplusplus
}
#endif

#endif
