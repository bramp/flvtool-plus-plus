/*
	flvtool++ 1.0
	This source is part of flvtool, a generic FLV file editor
	Copyright Andrew Brampton, Lancaster University
	
	This file is released free to use for academic and non-commercial purposes.
	If you wish to use this product for commercial reasons, then please contact us
*/

#include "AMF.h"
#include "common.h"

#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <assert.h>

class AMF * fread_AMF(FILE *fp) {
	unsigned char type;

	type = fread_8(fp);

	switch (type) {
		case AMF_Double: // double
			return new AMFDouble(fp);
		case AMF_Boolean: // boolean
			return new AMFBoolean(fp);
		case AMF_String: // string
			return new AMFString(fp);
		case AMF_Object: // object
			return new AMFObject(fp);
		case AMF_Mixed_Array: // mixed_array
			return new AMFMixed_Array(fp);
		case AMF_Array: // array
			return new AMFArray(fp);
		case AMF_Date: // date
			return new AMFDate(fp);
	}

	return NULL;
}

void fwrite_AMF(FILE *fp, class AMF *a) {
	unsigned char type = a->type();

	fwrite_8(fp, type);
	a->write(fp);
}

AMFDouble::AMFDouble(FILE *fp) { read(fp); }
AMFDouble::AMFDouble(double d) : d(d) {};

AMFBoolean::AMFBoolean(FILE *fp) { read(fp); }

AMFString::AMFString(FILE *fp) { read(fp); }
AMFString::AMFString(const char *s) : s(s) {};

AMFObject::AMFObject(FILE *fp) { read(fp); }
AMFObject::AMFObject() {}

AMFMixed_Array::AMFMixed_Array(FILE *fp) { read(fp); }
AMFMixed_Array::AMFMixed_Array() {};

AMFArray::AMFArray(FILE *fp) { read(fp); }
AMFArray::AMFArray() { }

AMFDate::AMFDate(FILE *fp) { read(fp); }

size_t AMFDouble::size() const { return 8; }
size_t AMFBoolean::size() const { return 1; }
size_t AMFString::size() const { return 2 + s.length(); }
size_t AMFObject::size() const {
	size_t length = 3;

	std::map<AMFString *, AMF *, AMFStringLess>::const_iterator i = m.begin();

	while (i != m.end()) {
		length += (*i).first->size();
		length += 1 + (*i).second->size();
		i++;
	}

	return length;
}
size_t AMFMixed_Array::size() const { 
	size_t length = 4 + 3;

	std::map<AMFString *, AMF *, AMFStringLess>::const_iterator i = m.begin();

	while (i != m.end()) {
		length += (*i).first->size();
		length += 1 + (*i).second->size();
		i++;
	}

	return length;
}
size_t AMFArray::size() const { 
	size_t length = 4;

	std::vector<AMF *>::const_iterator i = v.begin();

	while (i != v.end()) {
		length += 1 + (*i)->size();
		i++;
	}

	return length;
}
size_t AMFDate::size() const { return 10; }

AMFMap::~AMFMap() {
	std::map<AMFString *, AMF *, AMFStringLess>::iterator i = m.begin();

	while (i != m.end()) {
		delete (*i).first;
		delete (*i).second;

		i++;
	}

	m.clear();
}

AMFArray::~AMFArray() {
	std::vector<AMF *>::iterator i = v.begin();

	while (i != v.end()) {
		delete (*i);
		i++;
	}

	v.clear();
}

void AMFDouble::read(FILE *fp) {
	d = fread_64(fp); 
}

void AMFBoolean::read(FILE *fp) {
	unsigned char tmp = fread_8(fp);
	b = (tmp != 0);
}

void AMFString::read(FILE *fp) {

	unsigned short len = fread_16(fp);
	s.reserve(len);

	while (len > 0) {
		unsigned char c = fread_8(fp);		
		s += c;

		len--;
	}
}

void AMFObject::read(FILE *fp) {
	AMFString *key = new AMFString(fp);

	while (key->s.length() != 0) {
		AMF *object = fread_AMF(fp);
		m[key] = object;

		key = new AMFString(fp);
	}

	// Should be a single byte 9 now
	fseeko(fp, 1, SEEK_CUR);
}

