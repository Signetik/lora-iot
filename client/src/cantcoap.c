/*
Copyright (c) 2013, Ashley Mills.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Adapted to C by Bruce Carlson, Signetik LLC.
// Copyright 2016 Signetik, LLC.
// All rights reserved.
// Signetik Confidential Proprietary Information -- Disclosure prohibited.
// www.signetik.com


// version, 2 bits
// type, 2 bits
	// 00 Confirmable
	// 01 Non-confirmable
	// 10 Acknowledgement
	// 11 Reset

// token length, 4 bits
// length of token in bytes (only 0 to 8 bytes allowed)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
//#include <unistd.h>
#include <string.h>
#include "cantcoap.h"
//#include "arpa/inet.h"
#include "sysdep.h"

///// Memory-managed constructor. Buffer for PDU is dynamically sized and allocated by the object.
///**
// * When using this constructor, the CoapPDU class will allocate space for the PDU.
// * Contrast this with the parameterized constructors, which allow the use of an external buffer.
// *
// * Note, the PDU container and space can be reused by issuing a CoapPDU_reset(). If the new PDU exceeds the
// * space of the previously allocated memory, then further memory will be dynamically allocated.
// *
// * Deleting the object will free the Object container and all dynamically allocated memory.
// *
// * \note It would have been nice to use something like UDP_CORK or MSG_MORE, to allow separate buffers
// * for token, options, and payload but these FLAGS aren't implemented for UDP in LwIP so stuck with one buffer for now.
// *
// * CoAP version defaults to 1.
// *
// * \sa CoapPDU_Init(uint8_t *pdu, int pduLength), CoapPDU_Init_(uint8_t *buffer, int bufferLength, int pduLength),
// * CoapPDU:CoapPDU()~
// *
// */
//void CoapPDU_Init(struct CoapPDU *x) {
//	// pdu
//	x->_pdu = (uint8_t*)calloc(4,sizeof(uint8_t));
//	x->_pduLength = 4;
//	x->_bufferLength = x->_pduLength;
//
//	//options
//	x->_numOptions = 0;
//	x->_maxAddedOptionNumber = 0;
//
//	// payload
//	x->_payloadPointer = NULL;
//	x->_payloadLength = 0;
//
//	x->_constructedFromBuffer = 0;
//
//	CoapPDU_setVersion(1);
//}
//
///// Construct a PDU using an external buffer. No copy of the buffer is made.
///**
// * This constructor is normally used where a PDU has been received over the network, and it's length is known.
// * In this case the CoapPDU object is probably going to be used as a temporary container to access member values.
// *
// * It is assumed that \b pduLength is the length of the actual CoAP PDU, and consequently the buffer will also be this size,
// * contrast this with CoapPDU_Init(uint8_t *buffer, int bufferLength, int pduLength) which allows the buffer to
// * be larger than the PDU.
// *
// * A PDU constructed in this manner must be validated with CoapPDU_validate() before the member variables will be accessible.
// *
// * \warning The validation call parses the PDU structure to set some internal parameters. If you do
// * not validate the PDU, then the behaviour of member access functions will be undefined.
// *
// * The buffer can be reused by issuing a CoapPDU_reset() but the class will not change the size of the buffer. If the
// * newly constructed PDU exceeds the size of the buffer, the function called (for example CoapPDU_addOption) will fail.
// *
// * Deleting this object will only delete the Object container and will not delete the PDU buffer.
// *
// * @param pdu A pointer to an array of bytes which comprise the CoAP PDU
// * @param pduLength The length of the CoAP PDU pointed to by \b pdu
//
// * \sa CoapPDU_Init(), CoapPDU_Init(uint8_t *buffer, int bufferLength, int pduLength)
// */
//void CoapPDU_Init(struct CoapPDU *x, uint8_t *pdu, int pduLength) : CoapPDU(pdu,pduLength,pduLength) {
//	// delegated to CoapPDU_Init(uint8_t *buffer, int bufferLength, int pduLength)
//}

/// Construct object from external buffer that may be larger than actual PDU.
/**
 * This differs from CoapPDU_Init(uint8_t *pdu, int pduLength) in that the buffer may be larger
 * than the actual CoAP PDU contained int the buffer. This is typically used when a large buffer is reused
 * multiple times. Note that \b pduLength can be 0.
 *
 * If an actual CoAP PDU is passed in the buffer, \b pduLength should match its length. CoapPDU_validate() must
 * be called to initiate the object before member functions can be used.
 *
 * A PDU constructed in this manner must be validated with CoapPDU_validate() before the member variables will be accessible.
 *
 * \warning The validation call parses the PDU structure to set some internal parameters. If you do
 * not validate the PDU, then the behaviour of member access functions will be undefined.
 *
 * The buffer can be reused by issuing a CoapPDU_reset() but the class will not change the size of the buffer. If the
 * newly constructed PDU exceeds the size of the buffer, the function called (for example CoapPDU_addOption) will fail.
 *
 * Deleting this object will only delete the Object container and will not delete the PDU buffer.
 *
 * \param buffer A buffer which either contains a CoAP PDU or is intended to be used to construct one.
 * \param bufferLength The length of the buffer
 * \param pduLength If the buffer contains a CoAP PDU, this specifies the length of the PDU within the buffer.
 *
 * \sa CoapPDU_Init(), CoapPDU_Init(uint8_t *pdu, int pduLength)
 */
void CoapPDU_Init(struct CoapPDU *x, uint8_t *buffer, int bufferLength, int pduLength) {
	// sanity
	if(pduLength<4&&pduLength!=0) {
		DBG("PDU cannot have a length less than 4");
	}

	// pdu
	x->_pdu = buffer;
	x->_bufferLength = bufferLength;
	if(pduLength==0) {
		// this is actually a fresh pdu, header always exists
		x->_pduLength = 4;
		// make sure header is zeroed
		x->_pdu[0] = 0x00; x->_pdu[1] = 0x00; x->_pdu[2] = 0x00; x->_pdu[3] = 0x00;
		CoapPDU_setVersion(x, 1);
	} else {
		x->_pduLength = pduLength;
	}

	x->_constructedFromBuffer = 1;

	// options
	x->_numOptions = 0;
	x->_maxAddedOptionNumber = 0;

	// payload
	x->_payloadPointer = NULL;
	x->_payloadLength = 0;
}

/// Reset CoapPDU container so it can be reused to build a new PDU.
/**
 * This resets the CoapPDU container, setting the pdu length, option count, etc back to zero. The
 * PDU can then be populated as if it were newly constructed.
 *
 * Note that the space available will depend on how the CoapPDU was originally constructed:
 * -# CoapPDU_Init()
 *
 * 	Available space initially be \b x->_pduLength. But further space will be allocated as needed on demand,
 *    limited only by the OS/environment.
 *
 * -# CoapPDU_Init(uint8_t *pdu, int pduLength)
 *
 *		Space is limited by the variable \b pduLength. The PDU cannot exceed \b pduLength bytes.
 *
 * -# CoapPDU_Init(uint8_t *buffer, int bufferLength, int pduLength)
 *
 *		Space is limited by the variable \b bufferLength. The PDU cannot exceed \b bufferLength bytes.
 *
 * \return 0 on success, 1 on failure.
 */
int CoapPDU_reset(struct CoapPDU *x) {
	// pdu
	memset(x->_pdu,0x00,x->_bufferLength);
	// packet always has at least a header
	x->_pduLength = 4;

	// options
	x->_numOptions = 0;
	x->_maxAddedOptionNumber = 0;
	// payload
	x->_payloadPointer = NULL;
	x->_payloadLength = 0;
	return 0;
}

/// Validates a PDU constructed using an external buffer.
/**
 * When a CoapPDU is constructed using an external buffer, the programmer must call this function to
 * check that the received PDU is a valid CoAP PDU.
 *
 * \warning The validation call parses the PDU structure to set some internal parameters. If you do
 * not validate the PDU, then the behaviour of member access functions will be undefined.
 *
 * \return 1 if the PDU validates correctly, 0 if not. XXX maybe add some error codes
 */
