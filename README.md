# flvtool++ 1.0
This source is part of flvtool, a generic FLV file editor
Copyright 2008 Andrew Brampton, Lancaster University

This is a C++ re-write of a similar closed source tool [FLVMDI][1] and a open source Ruby app named [flvtool2][2]. I originally wrote flvtool++ due to the lack of large file support in the two previous tools. Well those tools do support large files, however it seems they require at least as much RAM as the size of the file, and when I was working with >1GB files this became a problem. Additionally this tool compiles cleanly on different OSes so I was not limited to just windows.

This tool has the following features:

  * Large file support while using minimum memory
  * Add "indexing" metadata, useful for [streaming applications][3]
  * Add other metadata such as the video's duration, or frames per second, etc
  * Displays all the metadata within a FLV file
  * Displays all the tags within a FLV file
  * Displays interesting statistics about the FLV file
  * Can chop the FLV file at arbitrary timecodes
  * Very fast processing time, the main bottleneck is the disk speed
  * Supports [Windows][4], [Linux][5] and [FreeBSD][6]
  * Source is provided under the [BSD licence][7]
  * Has been programmed in a way allowing other apps to easily use my FLV decode/encoding classes

I will add soon:

  * Ability to add arbitrary metadata
  * Demux the FLV into different video and audio files
  * And whatever [you request][8]&#8230;

#### Usage

flvtool++ is used from the command line, so far it has a few simple parameters

Takes the input file, indexes it and writes it out to the output file. The start and end times may optionally specify timecodes in seconds that are used to chop the FLV file.

```bash
flvtool++ &lt;input file&gt; &lt;output file&gt; (&lt;start time&gt; &lt;end time&gt;)
```

Displays all the metadata and tag information about the input file.

```bash
flvtool++ -i &lt;input file&gt;
```

#### Compiling

**Windows:**

Download the source package and locate the flvtool++.sln file. This is a Visual Studio 2003 solution and should build cleanly without modification. I have not tested this under Visual Studio 2005 or later, but I believe it should compile fine.

It should also be possible to compile this with other compilers under windows (such as GCC) however this is a exercise I leave to the reader

**Linux/FreeBSD:**

flvtool++ compiles cleanly under GCC, a makefile is provided such that you only need to extract the source package, and then type make.

 [1]: http://www.buraks.com/flvmdi/
 [2]: http://rubyforge.org/projects/flvtool2/
 [3]: http://www.flashcomguru.com/index.cfm/2005/11/2/Streaming-flv-video-via-PHP-take-two
 [4]: http://www.microsoft.com/windows/
 [5]: http://www.linux.org/
 [6]: http://www.freebsd.org/
 [7]: http://www.opensource.org/licenses/bsd-license.php
 [8]: /about#contact
 [9]: https://github.com/bramp/flvtool-plus-plus
 