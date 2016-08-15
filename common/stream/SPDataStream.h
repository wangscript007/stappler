/*
 * SPDataStream.h
 *
 *  Created on: 30 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_DATA_SPDATASTREAM_H_
#define COMMON_DATA_SPDATASTREAM_H_

#include "SPIO.h"
#include "SPData.h"
#include "SPTransferBuffer.h"

NS_SP_EXT_BEGIN(data)

class JsonBuffer;
class CborBuffer;
class DecompressBuffer;
class DecryptBuffer;

class StreamBuffer : public std::basic_streambuf<char, std::char_traits<char>> {
public:
	using traits_type = std::char_traits<char>;
	using size_type = size_t;
	using string_type = String;
	using char_type = char;
	using int_type = typename traits_type::int_type;
	using streambuf_type = std::basic_streambuf<char, std::char_traits<char>>;

	using streamsize = std::streamsize;
	using streamoff = std::streamoff;

	enum class Type {
		Undefined,
		Decrypt,
		Decompress,
		Cbor,
		Json,
	};

	StreamBuffer();
	virtual ~StreamBuffer();

	StreamBuffer(StreamBuffer &&);
	StreamBuffer & operator = (StreamBuffer &&);

	StreamBuffer(const StreamBuffer &) = delete;
	StreamBuffer & operator=(const StreamBuffer &) = delete;

	void swap(StreamBuffer &);

	void clear();
	bool empty() const;

	const data::Value &data() const;
	data::Value &data();

	size_t bytesWritten() const { return _bytesWritten; }

protected:
	bool header(const uint8_t* s, bool send);
	streamsize read(const uint8_t* s, streamsize count);
	streamsize rxsputn(const uint8_t* s, streamsize count);

	virtual streamsize xsputn(const char_type* s, streamsize count) override;
	virtual int_type overflow(int_type ch) override;
	virtual int sync() override;

	size_t _bytesWritten = 0;
	size_t _headerBytes = 0;
	std::array<char, 4> _header;
	std::array<char, 256> _buffer;
	Type _type = Type::Undefined;

	union {
		JsonBuffer * _json;
		CborBuffer * _cbor;
		DecompressBuffer * _comp;
		DecryptBuffer * _crypt;
	};
};

class Stream : public std::basic_ostream<char, std::char_traits<char>>, public AllocBase {
public:
	using char_type = char;
	using traits_type = std::char_traits<char>;

	using int_type = typename traits_type::int_type;
	using pos_type = typename traits_type::pos_type;
	using off_type = typename traits_type::off_type;
	using size_type = size_t;

	using ostream_type = std::basic_ostream<char_type, traits_type>;
	using buffer_type = StreamBuffer;

public:
	Stream() : ostream_type() {
		this->init(&_morph);
	}

	Stream(Stream && rhs) : ostream_type(std::move(rhs)), _morph(std::move(rhs._morph)) {
		ostream_type::set_rdbuf(&_morph);
	}

	Stream & operator=(Stream && rhs) {
		ostream_type::operator=(std::move(rhs));
		_morph = std::move(rhs._morph);
		ostream_type::set_rdbuf(&_morph);
		return *this;
	}

	Stream(const Stream &) = delete;
	Stream & operator=(const Stream &) = delete;

	void swap(Stream & rhs) {
		ostream_type::swap(rhs);
		_morph.swap(rhs._morph);
	}

	buffer_type * rdbuf() const {
		return const_cast<buffer_type *>(&_morph);
	}

	const data::Value &data() const { return _morph.data(); }
	data::Value &data() { return _morph.data(); }

protected:
	StreamBuffer _morph;
};

NS_SP_EXT_END(data)

#endif /* COMMON_DATA_SPDATASTREAM_H_ */
