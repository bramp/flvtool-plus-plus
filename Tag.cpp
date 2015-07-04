/*
	flvtool++ 1.0
	This source is part of flvtool, a generic FLV file editor
	Copyright Andrew Brampton, Lancaster University
	
	This file is released free to use for academic and non-commercial purposes.
	If you wish to use this product for commercial reasons, then please contact us
*/

#include "Tag.h"

#include <iostream>
#include <string.h>
#include <assert.h>
#include <vector>


TagHeader::TagHeader(FILE *fp) { read(fp); };

TagHeader::TagHeader() : version(1), flags(0), offset(9) {};

Tag::Tag() : fp(NULL), filepos (~0), length(0), timestamp(0), reserved(0) {}
AudioTag::AudioTag(FILE *fp) : flags(0) { read(fp); };
VideoTag::VideoTag(FILE *fp) : frame_type((FrameType)Undefined), codec(Undefined) { read(fp); };
MetaTag::MetaTag(FILE *fp) : extralen (0) { read(fp); };
UndefinedTag::UndefinedTag(FILE *fp) { read(fp); };

void TagHeader::read(FILE *fp) {

	assert(fp != NULL);

	// Read Signature 'FLV'
	char sig[4];
	fread_s(fp, sig, 3);
	sig[3] = '\0';

	if (strncmp(sig, "FLV", 3) != 0 ) {
		throw std::runtime_error("input file does not start with FLV signature");
	}
	
	// Read version
	version = fread_8(fp);

	// Read flags
	flags = fread_8(fp);

	// Read file header size
	offset = fread_32(fp);
	
	if ( offset < 9 ) {
		throw std::runtime_error("tag header offset is too small");
	}

	// Read to the end of the header
	if (offset - 9 > 0) {
		data.reset( new unsigned char[ offset - 9 ] );
		fread_s(fp, data.get(), offset - 9);
	}

	// Now read a zero prev_length field
	unsigned int prev_length = fread_32(fp);

	if (prev_length != 0)
		throw std::runtime_error("invalid prev_length field");
}

void TagHeader::write(FILE *fp) const {

	assert(fp != NULL);

	fwrite_s(fp, "FLV", 3);
	
	fwrite_8(fp, version);
	fwrite_8(fp, flags);

	fwrite_32(fp, offset);

	if (offset - 9 > 0) {
		fwrite_s(fp, data.get(), offset - 9);
	}

	// Write the previous length
	fwrite_32(fp, 0);
}

size_t TagHeader::size() const {
	return offset + 4; // plus 4 bytes for the zero prev length
}

class Tag * fread_Tag(FILE *fp) {

	assert(fp != NULL);

	Tag *tag = NULL;
	Tag::Types type = Tag::Undefined;

	off_t tagStart = ftello(fp);

	if (tagStart == -1)
		throw vargs_exception( "%s:%d: ftello failed errno(%d)", __FILE__, __LINE__, errno );

	// Read the type and then construct a new tag
	try {
		type = (Tag::Types) fread_8(fp);
	} catch (...) {
		// If a exception is thrown we have got to the end of the file
		if (feof(fp))
			return NULL;

		throw;
	}

	// Seek back one byte
	if (fseeko(fp, -1, SEEK_CUR ))
		throw vargs_exception( "%s:%d: fseeko failed errno(%d)", __FILE__, __LINE__, errno );

	switch (type) {
		case Tag::Audio:
			tag = new AudioTag(fp);
			break;
		case Tag::Video: {
			tag = new VideoTag(fp);
			break;
		}
		case Tag::Meta: {
			tag = new MetaTag(fp);
			break;
		}
		case Tag::Undefined:
		default:
			tag = new UndefinedTag(fp);
			break;
	}

	// Some validation code
	if (tag->size() != tag->length + 15)
		throw vargs_exception( "tag's size does not match tag's length (%ld != %ld)", tag->size(), (tag->length + 15) );

	off_t pos = ftello(fp);
	if (pos == -1)
		throw vargs_exception( "%s:%d: ftello failed errno(%d)", __FILE__, __LINE__, errno );

	if (pos != tagStart + (off_t)tag->size())
		throw vargs_exception( "did not read full tag (%ld != %ld) errno(%d)", pos, (tagStart + (off_t)tag->size()), errno );

	return tag;
}

