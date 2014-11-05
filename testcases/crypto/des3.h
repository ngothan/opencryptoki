#ifndef _DES3_H_
#define _DES3_H_
#endif

#include "pkcs11types.h"

#define MAX_KEY_SIZE  64
#define MAX_TEXT_SIZE 128
#define MAX_IV_SIZE  64
#define DES3_BLOCK_SIZE	8
#define DES3_KEY_SIZE 24
#define DES3_IV_SIZE 8
#define MAX_CHUNKS 8

struct des3_test_vector {
	char key[MAX_KEY_SIZE];
	unsigned char klen;
	char iv[MAX_IV_SIZE];
	unsigned char ivlen;
	char plaintext[MAX_TEXT_SIZE];
	unsigned char plen;
	char ciphertext[MAX_TEXT_SIZE];
	unsigned char clen;
	int chunks[MAX_CHUNKS];
	int num_chunks;
};

struct published_test_suite_info {
	const char *name;
	unsigned int tvcount;
	struct des3_test_vector *tv;
	unsigned int size;
	unsigned long mechanism;
};

struct generated_test_suite_info {
	const char *name;
	CK_MECHANISM mech;
};

struct CK_MECHANISM des3_keygen = {
	.mechanism = CKM_DES3_KEY_GEN,
	.ulParameterLen = 0,
	.pParameter = NULL,
};

char des3_cbc_iv[] = {0x67,0x9f,0xdb,0xee,0x16,0x6c,0x2e,0x0a};

/** http://csrc.nist.gov/groups/STM/cavp/documents/des/tdesmmt.zip
    TECBMMT1.rsp
**/
static struct des3_test_vector des3_ecb_tv[] = {

	{//count 0
	.key =	      { 0xc4,0x4a,0xef,0x54,0x5b,0x13,0x31,0xf2,
			0xc4,0x4a,0xef,0x54,0x5b,0x13,0x31,0xf2,
			0xc4,0x4a,0xef,0x54,0x5b,0x13,0x31,0xf2 },
	.klen = 24,
	.iv = {},
	.ivlen = 0,
	.plaintext =  { 0x6c,0x70,0xfa,0x4b,0x7a,0xba,0x00,0xca },
	.plen = 8,
	.ciphertext = { 0x94,0x13,0x63,0x8a,0x0b,0x54,0x01,0xbd },
	.clen = 8,
	}, {//count 1
	.key =	      { 0x2c,0xb5,0x64,0x2a,0x45,0xda,0xda,0x4c,
			0x2c,0xb5,0x64,0x2a,0x45,0xda,0xda,0x4c,
			0x2c,0xb5,0x64,0x2a,0x45,0xda,0xda,0x4c },
	.klen = 24,
	.iv = {},
	.ivlen = 0,
	.plaintext =  { 0x48,0x0d,0x56,0x62,0x7c,0xa3,0x3b,0xfa,
			0x1e,0xba,0x37,0x81,0x55,0xf4,0x35,0xc6 },
	.plen = 16,
	.ciphertext = { 0x45,0x97,0x96,0x7c,0x13,0x0f,0x10,0x0c,
			0xb4,0x4f,0x10,0x68,0x76,0x83,0xa8,0x08 },
	.clen = 16,
	}, {//count 2
	.key =	      { 0xf1,0x75,0xd5,0x2a,0x38,0x8f,0xf4,0xba,
			0xf1,0x75,0xd5,0x2a,0x38,0x8f,0xf4,0xba,
			0xf1,0x75,0xd5,0x2a,0x38,0x8f,0xf4,0xba },
	.klen = 24,
	.iv = {},
	.ivlen = 0,
	.plaintext =  { 0xff,0x2d,0xd1,0x4d,0xce,0xde,0xaa,0x43,
			0xb1,0x2a,0x26,0xee,0x8f,0x2d,0x82,0xea,
			0xa8,0xfa,0x15,0x84,0x26,0xdf,0x83,0x1c },
	.plen = 24,
	.ciphertext = { 0x56,0x53,0x5e,0x46,0x61,0xff,0x6b,0xf8,
			0x92,0xad,0x5f,0x35,0x93,0x17,0x9a,0x0f,
			0xc7,0xdd,0x64,0x75,0x67,0x93,0x0d,0xbc },
	.clen = 24,
	.num_chunks = 3 ,
	.chunks = 	{ 8, 8, 8 },
	}, {//count 3
	.key      =   { 0xb5,0x1a,0x4f,0xba,0xce,0x3d,0xe3,0x54,
			0xb5,0x1a,0x4f,0xba,0xce,0x3d,0xe3,0x54,
			0xb5,0x1a,0x4f,0xba,0xce,0x3d,0xe3,0x54 },
	.klen = 24,
	.iv = {},
	.ivlen = 0,
	.plaintext =  { 0x1d,0x1a,0x56,0x7f,0x1b,0xee,0xab,0x44,
			0xbb,0x7e,0xec,0xf0,0xae,0x08,0x6b,0x59,
			0xfd,0xbd,0xa9,0xd1,0x42,0x21,0x23,0xac,
			0x17,0xb4,0x6e,0xfc,0xcf,0xd2,0x86,0x9f },
	.plen = 32,
	.ciphertext = { 0xa6,0x86,0x11,0x33,0x71,0x8c,0x51,0x87,
			0xee,0xce,0xa8,0x2c,0x7e,0x26,0xbe,0x2e,
			0x0e,0x93,0x81,0x12,0xb1,0xda,0x96,0x51,
			0x1f,0x48,0xce,0xcd,0xb8,0x42,0xd6,0xba },
	.clen = 32,
	.num_chunks = 2,
	.chunks = 	{ 16, 16 },
	}, {//count 4
	.key =	      { 0xc1,0xcb,0xfd,0xe6,0x5b,0x52,0x98,0xae,
			0xc1,0xcb,0xfd,0xe6,0x5b,0x52,0x98,0xae,
			0xc1,0xcb,0xfd,0xe6,0x5b,0x52,0x98,0xae },
	.klen = 24,
	.iv = {},
	.ivlen = 0,
	.plaintext =  { 0xe4,0xf2,0xc8,0xb4,0x55,0x36,0x71,0x9b,
			0x42,0xca,0xe8,0xde,0xc1,0xb9,0x1b,0x84,
			0x4b,0xe7,0x6d,0x38,0x6f,0xf7,0x08,0x4c,
			0xc7,0x6b,0x70,0xff,0x2c,0x0d,0x5e,0xfb,
			0x26,0x8e,0xcb,0x22,0xa6,0x6e,0x4b,0x8a },
	.plen = 40,
	.ciphertext = { 0xfa,0x0c,0x0d,0x54,0xd3,0x34,0x0a,0xcb,
			0xeb,0xf9,0xed,0x6b,0x78,0xaa,0x4c,0x1a,
			0x91,0xab,0x7d,0x99,0x2e,0xa7,0xf6,0x59,
			0x76,0xba,0x56,0x78,0xe3,0x20,0x9f,0xc1,
			0x93,0x8e,0x12,0xa5,0x62,0x10,0xcf,0xff },
	.clen = 40,
	.num_chunks = 3,
	.chunks = 	{ 20, -1, 20 },
	}, {//count 5
	.key =	{ 0x9e,0xce,0x49,0x34,0x16,0x68,0xbf,0x3b,
			0x9e,0xce,0x49,0x34,0x16,0x68,0xbf,0x3b,
			0x9e,0xce,0x49,0x34,0x16,0x68,0xbf,0x3b },
	.klen = 24,
	.iv = {},
	.ivlen = 0,
	.plaintext =  { 0xb5,0xf0,0xde,0x24,0x52,0x38,0xe5,0xb2,
			0xff,0x69,0xe5,0x34,0x2c,0x04,0xed,0xd3,
			0x71,0x12,0x72,0x78,0x21,0x65,0x8c,0x31,
			0x22,0x20,0x63,0xd6,0xfc,0x7a,0xe3,0xec,
			0x2f,0xbe,0xb8,0xc9,0xca,0x96,0x76,0xf9,
			0x39,0x8a,0x17,0xe1,0x5f,0xc4,0xab,0xa2 },
	.plen = 48,
	.ciphertext = { 0x92,0x18,0xd8,0xde,0x25,0x3a,0x9a,0x7a,
			0x81,0xe6,0x63,0xd3,0x83,0x32,0xf0,0xc7,
			0x28,0x27,0xcd,0x97,0xa5,0x36,0x33,0x84,
			0x33,0xc1,0xb3,0x04,0x03,0x13,0x42,0x1c,
			0x3c,0xa3,0xf8,0xac,0x26,0x04,0x4f,0x4f,
			0xcb,0x2a,0xb3,0x23,0x4f,0x86,0x9b,0xaf },
	.clen = 48,
	.num_chunks = 4,
	.chunks =	{ 28, 13, 0, 7 },
	}


};