int CoapPDU_validate(struct CoapPDU *x) {
	if(x->_pduLength<4) {
		DBG("PDU has to be a minimum of 4 bytes. This: %d bytes",x->_pduLength);
		return 0;
	}

	// check header
	//  0                   1                   2                   3
	//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// |Ver| T |  TKL  |      Code     |          Message ID           |
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// |   Token (if any, TKL bytes) ...
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// |   Options (if any) ...
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// |1 1 1 1 1 1 1 1|    Payload (if any) ...
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	// version must be 1
	int version = CoapPDU_getVersion(x);
	if (version != 1) {
		DBG("Invalid version: %d", version);
		return 0;
	}
	DBG("Version: %d", version);
	DBG("Type: %d", CoapPDU_getType(x));

	// token length must be between 0 and 8
	int tokenLength = CoapPDU_getTokenLength(x);
	if(tokenLength<0||tokenLength>8) {
		DBG("Invalid token length: %d",tokenLength);
		return 0;
	}
	DBG("Token length: %d",tokenLength);
	// check total length
	if((COAP_HDR_SIZE+tokenLength)>x->_pduLength) {
		DBG("Token length would make pdu longer than actual length.");
		return 0;
	}

	// check that code is valid
	CoapPDU_Code code = CoapPDU_getCode(x);
	if(code<COAP_EMPTY ||
		(code>COAP_DELETE&&code<COAP_CREATED) ||
		(code>COAP_CONTENT&&code<COAP_BAD_REQUEST) ||
		(code>COAP_NOT_ACCEPTABLE&&code<COAP_PRECONDITION_FAILED) ||
		(code==0x8E) ||
		(code>COAP_UNSUPPORTED_CONTENT_FORMAT&&code<COAP_INTERNAL_SERVER_ERROR) ||
		(code>COAP_PROXYING_NOT_SUPPORTED) ) {
		DBG("Invalid CoAP code: %d",code);
		return 0;
	}
	DBG("CoAP code: %d",code);

	// token can be anything so nothing to check

	// check that options all make sense
	uint16_t optionDelta =0, optionNumber = 0, optionValueLength = 0;
	int totalLength = 0;

	// first option occurs after token
	int optionPos = COAP_HDR_SIZE + CoapPDU_getTokenLength(x);

	// may be 0 options
	if(optionPos==x->_pduLength) {
		DBG("No options. No payload.");
		x->_numOptions = 0;
		x->_payloadLength = 0;
		return 1;
	}

	int bytesRemaining = x->_pduLength-optionPos;
	int numOptions = 0;
	uint8_t upperNibble = 0x00, lowerNibble = 0x00;

	// walk over options and record information
	while(1) {
		// check for payload marker
		if(bytesRemaining>0) {
			uint8_t optionHeader = x->_pdu[optionPos];
			if(optionHeader==0xFF) {
				// payload
				if(bytesRemaining>1) {
					x->_payloadPointer = &x->_pdu[optionPos+1];
					x->_payloadLength = (bytesRemaining-1);
					x->_numOptions = numOptions;
					DBG("Payload found, length: %d",x->_payloadLength);
					return 1;
				}
				// payload marker but no payload
				x->_payloadPointer = NULL;
				x->_payloadLength = 0;
				DBG("Payload marker but no payload.");
				return 0;
			}

			// check that option delta and option length are valid values
			upperNibble = (optionHeader & 0xF0) >> 4;
			lowerNibble = (optionHeader & 0x0F);
			if(upperNibble==0x0F||lowerNibble==0x0F) {
				DBG("Expected option header or payload marker, got: 0x%x%x",upperNibble,lowerNibble);
				return 0;
			}
			DBG("Option header byte appears sane: 0x%x%x",upperNibble,lowerNibble);
		} else {
			DBG("No more data. No payload.");
			x->_payloadPointer = NULL;
			x->_payloadLength = 0;
			x->_numOptions = numOptions;
			return 1;
		}

		// skip over option header byte
		bytesRemaining--;

		// check that there is enough space for the extended delta and length bytes (if any)
		int headerBytesNeeded = computeExtraBytes(upperNibble);
		DBG("%d extra bytes needed for extended delta",headerBytesNeeded);
		if(headerBytesNeeded>bytesRemaining) {
			DBG("Not enough space for extended option delta, needed %d, have %d.",headerBytesNeeded,bytesRemaining);
			return 0;
		}
		headerBytesNeeded += computeExtraBytes(lowerNibble);
		if(headerBytesNeeded>bytesRemaining) {
			DBG("Not enough space for extended option length, needed %d, have %d.",
				(headerBytesNeeded-computeExtraBytes(upperNibble)),bytesRemaining);
			return 0;
		}
		DBG("Enough space for extended delta and length: %d, continuing.",headerBytesNeeded);

		// extract option details
		optionDelta = CoapPDU_getOptionDelta(x, &x->_pdu[optionPos]);
		optionNumber += optionDelta;
		optionValueLength = CoapPDU_getOptionValueLength(x, &x->_pdu[optionPos]);
		DBG("Got option: %d with length %d",optionNumber,optionValueLength);
		// compute total length
		totalLength = 1; // mandatory header
		totalLength += computeExtraBytes(optionDelta);
		totalLength += computeExtraBytes(optionValueLength);
		totalLength += optionValueLength;
		// check there is enough space
		if(optionPos+totalLength>x->_pduLength) {
			DBG("Not enough space for option payload, needed %d, have %d.",(totalLength-headerBytesNeeded-1),x->_pduLength-optionPos);
			return 0;
		}
		DBG("Enough space for option payload: %d %d",optionValueLength,(totalLength-headerBytesNeeded-1));

		// recompute bytesRemaining
		bytesRemaining -= totalLength;
		bytesRemaining++; // correct for previous --

		// move to next option
		optionPos += totalLength;

		// inc number of options XXX
		numOptions++;
	}

	return 1;
}

/// Destructor. Does not free buffer if constructor passed an external buffer.
/**
 * The destructor acts differently, depending on how the object was initially constructed (from buffer or not):
 *
 * -# CoapPDU_Destroy()
 *
 * 	Complete object is destroyed.
 *
 * -# CoapPDU_Init(uint8_t *pdu, int pduLength)
 *
 *		Only object container is destroyed. \b pdu is left intact.
 *
 * -# CoapPDU_Init(uint8_t *buffer, int bufferLength, int pduLength)
 *
 *		Only object container is destroyed. \b pdu is left intact.
 *
 */
void CoapPDU_Destroy(struct CoapPDU *x) {
	if(!x->_constructedFromBuffer) {
		free(x->_pdu);
	}
}

/// Returns a pointer to the internal buffer.
uint8_t* CoapPDU_getPDUPointer(struct CoapPDU *x) {
	return x->_pdu;
}

/// Set the PDU length to the length specified.
/**
 * This is used when re-using a PDU container before calling CoapPDU_validate() as it
 * is not possible to deduce the length of a PDU since the payload has no length marker.
 * \param len The length of the PDU
 */
void CoapPDU_setPDULength(struct CoapPDU *x, int len) {
	x->_pduLength = len;
}

int CoapPDU_setURI_len(struct CoapPDU *x, char *uri, int urilen);

///// Shorthand function for setting a resource URI.
///**
// * Calls CoapPDU_setURI(uri,strlen(uri).
// */
int CoapPDU_setURI(struct CoapPDU *x, char *uri) {
	return CoapPDU_setURI_len(x, uri,strlen(uri));
}

/// Shorthand function for setting a resource URI.
/**
 * This will parse the supplied \b uri and construct enough URI_PATH and URI_QUERY options to encode it.
 * The options are added to the PDU.
 *
 * At present only simple URI formatting is handled, only '/','?', and '&' separators, and no port or protocol specificaiton.
 *
 * The function will split on '/' and create URI_PATH elements until it either reaches the end of the string
 * in which case it will stop or if it reaches '?' it will start splitting on '&' and create URI_QUERY elements
 * until it reaches the end of the string.
 *
 * Here is an example:
 *
 * /a/b/c/d?x=1&y=2&z=3
 *
 * Will be broken into four URI_PATH elements "a", "b", "c", "d", and three URI_QUERY elements "x=1", "y=2", "z=3"
 *
 * TODO: Add protocol extraction, port extraction, and some malformity checking.
 *
 * \param uri The uri to parse.
 * \param urilen The length of the uri to parse.
 *
 * \return 1 on success, 0 on failure.
 */