void Tag::read(FILE *fp) {

	assert(fp != NULL);

	// Find out where we are in the file
	this->fp = fp;
	filepos = ftello(fp);

	Tag::Types type = (Tag::Types)fread_8(fp);

	// Check this is the correct tag type (if not some code went wrong)
	assert (type == this->type());

	// Read the length (and 24bit endian swap it)
	length = fread_24(fp);

	// Read the timestamp (and 24bit endian swap it)
    timestamp = fread_24(fp);

	// Read 4 reserved bytes
	reserved = fread_32(fp);
}

void Tag::read_tail(FILE *fp) {

	assert(fp != NULL);

	// Now read the prev length of this tag
	unsigned int prev_length = fread_32(fp);

	if (prev_length != length + TAGHEADERLEN)
		throw std::runtime_error( "prev_length is wrong" );
}

void AudioTag::read(FILE *fp) {

	assert(fp != NULL);

	Tag::read(fp);

	if ( length > 0 ) {
		flags = fread_8(fp);

		//data.reset( new unsigned char[ length - 1 ] );
		//fread_s(fp, data.get(), length - 1);
		if (fseeko(fp, length - 1, SEEK_CUR ))
			throw vargs_exception( "%s:%d: fseeko failed errno(%d)", __FILE__, __LINE__, errno);
	}

	Tag::read_tail(fp);
}

void VideoTag::read(FILE *fp) {

	assert(fp != NULL);

	Tag::read(fp);

	if ( length > 0 ) {
		unsigned char tmp = fread_8(fp);

		frame_type = (FrameType)(tmp >> 4);
		codec = (Codec) ( tmp & 0x0f );

		// Now read the Video data
		//data.reset( new unsigned char[ length - 1 ] );
		//fread_s(fp, data.get(), length - 1);
		if (fseeko(fp, length - 1, SEEK_CUR ))
			throw vargs_exception( "%s:%d: fseeko failed errno(%d)", __FILE__, __LINE__, errno);

	}

	Tag::read_tail(fp);
}

void MetaTag::read(FILE *fp) {

	assert(fp != NULL);

	AMF *amf;
	Tag::read(fp);

	if ( length >= 2 ) {
		// read the event
		amf = fread_AMF(fp);
		if ( amf->type() != AMF_String ) {
			throw std::runtime_error( "invalid event AMF type" );
		}
		event.reset ( (AMFString *)amf );

		// Read the metadata
		amf = fread_AMF(fp);
		if ( amf->type() != AMF_Mixed_Array && amf->type() != AMF_Object ) {
			throw std::runtime_error( "invalid metadata AMF type" );
		}
		metadata.reset ( (AMFMap *)amf );

		// Check if there is any extra to read!
		size_t read = event->size() + metadata->size() + 2; // +2 bytes for AMF indentifiers
		if (read < length) {
			extralen = length - read; 

			//data.reset( new unsigned char[ extralen ] );
			//fread_s(fp, data.get(), extralen);
			if (fseeko(fp, extralen, SEEK_CUR ))
				throw vargs_exception( "%s:%d: fseeko failed errno(%d)", __FILE__, __LINE__, errno);

		} else {
			extralen = 0;
		}

	}

	Tag::read_tail(fp);
}

MetaTag::MetaTag(const char *name) : event(new AMFString(name)), metadata(new AMFMixed_Array()), extralen(0) {}


void UndefinedTag::read(FILE *fp) {

	assert(fp != NULL);

	Tag::read(fp);

	if ( length > 0 ) {
		//data.reset( new unsigned char[ length ] );
		//fread_s(fp, data.get(), length);
		if (fseeko(fp, length, SEEK_CUR ))
			throw vargs_exception( "%s:%d: fseeko failed errno(%d)", __FILE__, __LINE__, errno);
	}

	Tag::read_tail(fp);
}

size_t Tag::read_data(off_t offset, unsigned char *buf, size_t len) const {
	
	assert ( fp != NULL );
	assert ( filepos != ~0 );
	
	// Seek to the data section
	if (fseeko(fp, filepos + TAGHEADERLEN + offset, SEEK_SET))
		throw vargs_exception( "%s:%d: fseeko failed errno(%d)", __FILE__, __LINE__, errno);

	// Now read the data section
	fread_s(fp, buf, len);

	return len;
}