/** http://csrc.nist.gov/groups/STM/cavp/documents/des/tdesmmt.zip
    TCBCMMT1.rsp
**/
static struct des3_test_vector des3_cbc_tv[] = {

	{//count 0
	.key =	      { 0xa4,0xe3,0x19,0x51,0x0b,0xef,0x76,0xea,
			0xa4,0xe3,0x19,0x51,0x0b,0xef,0x76,0xea,
			0xa4,0xe3,0x19,0x51,0x0b,0xef,0x76,0xea },
	.klen = 24,
	.iv =	      { 0x67,0x9f,0xdb,0xee,0x16,0x6c,0x2e,0x0a },
	.ivlen =  DES3_IV_SIZE,
	.plaintext =  { 0x36,0x92,0x6e,0x3a,0x27,0x20,0xea,0x9f },
	.plen = 8,
	.ciphertext = { 0xc1,0x08,0xc0,0xe2,0x5e,0xe8,0x1e,0xa3 },
	.clen = 8,
	}, {//count 1
	.key =	      { 0x0e,0xe5,0xc8,0x97,0xb6,0xea,0x01,0x51,
			0x0e,0xe5,0xc8,0x97,0xb6,0xea,0x01,0x51,
			0x0e,0xe5,0xc8,0x97,0xb6,0xea,0x01,0x51 },
	.klen = 24,
	.iv =	      { 0x44,0xc0,0x61,0x73,0xcd,0xbf,0xd9,0xed },
	.ivlen = DES3_IV_SIZE,
	.plaintext =  { 0x71,0x12,0xbd,0xc4,0x89,0xda,0x7a,0x91,
			0x59,0x04,0x69,0xba,0x37,0xa5,0x1e,0x19 },
	.plen = 16,
	.ciphertext = { 0xe9,0x94,0xa7,0x00,0x16,0xfe,0x7b,0x49,
			0xfa,0x32,0x00,0xfd,0x0f,0x37,0x7a,0x55 },
	.clen = 16,
	}, {//count 2
	.key =	      { 0x98,0x9d,0xd9,0x34,0x1a,0xec,0x9e,0xfd,
			0x98,0x9d,0xd9,0x34,0x1a,0xec,0x9e,0xfd,
			0x98,0x9d,0xd9,0x34,0x1a,0xec,0x9e,0xfd },
	.klen = 24,
	.iv =	      { 0x1d,0xad,0x34,0x2c,0x0f,0xb3,0xc9,0xb1 },
	.ivlen = DES3_IV_SIZE,
	.plaintext =  { 0xaa,0xfe,0x78,0x54,0xd3,0x4e,0x67,0x30,
			0x89,0x95,0x99,0xc8,0x79,0xdc,0xc2,0x8e,
			0xa0,0x39,0x73,0x61,0xb2,0xa1,0x9d,0x01 },
	.plen = 24,
	.ciphertext = { 0x69,0xce,0xb8,0x56,0x7a,0xcb,0xac,0x1b,
			0x47,0xd9,0x18,0x02,0xa2,0x35,0xc4,0xe9,
			0xe4,0xd0,0xaf,0x89,0x4c,0x53,0x37,0x59 },
	.clen = 24,
	.num_chunks = 3,
	.chunks = 	{ 8, 8, 8 },
	}, {//count 3
	.key =	      { 0x92,0x6b,0x25,0x62,0xcb,0xe0,0xa4,0x7f,
			0x92,0x6b,0x25,0x62,0xcb,0xe0,0xa4,0x7f,
			0x92,0x6b,0x25,0x62,0xcb,0xe0,0xa4,0x7f },
	.klen = 24,
	.iv =	      { 0x81,0x17,0xfa,0x8f,0x84,0x39,0x4f,0x9d },
	.ivlen = DES3_IV_SIZE,
	.plaintext =  { 0xcd,0x6c,0xea,0x0e,0xbf,0x83,0x20,0x3c,
			0x00,0x9a,0x8d,0x19,0x36,0xcc,0xb4,0xb1,
			0x90,0xa3,0xc4,0x00,0x62,0x2e,0x62,0xac,
			0x0f,0x54,0xa8,0xf3,0x2c,0x67,0x6e,0xe1 },
	.plen = 32,
	.ciphertext = { 0x57,0x4e,0x2f,0x32,0x31,0xb9,0x91,0xec,
			0xfc,0x53,0xfe,0x84,0x6f,0xa2,0xa0,0xac,
			0x09,0x2a,0x52,0xe0,0x12,0x5b,0x2f,0xe5,
			0x2c,0xf7,0xfb,0x68,0xe3,0xfa,0x41,0xa9 },
	.clen = 32,
	.num_chunks = 2,
	.chunks = 	{ 16, 16 },
	}, {//count 4
	.key =	      { 0xb3,0xf4,0xcb,0x6d,0x51,0xb6,0xfe,0xa8,
			0xb3,0xf4,0xcb,0x6d,0x51,0xb6,0xfe,0xa8,
			0xb3,0xf4,0xcb,0x6d,0x51,0xb6,0xfe,0xa8 },
	.klen = 24,
	.iv =	      { 0x58,0x89,0x36,0xa3,0x6e,0x8b,0xa9,0x1c },
	.ivlen = DES3_IV_SIZE,
	.plaintext =  { 0xed,0x2e,0xbf,0xbd,0x9e,0x9d,0x2a,0x94,
			0x14,0x8a,0xb4,0x41,0x68,0xbb,0xdd,0xd8,
			0xb0,0xfd,0x0a,0xb7,0x23,0xba,0xce,0xb8,
			0x7e,0xce,0x67,0xab,0xa6,0x97,0x8c,0x03,
			0x8e,0x4b,0x66,0xcf,0x5b,0x8f,0xdc,0xec },
	.plen = 40,
	.ciphertext = { 0xb2,0xa0,0xba,0xb9,0x58,0xea,0x35,0x8c,
			0x02,0x44,0x4c,0xa1,0xb6,0x25,0xcb,0xcc,
			0x40,0x28,0x55,0xf0,0x68,0x6a,0xcf,0x3c,
			0xf5,0x0e,0x86,0x8e,0xfd,0x15,0xbc,0x3c,
			0xa1,0x2f,0x12,0xb1,0x4d,0x83,0x6b,0x31 },
	.clen = 40,
	.num_chunks = 3,
	.chunks = 	{ 20, -1, 20 },
	}, {//count 5
	.key =	      { 0xf1,0xc8,0xf4,0x4c,0xd9,0xe5,0x6b,0xc7 },
	.klen = 24,
	.iv =	      { 0x60,0x49,0x9d,0xe8,0xe6,0x3e,0x6e,0xa9 },
	.ivlen = DES3_IV_SIZE,
	.plaintext =  { 0xca,0xad,0xed,0x64,0x8d,0xd5,0x63,0x7e,
			0x0b,0x37,0xed,0x0d,0x18,0x22,0x30,0x6f,
			0xa8,0x90,0xd2,0x0a,0x9c,0xa9,0xe7,0xea,
			0xdb,0x89,0xac,0xab,0xa5,0x43,0x78,0x9a,
			0x24,0x26,0xef,0x17,0x44,0x3f,0x79,0x3e,
			0xc5,0x1e,0xf3,0xc6,0x8f,0xdd,0xa4,0x28 },
	.plen = 48,
	.ciphertext = { 0x38,0x8c,0x5f,0xb0,0x5d,0x2c,0xd3,0x78,
			0x2b,0xb2,0xbc,0xc7,0x07,0x8b,0x95,0x3d,
			0xb0,0x89,0xc9,0xa3,0x2a,0x52,0x3c,0xc1,
			0x5d,0xd5,0x36,0xb4,0x46,0xeb,0xf5,0xa3,
			0xd8,0xce,0xc1,0x6c,0x16,0xb2,0xad,0x72,
			0x46,0xd3,0xb3,0x61,0x1f,0xbe,0xec,0x57 },
	.clen = 48,
	.num_chunks = 4,
	.chunks = 	{ 28, 13, 0, 7 },
	}
};