int CoapPDU_setURI_len(struct CoapPDU *x, char *uri, int urilen) {
	// only '/', '?', '&' and ascii chars allowed

	// sanitation
	if(urilen<=0||uri==NULL) {
		DBG("Null or zero-length uri passed.");
		return 1;
	}

	// single character URI path (including '/' case)
	if(urilen==1) {
		CoapPDU_addOption(x, COAP_OPTION_URI_PATH,1,(uint8_t*)uri);
		return 0;
	}

	// TODO, queries
	// extract ? to mark where to stop processing path components
	// and then process the query params

	// local vars
	char *startP=uri,*endP=NULL;
	int oLen = 0;
	char splitChar = '/';
	int queryStageTriggered = 0;
	uint16_t optionType = COAP_OPTION_URI_PATH;
	while(1) {
		// stop at end of string or query
		if(*startP==0x00||*(startP+1)==0x00) {
			break;
		}

		// ignore leading slash
		if(*startP==splitChar) {
			DBG("Skipping leading slash");
			startP++;
		}

		// find next split point
		endP = strchr(startP,splitChar);

		// might not be another slash
		if(endP==NULL) {
			DBG("Ending out of slash");
			// check if there is a ?
			endP = strchr(startP,'?');
			// done if no queries
			if(endP==NULL) {
				endP = uri+urilen;
			} else {
				queryStageTriggered = 1;
			}
		}

		// get length of segment
		oLen = endP-startP;

		#ifdef DEBUG
		char *b = (char*)malloc(oLen+1);
		memcpy(b,startP,oLen);
		b[oLen] = 0x00;
		DBG("Adding URI_PATH %s",b);
		free(b);
		#endif

		// add option
		if(CoapPDU_addOption(x, optionType,oLen,(uint8_t*)startP)!=0) {
			DBG("Error adding option");
			return 1;
		}
		startP = endP;

		if(queryStageTriggered) {
			splitChar = '&';
			optionType = COAP_OPTION_URI_QUERY;
			startP++;
			queryStageTriggered = false;
		}
	}

	return 0;
}

/// Shorthand for adding a URI QUERY to the option list.
/**
 * Adds a new option to the CoAP PDU that encodes a URI_QUERY.
 *
 * \param query The uri query to encode.
 * \return 0 on success, 1 on failure.
 */
int CoapPDU_addURIQuery(struct CoapPDU *x, char *query) {
	return CoapPDU_addOption(x, COAP_OPTION_URI_QUERY,strlen(query),(uint8_t*)query);
}

/// Concatenates any URI_PATH elements and URI_QUERY elements into a single string.
/**
 * Parses the PDU options and extracts all URI_PATH and URI_QUERY elements,
 * concatenating them into a single string with slash and amphersand separators accordingly.
 *
 * The produced string will be NULL terminated.
 *
 * \param dst Buffer into which to copy the concatenated path elements.
 * \param dstlen Length of buffer.
 * \param outLen Pointer to integer, into which URI length will be placed.
 *
 * \return 0 on success, 1 on failure. \b outLen will contain the length of the concatenated elements.
 */
int CoapPDU_getURI(struct CoapPDU *x, char *dst, int dstlen, int *outLen) {
	if(outLen==NULL) {
		DBG("Output length pointer is NULL");
		return 1;
	}

	if(dst==NULL) {
		DBG("NULL destination buffer");
		*outLen = 0;
		return 1;
	}

	// check destination space
	if(dstlen<=0) {
		*dst = 0x00;
		*outLen = 0;
		DBG("Destination buffer too small (0)!");
		return 1;
	}
	// check option count
	if(x->_numOptions==0) {
		*dst = 0x00;
		*outLen = 0;
		return 0;
	}
	// get options
	CoapPDU_CoapOption *options = CoapPDU_getOptions(x);
	if(options==NULL) {
		*dst = 0x00;
		*outLen = 0;
		return 0;
	}
	// iterate over options to construct URI
	CoapPDU_CoapOption *o = NULL;
	int bytesLeft = dstlen-1; // space for 0x00
	int oLen = 0;
	// add slash at beggining
	if(bytesLeft>=1) {
		*dst = '/';
		dst++;
		bytesLeft--;
	} else {
		DBG("No space for initial slash needed 1, got %d",bytesLeft);
		free(options);
		return 1;
	}

	char separator = '/';
	int firstQuery = 1;

	for(int i=0; i<x->_numOptions; i++) {
		o = &options[i];
		oLen = o->optionValueLength;
		if(o->optionNumber==COAP_OPTION_URI_PATH||o->optionNumber==COAP_OPTION_URI_QUERY) {
			// if the option is a query, change the separator to &
			if(o->optionNumber==COAP_OPTION_URI_QUERY) {
				if(firstQuery) {
					// change previous '/' to a '?'
					*(dst-1) = '?';
					firstQuery = 0;
				}
				separator = '&';
			}

			// check space
			if(oLen>bytesLeft) {
				DBG("Destination buffer too small, needed %d, got %d",oLen,bytesLeft);
				free(options);
				return 1;
			}

			// case where single '/' exists
			if(oLen==1&&o->optionValuePointer[0]=='/') {
				*dst = 0x00;
				*outLen = 1;
				free(options);
				return 0;
			}

			// copy URI path or query component
			memcpy(dst,o->optionValuePointer,oLen);

			// adjust counters
			dst += oLen;
			bytesLeft -= oLen;

			// add separator following (don't know at this point if another option is coming)
			if(bytesLeft>=1) {
				*dst = separator;
				dst++;
				bytesLeft--;
			} else {
				DBG("Ran out of space after processing option");
				free(options);
				return 1;
			}
		}
	}

	// remove terminating separator
	dst--;
	bytesLeft++;
	// add null terminating byte (always space since reserved)
	*dst = 0x00;
	*outLen = (dstlen-1)-bytesLeft;
	free(options);
	return 0;
}

/// Sets the CoAP version.
/**
 * \param version CoAP version between 0 and 3.
 * \return 0 on success, 1 on failure.
 */
int CoapPDU_setVersion(struct CoapPDU *x, uint8_t version) {
	if(version>3) {
		return 1;
	}

	x->_pdu[0] &= 0x3F;
	x->_pdu[0] |= (version << 6);
	return 0;
}

/**
 * Gets the CoAP Version.
 * @return The CoAP version between 0 and 3.
 */
uint8_t CoapPDU_getVersion(struct CoapPDU *x) {
	return (x->_pdu[0]&0xC0)>>6;
}

/**
 * Sets the type of this CoAP PDU.
 * \param mt The type, one of:
 * - COAP_CONFIRMABLE
 * - COAP_NON_CONFIRMABLE
 * - COAP_ACKNOWLEDGEMENT
 * - COAP_RESET.
 */
void CoapPDU_setType(struct CoapPDU *x, CoapPDU_Type mt) {
	x->_pdu[0] &= 0xCF;
	x->_pdu[0] |= mt;
}

/// Returns the type of the PDU.
CoapPDU_Type CoapPDU_getType(struct CoapPDU *x) {
	return (CoapPDU_Type)(x->_pdu[0]&0x30);
}


/// Set the token length.
/**
 * \param tokenLength The length of the token in bytes, between 0 and 8.
 * \return 0 on success, 1 on failure.
 */
int CoapPDU_setTokenLength(struct CoapPDU *x, uint8_t tokenLength) {
	if(tokenLength>8)
		return 1;

	x->_pdu[0] &= 0xF0;
	x->_pdu[0] |= tokenLength;
	return 0;
}

/// Returns the token length.
int CoapPDU_getTokenLength(struct CoapPDU *x) {
	return x->_pdu[0] & 0x0F;
}

/// Returns a pointer to the PDU token.
uint8_t* CoapPDU_getTokenPointer(struct CoapPDU *x) {
	if(CoapPDU_getTokenLength(x)==0) {
		return NULL;
	}
	return &x->_pdu[4];
}

/// Set the PDU token to the supplied byte sequence.
/**
 * This sets the PDU token to \b token and sets the token length to \b tokenLength.
 * \param token A sequence of bytes representing the token.
 * \param tokenLength The length of the byte sequence.
 * \return 0 on success, 1 on failure.
 */
