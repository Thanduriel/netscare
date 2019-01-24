#pragma once

#include <windows.h>
#include <cstdlib>
#include <ostream>
#include <vector>

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
	const wchar_t* DecodeUTF8() const;
	long DecodeInteger() const;
	const char* DecodeString() const;
	const unsigned char* GetRaw() const;
	bool DecodeBool() const;
	constexpr static unsigned long EncodeHeaderSize(unsigned long dataSize);
	constexpr static unsigned long GetSize(const unsigned char *data);
	unsigned long GetSize() const { return len + headerSize; }
	unsigned long GetDataSize() const { return len; }
	ASNObject(TYPE type, const unsigned char*const data, const unsigned long len) : type{ type }, data{ data }, len{ len }, headerSize{ 1 + EncodeLenSize(len)}, deep { 0 } {}
	ASNObject() : type{ FAILED }, data{ nullptr }, len{ 0 }, deep{ 0 } {}
	ASNObject(const unsigned char* data, std::uint8_t deep) : len{ 0 }, deep{ deep } {
		context = CONTEXT(context >> 6);
		type = TYPE(*data & 0x3F);
		DecodeLen(data);
		switch (type) {
		case BOOLEAN: {
			unsigned char x = *reinterpret_cast<const unsigned char*>(this->data) & 0xFF;
			if (!(x == 0xFF || x == 0x00)) throw ASNERRORS(INVALIDDATA); break;
		}	break;
		case INTEGER: if (len > sizeof(long)) throw ASNERRORS(MAXINTEGERSIZE); break;
		case SEQUENCE:
			this->data = reinterpret_cast<unsigned char*>(DecodeAsn(this->data, len, deep + 1).objects);
			break;
		}
	}
	~ASNObject() {
		if (type == SEQUENCE) delete[] reinterpret_cast<const ASNObject*>(data);
	}
	friend std::ostream& operator <<(std::ostream& os, const ASNObject& obj);
	struct ASNDecodeReturn {
		ASNDecodeReturn(ASNObject* obj, std::uint8_t len) : objects{ obj }, len{ len } {} 
		ASNObject* objects;
		std::uint8_t len;
		operator ASNObject* () { return objects; }
	};
	static ASNDecodeReturn DecodeAsn(const unsigned char* data, unsigned long size, std::uint8_t deep = 0);
	static unsigned long EncodingSize(const std::vector<unsigned char>& data);
	static const unsigned char* EncodeAsnPrimitives(const std::vector<unsigned char>& data, unsigned char* destionation = nullptr);
	static unsigned long EncodingSize(const int num);
	static const unsigned char* EncodeAsnPrimitives(const int num, unsigned char* destination = nullptr);
	static unsigned long EncodingSize(const wchar_t* msg);
	static unsigned char* EncodeAsnPrimitives(const wchar_t* msg, unsigned char* destination = nullptr);
	static unsigned long EncodingSize(const std::vector<std::unique_ptr<Command>>& commands);
	static const unsigned char* EncodeSequence(std::vector<std::unique_ptr<Command>> const& commands, unsigned char* destination = nullptr);
	static unsigned long EncodingSize(const char* msg);
	static const unsigned char* EncodeAsnPrimitives(const char* msg, unsigned char* destination = nullptr);
};