void Tag::write(FILE *fp) const {

	assert(fp != NULL);

	fwrite_8(fp, (unsigned char)type() );

	// Write the length
	fwrite_24(fp, (unsigned int)( size() - Tag::size() ) );

	// Write the timestamp
	fwrite_24(fp, timestamp );
	
	// Read 4 unknown bytes
	fwrite_32(fp, reserved);
}

void Tag::write_tail(FILE *fp) const {

	assert(fp != NULL);

	unsigned int prev_length = length + TAGHEADERLEN;

	// Now write the prev length of this tag
	fwrite_32(fp, prev_length);
}

void AudioTag::write(FILE *fp) const {

	assert(fp != NULL);

	Tag::write(fp);

	if ( length > 0 ) {
		fwrite_8(fp, flags);

		if ( length > 1 ) {

			// Because the data isn't stored, we need to read it (from disk) 
			std::vector<unsigned char> data ( length - 1 );
			read_data(1, &data[0], length - 1);

			fwrite_s(fp, &data[0], length - 1);
		}
	}

	Tag::write_tail(fp);
}

void VideoTag::write(FILE *fp) const {

	assert(fp != NULL);

	Tag::write(fp);

	unsigned char temp = (frame_type << 4 & 0xf0) | codec & 0x0f;

	if ( length > 0 ) {
		fwrite_8(fp, temp);

		if ( length > 1 ) {

			// Because the data isn't stored, we need to read it (from disk) 
			std::vector<unsigned char> data ( length - 1 );
			read_data(1, &data[0], length - 1);

			fwrite_s(fp, &data[0], length - 1);
		}
	}

	Tag::write_tail(fp);
}

void MetaTag::write(FILE *fp) const {

	assert(fp != NULL);

	Tag::write(fp);

	if ( length >= 2 ) {
		// Now the metadata struct
		fwrite_AMF(fp, event.get());
		fwrite_AMF(fp, metadata.get());
	}

	// Check if there was some random extra data, and write it
	if (extralen > 0) {
		// Because the data isn't stored, we need to read it (from disk) 
		std::vector<unsigned char> data ( extralen );
		read_data(event->size() + metadata->size() + 2, &data[0], extralen);

		fwrite_s(fp, &data[0], extralen);
	}

	Tag::write_tail(fp);
}

void UndefinedTag::write(FILE *fp) const {

	assert(fp != NULL);

	Tag::write(fp);

	if (length > 0) {
		// Because the data isn't stored, we need to read it (from disk) 
		std::vector<unsigned char> data ( length );
		read_data(0, &data[0], length );

		fwrite_s(fp, &data[0], length);
	}

	Tag::write_tail(fp);
}

size_t Tag::size() const {
	return 15;
}

size_t AudioTag::size() const {
	return Tag::size() + length;
}

size_t VideoTag::size() const {
	return Tag::size() + length;
}

size_t MetaTag::size() const {
	return Tag::size() + length;
}

void MetaTag::recalc_length() {
	// Recalc the size (incase the AMF structs have changed
	length = (unsigned int)extralen;

	if (event.get()) 
		length += (unsigned int)(event->size() + 1);

	if (metadata.get()) 
		length += (unsigned int)(metadata->size() + 1);

}

size_t UndefinedTag::size() const {
	return Tag::size() + length;
}

void MetaTag::remove(const char *key) {
	assert(key != NULL);

	if ( metadata->remove( key ) )
		recalc_length();
}

void MetaTag::set(const char *key, AMF *data) {

	assert(key != NULL);
	assert(data != NULL);

	metadata->set( key, data );
	recalc_length();
}

AMF *MetaTag::get(const char *key) const {

	assert(key != NULL);

	return metadata->get(key);
}

AudioTag::Codec AudioTag::getCodec() const {
	return (Codec)((flags & 0xF0) >> 4);
}

unsigned int AudioTag::getChannels() const { 
	return (flags & 0x01) + 1;
}

unsigned int AudioTag::getSampleRate() const { 
	unsigned int rate = (flags & 0x0C) >> 2;
	switch ( rate ) {
		case 0: rate = 5500; break;
		case 1: rate = 11000; break;
		case 2: rate = 22000; break;
		case 3: rate = 44000; break;
	}
	return rate;
}

