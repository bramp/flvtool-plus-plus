/*
	flvtool++ 1.0
	This source is part of flvtool, a generic FLV file editor
	Copyright Andrew Brampton, Lancaster University
	
	This file is released free to use for academic and non-commercial purposes.
	If you wish to use this product for commercial reasons, then please contact us
*/

#include "FLV.h"

#include <iostream>
#include <algorithm>
#include <assert.h>

#include "Functors.h"

using std::map;
using std::for_each;
using std::cout;
using std::endl;
using std::auto_ptr;
using std::vector;

FLVStream::FLVStream(const char* filename, unsigned long end, bool verbose) 
	: fp(NULL), meta( NULL ), ownTags(filename != NULL), 
		audiotags ( 0 ), videotags (0), metatags (0), undefinedtags (0), keyframes (0), 
		videocodec(VideoTag::Undefined), audiocodec(AudioTag::Undefined), 
		width(0), height(0), start (0), end (0)  {

	// Check if we are making a blank FLVStream
	if ( filename == NULL ) {
		header.reset ( new TagHeader() );

	} else {
		init(filename, end, verbose);
	}
}

void FLVStream::init(const char* filename, unsigned long end, bool verbose) {

	assert ( filename != NULL );
	assert ( strlen( filename ) > 0 );

	fp = fopen(filename, "rb");

	if (fp == NULL)
		throw vargs_exception("Error %d opening input file '%s'\n", errno, filename);

	header.reset ( new TagHeader ( fp ) );

	if ( verbose )
		cout << header.get() << endl;

	Tag *tag;

	while ( true ) { // Now start reading all the tags
		
		tag = fread_Tag(fp);

		if ( tag == NULL )
			break;

		// Do read past a certain timestamp
		if ( tag->getTimestamp() > end )
			break;

		if ( verbose )
			cout << tag->filepos << " " << tag << endl;

		addTagInformation( *tag );

		tags.push_back ( tag );
	}

	// If we specified an end, we should run the crop method to make sure we don't have any frames we shouldn't
	if ( end != (unsigned long)~0 ) {
		crop(0, end);
	}

	if ( tags.size() > 0 ) {
		this->start = tags.front()->getTimestamp();
		this->end = tags.back()->getTimestamp();
	}

}

FLVStream::~FLVStream() {

	if ( ownTags )
		for_each(tags.begin(), tags.end(), DeleteObject() );

	if ( fp != NULL )
		fclose(fp);
}

void FLVStream::addTagInformation(const Tag & tag ) {

	// Look at all the tags and count how many there are
	switch ( tag.type() ) {
		case Tag::Audio: {
			audiotags++;

			if ( videocodec == AudioTag::Undefined ) {
				const AudioTag *a = static_cast<const AudioTag*> ( &tag );
				audiocodec = a->getCodec();
			}

			break;
		}
		case Tag::Video: {
			videotags++;

			const VideoTag *v = static_cast<const VideoTag*> ( &tag );
			if (v->getFrameType() == VideoTag::KeyFrame) {
				keyframes++;

				// If we don't have width/height calculations, then work them out
				if ( width == 0 || height == 0 ) {
					width = v->getWidth();
					height = v->getHeight();
				}

				if ( videocodec == VideoTag::Undefined )
					videocodec = v->getCodec();
			}

			break;
		}
		case Tag::Meta: {
			// If we haven't got a meta tag yet, then use the first available
			if ( meta == NULL ) {
				// We do a nasty cast here!
				meta = const_cast<MetaTag *> ( static_cast<const MetaTag *> ( &tag ) );
			}

			// Read some data from the meta tag

			metatags++;
			break;
		}
		default: {
			undefinedtags++;
		}
	}
}

void FLVStream::calculateInformation() {
	audiotags = 0;
	videotags = 0;
	metatags = 0;
	undefinedtags = 0;
	keyframes = 0;
	meta = NULL;

	// Start & end times in ms
	start = tags.front()->getTimestamp();
	end = tags.back()->getTimestamp();

	tags_t::const_iterator i = tags.begin();

	for ( ; i != tags.end(); ++i) {
		const Tag *t = (*i);
		addTagInformation( *t );
	}

	// Make sure to correct the header
	header->setAudio( audiotags > 0 );
	header->setVideo( videotags > 0 );

}

void FLVStream::printFrames() const {
	FLVStream::printFrames( tags.begin(), tags.end() );
}

