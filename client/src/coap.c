#include "coap.h"
#include "cantcoap.h"

//forward declarations
// static uint64_t generate_random_token(void);
// static uint16_t generate_message_id(void);

/** @brief Wrap payload into CoAP.
	@param buf - pointer to payload.
	@param len - Length of payload.
	@param coap_buf - pointer of pointer to caller allocated buffer to hold resulting CoAP
	@param coap_len - pointer to variable that holds length of coap_buf on input, and size of CoAP PDU on output
	@return none.
*/
void createCoapPDU(CoapPDU_ContentFormat format, const uint8_t *buf, size_t len, uint8_t **coap_buf, size_t *coap_len)
{
	struct CoapPDU pdu;
	//uint8_t *pcoap_buf = *coap_buf;

	CoapPDU_Init(&pdu, *coap_buf, *coap_len, 0);
	CoapPDU_setVersion(&pdu, 1);
	CoapPDU_setType(&pdu, COAP_NON_CONFIRMABLE);
	CoapPDU_setCode(&pdu, COAP_POST);
	CoapPDU_setToken(&pdu, (uint8_t*)"\3\2\1\1", 4);
	CoapPDU_setMessageID(&pdu, 0x0005);
	CoapPDU_setURI(&pdu, (char*)"/");
	CoapPDU_setContentFormat(&pdu, format);
	CoapPDU_setPayload(&pdu, buf, len);

	*coap_len = CoapPDU_getPDULength(&pdu);
}

/** @brief Extract payload from CoAP.
	@param buf - pointer to CoAP data.
	@param len - Length of CoAP data.
	@param payload - pointer of pointer to caller allocated buffer to hold extracted payload
	@param payload_len - length of payload
	@return none.
*/
void extractPayloadFromCoapPDU(uint8_t *buf, size_t len, uint8_t **payload, size_t *payload_len)
{
	struct CoapPDU pdu;
	uint8_t *ppayload = *payload;
	int status;

	CoapPDU_Init(&pdu, buf, len, len);
	CoapPDU_setContentFormat(&pdu, COAP_CONTENT_FORMAT_TEXT_PLAIN);
	CoapPDU_setPDULength(&pdu, len);
	status = CoapPDU_validate(&pdu);
	if(status == 0){
		*payload_len = 0;
		return;
	}
	memcpy(ppayload, CoapPDU_getPayloadPointer(&pdu), CoapPDU_getPayloadLength(&pdu));
	*payload_len = CoapPDU_getPayloadLength(&pdu);
}

#if 0
// Helpers
static uint64_t generate_random_token(void)
{
	static uint64_t x = 0;
	// todo: generate cryptographically random number
	// maybe we could use a timer as a cycle counter and do a hash of the timer value along with the clock and some ADC readings?
	// maybe also include the chip unique identifier in the hash?
	// The mbed TLS libary has a random number generator we might use.
	// See https://tools.ietf.org/html/rfc7252#section-5.3.1
	return x++;		// not so random yet.
}

static uint16_t generate_message_id(void)
{
	static uint16_t next_message_id = 0;
	// static uint16_t last_message_id;

	// todo: when we receive a message, set next_message_id = received_message_id + 1;
	// don't use the same message id twice in a row
	// do {
	// get new message ID
	// } while (new message ID == last_message_id);
	// last_message_id = new_message_id
	return next_message_id++;
}
#endif
