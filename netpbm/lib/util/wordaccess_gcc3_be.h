/*=============================================================================
  This file is the part of wordaccess.h for use under these
  conditions:

  * GCC (>=3.4), GLIBC
  * Big-Endian machines
*===========================================================================*/

typedef unsigned long int wordint;
typedef unsigned char wordintBytes[sizeof(wordint)];

static __inline__ wordint
bytesToWordint(wordintBytes bytes) {
    return *((wordint *)bytes);
}



static __inline__ void
wordintToBytes(wordintBytes * const bytesP,
               wordint        const wordInt) {
    *(wordint *)bytesP = wordInt;
}
