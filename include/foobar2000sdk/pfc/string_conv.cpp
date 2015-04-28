#include "pfc.h"



namespace {
	template<typename t_char, bool isChecked = true> 
	class string_writer_t {
	public:
		string_writer_t(t_char * p_buffer,t_size p_size) : m_buffer(p_buffer), m_size(p_size), m_writeptr(0) {}
		
		void write(t_char p_char) {
			if (isChecked) {
				if (m_writeptr < m_size) {
					m_buffer[m_writeptr++] = p_char;
				}
			} else {
				m_buffer[m_writeptr++] = p_char;
			}
		}
		void write_multi(const t_char * p_buffer,t_size p_count) {
			if (isChecked) {
				const t_size delta = pfc::min_t<t_size>(p_count,m_size-m_writeptr);
				for(t_size n=0;n<delta;n++) {
					m_buffer[m_writeptr++] = p_buffer[n];
				}
			} else {
				for(t_size n = 0; n < p_count; ++n) {
					m_buffer[m_writeptr++] = p_buffer[n];
				}
			}
		}

		void write_as_utf8(unsigned p_char) {
			if (isChecked) {
				char temp[6];
				t_size n = pfc::utf8_encode_char(p_char,temp);
				write_multi(temp,n);
			} else {
				m_writeptr += pfc::utf8_encode_char(p_char, m_buffer + m_writeptr);
			}
		}

		void write_as_wide(unsigned p_char) {
			if (isChecked) {
				wchar_t temp[2];
				t_size n = pfc::wide_encode_char(p_char,temp);
				write_multi(temp,n);
			} else {
				m_writeptr += pfc::wide_encode_char(p_char, m_buffer + m_writeptr);
			}
		}

		t_size finalize() {
			if (isChecked) {
				if (m_size == 0) return 0;
				t_size terminator = pfc::min_t<t_size>(m_writeptr,m_size-1);
				m_buffer[terminator] = 0;
				return terminator;
			} else {
				m_buffer[m_writeptr] = 0;
				return m_writeptr;
			}
		}
		bool is_overrun() const {
			return m_writeptr >= m_size;
		}
	private:
		t_char * m_buffer;
		t_size m_size;
		t_size m_writeptr;
	};



    
    static const uint16_t mappings1252[] = {
        /*0x80*/	0x20AC, //	#EURO SIGN
        /*0x81*/	0,      //	#UNDEFINED
        /*0x82*/	0x201A, //	#SINGLE LOW-9 QUOTATION MARK
        /*0x83*/	0x0192, //	#LATIN SMALL LETTER F WITH HOOK
        /*0x84*/	0x201E, //	#DOUBLE LOW-9 QUOTATION MARK
        /*0x85*/	0x2026, //	#HORIZONTAL ELLIPSIS
        /*0x86*/	0x2020, //	#DAGGER
        /*0x87*/	0x2021, //	#DOUBLE DAGGER
        /*0x88*/	0x02C6, //	#MODIFIER LETTER CIRCUMFLEX ACCENT
        /*0x89*/	0x2030, //	#PER MILLE SIGN
        /*0x8A*/	0x0160, //	#LATIN CAPITAL LETTER S WITH CARON
        /*0x8B*/	0x2039, //	#SINGLE LEFT-POINTING ANGLE QUOTATION MARK
        /*0x8C*/	0x0152, //	#LATIN CAPITAL LIGATURE OE
        /*0x8D*/	0,      // 	#UNDEFINED
        /*0x8E*/	0x017D, //	#LATIN CAPITAL LETTER Z WITH CARON
        /*0x8F*/	0,      // 	#UNDEFINED
        /*0x90*/	0,      //  #UNDEFINED
        /*0x91*/	0x2018, //  #LEFT SINGLE QUOTATION MARK
        /*0x92*/	0x2019, //  #RIGHT SINGLE QUOTATION MARK
        /*0x93*/	0x201C, //  #LEFT DOUBLE QUOTATION MARK
        /*0x94*/	0x201D, //  #RIGHT DOUBLE QUOTATION MARK
        /*0x95*/	0x2022, //	#BULLET
        /*0x96*/	0x2013, //	#EN DASH
        /*0x97*/	0x2014, //	#EM DASH
        /*0x98*/	0x02DC, //  #SMALL TILDE
        /*0x99*/	0x2122, //	#TRADE MARK SIGN
        /*0x9A*/	0x0161, //	#LATIN SMALL LETTER S WITH CARON
        /*0x9B*/	0x203A, //	#SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
        /*0x9C*/	0x0153, //	#LATIN SMALL LIGATURE OE
        /*0x9D*/	0,      //	#UNDEFINED
        /*0x9E*/	0x017E, //	#LATIN SMALL LETTER Z WITH CARON
        /*0x9F*/	0x0178, //	#LATIN CAPITAL LETTER Y WITH DIAERESIS
    };
    