void FLVStream::printFrames(tags_t::const_iterator begin, tags_t::const_iterator end) const {
	tags_t::const_iterator i = begin;

	for ( ; i != end ; ++i) {
		cout << (*i)->filepos << " " << (*i) << endl;
	}
}

void FLVStream::printInfo() const {
	double startd = start / 1000.00;
	double endd = end / 1000.00;
	double duration = endd - startd;

	double fps = videotags / duration;
	double keyinterval = duration / keyframes;

	cout.precision(4);
	cout << "Stream Info " << endl;
	cout << "Tags Video: " << videotags << ", Audio: " << audiotags << ", Meta: " << metatags << ", Undefined: " << undefinedtags << endl;
	cout << "Start: " << startd << "s, End: " << endd << "s, Duration: " << duration << "s @ " << fps << "fps, Keyframe interval: " << keyinterval << "s" << endl;

}

void FLVStream::crop ( unsigned int start, unsigned int end ) {

	if (end <= start)
		throw vargs_exception("End time must be larger than start time '%ld vs %ld'\n", start, end);

	tags_t::iterator i = tags.begin();
	tags_t::iterator startTag = tags.begin();
	tags_t::iterator endTag = tags.end();

	// We need to find the smallest audio or keyframe to keep
	for ( ; i != tags.end() ; ++i ) {
		const Tag *t = (*i);

		if (start > t->getTimestamp() ) {
			// Make note of the last keyframe before the start
			if ( t->type() == Tag::Video ) {
				const VideoTag *vt = static_cast<const VideoTag *> ( t );

				if ( vt->getFrameType() == VideoTag::KeyFrame )
					startTag = i;
			}
		}

		if (end < t->getTimestamp())
			break;

	}
	endTag = i;

	// The keyframe may be slightly earlier than start, so readjust start
	start = std::min( start, (*startTag)->getTimestamp() );

	// Now roll back a little to find any audio/meta tags within the time period
	for ( i = startTag; i != tags.begin(); --i ) {
		const Tag *t = (*i);

		// If this is a non keyframe video tag, then break
		if ( t->type() == Tag::Video) {
			const VideoTag *vt = static_cast<const VideoTag *> ( t );

			if ( vt->getFrameType() != VideoTag::KeyFrame )
				break;
		}

		// If this tag is too early (and not the same time as us) then break
		if (t->getTimestamp() < start) {
			break;
		}

	} 
	startTag = ++i;

	// Remove all the tags not in the range
	//tags.insert(tags.begin(), startTag, endTag);
	for_each(endTag, tags.end(), DeleteObject() );
	tags.erase(endTag, tags.end());

	for_each(tags.begin(), startTag, DeleteObject() );
	tags.erase(tags.begin(), startTag);

	// The meta tag might have been chopped
	// TODO check it got chopped!, and/or keep a copy
	meta = NULL;

	// Reset the timestamp of the first tag!
	unsigned long offset = tags[0]->getTimestamp();

	for (i = tags.begin(); i != tags.end(); i++) {
		(*i)->setTimestamp( (*i)->getTimestamp() - offset );
	}

	// Don't forget to update information in this class
	calculateInformation();
}

MetaTag *FLVStream::getMetaTag() {
	
	// If we don't have a meta tag then create one
	if ( meta == NULL ) {
		meta = new MetaTag("onMetaData");
		
		// Place this meta tag at the beginning
		tags.insert( tags.begin(), meta );
	}

	return meta;
}

void FLVStream::save ( const char * filename ) {

	assert ( filename != NULL );
	assert ( strlen( filename ) > 0 );

	FILE *fp = fopen(filename, "wb");

	if (fp == NULL) {
		throw vargs_exception("Error %d opening output file '%s'\n", errno, filename);
	}

	// Write out the FLV header
	header->write(fp);

	// Now loop each tag
	tags_t::const_iterator i = tags.begin();

	for ( ; i != tags.end(); ++i) {
		(*i)->write(fp);
	}

	fclose(fp);
}

void FLVStream::findKeyFrames ( vector<off_t> & keyFramesBytes, vector<double> & keyFramesTimes ) {
	
	keyFramesBytes.clear();
	keyFramesBytes.reserve ( this->keyframes );

	keyFramesTimes.clear();
	keyFramesTimes.reserve ( this->keyframes );

	off_t offset = header->size();

	tags_t::const_iterator i = tags.begin();
	for ( ; i != tags.end(); ++i) {
		const Tag *t = (*i);

		if ( t->type() == Tag::Video ) {

			const VideoTag *v = static_cast<const VideoTag*> ( t );
			if ( v->getFrameType() == VideoTag::KeyFrame ) {
				keyFramesTimes.push_back( v->getTimestamp() / 1000.00 );
				keyFramesBytes.push_back( offset );
			}
		}

		offset += t->size();
	}

	assert ( keyFramesBytes.size() == keyFramesTimes.size() );

}