/** http://csrc.nist.gov/groups/STM/cavp/documents/des/tdesmmt.zip
    TCFB8MMT1.rsp 
**/
static struct des3_test_vector des3_cfb8_tv[] = {

	{//count 0
		.key =	 	{ 0xe5,0xad,0x25,0xc8,0x64,0xe5,0x91,0x7a,
				  0xe5,0xad,0x25,0xc8,0x64,0xe5,0x91,0x7a,
				  0xe5,0xad,0x25,0xc8,0x64,0xe5,0x91,0x7a },
		.klen = 24,
		.iv =		{ 0x2d,0x9c,0x16,0x42,0x22,0xc9,0xb2,0x4c},
		.ivlen =  DES3_IV_SIZE,
		.plaintext =	{ 0xc5 },
		.plen = 1,
		.ciphertext =	{ 0x35 },
		.clen = 1,
	}, {//count 1
		.key = 		{ 0x6d,0x70,0xfb,0x4a,0x7a,0xba,0x01,0xcb,
				  0x6d,0x70,0xfb,0x4a,0x7a,0xba,0x01,0xcb,
				  0x6d,0x70,0xfb,0x4a,0x7a,0xba,0x01,0xcb },
		.klen = 24,
		.iv = 		{ 0x2c,0xb4,0x64,0x2a,0x45,0xda,0xdb,0x4c },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0x48,0x0d },
		.plen = 2,
		.ciphertext = 	{ 0xfa,0xad },
		.clen = 2,
	}, {//count 2
		.key =		{ 0xf1,0x75,0xd5,0x2a,0x38,0x8f,0xf4,0xba,
				  0xf1,0x75,0xd5,0x2a,0x38,0x8f,0xf4,0xba,
				  0xf1,0x75,0xd5,0x2a,0x38,0x8f,0xf4,0xba },
		.klen = 24, 
		.iv =		{ 0xff,0x2d,0xd1,0x4d,0xce,0xde,0xaa,0x43 },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0x8b,0xd7,0x1b },
		.plen = 3,
		.ciphertext = 	{ 0xdd,0x10,0x3f },
		.clen = 3,
	}, {//count 3
		.key = 		{ 0x91,0x70,0x0b,0xc4,0x67,0xc2,0xdc,0x3d,	
				  0x91,0x70,0x0b,0xc4,0x67,0xc2,0xdc,0x3d,
				  0x91,0x70,0x0b,0xc4,0x67,0xc2,0xdc,0x3d },
		.klen = 24,
		.iv =		{ 0xcb,0x0a,0x9b,0x71,0x02,0x03,0x13,0x1e },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0xec,0x41,0x27,0x37 },
		.plen = 4,
		.ciphertext = 	{ 0x73,0x56,0xbc,0x8d },
		.clen = 4,
	}, { //count 4
		.key = 		{ 0x94,0xef,0x4a,0xea,0x1f,0xae,0x32,0xf8, 
				  0x94,0xef,0x4a,0xea,0x1f,0xae,0x32,0xf8,
				  0x94,0xef,0x4a,0xea,0x1f,0xae,0x32,0xf8 },
		.klen = 24,
		.iv = 		{ 0x5e,0xa1,0xaf,0x34,0x0c,0x00,0x9f,0x84 },	
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0xe9,0x42,0x65,0x24,0x63 },
		.plen = 5,
		.ciphertext = 	{ 0x31,0x62,0x42,0xd4,0x4f },
		.clen = 5,
	}, {//count 5
		.key =		{ 0x6d,0x4a,0x32,0x31,0x83,0x52,0x83,0x3e, 
				  0x6d,0x4a,0x32,0x31,0x83,0x52,0x83,0x3e,
				  0x6d,0x4a,0x32,0x31,0x83,0x52,0x83,0x3e },
		.klen = 24,
		.iv = 		{ 0xf9,0xd6,0x94,0x4d,0x79,0x5d,0x2b,0x40 },
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0xbc,0xf1,0x09,0xcc,0xcd,0x9c },
		.plen = 6,
		.ciphertext =	{ 0x37,0x14,0xdd,0xf8,0xee,0x40 },
		.clen = 6,
	}, {//count6
		.key =		{ 0x08,0xfb,0xb3,0xcb,0xf4,0x94,0x02,0xba,
				  0x08,0xfb,0xb3,0xcb,0xf4,0x94,0x02,0xba,
				  0x08,0xfb,0xb3,0xcb,0xf4,0x94,0x02,0xba },
		.klen = 24,
		.iv =		{ 0xb5,0x6b,0x74,0xc9,0xaa,0xdb,0x7a,0xf8 },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0x20,0xfa,0x28,0x1a,0xbe,0x98,0x2b },
		.plen = 7,
		.ciphertext = 	{ 0xca,0xd6,0x8c,0x8d,0xd1,0xfa,0xc4 },
		.clen = 7,
	}, {//count 7
		.key = 		{ 0xda,0xea,0x08,0x37,0xc8,0xb0,0x49,0x07,
				  0xda,0xea,0x08,0x37,0xc8,0xb0,0x49,0x07,
				  0xda,0xea,0x08,0x37,0xc8,0xb0,0x49,0x07 },
		.klen = 24,
		.iv = 		{ 0xc1,0x34,0x24,0x25,0xa4,0xd7,0x9d,0xd9 },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0x58,0x88,0x3c,0x83,0xf6,0x7a,0x4a,0xf5 },
		.plen = 8,
		.ciphertext = 	{ 0xcd,0xe8,0x99,0xa6,0x49,0x7a,0x3d,0xf7 },
		.clen = 8,
	}, {//count 8
		.key = 		{ 0x31,0xd9,0xa8,0x8a,0xd0,0x6d,0xc2,0xf1,
				  0x31,0xd9,0xa8,0x8a,0xd0,0x6d,0xc2,0xf1,
				  0x31,0xd9,0xa8,0x8a,0xd0,0x6d,0xc2,0xf1 },
		.klen = 24,
		.iv = 		{ 0x0d,0x0b,0x8b,0xd0,0x25,0x64,0xdd,0x03 },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0x0a,0xde,0x78,0x94,0xf8,0x58,0x7f,0x64,
				  0x1a },
		.plen = 9,
		.ciphertext = 	{ 0xcb,0x13,0x47,0xba,0x2a,0xb6,0x13,0x8b,
				  0xfc },
		.clen = 9,
		.num_chunks = 3,
		.chunks = 	{ 4, -1, 5 },
	}, {//count 9
		.key = 		{ 0xcb,0xe6,0xcb,0x6b,0xce,0x51,0x73,0xfb, 
				  0xcb,0xe6,0xcb,0x6b,0xce,0x51,0x73,0xfb,
				  0xcb,0xe6,0xcb,0x6b,0xce,0x51,0x73,0xfb },
		.klen = 24,
		.iv = 		{ 0x17,0x9f,0x4f,0x59,0x1a,0xab,0x2d,0xf7 },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0x45,0x0c,0xa7,0xf4,0xe0,0x10,0x57,0xec,
				  0x20,0x1f },	
		.plen = 10,
		.ciphertext = 	{ 0x4c,0xd4,0x18,0x46,0xcf,0x2f,0xf0,0x94,
				  0x6b,0x88 },
		.clen = 10,
		.num_chunks = 3,
		.chunks = 	{ 5, 0, 5 },
	}
};


