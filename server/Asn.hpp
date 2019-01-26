#pragma once

#include <windows.h>
#include <cstdlib>
#include <ostream>
#include <vector>
#include <cassert>

class Command;

enum ASNERRORS { DATATOLARGE, MAXINTEGERSIZE, INVALIDDATA, WRONGTYPE };

class ASNObject {
public:
	enum TYPE { FAILED, BOOLEAN = 0x01, SEQUENCE = 0x30, INTEGER = 0x02, OCTASTRING = 0x04, UTF8String = 0x0c, ASCISTRING = 0x13 };
private:
	const unsigned char* data;
	std::uint8_t deep;
	unsigned long len;
	unsigned long headerSize;
	TYPE type;
	enum CONTEXT { PUBLIC, APPLICATION, RELATION, PRIVAT } context;
	void DecodeLen(const unsigned char* data);
	static constexpr const unsigned long EncodeLenSize(unsigned long len);
	static constexpr void EncodeLen(unsigned long len, unsigned char* msg);
public:
	TYPE GetType() const { return type; }
	struct ASNDecodeReturn {
		ASNDecodeReturn(const ASNObject* obj, std::uint8_t len) : objects{ obj }, len{ len } {}
		const ASNObject* objects;
		std::uint8_t len;
		operator const ASNObject* () const { return objects; }
	};
	const wchar_t* DecodeUTF8() const;
	long DecodeInteger() const;
	const char* DecodeString() const;
	const unsigned char* GetRaw() const;
	bool DecodeBool() const;
	ASNObject::ASNDecodeReturn DecodeASNObjscets() const;
	static unsigned long EncodeHeaderSize(unsigned long dataSize);
	static unsigned long GetSize(const unsigned char *data);
	unsigned long GetSize() const { return len + headerSize; }
	unsigned long GetDataSize() const { return len; }
	ASNObject(TYPE type, const unsigned char*const data, const unsigned long len);
	ASNObject();
	ASNObject(const unsigned char* data, std::uint8_t deep);
	~ASNObject() {
		// if (type == SEQUENCE) delete[] reinterpret_cast<const ASNObject*>(data);
	}
	friend std::ostream& operator <<(std::ostream& os, const ASNObject& obj);
	static ASNDecodeReturn DecodeAsn(const unsigned char* data, unsigned long size, std::uint8_t deep = 0);
	
	
	static unsigned long EncodingSize(const std::vector<unsigned char>& data);
	static const unsigned char* EncodeAsnPrimitives(const std::vector<unsigned char>& data, unsigned char* destionation = nullptr);
	static const unsigned char* EncodeAsnPrimitives(const unsigned char* begin, const unsigned char* end, unsigned char* destination = nullptr);

	static unsigned long EncodingSize(const int num);
	static const unsigned char* EncodeAsnPrimitives(const int num, unsigned char* destination = nullptr);

	static unsigned long EncodingSize(const wchar_t* msg);
	static unsigned char* EncodeAsnPrimitives(const wchar_t* msg, unsigned char* destination = nullptr);
	
	static unsigned long EncodingSize(const char* msg);
	static const unsigned char* EncodeAsnPrimitives(const char* msg, unsigned char* destination = nullptr);

	template <typename T> // CommadnPtr
	static unsigned long EncodingSize(const std::vector<T>& commands) {
		unsigned long len = 0, b;
		for (const T& c : commands) {
			b = c->EncodeSize();
			len += b + EncodeHeaderSize(b);
		}
		return 1 + EncodeLenSize(len) + len;
	}
	template <typename T>
	static const unsigned char* EncodeSequence(const std::vector<T>& commands, unsigned char* destination) {
		unsigned long len = 0, b;
		for (const T& c : commands) {
			b = c->EncodeSize();
			len += b + EncodeHeaderSize(b);
		}

		const unsigned long header = 1 + EncodeLenSize(len);
		unsigned char* res;
		if (destination) res = destination;
		else {
			res = new unsigned char[header + len];
		}
		res[0] = TYPE::SEQUENCE;
		EncodeLen(len, res + 1);
		unsigned char* p = res + header;
		for (const T& c : commands) {
			*p = TYPE::SEQUENCE;
			b = c->EncodeSize();
			EncodeLen(b, ++p);
			p += EncodeLenSize(b);
			c->Encode(p);
			p += b;
		}
		assert(p - res == len + header);
		return res;
	}
};