int CoapPDU_setToken(struct CoapPDU *x, uint8_t *token, uint8_t tokenLength) {
	DBG("Setting token");
	if(token==NULL) {
		DBG("NULL pointer passed as token reference");
		return 1;
	}

	if(tokenLength==0) {
		DBG("Token has zero length");
		return 1;
	}

	// if tokenLength has not changed, just copy the new value
	uint8_t oldTokenLength = CoapPDU_getTokenLength(x);
	if(tokenLength==oldTokenLength) {
		memcpy((void*)&x->_pdu[4],token,tokenLength);
		return 0;
	}

	// otherwise compute new length of PDU
	uint8_t oldPDULength = x->_pduLength;
	x->_pduLength -= oldTokenLength;
	x->_pduLength += tokenLength;

	// now, have to shift old memory around, but shift direction depends
	// whether pdu is now bigger or smaller
	if(x->_pduLength>oldPDULength) {
		// new PDU is bigger, need to allocate space for new PDU
		if(!x->_constructedFromBuffer) {
			uint8_t *newMemory = (uint8_t*)realloc(x->_pdu,x->_pduLength);
			if(newMemory==NULL) {
				// malloc failed
				DBG("Failed to allocate memory for token");
				x->_pduLength = oldPDULength;
				return 1;
			}
			x->_pdu = newMemory;
			x->_bufferLength = x->_pduLength;
		} else {
			// constructed from buffer, check space
			if(x->_pduLength>x->_bufferLength) {
				DBG("Buffer too small to contain token, needed %d, got %d.",x->_pduLength-oldPDULength,x->_bufferLength-oldPDULength);
				x->_pduLength = oldPDULength;
				return 1;
			}
		}

		// and then shift everything after token up to end of new PDU
		// memory overlaps so do this manually so to avoid additional mallocs
		int shiftOffset = x->_pduLength-oldPDULength;
		int shiftAmount = x->_pduLength-tokenLength-COAP_HDR_SIZE; // everything after token
		CoapPDU_shiftPDUUp(x, shiftOffset,shiftAmount);

		// now copy the token into the new space and set official token length
		memcpy((void*)&x->_pdu[4],token,tokenLength);
		CoapPDU_setTokenLength(x, tokenLength);

		// and return success
		return 0;
	}

	// new PDU is smaller, copy the new token value over the old one
	memcpy((void*)&x->_pdu[4],token,tokenLength);
	// and shift everything after the new token down
	int startLocation = COAP_HDR_SIZE+tokenLength;
	int shiftOffset = oldPDULength-x->_pduLength;
	int shiftAmount = oldPDULength-oldTokenLength-COAP_HDR_SIZE;
	CoapPDU_shiftPDUDown(x, startLocation,shiftOffset,shiftAmount);
	// then reduce size of buffer
	if(!x->_constructedFromBuffer) {
		uint8_t *newMemory = (uint8_t*)realloc(x->_pdu,x->_pduLength);
		if(newMemory==NULL) {
			// malloc failed, PDU in inconsistent state
			DBG("Failed to shrink PDU for new token. PDU probably broken");
			return 1;
		}
		x->_pdu = newMemory;
		x->_bufferLength = x->_pduLength;
	}

	// and officially set the new tokenLength
	CoapPDU_setTokenLength(x, tokenLength);
	return 0;
}

/// Sets the CoAP response code
void CoapPDU_setCode(struct CoapPDU *x, CoapPDU_Code code) {
	x->_pdu[1] = code;
	// there is a limited set of response codes
}

/// Gets the CoAP response code
CoapPDU_Code CoapPDU_getCode(struct CoapPDU *x) {
	return (CoapPDU_Code)x->_pdu[1];
}


/// Converts a http status code as an integer, to a CoAP code.
/**
 * \param httpStatus the HTTP status code as an integer (e.g 200)
 * \return The correct corresponding CoapPDU_Code on success,
 * CoapPDU_COAP_UNDEFINED_CODE on failure.
 */
CoapPDU_Code CoapPDU_httpStatusToCode(struct CoapPDU *x, int httpStatus)
{
	switch(httpStatus) {
		case 1:		return COAP_GET;
		case 2:		return COAP_POST;
		case 3:		return COAP_PUT;
		case 4:		return COAP_DELETE;
		case 201:	return COAP_CREATED;
		case 202:	return COAP_DELETED;
		case 203:	return COAP_VALID;
		case 204:	return COAP_CHANGED;
		case 205:	return COAP_CONTENT;
		case 400:	return COAP_BAD_REQUEST;
		case 401:	return COAP_UNAUTHORIZED;
		case 402:	return COAP_BAD_OPTION;
		case 403:	return COAP_FORBIDDEN;
		case 404:	return COAP_NOT_FOUND;
		case 405:	return COAP_METHOD_NOT_ALLOWED;
		case 406:	return COAP_NOT_ACCEPTABLE;
		case 412:	return COAP_PRECONDITION_FAILED;
		case 413:	return COAP_REQUEST_ENTITY_TOO_LARGE;
		case 415:	return COAP_UNSUPPORTED_CONTENT_FORMAT;
		case 500:	return COAP_INTERNAL_SERVER_ERROR;
		case 501:	return COAP_NOT_IMPLEMENTED;
		case 502:	return COAP_BAD_GATEWAY;
		case 503:	return COAP_SERVICE_UNAVAILABLE;
		case 504:	return COAP_GATEWAY_TIMEOUT;
		case 505:	return COAP_PROXYING_NOT_SUPPORTED;
		default:	return COAP_UNDEFINED_CODE;
	}
	return COAP_UNDEFINED_CODE;
}

/// Set messageID to the supplied value.
/**
 * \param messageID A 16bit message id.
 * \return 0 on success, 1 on failure.
 */
int CoapPDU_setMessageID(struct CoapPDU *x, uint16_t messageID) {
	// message ID is stored in network byte order
	uint8_t *to = &x->_pdu[2];
	endian_store16(to, messageID);
	return 0;
}

/// Returns the 16 bit message ID of the PDU.
uint16_t CoapPDU_getMessageID(struct CoapPDU *x) {
	// mesasge ID is stored in network byteorder
	uint8_t *from = &x->_pdu[2];
	uint16_t messageID = endian_load16(from);
	return messageID;
}

/// Returns the length of the PDU.
int CoapPDU_getPDULength(struct CoapPDU *x) {
	return x->_pduLength;
}

/// Return the number of options that the PDU has.
int CoapPDU_getNumOptions(struct CoapPDU *x) {
	return x->_numOptions;
}


/**
 * This returns the options as a sequence of structs.
 */
CoapPDU_CoapOption* CoapPDU_getOptions(struct CoapPDU *x) {
	DBG("CoapPDU_getOptions() called, %d options.",x->_numOptions);

	uint16_t optionDelta =0, optionNumber = 0, optionValueLength = 0;
	int totalLength = 0;

	if(x->_numOptions==0) {
		return NULL;
	}

	// malloc space for options
	CoapPDU_CoapOption *options = (CoapPDU_CoapOption*)malloc(x->_numOptions*sizeof(CoapPDU_CoapOption));
	if(options==NULL) {
		DBG("Failed to allocate memory for options.");
		return NULL;
	}

	// first option occurs after token
	int optionPos = COAP_HDR_SIZE + CoapPDU_getTokenLength(x);

	// walk over options and record information
	for(int i=0; i<x->_numOptions; i++) {
		// extract option details
		optionDelta = CoapPDU_getOptionDelta(x, &x->_pdu[optionPos]);
		optionNumber += optionDelta;
		optionValueLength = CoapPDU_getOptionValueLength(x, &x->_pdu[optionPos]);
		// compute total length
		totalLength = 1; // mandatory header
		totalLength += computeExtraBytes(optionDelta);
		totalLength += computeExtraBytes(optionValueLength);
		totalLength += optionValueLength;
		// record option details
		options[i].optionNumber = optionNumber;
		options[i].optionDelta = optionDelta;
		options[i].optionValueLength = optionValueLength;
		options[i].totalLength = totalLength;
		options[i].optionPointer = &x->_pdu[optionPos];
		options[i].optionValuePointer = &x->_pdu[optionPos+totalLength-optionValueLength];
		// move to next option
		optionPos += totalLength;
	}

	return options;
}

