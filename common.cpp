/*
	flvtool++ 1.0
	This source is part of flvtool, a generic FLV file editor
	Copyright Andrew Brampton, Lancaster University
	
	This file is released free to use for academic and non-commercial purposes.
	If you wish to use this product for commercial reasons, then please contact us
*/

#include "common.h"

#include <assert.h>

#include <stdarg.h>
#include <errno.h>

vargs_exception::vargs_exception(const char * message, ...) : runtime_error("") {
	va_list args;

	va_start( args, message );
	vsprintf( buffer, message, args );
	va_end( args );
}

const char * vargs_exception::what () const throw () {
	return buffer;
}

unsigned int endian_swap32(unsigned int x) {
    return (x>>24) | 
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
}

unsigned short endian_swap16(unsigned short x) {
    return (x<<8) | (x>>8);
}

void endian_swap64(unsigned char b[8]) {
	register int i = 0;
	register int j = 7;

	while (i<j) {
		unsigned char t = b[i];
		b[i] = b[j];
		b[j] = t;
		i++, j--;
	}
}

// Reads N bits, from a certain offset
unsigned int read_N(unsigned char *data, unsigned int offset, unsigned int bits) {

	assert( bits > 0 );
	assert( ( (offset % 8) + bits ) <= 32 );

	// Move data along to start on the nearest 8 bit boundary
	data += offset / 8;
	offset -= (offset / 8) * 8;

	// Make sure the data is in Big Endian (TODO don't do this swap on big machines)
	unsigned int d = endian_swap32( *((unsigned int *)data) );

	// Create a mask that has bit 1s in the left hand column
	unsigned int mask = ~ ( (0x80000000 >> (bits-1)) - 1 );

	//1101 0100 1000 00010100000000000000
    //0000 0011 1000      mask 0x38
	//0000 0000 1000 0000 ret
	//0010 0000 0000 0000
	//0000 0000 0010 0000

	// Now mask off the correct bits, and move the result along
	unsigned int ret = endian_swap32( ( d & (mask >> offset) ) << offset );

	// Make sure we don't return a value too big
	assert( ret <= ((unsigned int)0x1 >> bits) );

	return ret;
}


unsigned char fread_8(FILE *fp) {
	unsigned char t;
	if (fread(&t, 1, 1, fp) < 1)
		throw std::runtime_error("could not read requested bytes");
	return t;
}

unsigned short fread_16(FILE *fp) {
	unsigned short temp;
	if (fread(&temp, 1, 2, fp) < 2)
        throw std::runtime_error("could not read requested bytes");

	return endian_swap16(temp);
}

unsigned int fread_24(FILE *fp) {
	unsigned int temp = 0;
	if (fread(((unsigned char *)&temp) + 1, 1, 3, fp) < 3)
		throw std::runtime_error("could not read requested bytes");
	return endian_swap32(temp);
}

unsigned int fread_32(FILE *fp) {
	unsigned int temp;
	if (fread(&temp, 1, 4, fp) < 4)
		throw std::runtime_error("could not read requested bytes");
	return endian_swap32(temp);
}

double fread_64(FILE *fp) {
	unsigned char b[8];

	if (fread(&b, 1, 8, fp) < 8)
		throw std::runtime_error("could not read requested bytes");
	endian_swap64(b);

	return *((double *)b);
}

void fread_s(FILE *fp, unsigned char *data, size_t len) {
	if (fread(data, 1, len, fp) < len)
		throw std::runtime_error("could not read requested bytes");
}

void fread_s(FILE *fp, char *data, size_t len) {
	if (fread(data, 1, len, fp) < len)
		throw std::runtime_error("could not read requested bytes");
}

void fwrite_8(FILE *fp, unsigned char t) {
	if (fwrite(&t, 1, 1, fp) < 1)
		throw std::runtime_error("could not write requested bytes");
}

void fwrite_16(FILE *fp, unsigned short i) {
	i = endian_swap16(i);

	if (fwrite(&i, 1, 2, fp) < 2)
        throw std::runtime_error("could not write requested bytes");
}

void fwrite_24(FILE *fp, unsigned int i) {
	i = endian_swap32(i);

	if (fwrite(((unsigned char *)&i) + 1, 1, 3, fp) < 3)
		throw std::runtime_error("could not write requested bytes");
}

void fwrite_32(FILE *fp, unsigned int i) {
	i = endian_swap32(i);

	if (fwrite(&i, 1, 4, fp) < 4)
		throw std::runtime_error("could not write requested bytes");
}

void fwrite_64(FILE *fp, double d) {
	endian_swap64((unsigned char *)&d);

	if (fwrite((unsigned char *)&d, 1, 8, fp) < 8)
		throw std::runtime_error("could not write requested bytes");
}

void fwrite_s(FILE *fp, const unsigned char *data, size_t len) {
	if (fwrite(data, 1, len, fp) < len)
		throw std::runtime_error("could not write requested bytes");
}

void fwrite_s(FILE *fp, const char *data, size_t len) {
	if (fwrite(data, 1, len, fp) < len)
		throw std::runtime_error("could not write requested bytes");
}

