
// Copyright 2016 Signetik, LLC -- All rights reserved.
// Signetik Confidential Proprietary Information -- Disclosure Prohibited.
// Author: Bruce Carlson
// www.signetik.com



#ifndef SYSDEP_H
#define SYSDEP_H



#include <stdint.h>

void endian_store16(void * to, uint16_t value);
uint16_t endian_load16(void * from);










#endif

