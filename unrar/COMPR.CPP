#define NC 298  /* alphabet = {0, 1, 2, ..., NC - 1} */
#define DC 48
#define RC 28
#define BC 19
#define MC 257

enum {CODE_HUFFMAN=0,CODE_LZ=1,CODE_LZ2=2,CODE_REPEATLZ=3,CODE_CACHELZ=4,
      CODE_STARTFILE=5,CODE_ENDFILE=6,CODE_STARTMM=8,CODE_ENDMM=7,
      CODE_MMDELTA=9};

struct AudioVariables
{
  int K1,K2,K3,K4,K5;
  int D1,D2,D3,D4;
  int LastDelta;
  unsigned int Dif[11];
  unsigned int ByteCount;
  int LastChar;
};

unsigned int BlockSymCode,BlockSymLength;
unsigned char PackOldTable[MC*4];

extern int SolidType;

#include "unpack.cpp"
#include "crccrypt.cpp"
#include "unpold.cpp"