    static bool charImport1252(uint32_t & unichar, char c) {
        uint8_t uc = (uint8_t) c;
        if (uc == 0) return false;
        else if (uc < 0x80) {unichar = uc; return true;}
        else if (uc < 0xA0) {
            uint32_t t = mappings1252[uc-0x80];
            if (t == 0) return false;
            unichar = t; return true;
        } else {
            unichar = uc; return true;
        }
    }
    
    static bool charExport1252(char & c, uint32_t unichar) {
        if (unichar == 0) return false;
        else if (unichar < 0x80 || (unichar >= 0xa0 && unichar <= 0xFF)) {c = (char)(uint8_t)unichar; return true;}
        for(size_t walk = 0; walk < PFC_TABSIZE(mappings1252); ++walk) {
            if (unichar == mappings1252[walk]) {
                c = (char)(uint8_t)(walk + 0x80);
                return true;
            }
        }
        return false;
    }
    struct asciiMap_t {uint16_t from; uint8_t to;};
    static const asciiMap_t g_asciiMap[] = {
        {160,32},{161,33},{162,99},{164,36},{165,89},{166,124},{169,67},{170,97},{171,60},{173,45},{174,82},{178,50},{179,51},{183,46},{184,44},{185,49},{186,111},{187,62},{192,65},{193,65},{194,65},{195,65},{196,65},{197,65},{198,65},{199,67},{200,69},{201,69},{202,69},{203,69},{204,73},{205,73},{206,73},{207,73},{208,68},{209,78},{210,79},{211,79},{212,79},{213,79},{214,79},{216,79},{217,85},{218,85},{219,85},{220,85},{221,89},{224,97},{225,97},{226,97},{227,97},{228,97},{229,97},{230,97},{231,99},{232,101},{233,101},{234,101},{235,101},{236,105},{237,105},{238,105},{239,105},{241,110},{242,111},{243,111},{244,111},{245,111},{246,111},{248,111},{249,117},{250,117},{251,117},{252,117},{253,121},{255,121},{256,65},{257,97},{258,65},{259,97},{260,65},{261,97},{262,67},{263,99},{264,67},{265,99},{266,67},{267,99},{268,67},{269,99},{270,68},{271,100},{272,68},{273,100},{274,69},{275,101},{276,69},{277,101},{278,69},{279,101},{280,69},{281,101},{282,69},{283,101},{284,71},{285,103},{286,71},{287,103},{288,71},{289,103},{290,71},{291,103},{292,72},{293,104},{294,72},{295,104},{296,73},{297,105},{298,73},{299,105},{300,73},{301,105},{302,73},{303,105},{304,73},{305,105},{308,74},{309,106},{310,75},{311,107},{313,76},{314,108},{315,76},{316,108},{317,76},{318,108},{321,76},{322,108},{323,78},{324,110},{325,78},{326,110},{327,78},{328,110},{332,79},{333,111},{334,79},{335,111},{336,79},{337,111},{338,79},{339,111},{340,82},{341,114},{342,82},{343,114},{344,82},{345,114},{346,83},{347,115},{348,83},{349,115},{350,83},{351,115},{352,83},{353,115},{354,84},{355,116},{356,84},{357,116},{358,84},{359,116},{360,85},{361,117},{362,85},{363,117},{364,85},{365,117},{366,85},{367,117},{368,85},{369,117},{370,85},{371,117},{372,87},{373,119},{374,89},{375,121},{376,89},{377,90},{378,122},{379,90},{380,122},{381,90},{382,122},{384,98},{393,68},{401,70},{402,102},{407,73},{410,108},{415,79},{416,79},{417,111},{427,116},{430,84},{431,85},{432,117},{438,122},{461,65},{462,97},{463,73},{464,105},{465,79},{466,111},{467,85},{468,117},{469,85},{470,117},{471,85},{472,117},{473,85},{474,117},{475,85},{476,117},{478,65},{479,97},{484,71},{485,103},{486,71},{487,103},{488,75},{489,107},{490,79},{491,111},{492,79},{493,111},{496,106},{609,103},{697,39},{698,34},{700,39},{708,94},{710,94},{712,39},{715,96},{717,95},{732,126},{768,96},{770,94},{771,126},{782,34},{817,95},{818,95},{8192,32},{8193,32},{8194,32},{8195,32},{8196,32},{8197,32},{8198,32},{8208,45},{8209,45},{8211,45},{8212,45},{8216,39},{8217,39},{8218,44},{8220,34},{8221,34},{8222,34},{8226,46},{8230,46},{8242,39},{8245,96},{8249,60},{8250,62},{8482,84},{65281,33},{65282,34},{65283,35},{65284,36},{65285,37},{65286,38},{65287,39},{65288,40},{65289,41},{65290,42},{65291,43},{65292,44},{65293,45},{65294,46},{65295,47},{65296,48},{65297,49},{65298,50},{65299,51},{65300,52},{65301,53},{65302,54},{65303,55},{65304,56},{65305,57},{65306,58},{65307,59},{65308,60},{65309,61},{65310,62},{65312,64},{65313,65},{65314,66},{65315,67},{65316,68},{65317,69},{65318,70},{65319,71},{65320,72},{65321,73},{65322,74},{65323,75},{65324,76},{65325,77},{65326,78},{65327,79},{65328,80},{65329,81},{65330,82},{65331,83},{65332,84},{65333,85},{65334,86},{65335,87},{65336,88},{65337,89},{65338,90},{65339,91},{65340,92},{65341,93},{65342,94},{65343,95},{65344,96},{65345,97},{65346,98},{65347,99},{65348,100},{65349,101},{65350,102},{65351,103},{65352,104},{65353,105},{65354,106},{65355,107},{65356,108},{65357,109},{65358,110},{65359,111},{65360,112},{65361,113},{65362,114},{65363,115},{65364,116},{65365,117},{65366,118},{65367,119},{65368,120},{65369,121},{65370,122},{65371,123},{65372,124},{65373,125},{65374,126}};
    
}