template <class T>
AMFArray *makeDoubleArray(vector<T> a) {
	auto_ptr<AMFArray> arr ( new AMFArray() );

	typename vector< T >::const_iterator i = a.begin();

	while ( i != a.end() ) {

		AMFDouble *d = new AMFDouble ( (double) (*i) );
		arr->v.push_back( d );

		++i;
	}

	return arr.release();
}

void FLVStream::addIndex ( ) {
	
	// Get the meta tag first ( to ensure it exists )
	MetaTag *meta = this->getMetaTag();

	// Two arrays to hold the byte pos of each keyframe, as well as the keyframes timestamp
	vector<off_t> keyFramesBytes;
	vector<double> keyFramesTimes;

	// Find all the keyframes and add them into these array
	findKeyFrames ( keyFramesBytes, keyFramesTimes );

	// Place the keyframes into the metadata struct
	AMFObject *o = new AMFObject();

	// Construct the keyframe arrays
	o->set( "times", makeDoubleArray(keyFramesTimes) );
	o->set( "filepositions", makeDoubleArray(keyFramesBytes) );

	meta->set("keyframes", o);

	// Now update the byte positions (since the meta tag might have changed in size)
	findKeyFrames ( keyFramesBytes, keyFramesTimes );

	// Re-add it to complete the index
	o->set( "filepositions", makeDoubleArray(keyFramesBytes) );
}

void FLVStream::addMetaData ( ) {
	MetaTag *meta = this->getMetaTag();

	// Now add some extra fields
	meta->set("duration", new AMFDouble ( (tags.back()->getTimestamp() - tags.front()->getTimestamp()) / 1000.0 ));
	meta->set("lasttimestamp", new AMFDouble ( tags.back()->getTimestamp() ));

	meta->set("metadatacreator", new AMFString("flvtool++ by bramp"));

	/*
	// HACK - Remove a bunch of stuff that might be causing problems
	meta->remove( "audiocodecid" );
	meta->remove( "audiosamplerate" );
	meta->remove( "audiosamplesize" );
	meta->remove( "filesize" );
	meta->remove( "framerate" );
	meta->remove( "height" );
	meta->remove( "width" );
	meta->remove( "stereo" );
	meta->remove( "videocodecid" );
	meta->remove( "videodatarate" );
	
	*/
}

void FLVStream::append ( FLVStream &flv ) {

	unsigned int offset = ~0;
	bool foundKeyFrame = false;

	if ( ownTags ) {
		throw std::runtime_error( "Current we can't append to a FLVStream that was read from a file! Sorry" );
	}

	// If we have some tags, we must make sure the new flv has the same specs
	if ( getTagCount() > 0 ) {

		// Check codecs
		// TODO Make a option to allow this, because a decode might be able to handle different codecs
		if ( getVideoCodec() != flv.getVideoCodec() )
			throw std::runtime_error( "We can't append a stream with a different video codec" );

		if ( getAudioCodec() != flv.getAudioCodec() )
			throw std::runtime_error( "We can't append a stream with a different audio codec" );

		// Check the flv stream has the same video resolution as us
		if ( getWidth() != flv.getWidth() )
			throw std::runtime_error( "We can't append a stream with a different video width" );

		if ( getHeight() != flv.getHeight() )
			throw std::runtime_error( "We can't append a stream with a different video height" );
		
	}

	// Copy all the tags to append
	tags_t::iterator i = flv.tags.begin();
	for ( ; i != flv.tags.end(); ++i ) {
		Tag *t = (*i);

		// We don't want to copy meta tags, 
		if ( t->type() == Tag::Meta ) {
			continue;
		}

		// AND the first video tag must be a keyframe
		if ( t->type() == Tag::Video ) {
			VideoTag *vt = static_cast<VideoTag *> ( t );

			if ( vt->getFrameType() == VideoTag::KeyFrame ) {
				foundKeyFrame = true;
			} else if ( !foundKeyFrame) {
				continue;
			}
		}

		// Work out the correct timestamp offset
		if (offset == (unsigned int)~0) {
			offset = end - t->getTimestamp();
		}
		
		t->setTimestamp ( t->getTimestamp() + offset );

		tags.push_back ( t );
	}

	calculateInformation();
}
