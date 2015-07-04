/*
	flvtool++ 1.0
	This source is part of flvtool, a generic FLV file editor
	Copyright Andrew Brampton, Lancaster University
	
	This file is released free to use for academic and non-commercial purposes.
	If you wish to use this product for commercial reasons, then please contact us
*/

#include "Tag.h"

#include <memory>

/**
	Class to represent a FLV file/stream
*/

class FLVStream {

	protected:

		// The filehandle associated with this stream
		FILE *fp;

		// The stream's header
		std::auto_ptr<TagHeader> header;

		// The meta data tag, we assume there is only one (and we use the first we encounter)
		MetaTag *meta;

		// Maps stream position (in bytes) to Tags
		typedef std::vector<Tag *> tags_t;
		tags_t tags;
		
		// Hack, when appending tags into this FLVStream, they are still owned by the original FLVStream
		// Thus some may be double deleted! This boolean option decides whether we own ALL the tags
		bool ownTags;

		// Some vars to record all sorts of information
		unsigned int audiotags;
		unsigned int videotags;
		unsigned int metatags;
		unsigned int undefinedtags;

		unsigned int keyframes;

		VideoTag::Codec videocodec;
		AudioTag::Codec audiocodec;

		unsigned int width;
		unsigned int height;

		// Start time in ms
		unsigned long start;

		// End time in ms
		unsigned long end;

		// Helper method that just finds the keyframes and creates some indexes
		void findKeyFrames ( std::vector<off_t> & keyFramesBytes, std::vector<double> & keyFramesTimes );

		// Loads the filename, and goes no futher than end
		void init(const char* filename, unsigned long end, bool verbose);

		// Prints the frames from begin to end
		void printFrames(tags_t::const_iterator begin, tags_t::const_iterator end) const;

		// Counts how many tags, we have, and records other information
		void calculateInformation();
		inline void addTagInformation(const Tag & tag );

	public:

		// Constructs a new FLV Stream from a file, but doesn't read past end, and prints out tag information
		FLVStream(const char *filename = NULL, unsigned long end = ~0, bool verbose = false);

		~FLVStream();

		// Adds a index into the meta data at the beginning of the file
		void addIndex ( );

		// Add some useful meta data TODO make this more flexible
		void addMetaData ( );

		// Append the argument on to the end of this stream
		void append ( FLVStream &flv );

		// Crop this stream at the start and end timestamps 
		void crop ( unsigned int start, unsigned int end );

		// Prints to stdout information about this stream
		void printFrames() const;
		void printInfo() const;

		// Returns the duration in milliseconds
		unsigned int duration() const { return end - start; }

		// Write this FLV file out to the filename
		void save ( const char *filename );

		// Returns the metadata tag (if there isn't one create it!
		MetaTag *getMetaTag();

		// Returns if we have video frames
		bool hasVideo() const { return header->hasVideo(); };

		// Returns if we have audio frames
		bool hasAudio() const { return header->hasAudio(); };

		unsigned char getVersion() const { return header->getVersion(); };

		double getWidth() const { return width; };
		double getHeight() const { return height; };

		double getFPS() const { if ( duration() > 0 ) return videotags / duration(); else return 0; };

		VideoTag::Codec getVideoCodec() const { return videocodec; };
		AudioTag::Codec getAudioCodec() const { return audiocodec; };

		unsigned int getTagCount() const { return (unsigned int) tags.size(); };
};