/** http://csrc.nist.gov/groups/STM/cavp/documents/des/tdesmmt.zip
    TCFB64MMT1.rsp 
**/
static struct des3_test_vector des3_cfb64_tv[] = {

	{	// #0
		.key =	 	{ 0x61,0x2a,0xba,0x31,0x8a,0xb9,0x29,0xdc,
				  0x61,0x2a,0xba,0x31,0x8a,0xb9,0x29,0xdc,
				  0x61,0x2a,0xba,0x31,0x8a,0xb9,0x29,0xdc },
		.klen = 24,
		.iv =		{ 0xe0,0xc6,0xb0,0x91,0xc6,0x8e,0x31,0xc3 }, 
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0x79,0xf2,0x0c,0xa1,0xe5,0x02,0x23,0xc4 },
		.plen = 8,
		.ciphertext =	{ 0xfe,0x64,0x15,0xe2,0xb6,0xa7,0x9b,0x96 },
		.clen = 8,
	}, {	// #1
		.key = 		{ 0x58,0xfd,0x38,0x79,0x8f,0xb9,0x23,0x94,
				  0x58,0xfd,0x38,0x79,0x8f,0xb9,0x23,0x94,
				  0x58,0xfd,0x38,0x79,0x8f,0xb9,0x23,0x94 },
		.klen = 24,
		.iv =		{ 0x9e,0x34,0xcf,0xd8,0x62,0x40,0xe5,0x27 },
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0xb1,0xe3,0xae,0x99,0xa6,0x04,0x8b,0x81,
				  0xb4,0xca,0xa7,0x22,0x2b,0x98,0xe7,0xa3 },
		.plen = 16,
		.ciphertext =	{ 0x5b,0xa5,0xcd,0x42,0x28,0x10,0x10,0xcf,
				  0xd4,0xfc,0x23,0x67,0xe6,0xc5,0x70,0x07 },
		.clen = 16,
	}, {	// #2
		.key =		{ 0xc1,0xe0,0x04,0x01,0x04,0x83,0x26,0xce,
				  0xc1,0xe0,0x04,0x01,0x04,0x83,0x26,0xce,
				  0xc1,0xe0,0x04,0x01,0x04,0x83,0x26,0xce },
		.klen = 24,
		.iv = 		{ 0xf9,0x67,0x7e,0xc0,0x57,0xad,0xcb,0xbf },
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0x5b,0xa3,0xed,0x1a,0x24,0x45,0x58,0x2c,
				  0x3a,0x5d,0x3e,0x34,0x33,0x80,0xed,0x19,
				  0xeb,0x66,0x50,0xec,0xbe,0x4c,0xc2,0x6d },
		.plen = 24,
		.ciphertext =	{ 0xf5,0x2f,0x4f,0x89,0x16,0x4b,0xde,0xed,
				  0x05,0x58,0x47,0xba,0x85,0x02,0xb8,0x69,
				  0x26,0x84,0xfc,0x52,0x65,0x09,0x22,0x1d },
		.clen = 24,
		.num_chunks = 3,
		.chunks = 	{ 12, 0, 12 },
	}, {	// #3
		.key = 		{ 0x7a,0xce,0x37,0xba,0x29,0x62,0xe3,0xc7,
				  0x7a,0xce,0x37,0xba,0x29,0x62,0xe3,0xc7,
				  0x7a,0xce,0x37,0xba,0x29,0x62,0xe3,0xc7 },
		.klen = 24,
		.iv = 		{ 0x15,0x9a,0xa7,0xdd,0x95,0x0f,0xad,0xe0 },
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0x64,0xa1,0x1f,0xb4,0xb5,0xa0,0x87,0x8b,
				  0x41,0x0f,0x49,0x5d,0xcc,0xa9,0x02,0xdc,
				  0x91,0xd5,0x4d,0xc9,0x9c,0x7b,0x44,0x68,
				  0x75,0x17,0x44,0x31,0xfe,0x0c,0xf1,0xc8 },
		.plen = 32,
		.ciphertext =	{ 0xdd,0xed,0x3b,0x58,0xd7,0x54,0x0b,0x12,
				  0xea,0xca,0x3a,0x2d,0xcd,0xf2,0xca,0x2b,
				  0x32,0x83,0xfa,0x14,0x7e,0x85,0x50,0x7a,
				  0xc8,0xdf,0xa3,0xf7,0xb2,0x61,0x41,0x0a },
		.clen = 32,
		.num_chunks = 4,
		.chunks = 	{ 8, 8, 8, 8 }
	}, {	// #4
		.key =		{ 0xfe,0xf8,0xd9,0xb5,0xdf,0x9e,0x54,0x45,
				  0xfe,0xf8,0xd9,0xb5,0xdf,0x9e,0x54,0x45,
				  0xfe,0xf8,0xd9,0xb5,0xdf,0x9e,0x54,0x45 },
		.klen = 24,
		.iv = 		{ 0x19,0xf5,0x1d,0xde,0xa0,0x39,0x04,0x52 },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0x91,0x51,0x22,0x2d,0x84,0x4d,0xf7,0x97,
				  0xd1,0x04,0x33,0xfe,0x87,0x60,0xea,0xba,
				  0xee,0x93,0x56,0xad,0xb3,0xaa,0x1c,0x77,
				  0x46,0x65,0xd0,0xe8,0x81,0x46,0xbc,0xbb,
				  0x8a,0xe8,0xdd,0x81,0xfa,0x92,0x2f,0xdb },
		.plen = 40,
		.ciphertext =	{ 0x55,0x98,0xed,0xa5,0x9f,0x67,0xd4,0xf4,
				  0xda,0xfd,0x24,0x48,0xfd,0x16,0x9b,0x8c,
				  0x34,0xa8,0x5a,0x7f,0xf4,0xcd,0x3d,0x74,
				  0x13,0x17,0x86,0xfb,0x84,0x9b,0x16,0x4e,
				  0x59,0x81,0xe8,0x71,0xbe,0x49,0x5a,0x6f },
		.clen = 40,
		.num_chunks = 3,
		.chunks = 	{ 20, -1, 20 },
	}, {	// #5
		.key = 		{ 0x86,0xd0,0xcd,0x04,0xc4,0x76,0xf7,0x5d,
				  0x86,0xd0,0xcd,0x04,0xc4,0x76,0xf7,0x5d,
				  0x86,0xd0,0xcd,0x04,0xc4,0x76,0xf7,0x5d },
		.klen = 24,
		.iv = 		{ 0x43,0xc8,0x41,0xef,0x07,0x04,0x30,0x73 },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0xb3,0x48,0xf0,0x70,0x78,0xfa,0x15,0x8e,
				  0xb4,0xc3,0x7e,0x47,0xee,0x20,0x84,0x1d,
				  0xf6,0xb2,0x16,0xd4,0xc8,0x91,0xb9,0x6e,
				  0xb3,0xe2,0xcc,0x5a,0xbb,0xac,0x11,0xe9,
				  0x3b,0xe7,0x69,0x01,0xcf,0xd6,0xdf,0x15,
				  0x70,0x97,0xe8,0x54,0xe1,0xf9,0xe9,0x21 },
		.plen = 48,
		.ciphertext = 	{ 0x9d,0x6e,0x21,0x39,0x52,0x91,0x65,0x67,
				  0x00,0xc8,0x6f,0x0b,0x0a,0x58,0xca,0xaf,
				  0x0f,0xc1,0xdc,0xc0,0x3f,0x02,0x55,0x2a,
				  0x8f,0x0f,0x90,0x48,0x2a,0x99,0x42,0x30,
				  0x15,0x68,0x05,0x38,0xf8,0xe7,0x7e,0x18,
				  0xba,0x75,0xc9,0xfd,0x0f,0xfb,0x91,0xd6 },
		.clen = 48,
	}, {	// #6
		.key = 		{ 0x62,0x6b,0x2c,0x01,0x29,0xfd,0x15,0xc4,
				  0x62,0x6b,0x2c,0x01,0x29,0xfd,0x15,0xc4,
				  0x62,0x6b,0x2c,0x01,0x29,0xfd,0x15,0xc4 },
		.klen = 24,
		.iv = 		{ 0x9e,0xc2,0xcc,0xfd,0x67,0x65,0x2a,0x4f },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0x69,0xd7,0xc0,0xdb,0xb8,0x85,0x20,0xbf,
				  0x43,0x84,0x88,0x91,0xe7,0x85,0xf8,0x26,
				  0xb8,0xa2,0x33,0x06,0xae,0x41,0x12,0x6f,
				  0x9b,0x3c,0x1d,0x9d,0x60,0x3f,0x5c,0x49,
				  0x61,0x71,0x6b,0x70,0x36,0x56,0xcd,0x55,
				  0xa4,0xf4,0x82,0xc9,0xcb,0x34,0xdc,0xaa,
				  0xa0,0x8c,0x8e,0xaa,0x9e,0xdb,0x06,0x51 },
		.plen = 56,
		.ciphertext =	{ 0xfb,0x94,0xac,0x30,0x7b,0xfb,0xef,0x4f,
				  0x27,0x87,0x97,0xe1,0x98,0x1b,0x38,0x1a,
				  0xbc,0x5d,0x33,0x7a,0xdc,0xf3,0xba,0x3b,
				  0x3b,0x5c,0x95,0xd8,0xfc,0x3a,0xb5,0x56,
				  0xdc,0xdd,0x88,0x93,0x35,0x5a,0x18,0x90,
				  0xb8,0x4b,0x6e,0x4c,0xaf,0x9f,0x8f,0x91,
				  0x6e,0x5b,0x2f,0x41,0x69,0x28,0x86,0x56 },
		.clen = 56,
		.num_chunks = 4,
		.chunks = 	{ 16, 16, 16, 8 },
	}, {	// #7
		.key = 		{ 0x38,0x40,0xfd,0x7f,0x49,0xb0,0x16,0x43,
				  0x38,0x40,0xfd,0x7f,0x49,0xb0,0x16,0x43,
				  0x38,0x40,0xfd,0x7f,0x49,0xb0,0x16,0x43 },
		.klen = 24,
		.iv = 		{ 0x31,0xf3,0x99,0x15,0xcb,0xd9,0x0d,0x8e },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0x9c,0x80,0x59,0x2c,0xc9,0x21,0x3f,0x66,
				  0x9d,0x66,0x6a,0x30,0x99,0x9c,0x8a,0xcf,
				  0x08,0xb4,0x57,0x64,0x8f,0x8c,0xcb,0x59,
				  0x04,0x71,0x8e,0xfe,0x83,0xe1,0xb2,0x74,
				  0xbf,0x95,0xa0,0xcb,0x97,0xbf,0x52,0x67,
				  0xea,0x90,0xc3,0xa8,0xab,0xfd,0xa4,0xca,
				  0x38,0xc3,0x51,0x09,0xe7,0xa8,0x03,0x09,
				  0xe2,0x04,0x47,0x76,0x36,0x81,0xaf,0xe7 },
		.plen = 64,
		.ciphertext =	{ 0xfb,0x39,0x5c,0x5e,0x3c,0xa8,0xb3,0xcb,
				  0x90,0x39,0x97,0x15,0x99,0xca,0x3d,0x24,
				  0xbd,0xa2,0x6e,0x93,0xe4,0xa9,0x01,0x1a,
				  0xf6,0x37,0x1a,0x76,0x87,0xde,0x0a,0x19,
				  0xfc,0xb7,0x5c,0xa6,0x00,0x70,0x43,0xf1,
				  0x31,0x43,0x33,0x87,0xe7,0xd3,0x94,0xd1,
				  0xed,0x25,0xd1,0xb2,0x25,0x31,0x9b,0x5e,
				  0x5a,0xe0,0x3d,0x73,0xb0,0xcc,0x22,0x6e },
		.clen = 64,
	}, {	// #8
		.key = 		{ 0x10,0x02,0x73,0x49,0x80,0x1a,0x2c,0x4f,
				  0x10,0x02,0x73,0x49,0x80,0x1a,0x2c,0x4f,
				  0x10,0x02,0x73,0x49,0x80,0x1a,0x2c,0x4f },
		.klen = 24,
		.iv = 		{ 0x35,0xdb,0x06,0x60,0x2e,0xe6,0x49,0xad },
		.ivlen = DES3_IV_SIZE,
		.plaintext = 	{ 0xb0,0x68,0x17,0xf4,0x4d,0x62,0xf5,0x3b,
				  0x18,0x64,0x06,0x02,0x18,0xa9,0xa9,0xfa,
				  0xc9,0x0b,0x67,0x5b,0x14,0x21,0x84,0x29,
				  0x7d,0xaa,0xd4,0xb6,0x9f,0x2e,0xd0,0x68,
				  0xd5,0xe2,0xe9,0xbc,0x3b,0xcd,0x1d,0x8b,
				  0xd5,0x29,0xc7,0x8c,0x0d,0x0c,0xbb,0x1b,
				  0x63,0x47,0x92,0x38,0x60,0x8e,0x43,0xd3,
				  0xcb,0xda,0xf4,0x47,0x20,0x0c,0x0f,0x24,
				  0x3e,0xd3,0x97,0x31,0x60,0xcf,0xbd,0xb1 },
		.plen = 72,
		.ciphertext =	{ 0x30,0x46,0x33,0x60,0xde,0x9a,0x82,0x3a,
				  0xeb,0xb1,0x77,0x1e,0x3d,0xbe,0x4f,0x0d,
				  0x5f,0xea,0x57,0x69,0xed,0x81,0xcb,0x5d,
				  0xd7,0x26,0xce,0xfa,0xd7,0xec,0x6b,0xf4,
				  0x3c,0x49,0xb2,0x4a,0x30,0x63,0xd7,0x01,
				  0x8d,0x2b,0x39,0x55,0xa8,0x66,0x9f,0x08,
				  0x6f,0xd6,0x92,0x7b,0x83,0x00,0x55,0xd2,
				  0x85,0x36,0x32,0xe7,0x24,0x40,0x26,0x46,
				  0xb8,0x6f,0x81,0x8d,0x80,0x12,0xe1,0x4d },
		.clen = 72,
		.num_chunks = 5,
		.chunks = 	{ 5, 20, -1, 17, 30 },
	}, {	// #9
		.key = 		{ 0x8c,0xa1,0xe5,0x80,0xa1,0xd6,0x29,0x45,
				  0x8c,0xa1,0xe5,0x80,0xa1,0xd6,0x29,0x45,
				  0x8c,0xa1,0xe5,0x80,0xa1,0xd6,0x29,0x45 },
		.klen = 24, 
		.iv = 		{ 0xce,0x26,0x45,0x22,0xce,0x32,0x3f,0x1c },
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0xda,0x5f,0x04,0x25,0x87,0x42,0xe0,0x47,
				  0x3f,0xff,0x34,0xe5,0xd3,0x36,0xf5,0xb2,
				  0x7d,0x49,0xcb,0x45,0xc4,0xb3,0x15,0x12,
				  0x9f,0x9b,0x2d,0x99,0xdd,0xa8,0xed,0xdd,
				  0xc4,0x18,0x72,0x18,0xf9,0x0c,0x1f,0xad,
				  0xa0,0x26,0xe5,0x5e,0xc3,0x56,0xc2,0xbf,
				  0xf8,0xf1,0x88,0xea,0x3e,0x04,0xe0,0x75,
				  0x29,0xe7,0x8e,0xa1,0x3f,0x15,0xf7,0xd4,
				  0xa1,0x3a,0xe0,0x4a,0xee,0x8e,0x78,0x07,
				  0x64,0x62,0x99,0x10,0x48,0xf8,0x4b,0xda },
		.plen = 80,
		.ciphertext =	{ 0x80,0xa6,0x23,0xeb,0x8b,0xee,0x1e,0xf8,
				  0x35,0x67,0x79,0x7d,0xfa,0x88,0x33,0xf7,
				  0x66,0x98,0x91,0x67,0x0c,0xe5,0x2a,0xb3,
				  0x5c,0x9b,0xb9,0xa6,0x74,0xd4,0x51,0x32,
				  0x80,0xf2,0x8c,0xaf,0xca,0xd4,0xcc,0x94,
				  0x83,0x1f,0x2e,0xa8,0x9e,0x47,0x3b,0xac,
				  0x43,0xb7,0x20,0x2d,0x65,0x3b,0xbe,0x0c,
				  0xc0,0xb8,0x03,0x4e,0xad,0x85,0xdd,0xe1,
				  0x96,0x6e,0x27,0x6a,0x3c,0x6c,0xbc,0xe9,
				  0x8f,0xef,0xaa,0x54,0xb6,0xad,0x59,0xfc },
		.clen = 80,
		.num_chunks = 2,
		.chunks =	{ 40, 40 },
	}
};


