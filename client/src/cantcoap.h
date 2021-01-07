#pragma once
//#pragma clang diagnostic ignored "-Wdeprecated-writable-strings"
//#pragma clang diagnostic ignored "-Wconstant-logical-operand"
//#pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"

/// Copyright (c) 2013, Ashley Mills.

// Adapted to C by Bruce Carlson, Signetik LLC.
// Copyright 2016 Signetik, LLC.
// All rights reserved.
// Signetik Confidential Proprietary Information -- Disclosure prohibited.
// www.signetik.com

//#include <unistd.h>
#include <stdint.h>
#include "dbg.h"

#define COAP_HDR_SIZE 4
#define COAP_OPTION_HDR_BYTE 1

// CoAP PDU format

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



/// CoAP message types. Note, values only work as enum.
typedef enum {
	COAP_CONFIRMABLE=0x00,
	COAP_NON_CONFIRMABLE=0x10,
	COAP_ACKNOWLEDGEMENT=0x20,
	COAP_RESET=0x30
} CoapPDU_Type;

// CoAP response codes.
typedef enum {
	COAP_EMPTY=0x00,
	COAP_GET,
	COAP_POST,
	COAP_PUT,
	COAP_DELETE,
	COAP_CREATED=0x41,
	COAP_DELETED,
	COAP_VALID,
	COAP_CHANGED,
	COAP_CONTENT,
	COAP_BAD_REQUEST=0x80,
	COAP_UNAUTHORIZED,
	COAP_BAD_OPTION,
	COAP_FORBIDDEN,
	COAP_NOT_FOUND,
	COAP_METHOD_NOT_ALLOWED,
	COAP_NOT_ACCEPTABLE,
	COAP_PRECONDITION_FAILED=0x8C,
	COAP_REQUEST_ENTITY_TOO_LARGE=0x8D,
	COAP_UNSUPPORTED_CONTENT_FORMAT=0x8F,
	COAP_INTERNAL_SERVER_ERROR=0xA0,
	COAP_NOT_IMPLEMENTED,
	COAP_BAD_GATEWAY,
	COAP_SERVICE_UNAVAILABLE,
	COAP_GATEWAY_TIMEOUT,
	COAP_PROXYING_NOT_SUPPORTED,
	COAP_UNDEFINED_CODE=0xFF
} CoapPDU_Code;

/// CoAP option numbers.
typedef enum {
	COAP_OPTION_IF_MATCH=1,
	COAP_OPTION_URI_HOST=3,
	COAP_OPTION_ETAG,
	COAP_OPTION_IF_NONE_MATCH,
	COAP_OPTION_OBSERVE,
	COAP_OPTION_URI_PORT,
	COAP_OPTION_LOCATION_PATH,
	COAP_OPTION_URI_PATH=11,
	COAP_OPTION_CONTENT_FORMAT,
	COAP_OPTION_MAX_AGE=14,
	COAP_OPTION_URI_QUERY,
	COAP_OPTION_ACCEPT=17,
	COAP_OPTION_LOCATION_QUERY=20,
	COAP_OPTION_BLOCK2=23,
	COAP_OPTION_BLOCK1=27,
	COAP_OPTION_SIZE2,
	COAP_OPTION_PROXY_URI=35,
	COAP_OPTION_PROXY_SCHEME=39,
	COAP_OPTION_SIZE1=60
} CoapPDU_Option;

/// CoAP content-formats.
typedef enum {
	COAP_CONTENT_FORMAT_TEXT_PLAIN = 0,
	COAP_CONTENT_FORMAT_APP_LINK  = 40,
	COAP_CONTENT_FORMAT_APP_XML,
	COAP_CONTENT_FORMAT_APP_OCTET,
	COAP_CONTENT_FORMAT_APP_EXI   = 47,
	COAP_CONTENT_FORMAT_APP_JSON  = 50,
	COAP_CONTENT_FORMAT_APP_CBOR  = 60,	// per http://www.iana.org/assignments/core-parameters/core-parameters.xhtml
	//61-95 are Unassigned by iana. so free for custom use
	COAP_CONTENT_FORMAT_CBOR_LORA = 61
} CoapPDU_ContentFormat;


/// Sequence of these is returned by CoapPDU::getOptions()
typedef struct {
	uint16_t optionDelta;
	uint16_t optionNumber;
	uint16_t optionValueLength;
	int totalLength;
	uint8_t *optionPointer;
	uint8_t *optionValuePointer;
} CoapPDU_CoapOption;


