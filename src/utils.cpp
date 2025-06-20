#include "utils.h"

#include <stdio.h>
#include <stdarg.h>

//void CpuFastSet( const void *source,  void *dest, u32 mode)
//{
//	SystemCall(12);
//}

//Print's a null-terminateed string to VBA's debugging console
void VBAPrint(char *s)
{
    //__asm("mov r0, %0;"
    asm volatile("mov r0, %0;"
                  "swi 0xff0000;"
                  : // no ouput
                  : "r" (s)
                  : "r0");

}

//Print's a formatted null-terminateed string to VBA's debugging console
void VBAPrintf(const char* a_Format, ...)
{
    char DebugBuffer[256];
    
    va_list args;

    va_start(args, a_Format);
    vsprintf(DebugBuffer, a_Format, args);
    VBAPrint(DebugBuffer);
    va_end(args);
}

/**
	 * C++ version 0.4 char* style "itoa":
	 * Written by Lukás Chmela
	 * Released under GPLv3.
	 */
	char* itoa(int value, char* result, int base) {
		// check that the base if valid
		if (base < 2 || base > 36) { *result = '\0'; return result; }

		char* ptr = result, *ptr1 = result, tmp_char;
		int tmp_value;

		do {
			tmp_value = value;
			value /= base;
			*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
		} while ( value );

		// Apply negative sign
		if (tmp_value < 0) *ptr++ = '-';
		*ptr-- = '\0';
		while(ptr1 < ptr) {
			tmp_char = *ptr;
			*ptr--= *ptr1;
			*ptr1++ = tmp_char;
		}
		return result;
	}