/** http://csrc.nist.gov/groups/STM/cavp/documents/des/tdesmmt.zip
    TCFB64MMT1.rsp 
**/
static struct des3_test_vector des3_ofb64_tv[] = {

	{ // #0
		.key =	 	{ 0x85,0x51,0x94,0xc4,0x70,0x26,0x83,0xda,
				  0x85,0x51,0x94,0xc4,0x70,0x26,0x83,0xda,
				  0x85,0x51,0x94,0xc4,0x70,0x26,0x83,0xda }, 
		.klen = 24,
		.iv =		{ 0xb2,0xfa,0xb7,0xb5,0x4f,0x34,0x19,0xa0 }, 
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0xa9,0x42,0x2a,0x0c,0x89,0xd8,0x0b,0xc1 },
		.plen = 8,
		.ciphertext =	{ 0x74,0x96,0x0e,0x14,0x75,0x48,0x02,0x28 },
		.clen = 8,
	}, { // #1
		.key = 		{ 0xa1,0x34,0x85,0x2c,0x61,0x70,0xab,0x6e,
				  0xa1,0x34,0x85,0x2c,0x61,0x70,0xab,0x6e,
				  0xa1,0x34,0x85,0x2c,0x61,0x70,0xab,0x6e }, 
		.klen = 24,
		.iv =		{ 0xd7,0xd2,0x51,0xb3,0x5e,0x9b,0x17,0x38 }, 
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0x2a,0x90,0xe0,0xd7,0xed,0xa6,0xb7,0x41,
				  0x1e,0x08,0xfb,0x80,0x86,0xa3,0xee,0x98 },
		.plen = 16,
		.ciphertext =	{ 0x81,0xa8,0x47,0xcd,0x75,0x0d,0xf0,0x29,
				  0x02,0xec,0x69,0x1c,0x06,0xaf,0x66,0x5d },
		.clen = 16,
	}, { // #2
		.key = 		{ 0x3e,0x15,0x0b,0x31,0x61,0xd9,0x85,0xb9,
				  0x3e,0x15,0x0b,0x31,0x61,0xd9,0x85,0xb9,
				  0x3e,0x15,0x0b,0x31,0x61,0xd9,0x85,0xb9 }, 
		.klen = 24,
		.iv =		{ 0xac,0xd3,0xd0,0xb2,0xed,0x46,0x11,0x5f }, 
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0xd3,0x99,0xb7,0x78,0x98,0x00,0xa2,0x9c,
				  0xbf,0x11,0x1b,0x6e,0xd1,0x84,0x0b,0x04,
				  0x1b,0xbe,0x53,0x8c,0xa7,0xf5,0x60,0x8d },
		.plen = 24,
		.ciphertext =	{ 0x3b,0xf3,0x38,0x55,0x8d,0x22,0xdf,0xf0,
				  0x25,0xf4,0x5a,0x5d,0x23,0x6f,0x00,0xde,
				  0xd9,0x05,0xdb,0xf1,0x5d,0xdb,0xe5,0x9e },
		.clen = 24,
		.num_chunks = 3,
		.chunks = 	{ 12, 0, 12 },
	}, { // #3
		.key = 		{ 0x01,0xab,0x54,0x46,0xd9,0x58,0xdc,0x3e,
				  0x01,0xab,0x54,0x46,0xd9,0x58,0xdc,0x3e,
				  0x01,0xab,0x54,0x46,0xd9,0x58,0xdc,0x3e }, 
		.klen = 24,
		.iv =		{ 0x28,0xba,0x95,0x3f,0xbe,0x39,0x90,0x24 }, 
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0x06,0x13,0xfa,0xcc,0xb6,0x09,0x4b,0x82,
				  0xd8,0x14,0x16,0x9e,0x4a,0xac,0xc4,0x79,
				  0x4f,0x19,0x49,0xc0,0x32,0x88,0xf1,0x25,
				  0x2d,0x5f,0x5d,0x54,0x88,0xce,0xc8,0x29 },
		.plen = 32,
		.ciphertext =	{ 0x59,0x96,0x76,0x6f,0x0d,0x67,0xc9,0xbe,
				  0x24,0x8b,0x93,0xc3,0x1f,0x6f,0x1d,0xfd,
				  0xb3,0x77,0xec,0x7d,0x76,0x7e,0x49,0x36,
				  0x57,0xd9,0x1a,0xfc,0x1a,0xf0,0xa5,0xd5 },
		.clen = 32,
		.num_chunks = 4,
		.chunks =	{ 8, 8, 8, 8 },
	}, { // #4
		.key = 		{ 0xc7,0xb6,0x0b,0xcd,0xb9,0x0d,0xb5,0x0d,
				  0xc7,0xb6,0x0b,0xcd,0xb9,0x0d,0xb5,0x0d,
				  0xc7,0xb6,0x0b,0xcd,0xb9,0x0d,0xb5,0x0d }, 
		.klen = 24,
		.iv =		{ 0x6b,0x85,0x55,0xff,0x62,0x04,0xec,0x3c }, 
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0xb9,0x03,0xb5,0x1f,0xce,0xad,0x3c,0x3c,
				  0xa3,0xed,0x80,0x9f,0x2e,0x6f,0xd3,0x60,
				  0xf7,0x2c,0x92,0x7d,0xd7,0x29,0x05,0x2a,
				  0xff,0x61,0x86,0x68,0x3d,0xff,0xe6,0xd9,
				  0x7b,0xd1,0x0c,0xd3,0x89,0xfa,0xdf,0xb3 },
		.plen = 40,
		.ciphertext =	{ 0x49,0x80,0x25,0xdc,0xc9,0xac,0x62,0xbb,
				  0xfd,0xe9,0x2a,0xf4,0xbc,0xc6,0x25,0x2f,
				  0xf5,0xe2,0x2f,0x35,0xaf,0xed,0x93,0x54,
				  0x1f,0xdd,0xaa,0x05,0xf8,0x7c,0xbf,0x3d,
				  0x7b,0x03,0x63,0xdf,0x92,0xbc,0xae,0x7e },
		.clen = 40,
		.num_chunks = 3,
		.chunks = { 20, -1, 20 },
	}, { // #5
		.key = 		{ 0x8c,0x7c,0x16,0x7c,0x6d,0x26,0x89,0x20,
				  0x8c,0x7c,0x16,0x7c,0x6d,0x26,0x89,0x20,
				  0x8c,0x7c,0x16,0x7c,0x6d,0x26,0x89,0x20 }, 
		.klen = 24,
		.iv =		{ 0xf9,0x84,0x6e,0x44,0x77,0x6c,0xd3,0xfd }, 
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0x55,0x0e,0xeb,0xab,0x18,0xb9,0x4f,0x90,
				  0x72,0xcd,0xe2,0xa2,0x18,0x82,0x13,0x62,
				  0x1b,0xf0,0xa4,0x5e,0x5f,0xbc,0xb8,0xf8,
				  0x4e,0xf7,0x2d,0x69,0xcd,0x43,0x2b,0x06,
				  0x6f,0x38,0xfd,0xc1,0xe9,0x06,0x20,0xf2,
				  0xc7,0x5b,0x9a,0x58,0x5d,0x87,0x8d,0x5a },
		.plen = 48,
		.ciphertext =	{ 0x4e,0xc3,0x27,0xf9,0x70,0xdf,0x43,0xc5,
				  0x12,0x6f,0x44,0x7b,0xba,0x8a,0xbf,0x93,
				  0x01,0x7c,0x8b,0xa9,0x5c,0x39,0xfe,0x52,
				  0x1e,0x95,0x67,0x11,0x00,0xd5,0xbe,0x4e,
				  0xb0,0xd7,0xe0,0x67,0x1b,0xf3,0x02,0x87,
				  0x9d,0x9e,0x85,0x70,0x3c,0xc7,0xa4,0x69 },
		.clen = 48,
	}, { // #6
		.key = 		{ 0xc7,0x57,0x70,0x70,0x6d,0xb5,0xa8,0x15,
				  0xc7,0x57,0x70,0x70,0x6d,0xb5,0xa8,0x15,
				  0xc7,0x57,0x70,0x70,0x6d,0xb5,0xa8,0x15 }, 
		.klen = 24,
		.iv =		{ 0xc0,0x4b,0x79,0xf3,0x2a,0x84,0xa8,0x12 }, 
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0xde,0x15,0x2e,0x19,0x68,0x36,0xc8,0x91,
				  0x6b,0x2f,0x18,0x31,0x17,0xbd,0xf4,0x45,
				  0x20,0x18,0xc9,0x59,0x6e,0xd3,0xea,0x41,
				  0x15,0x03,0x28,0x5c,0x87,0xa3,0xa3,0x19,
				  0x09,0x37,0xe0,0xb9,0x63,0xb0,0x1b,0x57,
				  0xdc,0x90,0xde,0x26,0x65,0x4e,0xd2,0xfb,
				  0x15,0xfc,0xd6,0x0b,0xd5,0x27,0x4e,0xf5 },
		.plen = 56,
		.ciphertext =	{ 0x19,0x45,0xee,0xf0,0x6e,0x48,0x12,0x8a,
				  0xa6,0xe8,0xec,0x43,0xc7,0x3e,0xdc,0x9f,
				  0xa5,0xfa,0xe4,0xfe,0x09,0xc4,0xbf,0x15,
				  0xf8,0x13,0xe9,0xbb,0xca,0xe4,0xf6,0x56,
				  0xba,0xf5,0xa6,0xf5,0x2e,0x92,0xb1,0x5a,
				  0x58,0xa2,0xf7,0xb2,0x94,0xff,0xe3,0xef,
				  0xae,0xe5,0x37,0x93,0x8d,0x9a,0x38,0x5f },
		.clen = 56,
		.num_chunks = 4,
		.chunks =	{ 16, 16, 16, 8 },
	}, { // #7
		.key = 		{ 0x79,0x02,0x75,0x26,0x6d,0x31,0x15,0x07,
				  0x79,0x02,0x75,0x26,0x6d,0x31,0x15,0x07,
				  0x79,0x02,0x75,0x26,0x6d,0x31,0x15,0x07 }, 
		.klen = 24,
		.iv =		{ 0x8b,0xd4,0x28,0xcd,0x6c,0x95,0xfa,0x5c }, 
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0xb3,0x1b,0x8d,0xa6,0xc0,0x2f,0xdc,0x5c,
				  0xf5,0xce,0x85,0x0e,0x19,0xf2,0xad,0x1a,
				  0xa6,0x7b,0xc0,0x0f,0x7c,0x1a,0xc1,0xb4,
				  0xd5,0x6f,0xde,0x4d,0x7c,0x6e,0x48,0x19,
				  0x3a,0x07,0x41,0x1f,0x91,0x06,0x77,0x2b,
				  0xad,0x38,0x90,0x13,0x98,0xcf,0x06,0x3e,
				  0xad,0x07,0x35,0x67,0x4f,0x0a,0xb2,0x9f,
				  0xdb,0x11,0xdf,0x91,0xda,0xcd,0xbc,0x0a },
		.plen = 64,
		.ciphertext =	{ 0xce,0xce,0xae,0xe6,0xd1,0x18,0xcb,0xa9,
				  0xfc,0x0c,0xe5,0xfa,0x4a,0x9e,0x2f,0x70,
				  0xb5,0x54,0x55,0xa9,0x6e,0x60,0xd2,0xa3,
				  0x97,0x7d,0x35,0x20,0x3a,0x79,0xdf,0xa4,
				  0x48,0x36,0x61,0xc5,0x73,0xea,0x01,0xf6,
				  0x3f,0x11,0xd5,0xa2,0x15,0xce,0xb0,0xa5,
				  0xf5,0x01,0x0a,0x09,0xd8,0x0c,0xe8,0x13,
				  0xcc,0xf0,0x1a,0x65,0x76,0x8b,0x25,0x6c },
		.clen = 64,
		.num_chunks = 4,
		.chunks = 	{ 32, 0, -1, 32 },
	}, { //  #8
		.key = 		{ 0xe6,0x62,0x8c,0xa8,0x38,0xcd,0x73,0xcb,
				  0xe6,0x62,0x8c,0xa8,0x38,0xcd,0x73,0xcb,
				  0xe6,0x62,0x8c,0xa8,0x38,0xcd,0x73,0xcb }, 
		.klen = 24,
		.iv =		{ 0x24,0x20,0x61,0xaf,0x85,0xf2,0x50,0xf1 }, 
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0x72,0x50,0x88,0x1a,0xa2,0x82,0xe5,0x64,
				  0xe4,0x9d,0x35,0xda,0x93,0x9d,0x4b,0x92,
				  0xbc,0x6f,0x35,0xf2,0x7e,0xbe,0xc1,0x21,
				  0xb7,0x0a,0x41,0xbb,0xdd,0xfa,0x8c,0xc9,
				  0x29,0xf3,0x45,0xf0,0xfb,0xe9,0x60,0xe8,
				  0x7a,0x75,0xe1,0x30,0x7a,0x28,0xe7,0x00,
				  0xa4,0x20,0xea,0x80,0xd0,0xd8,0x41,0x37,
				  0xb6,0x44,0xe7,0xc6,0xc6,0x1e,0x7e,0xd6,
				  0x54,0x7b,0x94,0x4f,0xf8,0xa1,0x26,0xca },
		.plen = 72,
		.ciphertext =	{ 0x9b,0x33,0x43,0x2d,0x70,0x70,0xc1,0xaa,
				  0xfd,0x44,0xb6,0x77,0xf4,0x7b,0xb1,0x0f,
				  0xef,0x5f,0xfc,0x0b,0x6a,0x68,0xc1,0x1f,
				  0x26,0x64,0x28,0x75,0x8c,0x62,0x6b,0x54,
				  0x65,0xb5,0xad,0x5a,0x35,0x25,0x52,0x06,
				  0x4a,0x4c,0x94,0xb7,0xb6,0x41,0xe1,0xf8,
				  0x8c,0xe9,0x3d,0xf0,0x45,0xbb,0x53,0xea,
				  0xa4,0x8b,0x79,0xb8,0x14,0x09,0xb9,0xcf,
				  0xd7,0xd9,0x38,0x2c,0xd4,0xd5,0x97,0x95 },
		.clen = 72,
		.num_chunks = 5,
		.chunks = 	{ 5, 20, -1, 17, 30 },
	}, { // #9
		.key = 		{ 0x4a,0x9b,0x08,0xd5,0xa4,0x4a,0x02,0x08,
				  0x4a,0x9b,0x08,0xd5,0xa4,0x4a,0x02,0x08,
				  0x4a,0x9b,0x08,0xd5,0xa4,0x4a,0x02,0x08 }, 
		.klen = 24,
		.iv =		{ 0xc0,0xd5,0x83,0xe4,0xf3,0xe1,0xb0,0xab }, 
		.ivlen = DES3_IV_SIZE,
		.plaintext =	{ 0xa5,0x8c,0x00,0x18,0xa7,0xe3,0x23,0x77,
				  0x73,0x07,0x65,0xea,0xb1,0xcd,0xd7,0x8e,
				  0x16,0xe4,0xf7,0x38,0x39,0x5d,0x27,0x13,
				  0xa2,0x6b,0x5e,0x8b,0x24,0x68,0x1e,0x6a,
				  0x6e,0x19,0xa6,0x41,0xdc,0x33,0x68,0x0a,
				  0x93,0x1e,0xbd,0x0b,0xcc,0xf8,0xd8,0xfe,
				  0x70,0xdd,0xf8,0x6e,0xe6,0x66,0xe2,0x64,
				  0x28,0x6f,0x8f,0x43,0x1d,0x1e,0x22,0xf1,
				  0x23,0x21,0x1d,0x37,0xea,0x36,0xd3,0x6b,
				  0x89,0xf1,0x21,0x4c,0xcb,0xba,0x59,0x2d },
		.plen = 80,
		.ciphertext =	{ 0x8e,0x63,0xec,0x2c,0x68,0x3b,0x09,0xa4,
				  0xab,0xb1,0xba,0xf0,0x8a,0xc0,0x3f,0x20,
				  0x88,0x53,0x26,0x13,0xa0,0x69,0x3e,0xb9,
				  0xef,0xf3,0xf6,0x72,0x9a,0xd2,0xb8,0xa5,
				  0x56,0xd0,0x83,0x0b,0xf8,0x58,0x8e,0x03,
				  0xd3,0xf0,0x30,0x7b,0x0a,0xe5,0x0c,0xd8,
				  0xf7,0xb9,0xa4,0xc7,0xb9,0x34,0x75,0xc2,
				  0xf5,0x41,0xb4,0x07,0x08,0x64,0x9b,0xcf,
				  0x20,0x57,0x55,0x95,0xa8,0x86,0x3e,0x59,
				  0x9f,0xc0,0x4d,0x14,0xb3,0xe5,0x36,0xf0 },
		.clen = 80,
		.num_chunks = 2,
		.chunks = 	{ 40, 40 },
	}
};
	
