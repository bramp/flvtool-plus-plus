/*
	flvtool++ 1.0
	This source is part of flvtool, a generic FLV file editor
	Copyright Andrew Brampton, Lancaster University
	
	This file is released free to use for academic and non-commercial purposes.
	If you wish to use this product for commercial reasons, then please contact us
*/

#include <string>
#include <map>
#include <vector>
#include <stdio.h>

class AMF {

		#define AMF_Double 0
		#define AMF_Boolean 1
		#define AMF_String 2
		#define AMF_Object 3
		#define AMF_Mixed_Array 8
		#define AMF_Array 10
		#define AMF_Date 11

	public:
		virtual void read(FILE *fp) = 0;
		virtual void write(FILE *fp) const = 0 ;

		virtual size_t size() const = 0;

		virtual unsigned int type() const = 0;

		virtual ~AMF() {};

		virtual std::ostream& operator << (std::ostream& os) const = 0;
};

std::ostream& operator << (std::ostream& os, const AMF *amf);

class AMFDouble : public AMF {
	public:

		double d;

		virtual void read(FILE *fp);
		virtual void write(FILE *fp) const;

		virtual size_t size() const;

		virtual unsigned int type() const { return AMF_Double; };

		AMFDouble(FILE *fp);
		AMFDouble(double d);

		virtual std::ostream& operator << (std::ostream& os) const;
};

class AMFBoolean : public AMF {
	public:

		bool b;

		virtual void read(FILE *fp);
		virtual void write(FILE *fp) const;

		virtual size_t size() const;

		virtual unsigned int type() const { return AMF_Boolean; };

		AMFBoolean(FILE *fp);

		virtual std::ostream& operator << (std::ostream& os) const;
};

class AMFString : public AMF {
	public:
		std::string s;

		virtual void read(FILE *fp);
		virtual void write(FILE *fp) const;

		virtual size_t size() const;

		virtual unsigned int type() const { return AMF_String; };

		AMFString(FILE *fp);
		AMFString(const char *s);

		virtual std::ostream& operator << (std::ostream& os) const;
};


struct AMFStringLess : public std::binary_function <class AMFString *, class AMFString *, bool> {
	bool operator()(const class AMFString * _Left, const class AMFString * _Right) const {
		return (_Left->s < _Right->s);
	}
};

/*
struct AMFStringLess : public std::binary_function <class AMFString *, class AMFString *, bool> {
	bool operator()(const class AMFString * _Left, const class AMFString * _Right) const {
		return (_Left < _Right);
	}
};
*/

class AMFMap : public AMF {
	protected:	
		std::map<AMFString *, AMF *, AMFStringLess > m;

	public:

		virtual void read(FILE *fp) = 0;
		virtual void write(FILE *fp) const = 0;

		virtual size_t size() const = 0;

		virtual unsigned int type() const = 0;

		virtual std::ostream& operator << (std::ostream& os) const;

		// Sets this key, both key and data will be deleted when removed from the map
		void set(const char *key, AMF *data);

		// Removes this key, returns true if the key was removed
		bool remove(const char *key);

		// Gets this key		
		AMF *get(const char *key) const;

		virtual ~AMFMap();
};

class AMFObject : public AMFMap {
	public:

		virtual void read(FILE *fp);
		virtual void write(FILE *fp) const;

		virtual size_t size() const;

		virtual unsigned int type() const { return AMF_Object; };

		AMFObject();
		AMFObject(FILE *fp);
};

class AMFMixed_Array : public AMFMap {
	public:

		virtual void read(FILE *fp);
		virtual void write(FILE *fp) const;

		virtual size_t size() const;

		virtual unsigned int type() const { return AMF_Mixed_Array; };

		AMFMixed_Array();
		AMFMixed_Array(FILE *fp);
};

class AMFArray : public AMF {
	public:

		std::vector<AMF *> v;

		virtual void read(FILE *fp);
		virtual void write(FILE *fp) const;

		virtual size_t size() const;

		virtual unsigned int type() const { return AMF_Array; };

		AMFArray(FILE *fp);
		AMFArray();
		virtual ~AMFArray();

		virtual std::ostream& operator << (std::ostream& os) const;
};

class AMFDate : public AMF {
	public:

		unsigned char b[10];

		virtual void read(FILE *fp);
		virtual void write(FILE *fp) const;

		virtual size_t size() const;

		virtual unsigned int type() const { return AMF_Date; };

		AMFDate(FILE *fp);

		virtual std::ostream& operator << (std::ostream& os) const;
};

class AMF * fread_AMF(FILE *fp);
void fwrite_AMF(FILE *fp, class AMF *a);