/// Add an option to the PDU.
/**
 * Unlike other implementations, options can be added in any order, and in-memory manipulation will be
 * performed to ensure the correct ordering of options (they use a delta encoding of option numbers).
 * Re-ordering memory like this incurs a small performance cost, so if you care about this, then you
 * might want to add options in ascending order of option number.
 * \param optionNumber The number of the option, see the enum CoapPDU_Option for shorthand notations.
 * \param optionLength The length of the option payload in bytes.
 * \param optionValue A pointer to the byte sequence that is the option payload (bytes will be copied).
 * \return 0 on success, 1 on failure.
 */
int CoapPDU_addOption(struct CoapPDU *x, uint16_t insertedOptionNumber, uint16_t optionValueLength, uint8_t *optionValue) {
	// this inserts the option in memory, and re-computes the deltas accordingly
	// prevOption <-- insertionPosition
	// nextOption

	// find insertion location and previous option number
	uint16_t prevOptionNumber = 0; // option number of option before insertion point
	int insertionPosition = CoapPDU_findInsertionPosition(x, insertedOptionNumber,&prevOptionNumber);
	DBG("inserting option at position %d, after option with number: %hu",insertionPosition,prevOptionNumber);

	// compute option delta length
	uint16_t optionDelta = insertedOptionNumber-prevOptionNumber;
	uint8_t extraDeltaBytes = computeExtraBytes(optionDelta);

	// compute option length length
	uint16_t extraLengthBytes = computeExtraBytes(optionValueLength);

	// compute total length of option
	uint16_t optionLength = COAP_OPTION_HDR_BYTE + extraDeltaBytes + extraLengthBytes + optionValueLength;

	// if this is at the end of the PDU, job is done, just malloc and insert
	if(insertionPosition==x->_pduLength) {
		DBG("Inserting at end of PDU");
		// optionNumber must be biggest added
		x->_maxAddedOptionNumber = insertedOptionNumber;

		// set new PDU length and allocate space for extra option
		int oldPDULength = x->_pduLength;
		x->_pduLength += optionLength;
		if(!x->_constructedFromBuffer) {
			uint8_t *newMemory = (uint8_t*)realloc(x->_pdu,x->_pduLength);
			if(newMemory==NULL) {
				DBG("Failed to allocate memory for option.");
				x->_pduLength = oldPDULength;
				// malloc failed
				return 1;
			}
			x->_pdu = newMemory;
			x->_bufferLength = x->_pduLength;
		} else {
			// constructed from buffer, check space
			if(x->_pduLength>x->_bufferLength) {
				DBG("Buffer too small for new option: needed %d, got %d.",x->_pduLength-oldPDULength,x->_bufferLength-oldPDULength);
				x->_pduLength = oldPDULength;
				return 1;
			}
		}

		// insert option at position
		CoapPDU_insertOption(x, insertionPosition,optionDelta,optionValueLength,optionValue);
		x->_numOptions++;
		return 0;
	}
	// XXX could do 0xFF pdu payload case for changing of dynamically allocated application space SDUs < yeah, if you're insane

	// the next option might (probably) needs it's delta changing
	// I want to take this into account when allocating space for the new
	// option, to avoid having to do two mallocs, first CoapPDU_get info about this option
	int nextOptionDelta = CoapPDU_getOptionDelta(x, &x->_pdu[insertionPosition]);
	int nextOptionNumber = prevOptionNumber + nextOptionDelta;
	int nextOptionDeltaBytes = computeExtraBytes(nextOptionDelta);
	// recompute option delta, relative to inserted option
	int newNextOptionDelta = nextOptionNumber-insertedOptionNumber;
	int newNextOptionDeltaBytes = computeExtraBytes(newNextOptionDelta);
	// determine adjustment
	int optionDeltaAdjustment = newNextOptionDeltaBytes-nextOptionDeltaBytes;

	// create space for new option, including adjustment space for option delta
	DBG_PDU(x);
	DBG("Creating space");
	int mallocLength = optionLength+optionDeltaAdjustment;
	int oldPDULength = x->_pduLength;
	x->_pduLength += mallocLength;

	if(!x->_constructedFromBuffer) {
		uint8_t *newMemory = (uint8_t*)realloc(x->_pdu,x->_pduLength);
		if(newMemory==NULL) {
			DBG("Failed to allocate memory for option");
			x->_pduLength = oldPDULength;
			return 1;
		}
		x->_pdu = newMemory;
		x->_bufferLength = x->_pduLength;
	} else {
		// constructed from buffer, check space
		if(x->_pduLength>x->_bufferLength) {
			DBG("Buffer too small to contain option, needed %d, got %d.",x->_pduLength-oldPDULength,x->_bufferLength-oldPDULength);
			x->_pduLength = oldPDULength;
			return 1;
		}
	}

	// move remainder of PDU data up to create hole for new option
	DBG_PDU(x);
	DBG("Shifting PDU.");
	CoapPDU_shiftPDUUp(x, mallocLength,x->_pduLength-(insertionPosition+mallocLength));
	DBG_PDU(x);

	// adjust option delta bytes of following option
	// move the option header to the correct position
	int nextHeaderPos = insertionPosition+mallocLength;
	x->_pdu[nextHeaderPos-optionDeltaAdjustment] = x->_pdu[nextHeaderPos];
	nextHeaderPos -= optionDeltaAdjustment;
	// and set the new value
	CoapPDU_setOptionDelta(x, nextHeaderPos, newNextOptionDelta);

	// new option shorter
	// p p n n x x x x x
	// p p n n x x x x x -
	// p p - n n x x x x x
	// p p - - n x x x x x
	// p p o o n x x x x x

	// new option longer
	// p p n n x x x x x
	// p p n n x x x x x - - -
	// p p - - - n n x x x x x
	// p p - - n n n x x x x x
	// p p o o n n n x x x x x

	// note, it can only ever be shorter or the same since if an option was inserted the delta got smaller
	// but I'll leave that little comment in, just to show that it would work even if the delta got bigger

	// now insert the new option into the gap
	DBGLX("Inserting new option...");
	CoapPDU_insertOption(x, insertionPosition,optionDelta,optionValueLength,optionValue);
	DBGX("done\r\n");
	DBG_PDU(x);

	// done, mark it with B!
	return 0;
}

int CoapPDU_addPath(struct CoapPDU *x, const char *path)
{
	return CoapPDU_addOption(x, COAP_OPTION_URI_PATH, strlen(path), (uint8_t *) path);
}

/// Allocate space for a payload.
/**
 * For dynamically constructed PDUs, this will allocate space for a payload in the object
 * and return a pointer to it. If the PDU was constructed from a buffer, this doesn't
 * malloc anything, it just changes the x->_pduLength and returns the payload pointer.
 *
 * \note The pointer returned points into the PDU buffer.
 * \param len The length of the payload buffer to allocate.
 * \return Either a pointer to the payload buffer, or NULL if there wasn't enough space / allocation failed.
 */
uint8_t* CoapPDU_mallocPayload(struct CoapPDU *x, int len) {
	DBG("Entering mallocPayload");
	// sanity checks
	if(len==0) {
		DBG("Cannot allocate a zero length payload");
		return NULL;
	}

	// further sanity
	if(len==x->_payloadLength) {
		DBG("Space for payload of specified length already exists");
		if(x->_payloadPointer==NULL) {
			DBG("Garbage PDU. Payload length is %d, but existing x->_payloadPointer NULL",x->_payloadLength);
			return NULL;
		}
		return x->_payloadPointer;
	}

	DBG("x->_bufferLength: %d, x->_pduLength: %d, x->_payloadLength: %d",x->_bufferLength,x->_pduLength,x->_payloadLength);

	// might be making payload bigger (including bigger than 0) or smaller
	int markerSpace = 1;
	int payloadSpace = len;
	// is this a resizing?
	if(x->_payloadLength!=0) {
		// marker already exists
		markerSpace = 0;
		// compute new payload length (can be negative if shrinking payload)
		payloadSpace = len-x->_payloadLength;
	}

	// make space for payload (and payload marker if necessary)
	int newLen = x->_pduLength+payloadSpace+markerSpace;
	if(!x->_constructedFromBuffer) {
		uint8_t* newPDU = (uint8_t*)realloc(x->_pdu,newLen);
		if(newPDU==NULL) {
			DBG("Cannot allocate (or shrink) space for payload");
			return NULL;
		}
		x->_pdu = newPDU;
		x->_bufferLength = newLen;
	} else {
		// constructed from buffer, check space
		DBG("newLen: %d, x->_bufferLength: %d",newLen,x->_bufferLength);
		if(newLen>x->_bufferLength) {
			DBG("Buffer too small to contain desired payload, needed %d, got %d.",newLen-x->_pduLength,x->_bufferLength-x->_pduLength);
			return NULL;
		}
	}

	// deal with fresh allocation case separately
	if(x->_payloadPointer==NULL) {
		// set payload marker
		x->_pdu[x->_pduLength] = 0xFF;
		// payload at end of old PDU
		x->_payloadPointer = &x->_pdu[x->_pduLength+1];
		x->_pduLength = newLen;
		x->_payloadLength = len;
		return x->_payloadPointer;
	}

	// otherwise, just adjust length of PDU
	x->_pduLength = newLen;
	x->_payloadLength = len;
	DBG("Leaving mallocPayload");
	return x->_payloadPointer;
}

