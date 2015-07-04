#	flvtool++ 0.9
#	This source is part of flvtool, a generic FLV file editor
#	Copyright Andrew Brampton, Lancaster University
	
#	This file is released free to use for academic and non-commercial purposes.
#	If you wish to use this product for commercial reasons, then please contact us


CPP = g++
#CFLAGS = -O2 -c -Wall -D_FILE_OFFSET_BITS=64
CFLAGS = -g -O0 -c -Wall -D_FILE_OFFSET_BITS=64

# -g -O0
# -D_GLIBCPP_CONCEPT_CHECKS

SOURCES =	 flvtool.cpp Tag.cpp AMF.cpp FLV.cpp

OBJECTS=$(SOURCES:.cpp=.o)

EXECUTABLE=bin/flvtool++

all: $(SOURCES) $(EXECUTABLE)
#	strip $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	mkdir -p bin
	$(CPP) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CPP) $(CFLAGS) $< -o $@
	
clean:
	rm -f ${OBJECTS} $(EXECUTABLE)