void AMFMixed_Array::read(FILE *fp) {
	
	// Read the size of this array (however we never actually use it)
	fread_32(fp);

	AMFString *key = new AMFString(fp);

	//while (size > 0) {
	while (key->s.length() != 0) {
		AMF *object = fread_AMF(fp);
		m[key] = object;

		key = new AMFString(fp);
	}
	
	delete key;

	// Should be a single byte 9 now
	fseeko(fp, 1, SEEK_CUR);
}

void AMFArray::read(FILE *fp) {
	unsigned int size = fread_32(fp);

	while (size > 0) {
		v.push_back ( fread_AMF(fp) );
		size--;
	}
}

void AMFDate::read(FILE *fp) {
	//TODO
	fread(b, 1, 10, fp);
}


void AMFDouble::write(FILE *fp) const {
	fwrite_64(fp, d);
}

void AMFBoolean::write(FILE *fp) const {
	fwrite_8(fp, b);
}

void AMFString::write(FILE *fp) const {
	fwrite_16(fp, (unsigned short) s.length() );
	fwrite_s(fp, s.data(), s.length());
}

void AMFObject::write(FILE *fp) const {
	std::map<AMFString *, AMF *, AMFStringLess>::const_iterator i = m.begin();

	while (i != m.end()) {
		(*i).first->write(fp);
		fwrite_AMF(fp, (*i).second);

		i++;
	}

	fwrite_s(fp, "\x00\x00\x09", 3); 
}

void AMFMixed_Array::write(FILE *fp) const {
	fwrite_32(fp, (unsigned short) m.size());

	std::map<AMFString *, AMF *, AMFStringLess>::const_iterator i = m.begin();

	while (i != m.end()) {
		(*i).first->write(fp);
		fwrite_AMF(fp, (*i).second);

		i++;
	}

	fwrite_s(fp, "\x00\x00\x09", 3); 
}

void AMFArray::write(FILE *fp) const {
	fwrite_32(fp, (unsigned int)v.size());

	std::vector<AMF *>::const_iterator i = v.begin();

	while (i != v.end()) {
		fwrite_AMF(fp, (*i));

		i++;
	}
}

void AMFDate::write(FILE *fp) const {
	fwrite_s(fp, b, 10);
}

void AMFMap::set(const char *key, AMF *data) {
	assert(key != NULL);
	assert(data != NULL);

	// Remove the old one (if it exists)
	remove(key);

	AMFString *k = new AMFString(key);

	// Insert the data
	m[k] = data;
}

bool AMFMap::remove(const char *key) {
	assert(key != NULL);

	AMFString k ( key );

	// Check if this key already exists
	std::map<AMFString *, AMF *, AMFStringLess >::iterator i = m.find( &k );

	if (i == m.end() )
		return false;

	// Free old AMF data
	delete (*i).first;
	delete (*i).second;

	m.erase(i);

	return true;
}

AMF * AMFMap::get(const char *key) const {
	assert(key != NULL);

	AMFString k ( key );

	// Check if this key already exists
	std::map<AMFString *, AMF *, AMFStringLess >::const_iterator i = m.find( &k );

	if (i != m.end() ) {
		return (*i).second;
	}

	return NULL;
}


std::ostream& operator << (std::ostream& os, const AMF *amf) { return amf->operator <<(os); }

std::ostream& AMFDouble::operator << (std::ostream& os) const {
	os.width(10);
	os.precision(10);
	return os << this->d;
}
std::ostream& AMFBoolean::operator << (std::ostream& os) const {
	return os << this->b;
}
std::ostream& AMFString::operator << (std::ostream& os) const {
	return os << "\"" << this->s << "\"";
}
std::ostream& AMFMap::operator << (std::ostream& os) const {
	std::map<AMFString *, AMF *, AMFStringLess>::const_iterator i = m.begin();

	for ( ;i != m.end(); i++) {
		os << "\t\t" << (*i).first << ": " << (*i).second << std::endl;
	}

	return os;
}
std::ostream& AMFArray::operator << (std::ostream& os) const {
	std::vector<AMF *>::const_iterator i = v.begin();

	for (; i != v.end(); i++) {
		os << "\t\t\t" << (*i) << std::endl;
		
	}

	return os;
}
std::ostream& AMFDate::operator << (std::ostream& os) const {
	return os << "TODO Date";
}