# define NUM_OF_PUBLISHED_TESTSUITES	5

struct published_test_suite_info published_test_suites[] = {
	{
		.name ="DES3_ECB",
		.tvcount = 6,
		.tv = des3_ecb_tv,
		.size = DES3_BLOCK_SIZE,
		.mechanism = CKM_DES3_ECB,
	}, {
		.name = "DES3_CBC",
		.tvcount = 6,
		.tv = des3_cbc_tv,
		.size = DES3_BLOCK_SIZE,
		.mechanism = CKM_DES3_CBC,
	}, {
		.name = "DES3_CFB8",
		.tvcount = 10,
		.tv = des3_cfb8_tv,
		.size = 1,
		.mechanism = CKM_DES_CFB8,
	}, {
		.name = "DES3_CFB64",
		.tvcount = 10,
		.tv = des3_cfb64_tv,
		.size = DES3_BLOCK_SIZE, 
		.mechanism = CKM_DES_CFB64,
	}, {
		.name = "DES3_OFB64",
		.tvcount = 10,
		.tv = des3_ofb64_tv,
		.size = DES3_BLOCK_SIZE, 
		.mechanism = CKM_DES_OFB64,
	}
};

#define NUM_OF_GENERATED_TESTSUITES 3

static struct generated_test_suite_info generated_test_suites[]  = {
    {
	.name = "DES3_ECB",
	.mech = {CKM_DES3_ECB, 0, 0},
    }, {
	.name = "DES3_CBC",
	.mech = {CKM_DES3_CBC, &des3_cbc_iv, DES3_IV_SIZE},
    }, {
	.name = "DES3_CBC_PAD",
	.mech = {CKM_DES3_CBC_PAD, &des3_cbc_iv, DES3_IV_SIZE},
    }
};
