#ifndef UTILS_H
#define UTILS_H

/*Define SystemCall function for bios calls (This is already in libgba but pimpmobile
refuses to know that it's there and therefore doesn't compile unless I put it inside
my project itself*/
#if	defined	( __thumb__ )
#define	SystemCall(Number)	 __asm ("SWI	  "#Number"\n" :::  "r0", "r1", "r2", "r3")
#else
#define	SystemCall(Number)	 __asm ("SWI	  "#Number"	<< 16\n" :::"r0", "r1", "r2", "r3")
#endif

//The CpuFastSet function is the one that libpimpmobile can't see in libtonc
//void CpuFastSet( const void *source,  void *dest, u32 mode);

//Debugging functions for use in the VBA emulator. Print to the VBA debugging console.
void VBAPrint(char *s);
void VBAPrintf(const char* a_Format, ...);

//Custom number to string converter
char* itoa(int value, char* result, int base);

#endif