void VideoTag::readDimensions(unsigned int &width, unsigned int &height) const {
	width = 0;
	height = 0;

	// Because the data isn't stored, we need to read it (from disk) 
	std::vector<unsigned char> data ( length - 1 );
	read_data(1, &data[0], length - 1);

	switch ( getCodec() ) {
		case VideoTag::SorensonH263: {

			// |pictureStartCode|version|temporalReference|pictureSize|
			// |    17 bits     | 5 bits|     8 bits      | 3 bits |

			unsigned int pictureSize = read_N( &data[0], 30, 3 );

			switch ( pictureSize ) {
				case 0: 
					width = read_N( &data[0], 33, 8 ); 
					height= read_N( &data[0], 41, 8 );
					break;
				case 1:
					width = read_N( &data[0], 33, 16 );
					height= read_N( &data[0], 49, 16 );
					break;
				case 2: width=352; height=288; break; // CIF
				case 3: width=176; height=144; break; // QCIF
				case 4: width=128; height=96; break; // SQCIF
				case 5: width=320; height=240; break;
				case 6: width=160; height=120; break;
			}

			break;
		}

		case VideoTag::On2VP6 :
		case VideoTag::On2VP6F :
			// TODO finish this
			break;

		case VideoTag::ScreenVideo :
		case VideoTag::ScreenVideo2 :
			// TODO finish this
			break;
	}
}

unsigned int VideoTag::getWidth() const {
	unsigned int width, height;

	readDimensions(width, height);

	return width;
}

unsigned int VideoTag::getHeight() const {
	unsigned int width, height;

	readDimensions(width, height);

	return height;
}

std::ostream& operator << (std::ostream& os, const TagHeader *tag) { return tag->operator <<(os); }
std::ostream& operator << (std::ostream& os, const Tag *tag) { return tag->operator <<(os); }

std::ostream& TagHeader::operator << (std::ostream& os) const {
	os << "Header v:" << (int)version << " flags: " << (int)flags;
	
	if ( hasVideo() || hasAudio() ) {
		if ( hasVideo() && hasAudio() )
			os << " (Video+Audio)";
		else if ( hasVideo() )
			os << " (Video)";
		else
			os << " (Audio)";
	}
	
	os << " offset:" << offset;

	return os;
}

std::ostream& AudioTag::operator << (std::ostream& os) const {
	const char *type[] = {"?", "Mono", "Stereo"};
	const char *size[] = {"8", "16"};
	const char *rate[] = {"5.5", "11", "22", "44"};
	const char *codec[] = {"Uncompressed", "ADPCM", "MP3", "?", "?", "Nellymoser", "Nellymoser", "?", "?", "?", "?", "?", "?", "?", "?", "?"};

	os << "AudioTag time:" << timestamp << " length:" << length << " ";
	os << type[ getChannels() ] << " " << size[ (flags & 0x02) >> 1 ] << "bit " << rate[ (flags & 0x0C) >> 2 ] << "khz " << codec[ getCodec() ];

	return os;
}

std::ostream& VideoTag::operator << (std::ostream& os) const {
	char types[] = {'?', 'K', 'I', 'D', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?'}; // Unknown, Keyframe, Interframe, Disposable Interframe
	const char *codecs[] = {"?", "?",
		"Sorenson H.263", "Screen Video", "On2 VP6", "On2 VP6 Flipped", "ScreenVideo2", "?", "?", "?", "?", "?", "?", "?", "?", "?"};

	assert(frame_type >= 0 && frame_type < (FrameType)(sizeof(types) / sizeof(types[0])));
	assert(codec >= 0 && codec < (Codec)(sizeof(codecs) / sizeof(codecs[0])));

	os << "VideoTag time:" << timestamp << " length:" << length << " type:" << types[frame_type] << " codec:" << codecs[codec];

	if ( frame_type == KeyFrame ) {
		unsigned int width, height;

		readDimensions(width, height);

		os << std::endl << "\t" << "width:" << width << " height:" << height;
	}
		
	return os;
}

std::ostream& MetaTag::operator << (std::ostream& os) const {
	os << "MetaTag time:" << timestamp << " length:" << length << std::endl;
	os << "\t" << this->event.get() << std::endl;
	os <<  this->metadata.get() << std::endl;
	return os;
}

std::ostream& UndefinedTag::operator << (std::ostream& os) const {
	os << "UndefinedTag time:" << timestamp << " length:" << length;
	return os;
}
