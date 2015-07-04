/*
	flvtool++ 1.0
	This source is part of flvtool, a generic FLV file editor
	Copyright Andrew Brampton, Lancaster University
	
	This file is released free to use for academic and non-commercial purposes.
	If you wish to use this product for commercial reasons, then please contact us
*/

#include <stdio.h>
#include <stdexcept>

#ifdef WIN32
	// Tweak some #defines to make windows support large files

	#ifndef _fseeki64
		extern "C" int __cdecl _fseeki64(FILE *, __int64, int);
	#endif

	#ifndef _ftelli64
		extern "C" __int64 __cdecl _ftelli64(FILE *);
	#endif

	#define fseeko _fseeki64
	#define ftello _ftelli64
	#define off_t __int64
#endif

class vargs_exception : public virtual std::runtime_error {

	private:
		char buffer[1024];

	public:
		vargs_exception(const char * message, ...);
		virtual const char * what () const throw ();
};

unsigned int endian_swap32(unsigned int x);
unsigned short endian_swap16(unsigned short x);
void endian_swap64(unsigned char b[8]);

// Reads N bits, from a certain offset
unsigned int read_N(unsigned char *data, unsigned int offset, unsigned int bits);
unsigned char fread_8(FILE *fp) ;
unsigned short fread_16(FILE *fp);
unsigned int fread_24(FILE *fp);
unsigned int fread_32(FILE *fp);
double fread_64(FILE *fp);

void fread_s(FILE *fp, unsigned char *data, size_t len);
void fread_s(FILE *fp, char *data, size_t len);
void fwrite_8(FILE *fp, unsigned char t);
void fwrite_16(FILE *fp, unsigned short i);
void fwrite_24(FILE *fp, unsigned int i);
void fwrite_32(FILE *fp, unsigned int i);
void fwrite_64(FILE *fp, double d);

void fwrite_s(FILE *fp, const unsigned char *data, size_t len);
void fwrite_s(FILE *fp, const char *data, size_t len);
