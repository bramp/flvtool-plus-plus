/*
	flvtool++ 1.0
	This source is part of flvtool, a generic FLV file editor
	Copyright Andrew Brampton, Lancaster University
	
	This file is released free to use for academic and non-commercial purposes.
	If you wish to use this product for commercial reasons, then please contact us

	What this code does:
		A tool to add keyframe positions into the metadata of a FLV file
		We can decode FLV Tags, and the FLV metadata format
		We can also modify these and write them back out

	TODO make flvtool++ handle timestamps > 2^24 ms (ie when the timestamps wrap around)
	TODO add __FILE__, __LINE__ to exceptions 
	TODO write test suite ;)
	TODO more documentation ;)
	TODO change the command line arguments to give more flexibility. Ie chop/join without adding meta data
	TODO when merging file carry across metadata
	TODO split FLV files
	TODO add more methods on audio/video tags to obtain codec, rates, sizes, etc

	WARNING: FLV files can't index over 2^53 bytes, since the indexs are stored as doubles, and doubles lose int precession
*/

#include "FLV.h"
//#include "Tag.h"
//#include "AMF.h"
#include "Functors.h"

#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <float.h>
#include <iostream>
#include <algorithm>

using std::cerr;
using std::cout;
using std::for_each;

/*
void createIndex(FILE *fp, std::vector<off_t> &keyFramesBytes, std::vector<double> &keyFramesTimes) {
	fseeko(fp, 0, SEEK_SET);

	// Read the header and throw it away
	std::auto_ptr<TagHeader> header ( new TagHeader(fp) );

	off_t tagStart = ftello(fp);

	// Now start reading all the tags
	std::auto_ptr<Tag> tag ( fread_Tag(fp) );

	// Loop reading all the info from the file that we need
	while (tag.get() != NULL) {

		//cout << tag.get() << std::endl;

		// If its a videotag check if its a keyframe
		switch (tag->type()) {
			case Tag::Video: {
				VideoTag *vid = (VideoTag *)tag.get();

				if (vid->getFrameType() == VideoTag::KeyFrame) {
					keyFramesTimes.push_back( (double)tag->getTimestamp() / 1000.00 );
					keyFramesBytes.push_back( tagStart );
				}

				break;
			}
			default:
				break;
		}

		tagStart = ftello(fp);
		tag.reset ( fread_Tag(fp) );
	}
}
*/

void display_help() {
	cerr << "flvtool++ version " REVISION " " REVISIONDATE << std::endl << std::endl;

	cerr << "Display information about a FLV file:" << std::endl;
	cerr << "  flvtool++ -i <input file>" << std::endl << std::endl;

	cerr << "Indexes a FLV file, and optionally trims at start/end times (in seconds):" << std::endl;
	cerr << "  flvtool++ <input file> <output file> (<start time> <end time>)" << std::endl << std::endl;

	cerr << "Joins one or more FLV files together:" << std::endl;
	cerr << "  flvtool++ -j <input files> <output file>" << std::endl << std::endl;
}

int main(int argc, char* argv[]) {

	if (argc <= 1) {
		display_help();
		return -1;
	}

	// Do we want to join files?
	if (strcmp(argv[1], "-j") == 0) {
		//-j test/barsandtone.flv test/barsandtone.flv test/barsandtone2.flv

		if (argc < 4) {
			display_help();
			return -1;
		}

		// This stores all the input FLVStreams
		std::vector<FLVStream *> files;

		try {
			// Create a output stream
			FLVStream out;

			// Now create a FLVStream for each input file
			for (int i = 2; i < (argc - 1); i++ ) {
				FLVStream * in = new FLVStream ( argv[ i ] );

				// We store the FLVStream because it has to exist atleast until we save the output file,
				// otherwise FILE* get broken. Really this should be done in a "better" way
				files.push_back ( in );

				out.append( *in );
			}

			// Add some useful metadata & index
			out.addMetaData();
			out.addIndex();

			out.save( argv[ argc - 1 ] );

			for_each(files.begin(), files.end(), DeleteObject() );
			files.clear();

		} catch (const std::runtime_error & e) {

			// Clean up a little
			for_each(files.begin(), files.end(), DeleteObject() );

			cerr << e.what() << std::endl;

			return -1;

		} catch (const char *c) {

			// Clean up a little
			for_each(files.begin(), files.end(), DeleteObject() );

			cerr << c << std::endl;

			return -1;
		}

		return 0;
	}

	// All the remaining commands need 2 or 4 arguments
	if (argc != 3 && argc != 5) {
		display_help();
		return -1;
	}

	// Do we want to print info?
	if (strcmp(argv[1], "-i") == 0) {

		try {
			FLVStream flv ( argv[2], ~0, true );
			flv.printInfo();

		} catch ( const std::runtime_error &e ) {
			cerr << e.what() << std::endl;
			return -1;
		}

		return 0;
	}

	try {
		std::auto_ptr<FLVStream> flv;

		// Check if we want to chop
		if (argc == 5) {
			unsigned long startTime = (unsigned long) ( atof( argv[3] ) * 1000 );
			unsigned long endTime = (unsigned long) ( atof( argv[4] ) * 1000 );

			flv.reset (  new FLVStream ( argv[1], endTime ) );
			flv->crop( startTime, endTime );

		} else {
			flv.reset (  new FLVStream ( argv[1] ) );
		}

		// Add some useful metadata
		flv->addMetaData();

		// Now add the index
		flv->addIndex();

		// Now write this new flv file out
		flv->save( argv[2] );

	} catch (const std::runtime_error & e) {
		cerr << e.what() << std::endl;
		return -1;

	} catch (const char *c) {
		cerr << c << std::endl;
		return -1;
	}

	return 0;
}