/// Set the payload to the byte sequence specified. Allocates memory in dynamic PDU if necessary.
/**
 * This will set the payload to \b payload. It will allocate memory in the case where the PDU was
 * constructed without an external buffer.
 *
 * This will fail either if the fixed buffer isn't big enough, or if memory could not be allocated
 * in the non-external-buffer case.
 *
 * \param payload Pointer to payload byte sequence.
 * \param len Length of payload byte sequence.
 * \return 0 on success, 1 on failure.
 */
int CoapPDU_setPayload(struct CoapPDU *x, const uint8_t *payload, int len) {
	if(payload==NULL) {
		DBG("NULL payload pointer.");
		return 1;
	}

	uint8_t *payloadPointer = CoapPDU_mallocPayload(x, len);
	if(payloadPointer==NULL) {
		DBG("Allocation of payload failed");
		return 1;
	}

	// copy payload contents
	memcpy(payloadPointer,payload,len);

	return 0;
}

/// Returns a pointer to the payload buffer.
uint8_t* CoapPDU_getPayloadPointer(struct CoapPDU *x) {
	return x->_payloadPointer;
}

/// Gets the length of the payload buffer.
int CoapPDU_getPayloadLength(struct CoapPDU *x) {
	return x->_payloadLength;
}

/// Returns a pointer to a buffer which is a copy of the payload buffer (dynamically allocated).
uint8_t* CoapPDU_getPayloadCopy(struct CoapPDU *x) {
	if(x->_payloadLength==0) {
		return NULL;
	}

	// malloc space for copy
	uint8_t *payload = (uint8_t*)malloc(x->_payloadLength);
	if(payload==NULL) {
		DBG("Unable to allocate memory for payload");
		return NULL;
	}

	// copy and return
	memcpy(payload,x->_payloadPointer,x->_payloadLength);
	return payload;
}

/// Shorthand for setting the content-format option.
/**
 * Sets the content-format to the specified value (adds an option).
 * \param format The content format, one of:
 *
 * - COAP_CONTENT_FORMAT_TEXT_PLAIN
 * - COAP_CONTENT_FORMAT_APP_LINK
 * - COAP_CONTENT_FORMAT_APP_XML
 * - COAP_CONTENT_FORMAT_APP_OCTET
 * - COAP_CONTENT_FORMAT_APP_EXI
 * - COAP_CONTENT_FORMAT_APP_JSON
 *
 * \return 0 on success, 1 on failure.
 */
int CoapPDU_setContentFormat(struct CoapPDU *x, CoapPDU_ContentFormat format) {
	if(format==0) {
		// minimal representation means null option value
		if(CoapPDU_addOption(x, COAP_OPTION_CONTENT_FORMAT,0,NULL)!=0) {
			DBG("Error setting content format");
			return 1;
		}
		return 0;
	}

	uint8_t c[2];

	// just use 1 byte if can do it
	if(format<256) {
		c[0] = format;
		if(CoapPDU_addOption(x, COAP_OPTION_CONTENT_FORMAT,1,c)!=0) {
			DBG("Error setting content format");
			return 1;
		}
		return 0;
	}

	uint8_t *to = c;
	endian_store16(to, format);
	if(CoapPDU_addOption(x, COAP_OPTION_CONTENT_FORMAT,2,c)!=0) {
		DBG("Error setting content format");
		return 1;
	}
	return 0;
}

/// PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE
/// PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE

/// Moves a block of bytes to end of PDU from given offset.
/**
 * This moves the block of bytes x->_pdu[x->_pduLength-1-shiftOffset-shiftAmount] ... x->_pdu[x->_pduLength-1-shiftOffset]
 * to the end of the PDU.
 * \param shiftOffset End of block to move, relative to end of PDU (-1).
 * \param shiftAmount Length of block to move.
 */
void CoapPDU_shiftPDUUp(struct CoapPDU *x, int shiftOffset, int shiftAmount) {
	DBG("shiftOffset: %d, shiftAmount: %d",shiftOffset,shiftAmount);
	int destPointer = x->_pduLength-1;
	int srcPointer  = destPointer-shiftOffset;
	while(shiftAmount--) {
		x->_pdu[destPointer] = x->_pdu[srcPointer];
		destPointer--;
		srcPointer--;
	}
}

/// Moves a block of bytes down a specified number of steps.
/**
 * Moves the block of bytes x->_pdu[startLocation+shiftOffset] ... x->_pdu[startLocation+shiftOffset+shiftAmount]
 * down to \b startLocation.
 * \param startLocation Index where to shift the block to.
 * \param shiftOffset Where the block starts, relative to start index.
 * \param shiftAmount Length of block to shift.
 */
void CoapPDU_shiftPDUDown(struct CoapPDU *x, int startLocation, int shiftOffset, int shiftAmount) {
	DBG("startLocation: %d, shiftOffset: %d, shiftAmount: %d",startLocation,shiftOffset,shiftAmount);
	int srcPointer = startLocation+shiftOffset;
	while(shiftAmount--) {
		x->_pdu[startLocation] = x->_pdu[srcPointer];
		startLocation++;
		srcPointer++;
	}
}

/// Gets the payload length of an option.
/**
 * \param option Pointer to location of option in PDU.
 * \return The 16 bit option-payload length.
 */
uint16_t CoapPDU_getOptionValueLength(struct CoapPDU *x, uint8_t *option) {
	uint16_t delta = (option[0] & 0xF0) >> 4;
	uint16_t length = (option[0] & 0x0F);
	// no extra bytes
	if(length<13) {
		return length;
	}

	// extra bytes skip header
	int offset = 1;
	// skip extra option delta bytes
	if(delta==13) {
		offset++;
	} else if(delta==14) {
		offset+=2;
	}

	// process length
	if(length==13) {
		return (option[offset]+13);
	} else {
		uint8_t *from = &option[offset];
		uint16_t value = endian_load16(from);
		return value+269;
	}

}

/// Gets the delta of an option.
/**
 * \param option Pointer to location of option in PDU.
 * \return The 16 bit delta.
 */
uint16_t CoapPDU_getOptionDelta(struct CoapPDU *x, uint8_t *option) {
	uint16_t delta = (option[0] & 0xF0) >> 4;
	if(delta<13) {
		return delta;
	} else if(delta==13) {
		// single byte option delta
		return (option[1]+13);
	} else if(delta==14) {
		uint8_t *from = &option[1];
		uint16_t value = endian_load16(from);
		return value+269;
	} else {
		// should only ever occur in payload marker
		return delta;
	}
}

/// Finds the insertion position in the current list of options for the specified option.
/**
 * \param optionNumber The option's number.
 * \param prevOptionNumber A pointer to a uint16_t which will store the option number of the option previous
 * to the insertion point.
 * \return 0 on success, 1 on failure. \b prevOptionNumber will contain the option number of the option
 * before the insertion position (for example 0 if no options have been inserted).
 */
