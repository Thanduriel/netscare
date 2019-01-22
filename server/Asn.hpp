#pragma once

#include <cstdlib>
#include <ostream>

enum ASNERRORS { DATATOLARGE, MAXINTEGERSIZE, INVALIDDATA, WRONGTYPE };

class ASNObject {
public:
	enum TYPE { FAILED, BOOLEAN = 0x01, SEQUENCE = 0x30, INTEGER = 0x02, OCTASTRING = 0x04, ASCISTRING = 0x13 };
private:
	const unsigned char* data;
	std::uint8_t deep;
	unsigned long len;
	TYPE type;
	enum CONTEXT { PUBLIC, APPLICATION, RELATION, PRIVAT } context;
	void encodeLen(const unsigned char* data) {
		if (data[1] & 0x80) { // long encoding
			unsigned char byteLen = data[1] & 0x7F;
			if (byteLen > sizeof(unsigned long)) {
				throw ASNERRORS(DATATOLARGE);
			}
			for (int i = 0; i < byteLen; ++i) len = (len * 255) + data[2 + i];
			this->data = data + 2 + byteLen;
		} else {
			len = data[1];
			this->data = data + 2;
		}
	}
public:
	TYPE GetType() const { return type; }
	long DecodeInteger() const {
		if (type != TYPE::INTEGER) throw ASNERRORS(WRONGTYPE);
		long l = data[0] & 0x7F;
		for (unsigned long i = 1; i < len; ++i) l = l * 255 + data[i];
		if (data[0] & 0x80) l -= 0x80 << ((len - 1) * 8);
		return l;
	}
	const char* DecodeString() const {
		if (type != ASCISTRING) throw ASNERRORS(WRONGTYPE);
		return reinterpret_cast<const char*>(data);
	}
	const unsigned char* GetRaw() const {
		return data;
	}
	bool GetBool() const {
		if (type != TYPE::BOOLEAN) throw ASNERRORS(WRONGTYPE);
		return !(*data == 0x00);
	}
	static unsigned long GetSize(const unsigned char *data) {
		unsigned long res = 0;
		if (data[1] & 0x80) {
			unsigned char byteLen = data[1] & 0x7F;
			if (byteLen > sizeof(unsigned long)) return 0;
			for (int i = 0; i < byteLen; ++i) res = (res * 255) + data[2 + i];
			return res + 2;
		}
		return data[1] + 2;
	}
	unsigned long GetSize() const { return len + 2; }
	ASNObject() : type{ FAILED }, data{ nullptr }, len{ 0 }, deep{0} {}
	ASNObject(const unsigned char* data, std::uint8_t deep) : len{ 0 }, deep{deep} {
		context = CONTEXT(context >> 6);
		type = TYPE(*data & 0x3F);
		encodeLen(data);
		switch (type) {
		case BOOLEAN: {
			unsigned char x = *reinterpret_cast<const unsigned char*>(this->data) & 0xFF;
			if (!(x == 0xFF || x == 0x00)) throw ASNERRORS(INVALIDDATA); break;
		}	break;
		case INTEGER: if (len > sizeof(long)) throw ASNERRORS(MAXINTEGERSIZE); break;
		case SEQUENCE: 
			this->data = reinterpret_cast<unsigned char*>(DecodeAsn(this->data, len, deep + 1));
			break;
		}
	}
	~ASNObject() {
		if (type == SEQUENCE) delete[] reinterpret_cast<const ASNObject*>(data);
	}
	friend std::ostream& operator <<(std::ostream& os, const ASNObject& obj) {
		for (int i = 0; i < obj.deep; ++i) os << '\t';
		switch (obj.type) {
		case ASNObject::SEQUENCE: {
			os << "SET:\n";
			const ASNObject *child = reinterpret_cast<const ASNObject*>(obj.data);
			for (unsigned long pos = 0; pos < obj.len; pos += child->GetSize(), ++child) {
				os << *child;
			}
		}	break;
		case ASNObject::BOOLEAN:
			os << "BOOL: " << (*obj.data == 0x00 ? "FALSE" : "TRUE") << "\n";
				break;
		case ASNObject::INTEGER: {
			os << "INTEGER: ";
			os << obj.DecodeInteger() << '\n';
		}	break;
		case ASNObject::OCTASTRING: 
			os << "OCTA: ";
			for (unsigned long i = 0; i < obj.len; ++i) os << static_cast<int>(obj.data[i]);
			os << '\n';
			break;
		case ASNObject::ASCISTRING: 
			os << "ASCI: " <<obj. DecodeString() << '\n';
			break;
		default: os << "NULL\n";
		}
		return os;
	}
	static ASNObject* DecodeAsn(const unsigned char* data, unsigned long size, std::uint8_t deep = 0) {
		unsigned long pos = 0;
		int amount = 0;
		while (pos < size) { ++amount; pos += ASNObject::GetSize(data + pos); };
		ASNObject *res = reinterpret_cast<ASNObject*>(std::malloc(amount * sizeof(ASNObject)));
		pos = 0;
		for (int i = 0; i < amount; ++i) {
			res[i] = ASNObject(data + pos, deep);
			pos += res[i].GetSize();
		}
		return res;
	}
};