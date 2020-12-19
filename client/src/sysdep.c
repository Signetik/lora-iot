
// Copyright 2016 Signetik, LLC -- All rights reserved.
// Signetik Confidential Proprietary Information -- Disclosure Prohibited.
// Author: Bruce Carlson
// www.signetik.com

// Platform-dependent layer supporting the cantcoap library

#include "sysdep.h"

void endian_store16(void * to, uint16_t value)
{
	uint8_t *p;
	p = to;
	p[0] = value & 0x00ff;		// todo: this might be wrong - maybe need to swap p[0] and p[1]
	p[1] = value >> 8;
}


uint16_t endian_load16(void * from)
{
	uint16_t x;
	uint8_t *p;
	p = from;
	x = ((uint16_t) p[1]) << 8 | p[0];		// todo: this might be wrong - maybe need to swap p[0] and p[1]
	return x;
}
