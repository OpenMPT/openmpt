#include "pfc.h"

namespace bitWriter {
	static void set_bit(t_uint8 * p_stream,size_t p_offset, bool state) {
		t_uint8 mask = 1 << (7-(p_offset&7));
		t_uint8 & byte = p_stream[p_offset>>3];
		byte = (byte & ~mask) | (state ? mask : 0);
	}
	static void set_bits(t_uint8 * stream, t_size offset, t_size word, t_size bits) {
		for(t_size walk = 0; walk < bits; ++walk) {
			t_uint8 bit = (t_uint8)((word >> (bits - walk - 1))&1);
			set_bit(stream, offset+walk, bit != 0);
		}
	}
};

namespace pfc {
	t_size base64_decode_estimate(const char * text) {
		t_size textLen = strlen(text);
		if (textLen % 4 != 0) throw pfc::exception_invalid_params();
		t_size outLen = (textLen / 4) * 3;
		
		if (textLen >= 4) {
			if (text[textLen-1] == '=') {
				textLen--; outLen--;
				if (text[textLen-1] == '=') {
					textLen--; outLen--;
				}
			}
		}
		return outLen;
	}

	

	void base64_decode(const char * text, void * out) {
		static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		t_uint8 alphabetRev[256];
		for(t_size walk = 0; walk < PFC_TABSIZE(alphabetRev); ++walk) alphabetRev[walk] = 0xFF;
		for(t_size walk = 0; walk < PFC_TABSIZE(alphabet); ++walk) alphabetRev[alphabet[walk]] = (t_uint8)walk;
		const t_size textLen = strlen(text);

		if (textLen % 4 != 0) throw pfc::exception_invalid_params();
		if (textLen == 0) return;
		
		t_size outWritePtr = 0;

		{
			const t_size max = textLen - 4;
			t_size textWalk = 0;
			for(; textWalk < max; textWalk ++) {
				const t_uint8 v = alphabetRev[(t_uint8)text[textWalk]];
				if (v == 0xFF) throw pfc::exception_invalid_params();
				bitWriter::set_bits(reinterpret_cast<t_uint8*>(out),outWritePtr,v,6);
				outWritePtr += 6;
			}

			t_uint8 temp[3];
			t_size tempWritePtr = 0;
			for(; textWalk < textLen; textWalk ++) {
				const char c = text[textWalk];
				if (c == '=') break;
				const t_uint8 v = alphabetRev[(t_uint8)c];
				if (v == 0xFF) throw pfc::exception_invalid_params();
				bitWriter::set_bits(temp,tempWritePtr,v,6);
				tempWritePtr += 6;
			}
			for(; textWalk < textLen; textWalk ++) {
				if (text[textWalk] != '=') throw pfc::exception_invalid_params();
			}
			memcpy(reinterpret_cast<t_uint8*>(out) + (outWritePtr/8), temp, tempWritePtr/8);
		}
	}
	void base64_encode(pfc::string_base & out, const void * in, t_size inSize) {
		out.reset(); base64_encode_append(out, in, inSize);
	}
	void base64_encode_append(pfc::string_base & out, const void * in, t_size inSize) {
		static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		int shift = 0;
		int accum = 0;
		const t_uint8 * inPtr = reinterpret_cast<const t_uint8*>(in);

		for(t_size walk = 0; walk < inSize; ++walk) {
			accum <<= 8;
			shift += 8;
			accum |= inPtr[walk];
			while ( shift >= 6 ) {
				shift -= 6;
				out << format_char( alphabet[(accum >> shift) & 0x3F] );
			}
		}
		if (shift == 4) {
			out << format_char( alphabet[(accum & 0xF)<<2] ) << "=";
		} else if (shift == 2) {
			out << format_char( alphabet[(accum & 0x3)<<4] ) << "==";
		}
	}

}