namespace pfc {
    namespace stringcvt {
        
        char charToASCII( unsigned c ) {
            if (c < 128) return (char)c;
            unsigned lo = 0, hi = PFC_TABSIZE(g_asciiMap);
            while( lo < hi ) {
                const unsigned mid = (lo + hi) / 2;
                const asciiMap_t entry = g_asciiMap[mid];
                if ( c > entry.from ) {
                    lo = mid + 1;
                } else if (c < entry.from) {
                    hi = mid;
                } else {
                    return (char)entry.to;
                }
            }
            return '?';
        }
        
		t_size convert_utf8_to_wide(wchar_t * p_out,t_size p_out_size,const char * p_in,t_size p_in_size) {
			const t_size insize = p_in_size;
			t_size inptr = 0;
			string_writer_t<wchar_t> writer(p_out,p_out_size);
            
			while(inptr < insize && !writer.is_overrun()) {
				unsigned newchar = 0;
				t_size delta = utf8_decode_char(p_in + inptr,newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				PFC_ASSERT(inptr + delta <= insize);
				inptr += delta;
				writer.write_as_wide(newchar);
			}
            
			return writer.finalize();
		}
        
		t_size convert_utf8_to_wide_unchecked(wchar_t * p_out,const char * p_in) {
			t_size inptr = 0;
			string_writer_t<wchar_t,false> writer(p_out,~0);
            
			while(!writer.is_overrun()) {
				unsigned newchar = 0;
				t_size delta = utf8_decode_char(p_in + inptr,newchar);
				if (delta == 0 || newchar == 0) break;
				inptr += delta;
				writer.write_as_wide(newchar);
			}
            
			return writer.finalize();
		}
        
		t_size convert_wide_to_utf8(char * p_out,t_size p_out_size,const wchar_t * p_in,t_size p_in_size) {
			const t_size insize = p_in_size;
			t_size inptr = 0;
			string_writer_t<char> writer(p_out,p_out_size);
            
			while(inptr < insize && !writer.is_overrun()) {
				unsigned newchar = 0;
				t_size delta = wide_decode_char(p_in + inptr,&newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				PFC_ASSERT(inptr + delta <= insize);
				inptr += delta;
				writer.write_as_utf8(newchar);
			}
            
			return writer.finalize();
		}
        
		t_size estimate_utf8_to_wide(const char * p_in) {
			t_size inptr = 0;
			t_size retval = 1;//1 for null terminator
			for(;;) {
				unsigned newchar = 0;
				t_size delta = utf8_decode_char(p_in + inptr,newchar);
				if (delta == 0 || newchar == 0) break;
				inptr += delta;
				
				{
					wchar_t temp[2];
					retval += wide_encode_char(newchar,temp);
				}
			}
			return retval;
		}
        
		t_size estimate_utf8_to_wide(const char * p_in,t_size p_in_size) {
			const t_size insize = p_in_size;
			t_size inptr = 0;
			t_size retval = 1;//1 for null terminator
			while(inptr < insize) {
				unsigned newchar = 0;
				t_size delta = utf8_decode_char(p_in + inptr,newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				PFC_ASSERT(inptr + delta <= insize);
				inptr += delta;
				
				{
					wchar_t temp[2];
					retval += wide_encode_char(newchar,temp);
				}
			}
			return retval;
		}
        
		t_size estimate_wide_to_utf8(const wchar_t * p_in,t_size p_in_size) {
			const t_size insize = p_in_size;
			t_size inptr = 0;
			t_size retval = 1;//1 for null terminator
			while(inptr < insize) {
				unsigned newchar = 0;
				t_size delta = wide_decode_char(p_in + inptr,&newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				PFC_ASSERT(inptr + delta <= insize);
				inptr += delta;
				
				{
					char temp[6];
					delta = utf8_encode_char(newchar,temp);
					if (delta == 0) break;
					retval += delta;
				}
			}
			return retval;
		}
        
        t_size estimate_wide_to_win1252( const wchar_t * p_in, t_size p_in_size ) {
			const t_size insize = p_in_size;
			t_size inptr = 0;
			t_size retval = 1;//1 for null terminator
			while(inptr < insize) {
				unsigned newchar = 0;
				t_size delta = wide_decode_char(p_in + inptr,&newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				PFC_ASSERT(inptr + delta <= insize);
				inptr += delta;
				
                ++retval;
			}
			return retval;
            
        }
        
        t_size convert_wide_to_win1252( char * p_out, t_size p_out_size, const wchar_t * p_in, t_size p_in_size ) {
			const t_size insize = p_in_size;
			t_size inptr = 0;
			string_writer_t<char> writer(p_out,p_out_size);
            
			while(inptr < insize && !writer.is_overrun()) {
				unsigned newchar = 0;
				t_size delta = wide_decode_char(p_in + inptr,&newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				PFC_ASSERT(inptr + delta <= insize);
				inptr += delta;
                
                char temp;
                if (!charExport1252( temp, newchar )) temp = '?';
                writer.write( temp );
			}
            
			return writer.finalize();
            
        }
        
        t_size estimate_win1252_to_wide( const char * p_source, t_size p_source_size ) {
            return strlen_max_t( p_source, p_source_size ) + 1;
        }
        
        t_size convert_win1252_to_wide( wchar_t * p_out, t_size p_out_size, const char * p_in, t_size p_in_size ) {
			const t_size insize = p_in_size;
			t_size inptr = 0;
			string_writer_t<wchar_t> writer(p_out,p_out_size);
            
			while(inptr < insize && !writer.is_overrun()) {
                char inChar = p_in[inptr];
                if (inChar == 0) break;
                ++inptr;
                
                unsigned out;
                if (!charImport1252( out , inChar )) out = '?';
                writer.write_as_wide( out );
			}
            
			return writer.finalize();
        }
        
        t_size estimate_utf8_to_win1252( const char * p_in, t_size p_in_size ) {
			const t_size insize = p_in_size;
			t_size inptr = 0;
			t_size retval = 1;//1 for null terminator
			while(inptr < insize) {
				unsigned newchar = 0;
				t_size delta = utf8_decode_char(p_in + inptr,newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				PFC_ASSERT(inptr + delta <= insize);
				inptr += delta;
				
                ++retval;
			}
			return retval;
        }
        t_size convert_utf8_to_win1252( char * p_out, t_size p_out_size, const char * p_in, t_size p_in_size ) {
			const t_size insize = p_in_size;
			t_size inptr = 0;
			string_writer_t<char> writer(p_out,p_out_size);
            
			while(inptr < insize && !writer.is_overrun()) {
				unsigned newchar = 0;
				t_size delta = utf8_decode_char(p_in + inptr,newchar,insize - inptr);
				if (delta == 0 || newchar == 0) break;
				PFC_ASSERT(inptr + delta <= insize);
				inptr += delta;
                
                char temp;
                if (!charExport1252( temp, newchar )) temp = '?';
                writer.write( temp );
			}
            
			return writer.finalize();
        }
        
        t_size estimate_win1252_to_utf8( const char * p_in, t_size p_in_size ) {
            const size_t insize = p_in_size;
            t_size inptr = 0;
            t_size retval = 1; // 1 for null terminator
            while(inptr < insize) {
                unsigned newchar;
                char c = p_in[inptr];
                if (c == 0) break;
				++inptr;
                if (!charImport1252( newchar, c)) newchar = '?';
                
                char temp[6];
                retval += pfc::utf8_encode_char( newchar, temp );
            }
            return retval;
        }
        
        t_size convert_win1252_to_utf8( char * p_out, t_size p_out_size, const char * p_in, t_size p_in_size ) {
			const t_size insize = p_in_size;
			t_size inptr = 0;
			string_writer_t<char> writer(p_out,p_out_size);
            
			while(inptr < insize && !writer.is_overrun()) {
                char inChar = p_in[inptr];
                if (inChar == 0) break;
                ++inptr;
                
                unsigned out;
                if (!charImport1252( out , inChar )) out = '?';
                writer.write_as_utf8( out );
			}
            
			return writer.finalize();
        }

        t_size estimate_utf8_to_ascii( const char * p_source, t_size p_source_size ) {
            return estimate_utf8_to_win1252( p_source, p_source_size );
        }
        t_size convert_utf8_to_ascii( char * p_out, t_size p_out_size, const char * p_in, t_size p_in_size ) {
            const t_size insize = p_in_size;
            t_size inptr = 0;
            string_writer_t<char> writer(p_out,p_out_size);
            
            while(inptr < insize && !writer.is_overrun()) {
                unsigned newchar = 0;
                t_size delta = utf8_decode_char(p_in + inptr,newchar,insize - inptr);
                if (delta == 0 || newchar == 0) break;
                PFC_ASSERT(inptr + delta <= insize);
                inptr += delta;
                
                writer.write( charToASCII(newchar) );
            }
            
            return writer.finalize();
        }

    }
}

#ifdef _WINDOWS


namespace pfc {
	namespace stringcvt {


		t_size convert_codepage_to_wide(unsigned p_codepage,wchar_t * p_out,t_size p_out_size,const char * p_source,t_size p_source_size) {
			if (p_out_size == 0) return 0;
			memset(p_out,0,p_out_size * sizeof(*p_out));
			MultiByteToWideChar(p_codepage,0,p_source,p_source_size,p_out,p_out_size);
			p_out[p_out_size-1] = 0;
			return wcslen(p_out);
		}

		t_size convert_wide_to_codepage(unsigned p_codepage,char * p_out,t_size p_out_size,const wchar_t * p_source,t_size p_source_size) {
			if (p_out_size == 0) return 0;
			memset(p_out,0,p_out_size * sizeof(*p_out));
			WideCharToMultiByte(p_codepage,0,p_source,p_source_size,p_out,p_out_size,0,FALSE);
			p_out[p_out_size-1] = 0;
			return strlen(p_out);
		}

		t_size estimate_codepage_to_wide(unsigned p_codepage,const char * p_source,t_size p_source_size) {
			return MultiByteToWideChar(p_codepage,0,p_source,strlen_max(p_source,p_source_size),0,0) + 1;
		}
		t_size estimate_wide_to_codepage(unsigned p_codepage,const wchar_t * p_source,t_size p_source_size) {
			return WideCharToMultiByte(p_codepage,0,p_source,wcslen_max(p_source,p_source_size),0,0,0,FALSE) + 1;
		}
	}

}

#endif //_WINDOWS