int CoapPDU_findInsertionPosition(struct CoapPDU *x, uint16_t optionNumber, uint16_t *prevOptionNumber) {
	// zero this for safety
	*prevOptionNumber = 0x00;

	DBG("x->_pduLength: %d",x->_pduLength);

	// if option is bigger than any currently stored, it goes at the end
	// this includes the case that no option has yet been added
	if( (optionNumber >= x->_maxAddedOptionNumber) || (x->_pduLength == (COAP_HDR_SIZE+CoapPDU_getTokenLength(x))) ) {
		*prevOptionNumber = x->_maxAddedOptionNumber;
		return x->_pduLength;
	}

	// otherwise walk over the options
	int optionPos = COAP_HDR_SIZE + CoapPDU_getTokenLength(x);
	uint16_t optionDelta = 0, optionValueLength = 0;
	uint16_t currentOptionNumber = 0;
	while(optionPos<x->_pduLength && x->_pdu[optionPos]!=0xFF) {
		optionDelta = CoapPDU_getOptionDelta(x, &x->_pdu[optionPos]);
		currentOptionNumber += optionDelta;
		optionValueLength = CoapPDU_getOptionValueLength(x, &x->_pdu[optionPos]);
		// test if this is insertion position
		if(currentOptionNumber>optionNumber) {
			return optionPos;
		}
		// keep track of the last valid option number
		*prevOptionNumber = currentOptionNumber;
		// move onto next option
		optionPos += computeExtraBytes(optionDelta);
		optionPos += computeExtraBytes(optionValueLength);
		optionPos += optionValueLength;
		optionPos++; // (for mandatory option header byte)
	}
	return optionPos;

}

/// CoAP uses a minimal-byte representation for length fields. This returns the number of bytes needed to represent a given length.
int computeExtraBytes(uint16_t n) {
	if(n<13) {
		return 0;
	}

	if(n<269) {
		return 1;
	}

	return 2;
}

/// Set the option delta to the specified value.
/**
 * This assumes space has been made for the option delta.
 * \param optionPosition The index of the option in the PDU.
 * \param optionDelta The option delta value to set.
 */
void CoapPDU_setOptionDelta(struct CoapPDU *x, int optionPosition, uint16_t optionDelta) {
	int headerStart = optionPosition;
	// clear the old option delta bytes
	x->_pdu[headerStart] &= 0x0F;

	// set the option delta bytes
	if(optionDelta<13) {
		x->_pdu[headerStart] |= (optionDelta << 4);
	} else if(optionDelta<269) {
	   // 1 extra byte
		x->_pdu[headerStart] |= 0xD0; // 13 in first nibble
		x->_pdu[++optionPosition] &= 0x00;
		x->_pdu[optionPosition] |= (optionDelta-13);
	} else {
		// 2 extra bytes, network byte order uint16_t
		x->_pdu[headerStart] |= 0xE0; // 14 in first nibble
		optionDelta -= 269;
		uint8_t *to = &x->_pdu[optionPosition];
		optionPosition += 2;
		endian_store16(to, optionDelta);
	}
}

/// Insert an option in-memory at the specified location.
/**
 * This assumes that there is enough space at the location specified.
 * \param insertionPosition Position in the PDU where the option should be placed.
 * \param optionDelta The delta value for the option.
 * \param optionValueLength The length of the option value.
 * \param optionValue A pointer to the sequence of bytes representing the option value.
 * \return 0 on success, 1 on failure.
 */
int CoapPDU_insertOption(struct CoapPDU *x,
	int insertionPosition,
	uint16_t optionDelta,
	uint16_t optionValueLength,
	uint8_t *optionValue) {

	int headerStart = insertionPosition;

	// clear old option header start
	x->_pdu[headerStart] &= 0x00;

	// set the option delta bytes
	if(optionDelta<13) {
		x->_pdu[headerStart] |= (optionDelta << 4);
	} else if(optionDelta<269) {
	   // 1 extra byte
		x->_pdu[headerStart] |= 0xD0; // 13 in first nibble
		x->_pdu[++insertionPosition] &= 0x00;
		x->_pdu[insertionPosition] |= (optionDelta-13);
	} else {
		// 2 extra bytes, network byte order uint16_t
		x->_pdu[headerStart] |= 0xE0; // 14 in first nibble
		optionDelta -= 269;
		uint8_t *to = &x->_pdu[insertionPosition];
		insertionPosition += 2;
		endian_store16(to, optionDelta);
	}

	// set the option value length bytes
	if(optionValueLength<13) {
		x->_pdu[headerStart] |= (optionValueLength & 0x000F);
	} else if(optionValueLength<269) {
		x->_pdu[headerStart] |= 0x0D; // 13 in second nibble
		x->_pdu[++insertionPosition] &= 0x00;
		x->_pdu[insertionPosition] |= (optionValueLength-13);
	} else {
		x->_pdu[headerStart] |= 0x0E; // 14 in second nibble
		// this is in network byte order
		DBG("optionValueLength: %u",optionValueLength);
		uint8_t *to = &x->_pdu[insertionPosition];
		optionValueLength -= 269;
		endian_store16(to, optionValueLength);
		insertionPosition += 2;
	}

	// and finally copy the option value itself
	memcpy(&x->_pdu[++insertionPosition],optionValue,optionValueLength);

	return 0;
}

// DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
// DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG

