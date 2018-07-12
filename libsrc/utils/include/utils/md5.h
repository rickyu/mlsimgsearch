/* *  RFC 1321 compliant MD5 implementation
 *  Copyright (C) 2001-2003  Christophe Devine
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdint.h>

#ifndef _UTILS_MD5_H
#define _UTILS_MD5_H


typedef struct
{
    uint32_t total[2];
    uint32_t state[4];
    uint8_t buffer[64];
}md5_context;

#ifndef MD5SIZE
#define MD5SIZE 16
#endif
void md5_starts( md5_context *ctx );

void md5_update( md5_context *ctx, uint8_t *input, uint32_t length );

void md5_finish( md5_context *ctx, uint8_t digest[MD5SIZE] );
void MD5(uint8_t* source, uint32_t srclen, uint8_t digest[MD5SIZE]);
uint32_t MD5_N(const char* str, uint32_t len, uint8_t n);
void MD5_str(uint8_t* source, uint32_t srclen, char *const md5, const int md5_size);


#endif  // _UTILS_MD5_H_
 
