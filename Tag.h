/*
	flvtool++ 1.0
	This source is part of flvtool, a generic FLV file editor
	Copyright Andrew Brampton, Lancaster University
	
	This file is released free to use for academic and non-commercial purposes.
	If you wish to use this product for commercial reasons, then please contact us
*/

#include "common.h"
#include "AMF.h"

#include <memory>
#include <stdio.h>

class TagHeader {

	protected:

		friend std::ostream& operator << (std::ostream& os, const TagHeader& tag);
		
		unsigned char version;
		unsigned char flags;
		unsigned int offset;

		std::auto_ptr<unsigned char> data;

	public:

		enum Flags {
			Video = 0x01,
			Audio = 0x04,
		};

		void read(FILE *fp);
		void write(FILE *fp) const;

		TagHeader(FILE *fp);
		TagHeader();

		std::ostream& operator << (std::ostream& os) const;
		size_t size() const;

		bool hasVideo() const { return (flags & Video) == Video; };
		bool hasAudio() const { return (flags & Audio) == Audio; };

		void setVideo(bool b) { if ( b ) { flags |= Video; } else { flags &= ~Video; } };
		void setAudio(bool b) { if ( b ) { flags |= Audio; } else { flags &= ~Audio; } };

		unsigned char getVersion() const { return version; };
};

class Tag {

		friend std::ostream& operator << (std::ostream& os, const Tag& tag);
		friend Tag * fread_Tag(FILE *fp);
		friend class FLVStream;


	protected:

		const static int TAGHEADERLEN = 11;

		// Position in the file this tag starts
		FILE *fp;
		off_t filepos;

		unsigned int length;
		unsigned int timestamp;

		unsigned int reserved;

		virtual void read(FILE *fp);
		void read_tail(FILE *fp);

		// We don't need a data field if we are storing a filepos (since we can read from the source again)
		//std::auto_ptr<unsigned char> data;

		// Reads the data from the original file, and places it into buf
		size_t read_data(off_t offset, unsigned char *buf, size_t len) const;

	public:
		enum Types {
			Audio     = 0x08,
			Video     = 0x09,
			Meta      = 0x12,
			Undefined = 0x00,
		};

		Tag();
		virtual ~Tag() {};

		virtual Types type() const = 0;

		virtual void write(FILE *fp) const;
		void write_tail(FILE *fp) const;

		virtual size_t size() const;

		unsigned int getTimestamp() const { return timestamp; };
		void setTimestamp(unsigned int timestamp) { this->timestamp = timestamp; }; //TODO make sure the timestamp is within 24bits

		virtual std::ostream& operator << (std::ostream& os) const = 0;
};

std::ostream& operator << (std::ostream& os, const TagHeader *tag);
std::ostream& operator << (std::ostream& os, const Tag *tag);

class AudioTag : public Tag {

	public:

		virtual void read(FILE *fp);
		virtual void write(FILE *fp) const;

		virtual size_t size() const;

		virtual Types type() const { return Audio; };

		AudioTag(FILE *fp);

		virtual std::ostream& operator << (std::ostream& os) const;

		enum Codec {
			Undefined = -1,

			Uncompressed	= 0x0,
			ADPCM			= 0x1,
			MP3				= 0x2,
			NellyMono		= 0x5,
			Nelly			= 0x6,
		};

		Codec getCodec() const;
		unsigned int getChannels() const;
		unsigned int getSampleRate() const;

	private:
		unsigned int flags;

};

class VideoTag : public Tag {

	public:

		enum FrameType {
			//Undefined = 0,

			KeyFrame = 1,
			InterFrame = 2,
			DisposableInterFrame = 3,
		};

		enum Codec {
			Undefined = 0,

			SorensonH263 = 2,
			ScreenVideo = 3,
			On2VP6 = 4,
			On2VP6F = 5,
			ScreenVideo2 = 6,
		};

		virtual void read(FILE *fp);
		virtual void write(FILE *fp) const;

		virtual size_t size() const;

		virtual Types type() const { return Video; };

		VideoTag(FILE *fp);

		virtual std::ostream& operator << (std::ostream& os) const;

		FrameType getFrameType() const { return frame_type; };
		Codec getCodec() const { return codec; };

		unsigned int getWidth() const;
		unsigned int getHeight() const;

	private:
		FrameType frame_type;
		Codec codec;

		void readDimensions(unsigned int &width, unsigned int &height) const;

};

class MetaTag : public Tag {
	
	private:
		std::auto_ptr<AMFString> event;
		std::auto_ptr<AMFMap> metadata;

		// If there is extra data after the meta tags
		size_t extralen;

		void recalc_length();
	public:

		virtual void read(FILE *fp);
		virtual void write(FILE *fp) const;

		virtual size_t size() const;

		virtual Types type() const { return Meta; };

		MetaTag(FILE *fp);
		MetaTag(const char *name);

		// Removes this key
		void remove(const char *key);
		
		// Sets this key
		void set(const char *key, AMF *data);
		
		// Gets this key		
		AMF *get(const char *key) const;

		virtual std::ostream& operator << (std::ostream& os) const;
};

class UndefinedTag : public Tag {

	public:

		virtual void read(FILE *fp);
		virtual void write(FILE *fp) const;

		virtual size_t size() const;

		virtual Types type() const { return Undefined; };

		UndefinedTag(FILE *fp);

		virtual std::ostream& operator << (std::ostream& os) const;
};

class Tag * fread_Tag(FILE *fp);