struct CoapPDU {

// 	// construction and destruction
// 	CoapPDU();
// 	CoapPDU(uint8_t *pdu, int pduLength);
// 	CoapPDU(uint8_t *buffer, int bufferLength, int pduLength);
// 	~CoapPDU();
// 	int reset();
// 	int validate();

// 	// version
// 	int setVersion(uint8_t version);
// 	uint8_t getVersion();

// 	// message type
// 	void setType(CoapPDU::Type type);
// 	CoapPDU::Type getType();

// 	// tokens
// 	int setTokenLength(uint8_t tokenLength);
// 	int getTokenLength();
// 	uint8_t* getTokenPointer();
// 	int setToken(uint8_t *token, uint8_t tokenLength);

// 	// message code
// 	void setCode(CoapPDU::Code code);
// 	CoapPDU::Code getCode();
// 	CoapPDU::Code httpStatusToCode(int httpStatus);

// 	// message ID
// 	int setMessageID(uint16_t messageID);
// 	uint16_t getMessageID();

// 	// options
// 	int addOption(uint16_t optionNumber, uint16_t optionLength, uint8_t *optionValue);
// 	// gets a list of all options
// 	CoapOption* getOptions();
// 	int getNumOptions();
// 	// shorthand helpers
// 	int setURI(char *uri);
// 	int setURI(char *uri, int urilen);
// 	int getURI(char *dst, int dstlen, int *outLen);
// 	int addURIQuery(char *query);

// 	// content format helper
// 	int setContentFormat(CoapPDU::ContentFormat format);
// 	// payload
// 	uint8_t* mallocPayload(int bytes);
// 	int setPayload(uint8_t *value, int len);
// 	uint8_t* getPayloadPointer();
// 	int getPayloadLength();
// 	uint8_t* getPayloadCopy();

// 	// pdu
// 	int getPDULength();
// 	uint8_t* getPDUPointer();
// 	void setPDULength(int len);

// 	// debugging
// 	static void printBinary(uint8_t b);
// 	void print();
// 	void printBin();
// 	void printHex();
// 	void printOptionHuman(uint8_t *option);
// 	void printHuman();
// 	void printPDUAsCArray();

// private:
	// variables
	uint8_t *_pdu;
	int _pduLength;

	int _constructedFromBuffer;
	int _bufferLength;

	uint8_t *_payloadPointer;
	int _payloadLength;

	int _numOptions;
	uint16_t _maxAddedOptionNumber;

	// // functions
	// void shiftPDUUp(int shiftOffset, int shiftAmount);
	// void shiftPDUDown(int startLocation, int shiftOffset, int shiftAmount);
	// uint8_t codeToValue(CoapPDU::Code c);

	// // option stuff
	// int findInsertionPosition(uint16_t optionNumber, uint16_t *prevOptionNumber);
	// int computeExtraBytes(uint16_t n);
	// int insertOption(int insertionPosition, uint16_t optionDelta, uint16_t optionValueLength, uint8_t *optionValue);
	// uint16_t getOptionDelta(uint8_t *option);
	// void setOptionDelta(int optionPosition, uint16_t optionDelta);
	// uint16_t getOptionValueLength(uint8_t *option);

};

/*
#define COAP_CODE_EMPTY 0x00

// method codes 0.01-0.31
#define COAP_CODE_GET 	0x01
#define COAP_CODE_POST 	0x02
#define COAP_CODE_PUT 	0x03
#define COAP_CODE_DELETE 0x04

// Response codes 2.00 - 5.31
// 2.00 - 2.05
#define COAP_CODE_CREATED 0x41
#define COAP_CODE_DELETED 0x42
#define COAP_CODE_VALID   0x43
#define COAP_CODE_CHANGED 0x44
#define COAP_CODE_CONTENT 0x45

// 4.00 - 4.15
#define COAP_CODE_BAD_REQUEST                0x80
#define COAP_CODE_UNAUTHORIZED               0x81
#define COAP_CODE_BAD_OPTION                 0x82
#define COAP_CODE_FORBIDDEN                  0x83
#define COAP_CODE_NOT_FOUND                  0x84
#define COAP_CODE_METHOD_NOT_ALLOWED         0x85
#define COAP_CODE_NOT_ACCEPTABLE             0x86
#define COAP_CODE_PRECONDITION_FAILED        0x8C
#define COAP_CODE_REQUEST_ENTITY_TOO_LARGE   0x8D
#define COAP_CODE_UNSUPPORTED_CONTENT_FORMAT 0x8F

// 5.00 - 5.05
#define COAP_CODE_INTERNAL_SERVER_ERROR      0xA0
#define COAP_CODE_NOT_IMPLEMENTED            0xA1
#define COAP_CODE_BAD_GATEWAY                0xA2
#define COAP_CODE_SERVICE_UNAVAILABLE        0xA3
#define COAP_CODE_GATEWAY_TIMEOUT            0xA4
#define COAP_CODE_PROXYING_NOT_SUPPORTED     0xA5
*/



