#include "pfc.h"

#include <stdarg.h>

//implementations of deprecated string_printf methods, with a pragma to disable warnings when they reference other deprecated methods.

#ifndef _MSC_VER
#define _itoa_s itoa
#define _ultoa_s ultoa
#endif

#pragma warning(disable:4996)

namespace pfc {

void string_printf::run(const char * fmt,va_list list) {g_run(*this,fmt,list);}

string_printf_va::string_printf_va(const char * fmt,va_list list) {string_printf::g_run(*this,fmt,list);}

void string_printf::g_run(string_base & out,const char * fmt,va_list list)
{
	out.reset();
	while(*fmt)
	{
		if (*fmt=='%')
		{
			fmt++;
			if (*fmt=='%')
			{
				out.add_char('%');
				fmt++;
			}
			else
			{
				bool force_sign = false;
				if (*fmt=='+')
				{
					force_sign = true;
					fmt++;
				}
				char padchar = (*fmt == '0') ? '0' : ' ';
				t_size pad = 0;
				while(*fmt>='0' && *fmt<='9')
				{
					pad = pad * 10 + (*fmt - '0');
					fmt++;
				}

				if (*fmt=='s' || *fmt=='S')
				{
					const char * ptr = va_arg(list,const char*);
					t_size len = strlen(ptr);
					if (pad>len) out.add_chars(padchar,pad-len);
					out.add_string(ptr);
					fmt++;

				}
				else if (*fmt=='i' || *fmt=='I' || *fmt=='d' || *fmt=='D')
				{
					int val = va_arg(list,int);
					if (force_sign && val>0) out.add_char('+');
                    pfc::format_int temp( val );
					t_size len = strlen(temp);
					if (pad>len) out.add_chars(padchar,pad-len);
					out.add_string(temp);
					fmt++;
				}
				else if (*fmt=='u' || *fmt=='U')
				{
					int val = va_arg(list,int);
					if (force_sign && val>0) out.add_char('+');
                    pfc::format_uint temp(val);
					t_size len = strlen(temp);
					if (pad>len) out.add_chars(padchar,pad-len);
					out.add_string(temp);
					fmt++;
				}
				else if (*fmt=='x' || *fmt=='X')
				{
					int val = va_arg(list,int);
					if (force_sign && val>0) out.add_char('+');
                    pfc::format_uint temp(val, 0, 16);
					if (*fmt=='X')
					{
						char * t = const_cast< char* > ( temp.get_ptr() );
						while(*t)
						{
							if (*t>='a' && *t<='z')
								*t += 'A' - 'a';
							t++;
						}
					}
					t_size len = strlen(temp);
					if (pad>len) out.add_chars(padchar,pad-len);
					out.add_string(temp);
					fmt++;
				}
				else if (*fmt=='c' || *fmt=='C')
				{
					out.add_char(va_arg(list,int));
					fmt++;
				}
			}
		}
		else
		{
			out.add_char(*(fmt++));
		}
	}
}

string_printf::string_printf(const char * fmt,...)
{
	va_list list;
	va_start(list,fmt);
	run(fmt,list);
	va_end(list);
}


}
