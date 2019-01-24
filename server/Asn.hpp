#pragma once

#include <cstdlib>
#include <ostream>
#include <vector>

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
	void DecodeLen(const unsigned char* data) {
		if (data[1] & 0x80) { // long encoding
			unsigned char byteLen = data[1] & 0x7F;
			if (byteLen > sizeof(unsigned long)) {
				throw ASNERRORS(DATATOLARGE);
			}
			len = 0;
			for (int i = 0; i < byteLen; ++i) len = (len * 256) + data[2 + i];
			this->data = data + 2 + byteLen;
			headerSize = 2 + byteLen;
		}
		else {
			len = data[1];
			headerSize = 2;
			this->data = data + 2;
		}
	}
	static constexpr const unsigned long EncodeLenSize(unsigned long len) {
		if (len < 0x80) return 1;
		std::uint8_t size = 0x00;
		while (len) {
			len >>= 8;
			++size;
		}
		return size + 1;
	}
	static constexpr void EncodeLen(unsigned long len, unsigned char* msg) {
		if (len < 0x80) {
			msg[0] = static_cast<unsigned char>(len);
			return;
		}
		std::uint8_t size = 0x00;
		unsigned int long x = len;
		while (x) {
			x >>= 8;
			++size;
		}
		msg[0] = size | 0x80;
		unsigned char* itr = msg;
		for (int i = size - 1; i >= 0; --i)
			*(++itr) = static_cast<unsigned char>((len & (0xFF << (i * 8))) >> (i * 8));
		return;
	}
public:
	TYPE GetType() const { return type; }
	const wchar_t* DecodeUTF8() const {
		if (type != TYPE::UTF8String) throw ASNERRORS(WRONGTYPE);
		return reinterpret_cast<const wchar_t*>(data);
	}
	long DecodeInteger() const {
		if (type != TYPE::INTEGER) throw ASNERRORS(WRONGTYPE);
		if (len == 0) return 0;
		long l = data[0] & 0x7F;
		for (unsigned long i = 1; i < len; ++i) l = l * 256 + data[i];
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
	bool DecodeBool() const {
		if (type != TYPE::BOOLEAN) throw ASNERRORS(WRONGTYPE);
		return !(*data == 0x00);
	}
	constexpr static unsigned long GetSize(const unsigned char *data) {
		unsigned long res = 0;
		if (data[1] & 0x80) {
			unsigned char byteLen = data[1] & 0x7F;
			if (byteLen > sizeof(unsigned long)) {
				MessageBoxW(NULL, L"MessagTOOOLOng", L"Encoding error", MB_OK | MB_ICONERROR);
				return 0;
			}
			for (int i = 0; i < byteLen; ++i) 
				res = (res * 256) + data[2 + i];
			return res + 2 + byteLen;
		}
		return data[1] + 2;
	}
	unsigned long GetSize() const { return len + headerSize; }
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
			os << "OCTA: size: " << obj.len << '\n';
			// for (unsigned long i = 0; i < obj.len; ++i) os << static_cast<int>(obj.data[i]);
			break;
		case ASNObject::ASCISTRING:
			os << "ASCI: " << obj.DecodeString() << '\n';
			break;
		case ASNObject::UTF8String:
			os << "UTF8: " << obj.DecodeUTF8() << '\n';
			break;
		default: os << "NULL\n";
		}
		return os;
	}
	struct ASNDecodeReturn {
		ASNDecodeReturn(ASNObject* obj, std::uint8_t len) : objects{ obj }, len{ len } {} 
		ASNObject* objects;
		std::uint8_t len;
		operator ASNObject* () { return objects; }
	};
	static ASNDecodeReturn DecodeAsn(const unsigned char* data, unsigned long size, std::uint8_t deep = 0) {
		unsigned long pos = 0;
		int amount = 0;
		while (pos < size) { ++amount; pos += ASNObject::GetSize(data + pos); };
		ASNObject *res = reinterpret_cast<ASNObject*>(std::malloc(amount * sizeof(ASNObject)));
		pos = 0;
		for (int i = 0; i < amount; ++i) {
			res[i] = ASNObject(data + pos, deep);
			pos += res[i].GetSize();
		}
		return ASNDecodeReturn(res, amount);
	}
	static unsigned long EncodingSize(const std::vector<unsigned char>& data) {
		return 1 + EncodeLenSize(data.size()) + data.size();
	}
	static const unsigned char* EncodeAsnPrimitives(const std::vector<unsigned char>& data, unsigned char* destionation = nullptr) {
		unsigned long header = 1 + EncodeLenSize(data.size());
		unsigned char *res;
		if (destionation) res = destionation;
		else res = new unsigned char[header + data.size()];
		res[0] = OCTASTRING;
		EncodeLen(data.size(), res + 1);
		memcpy(res + header, data.data(), data.size());
		return res;
	}
	static unsigned long EncodingSize(const int num) { // only support integer with < 128 byte size
		unsigned long len = sizeof(int);
		for (int i = sizeof(int) - 1; i >= 0; --i) {
			if (num & (0xFF << (i * 8)) || num & (0x80 << ((i - 1) * 8))) break;
			else --len;
		}
		return 2 + len;
	}
	static const unsigned char* EncodeAsnPrimitives(const int num, unsigned char* destination = nullptr) {
		unsigned long len = EncodingSize(num);
		unsigned char* res;
		if (destination) res = destination;
		else res = new unsigned char[len];
		res[0] = 0x02;
		res[1] = static_cast<unsigned char>(len - 2);
		const unsigned char* itr = reinterpret_cast<const unsigned char*>(&num);
		for (unsigned long i = 2; i < len; ++i, ++itr) res[i] = *itr;
		return res;
	}
	static unsigned long EncodingSize(const wchar_t* msg) {
		unsigned long len = (wcslen(msg) + 1) * sizeof(wchar_t);
		return 1 + EncodeLenSize(len) + len;
	}
	static unsigned char* EncodeAsnPrimitives(const wchar_t* msg, unsigned char* destination = nullptr) {
		unsigned long len = (wcslen(msg) + 1) * sizeof(wchar_t);
		unsigned long header = 1 + EncodeLenSize(len);
		unsigned char* res;
		if (destination) res = destination;
		else res = new unsigned char[len + header];
		res[0] = 0x0c;
		EncodeLen(len, res + 1);
		wcscpy(reinterpret_cast<wchar_t*>(res + header), msg);
		return res;
	}
	static unsigned long EncodingSize(const char* msg) {
		unsigned long len = strlen(msg) + 1;
		return 1 + EncodeLenSize(len) + len;
	}
	static const unsigned char* EncodeAsnPrimitives(const char* msg, unsigned char* destination = nullptr) {
		unsigned long len = (strlen(msg) + 1) * sizeof(char); // 0 terminated
		const unsigned long header = 1 + EncodeLenSize(len);
		unsigned char *res;
		if (destination) res = destination;
		else {
			unsigned long size = header + len;
			res = new unsigned char[size];
		}
		res[0] = 0x13; // asci string byte 
		EncodeLen(len, res + 1);
		strcpy(reinterpret_cast<char*>(res + header), msg);
		return res;
	}
};