void CoapPDU_Init(struct CoapPDU *x, uint8_t *buffer, int bufferLength, int pduLength);
int CoapPDU_reset(struct CoapPDU *x);
int CoapPDU_validate(struct CoapPDU *x);
void CoapPDU_Destroy(struct CoapPDU *x);
uint8_t* CoapPDU_getPDUPointer(struct CoapPDU *x);
void CoapPDU_setPDULength(struct CoapPDU *x, int len);
//int CoapPDU_setURI(struct CoapPDU *x, char *uri);
int CoapPDU_setURI(struct CoapPDU *x, char *uri);
int CoapPDU_addURIQuery(struct CoapPDU *x, char *query);
int CoapPDU_getURI(struct CoapPDU *x, char *dst, int dstlen, int *outLen);
int CoapPDU_setVersion(struct CoapPDU *x, uint8_t version);
uint8_t CoapPDU_getVersion(struct CoapPDU *x);
void CoapPDU_setType(struct CoapPDU *x, CoapPDU_Type mt);
CoapPDU_Type CoapPDU_getType(struct CoapPDU *x);
int CoapPDU_setTokenLength(struct CoapPDU *x, uint8_t tokenLength);
int CoapPDU_getTokenLength(struct CoapPDU *x);
uint8_t* CoapPDU_getTokenPointer(struct CoapPDU *x);
int CoapPDU_setToken(struct CoapPDU *x, uint8_t *token, uint8_t tokenLength);
void CoapPDU_setCode(struct CoapPDU *x, CoapPDU_Code code);
CoapPDU_Code CoapPDU_getCode(struct CoapPDU *x);
CoapPDU_Code CoapPDU_httpStatusToCode(struct CoapPDU *x, int httpStatus);
int CoapPDU_setMessageID(struct CoapPDU *x, uint16_t messageID);
uint16_t CoapPDU_getMessageID(struct CoapPDU *x);
int CoapPDU_getPDULength(struct CoapPDU *x);
int CoapPDU_getNumOptions(struct CoapPDU *x);
CoapPDU_CoapOption* CoapPDU_getOptions(struct CoapPDU *x);
int CoapPDU_addOption(struct CoapPDU *x, uint16_t insertedOptionNumber, uint16_t optionValueLength, uint8_t *optionValue);
int CoapPDU_addPath(struct CoapPDU *x, const char *path);
uint8_t* CoapPDU_mallocPayload(struct CoapPDU *x, int len);
int CoapPDU_setPayload(struct CoapPDU *x, const uint8_t *payload, int len);
uint8_t* CoapPDU_getPayloadPointer(struct CoapPDU *x);
int CoapPDU_getPayloadLength(struct CoapPDU *x);
uint8_t* CoapPDU_getPayloadCopy(struct CoapPDU *x);
int CoapPDU_setContentFormat(struct CoapPDU *x, CoapPDU_ContentFormat format);

/// PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE PRIVATE

void CoapPDU_shiftPDUUp(struct CoapPDU *x, int shiftOffset, int shiftAmount);
void CoapPDU_shiftPDUDown(struct CoapPDU *x, int startLocation, int shiftOffset, int shiftAmount);
uint16_t CoapPDU_getOptionValueLength(struct CoapPDU *x, uint8_t *option);
uint16_t CoapPDU_getOptionDelta(struct CoapPDU *x, uint8_t *option);
int CoapPDU_findInsertionPosition(struct CoapPDU *x, uint16_t optionNumber, uint16_t *prevOptionNumber);
int computeExtraBytes(uint16_t n);
void CoapPDU_setOptionDelta(struct CoapPDU *x, int optionPosition, uint16_t optionDelta);
int CoapPDU_insertOption(struct CoapPDU *x, int insertionPosition, uint16_t optionDelta, uint16_t optionValueLength, uint8_t *optionValue);

// DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG

void CoapPDU_printHuman(struct CoapPDU *x);
void CoapPDU_printPDUAsCArray(struct CoapPDU *x);
void CoapPDU_printOptionHuman(struct CoapPDU *x, uint8_t *option);
void CoapPDU_printHex(struct CoapPDU *x);
void CoapPDU_printBin(struct CoapPDU *x);
void CoapPDU_printBinary(uint8_t b);
void CoapPDU_print(struct CoapPDU *x);
