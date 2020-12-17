#ifndef COAP_H
#define COAP_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "cantcoap.h"
void createCoapPDU(CoapPDU_ContentFormat format, const uint8_t *buf, size_t len, uint8_t **coap_buf, size_t *coap_len);
void extractPayloadFromCoapPDU(uint8_t *buf, size_t len, uint8_t **payload, size_t *payload_len);

#ifdef __cplusplus
}
#endif

#endif // COAP_H