/// Prints the PDU in human-readable format.
void CoapPDU_printHuman(struct CoapPDU *x) {
	INFO("x->__________________");
	if(x->_constructedFromBuffer) {
		INFO("PDU was constructed from buffer of %d bytes",x->_bufferLength);
	}
	INFO("PDU is %d bytes long",x->_pduLength);
	INFO("CoAP Version: %d",CoapPDU_getVersion(x));
	INFOX("Message Type: ");
	switch(CoapPDU_getType(x)) {
		case COAP_CONFIRMABLE:
			INFO("Confirmable");
			break;

		case COAP_NON_CONFIRMABLE:
			INFO("Non-Confirmable");
			break;

		case COAP_ACKNOWLEDGEMENT:
			INFO("Acknowledgement");
			break;

		case COAP_RESET:
			INFO("Reset");
			break;
	}
	INFO("Token length: %d",CoapPDU_getTokenLength(x));
	INFOX("Code: ");
	switch(CoapPDU_getCode(x)) {
		case COAP_EMPTY:
			INFO("0.00 Empty");
			break;
		case COAP_GET:
			INFO("0.01 GET");
			break;
		case COAP_POST:
			INFO("0.02 POST");
			break;
		case COAP_PUT:
			INFO("0.03 PUT");
			break;
		case COAP_DELETE:
			INFO("0.04 DELETE");
			break;
		case COAP_CREATED:
			INFO("2.01 Created");
			break;
		case COAP_DELETED:
			INFO("2.02 Deleted");
			break;
		case COAP_VALID:
			INFO("2.03 Valid");
			break;
		case COAP_CHANGED:
			INFO("2.04 Changed");
			break;
		case COAP_CONTENT:
			INFO("2.05 Content");
			break;
		case COAP_BAD_REQUEST:
			INFO("4.00 Bad Request");
			break;
		case COAP_UNAUTHORIZED:
			INFO("4.01 Unauthorized");
			break;
		case COAP_BAD_OPTION:
			INFO("4.02 Bad Option");
			break;
		case COAP_FORBIDDEN:
			INFO("4.03 Forbidden");
			break;
		case COAP_NOT_FOUND:
			INFO("4.04 Not Found");
			break;
		case COAP_METHOD_NOT_ALLOWED:
			INFO("4.05 Method Not Allowed");
			break;
		case COAP_NOT_ACCEPTABLE:
			INFO("4.06 Not Acceptable");
			break;
		case COAP_PRECONDITION_FAILED:
			INFO("4.12 Precondition Failed");
			break;
		case COAP_REQUEST_ENTITY_TOO_LARGE:
			INFO("4.13 Request Entity Too Large");
			break;
		case COAP_UNSUPPORTED_CONTENT_FORMAT:
			INFO("4.15 Unsupported Content-Format");
			break;
		case COAP_INTERNAL_SERVER_ERROR:
			INFO("5.00 Internal Server Error");
			break;
		case COAP_NOT_IMPLEMENTED:
			INFO("5.01 Not Implemented");
			break;
		case COAP_BAD_GATEWAY:
			INFO("5.02 Bad Gateway");
			break;
		case COAP_SERVICE_UNAVAILABLE:
			INFO("5.03 Service Unavailable");
			break;
		case COAP_GATEWAY_TIMEOUT:
			INFO("5.04 Gateway Timeout");
			break;
		case COAP_PROXYING_NOT_SUPPORTED:
			INFO("5.05 Proxying Not Supported");
			break;
		case COAP_UNDEFINED_CODE:
			INFO("Undefined Code");
			break;
	}

	// print message ID
	INFO("Message ID: %u",CoapPDU_getMessageID(x));

	// print token value
	int tokenLength = CoapPDU_getTokenLength(x);
	uint8_t *tokenPointer = CoapPDU_getPDUPointer(x)+COAP_HDR_SIZE;
	if(tokenLength==0) {
		INFO("No token.");
	} else {
		INFO("Token of %d bytes.",tokenLength);
		INFOX("   Value: 0x");
		for(int j=0; j<tokenLength; j++) {
			INFOX("%.2x",tokenPointer[j]);
		}
		INFO(" ");
	}

	// print options
	CoapPDU_CoapOption* options = CoapPDU_getOptions(x);
	if(options==NULL) {
		return;
	}

	INFO("%d options:",x->_numOptions);
	for(int i=0; i<x->_numOptions; i++) {
		INFO("OPTION (%d/%d)",i + 1,x->_numOptions);
		INFO("   Option number (delta): %hu (%hu)",options[i].optionNumber,options[i].optionDelta);
		INFOX("   Name: ");
		switch(options[i].optionNumber) {
			case COAP_OPTION_IF_MATCH:
				INFO("IF_MATCH");
				break;
			case COAP_OPTION_URI_HOST:
				INFO("URI_HOST");
				break;
			case COAP_OPTION_ETAG:
				INFO("ETAG");
				break;
			case COAP_OPTION_IF_NONE_MATCH:
				INFO("IF_NONE_MATCH");
				break;
			case COAP_OPTION_OBSERVE:
				INFO("OBSERVE");
				break;
			case COAP_OPTION_URI_PORT:
				INFO("URI_PORT");
				break;
			case COAP_OPTION_LOCATION_PATH:
				INFO("LOCATION_PATH");
				break;
			case COAP_OPTION_URI_PATH:
				INFO("URI_PATH");
				break;
			case COAP_OPTION_CONTENT_FORMAT:
				INFO("CONTENT_FORMAT");
				break;
			case COAP_OPTION_MAX_AGE:
				INFO("MAX_AGE");
				break;
			case COAP_OPTION_URI_QUERY:
				INFO("URI_QUERY");
				break;
			case COAP_OPTION_ACCEPT:
				INFO("ACCEPT");
				break;
			case COAP_OPTION_LOCATION_QUERY:
				INFO("LOCATION_QUERY");
				break;
			case COAP_OPTION_PROXY_URI:
				INFO("PROXY_URI");
				break;
			case COAP_OPTION_PROXY_SCHEME:
				INFO("PROXY_SCHEME");
				break;
			case COAP_OPTION_BLOCK1:
				INFO("BLOCK1");
				break;
			case COAP_OPTION_BLOCK2:
				INFO("BLOCK2");
				break;
			case COAP_OPTION_SIZE1:
				INFO("SIZE1");
				break;
			case COAP_OPTION_SIZE2:
				INFO("SIZE2");
				break;
			default:
				INFO("Unknown option");
				break;
		}
		INFO("   Value length: %u",options[i].optionValueLength);
		INFOX("   Value: \"");
		for(int j=0; j<options[i].optionValueLength; j++) {
			char c = options[i].optionValuePointer[j];
			if((c>='!'&&c<='~')||c==' ') {
				INFOX("%c",c);
			} else {
				INFOX("\\%.2d",c);
			}
		}
		INFO("\"");
	}

	// print payload
	if(x->_payloadLength==0) {
		INFO("No payload.");
	} else {
		INFO("Payload of %d bytes",x->_payloadLength);
		INFOX("   Value: \"");
		for(int j=0; j<x->_payloadLength; j++) {
			char c = x->_payloadPointer[j];
			if((c>='!'&&c<='~')||c==' ') {
				INFOX("%c",c);
			} else {
				INFOX("\\%.2x",c);
			}
		}
		INFO("\"");
	}
	free(options);
	INFO("x->__________________");
}

/// Prints the PDU as a c array (useful for debugging or hardcoding PDUs)
void CoapPDU_printPDUAsCArray(struct CoapPDU *x) {
	printf("const uint8_t array[] = {\r\n   ");
	for(int i=0; i<x->_pduLength; i++) {
		printf("0x%.2x, ",x->_pdu[i]);
	}
	printf("\r\n};\r\n");
}

/// A routine for printing an option in human-readable format.
/**
 * \param option This is a pointer to where the option begins in the PDU.
 */
void CoapPDU_printOptionHuman(struct CoapPDU *x, uint8_t *option) {
	// compute some useful stuff
	uint16_t optionDelta = CoapPDU_getOptionDelta(x, option);
	uint16_t optionValueLength = CoapPDU_getOptionValueLength(x, option);
	int extraDeltaBytes = computeExtraBytes(optionDelta);
	int extraValueLengthBytes = computeExtraBytes(optionValueLength);
	int totalLength = 1+extraDeltaBytes+extraValueLengthBytes+optionValueLength;

	if(totalLength>x->_pduLength) {
		totalLength = &x->_pdu[x->_pduLength-1]-option;
		DBG("New length: %u",totalLength);
	}

	// print summary
	DBG("~~~~~~ Option ~~~~~~");
	DBG("Delta: %u, Value length: %u",optionDelta,optionValueLength);

	// print all bytes
	DBG("All bytes (%d):",totalLength);
	for(int i=0; i<totalLength; i++) {
		if(i%4==0) {
			DBG(" ");
			DBGX("   %.2d ",i);
		}
		CoapPDU_printBinary(option[i]); DBGX(" ");
	}
	DBG(" "); DBG(" ");

	// print header byte
	DBG("Header byte:");
	DBGX("   ");
	CoapPDU_printBinary(*option++);
	DBG(" "); DBG(" ");

	// print extended delta bytes
	if(extraDeltaBytes) {
		DBG("Extended delta bytes (%d) in network order: ",extraDeltaBytes);
		DBGX("   ");
		while(extraDeltaBytes--) {
			CoapPDU_printBinary(*option++); DBGX(" ");
		}
	} else {
		DBG("No extended delta bytes");
	}
	DBG(" "); DBG(" ");

	// print extended value length bytes
	if(extraValueLengthBytes) {
		DBG("Extended value length bytes (%d) in network order: ",extraValueLengthBytes);
		DBGX("   ");
		while(extraValueLengthBytes--) {
			CoapPDU_printBinary(*option++); DBGX(" ");
		}
	} else {
		DBG("No extended value length bytes");
	}
	DBG(" ");

	// print option value
	DBG("Option value bytes:");
	for(int i=0; i<optionValueLength; i++) {
		if(i%4==0) {
			DBG(" ");
			DBGX("   %.2d ",i);
		}
		CoapPDU_printBinary(*option++);
		DBGX(" ");
	}
	DBG(" ");
}

/// Dumps the PDU header in hex.
void CoapPDU_printHex(struct CoapPDU *x) {
	printf("Hexdump dump of PDU\r\n");
	printf("%.2x %.2x %.2x %.2x",x->_pdu[0],x->_pdu[1],x->_pdu[2],x->_pdu[3]);
}

/// Dumps the entire PDU in binary.
void CoapPDU_printBin(struct CoapPDU *x) {
	for(int i=0; i<x->_pduLength; i++) {
		if(i%4==0) {
			printf("\r\n");
			printf("%.2d ",i);
		}
		CoapPDU_printBinary(x->_pdu[i]); printf(" ");
	}
	printf("\r\n");
}

/// Prints a single byte in binary.
void CoapPDU_printBinary(uint8_t b) {
	printf("%d%d%d%d%d%d%d%d",
		(b&0x80)&&0x01,
		(b&0x40)&&0x01,
		(b&0x20)&&0x01,
		(b&0x10)&&0x01,
		(b&0x08)&&0x01,
		(b&0x04)&&0x01,
		(b&0x02)&&0x01,
		(b&0x01)&&0x01);
}

/// Dumps the PDU as a byte sequence to stdout.
void CoapPDU_print(struct CoapPDU *x) {
	fwrite(x->_pdu,1,x->_pduLength,stdout);
}
