
#define NC 298  /* alphabet = {0, 1, 2, ..., NC - 1} */
#define DC 48
#define RC 28
#define BC 19
#define MC 257

struct Decode
{
  unsigned int MaxNum;
  unsigned int DecodeLen[16];
  unsigned int DecodePos[16];
  unsigned int DecodeNum[2];
};

struct AudioVariables AudV[4];

#define GetBits()                                                 \
        BitField = ( ( ( (UDWORD)InBuf[InAddr]   << 16 ) |        \
                       ( (UWORD) InBuf[InAddr+1] <<  8 ) |        \
                       (         InBuf[InAddr+2]       ) )        \
                       >> (8-InBit) ) & 0xffff;


#define AddBits(Bits)                          \
        InAddr += ( InBit + (Bits) ) >> 3;     \
        InBit  =  ( InBit + (Bits) ) &  7;

static unsigned char *UnpBuf;
unsigned int UnpPtr,WrPtr;
static unsigned int BitField;
static unsigned int Number;

unsigned char InBuf[8192];

unsigned char UnpOldTable[MC*4];

unsigned int InAddr,InBit,ReadTop;

unsigned int LastDist,LastLength;
static unsigned int Length,Distance;

unsigned int OldDist[4],OldDistPtr;


struct LitDecode
{
  unsigned int MaxNum;
  unsigned int DecodeLen[16];
  unsigned int DecodePos[16];
  unsigned int DecodeNum[NC];
} LD;

struct DistDecode
{
  unsigned int MaxNum;
  unsigned int DecodeLen[16];
  unsigned int DecodePos[16];
  unsigned int DecodeNum[DC];
} DD;

struct RepDecode
{
  unsigned int MaxNum;
  unsigned int DecodeLen[16];
  unsigned int DecodePos[16];
  unsigned int DecodeNum[RC];
} RD;

struct MultDecode
{
  unsigned int MaxNum;
  unsigned int DecodeLen[16];
  unsigned int DecodePos[16];
  unsigned int DecodeNum[MC];
} MD[4];

struct BitDecode
{
  unsigned int MaxNum;
  unsigned int DecodeLen[16];
  unsigned int DecodePos[16];
  unsigned int DecodeNum[BC];
} BD;

static struct MultDecode *MDPtr[4]={&MD[0],&MD[1],&MD[2],&MD[3]};

int UnpAudioBlock,UnpChannels,CurChannel,ChannelDelta;


extern void SetPackAudioVars(struct AudioVariables *AudV,int CurChannel,int ChnDelta,int UnpChannels);
extern long DestUnpSize;

static void MakeDecodeTables(unsigned char *LenTab,struct Decode *Dec,int Size);
static void DecodeNumber(struct Decode *Dec);
void CopyString(void);
void UnpInitData(int Solid);
UBYTE DecodeAudio(int Delta);

void CRarArchive::Unpack(unsigned char *UnpAddr,int Solid)
//--------------------------------------------------------
{
  static unsigned char LDecode[]={0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224};
  static unsigned char LBits[]=  {0,0,0,0,0,0,0,0,1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5};
  static int DDecode[]={0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072,4096,6144,8192,12288,16384,24576,32768U,49152U,65536,98304,131072,196608,262144,327680,393216,458752,524288,589824,655360,720896,786432,851968,917504,983040};
  static unsigned char DBits[]=  {0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13,    14,    14,   15,   15,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16};
  static unsigned char SDDecode[]={0,4,8,16,32,64,128,192};
  static unsigned char SDBits[]={2,2,3, 4, 5, 6,  6,  6};
  unsigned int Bits;

  UnpBuf=UnpAddr;
  {
    UnpInitData(Solid);
    UnpReadBuf(1);
    if (!Solid)
      ReadTables();
    DestUnpSize--;
  }

  while (DestUnpSize>=0)
  {
    UnpPtr&=MAXWINMASK;

    if (InAddr>sizeof(InBuf)-30)
      UnpReadBuf(0);
    if (((WrPtr-UnpPtr) & MAXWINMASK)<270 && WrPtr!=UnpPtr)
    {
      UnpWriteBuf();
    }
    if (UnpAudioBlock)
    {
      DecodeNumber((struct Decode *)MDPtr[CurChannel]);
      if (Number==256)
      {
        ReadTables();
        continue;
      }
      UnpBuf[UnpPtr++]=DecodeAudio(Number);
      if (++CurChannel==UnpChannels)
        CurChannel=0;
      DestUnpSize--;
      continue;
    }

    DecodeNumber((struct Decode *)&LD);
    if (Number<256)
    {
      UnpBuf[UnpPtr++]=(UBYTE)Number;
      DestUnpSize--;
      continue;
    }
    if (Number>269)
    {
      Length=LDecode[Number-=270]+3;
      if ((Bits=LBits[Number])>0)
      {
        GetBits();
        Length+=BitField>>(16-Bits);
        AddBits(Bits);
      }

      DecodeNumber((struct Decode *)&DD);
      Distance=DDecode[Number]+1;
      if ((Bits=DBits[Number])>0)
      {
        GetBits();
        Distance+=BitField>>(16-Bits);
        AddBits(Bits);
      }

      if (Distance>=0x40000L)
        Length++;

      if (Distance>=0x2000)
        Length++;

      CopyString();
      continue;
    }
    if (Number==269)
    {
      ReadTables();
      continue;
    }
    if (Number==256)
    {
      Length=LastLength;
      Distance=LastDist;
      CopyString();
      continue;
    }
    if (Number<261)
    {
      Distance=OldDist[(OldDistPtr-(Number-256)) & 3];
      DecodeNumber((struct Decode *)&RD);
      Length=LDecode[Number]+2;
      if ((Bits=LBits[Number])>0)
      {
        GetBits();
        Length+=BitField>>(16-Bits);
        AddBits(Bits);
      }
      if (Distance>=0x40000)
        Length++;
      if (Distance>=0x2000)
        Length++;
      if (Distance>=0x101)
        Length++;
      CopyString();
      continue;
    }
    if (Number<270)
    {
      Distance=SDDecode[Number-=261]+1;
      if ((Bits=SDBits[Number])>0)
      {
        GetBits();
        Distance+=BitField>>(16-Bits);
        AddBits(Bits);
      }
      Length=2;
      CopyString();
      continue;
   }
  }
  ReadLastTables();
  UnpWriteBuf();
}


void CRarArchive::UnpReadBuf(int FirstBuf)
//----------------------------------------
{
	int RetCode;
	if (FirstBuf)
	{
		ReadTop=UnpRead(InBuf,sizeof(InBuf));
		InAddr=0;
	} else
	{
		memcpy(InBuf,&InBuf[sizeof(InBuf)-32],32);
		InAddr&=0x1f;
		RetCode=UnpRead(&InBuf[32],sizeof(InBuf)-32);
		if (RetCode>0)
			ReadTop=RetCode+32;
		else
			ReadTop=InAddr;
	}
}


void CRarArchive::UnpWriteBuf()
//-----------------------------
{
	if (UnpPtr<WrPtr)
	{
		UnpWrite(&UnpBuf[WrPtr],-(LONG)WrPtr & MAXWINMASK);
		UnpWrite(UnpBuf,UnpPtr);
	} else
	{
		UnpWrite(&UnpBuf[WrPtr],UnpPtr-WrPtr);
	}
	WrPtr=UnpPtr;
}



void CRarArchive::ReadTables()
//----------------------------
{
  UBYTE BitLength[BC];
  unsigned char Table[MC*4];
  int TableSize,N,I;
  if (InAddr>sizeof(InBuf)-25)
    UnpReadBuf(0);
  GetBits();
  UnpAudioBlock=(BitField & 0x8000);

  if (!(BitField & 0x4000))
    memset(UnpOldTable,0,sizeof(UnpOldTable));
  AddBits(2);


  if (UnpAudioBlock)
  {
    UnpChannels=((BitField>>12) & 3)+1;
    if (CurChannel>=UnpChannels)
      CurChannel=0;
    AddBits(2);
    TableSize=MC*UnpChannels;
  }
  else
    TableSize=NC+DC+RC;


  for (I=0;I<BC;I++)
  {
    GetBits();
    BitLength[I]=(UBYTE)(BitField >> 12);
    AddBits(4);
  }
  MakeDecodeTables(BitLength,(struct Decode *)&BD,BC);
  I=0;
  while (I<TableSize)
  {
    if (InAddr>sizeof(InBuf)-5)
      UnpReadBuf(0);
    DecodeNumber((struct Decode *)&BD);
    if (Number<16)
      Table[I++]=(Number+UnpOldTable[I]) & 0xf;
    else
      if (Number==16)
      {
        GetBits();
        N=(BitField >> 14)+3;
        AddBits(2);
        while (N-- > 0 && I<TableSize)
        {
          Table[I]=Table[I-1];
          I++;
        }
      }
      else
      {
        if (Number==17)
        {
          GetBits();
          N=(BitField >> 13)+3;
          AddBits(3);
        }
        else
        {
          GetBits();
          N=(BitField >> 9)+11;
          AddBits(7);
        }
        while (N-- > 0 && I<TableSize)
          Table[I++]=0;
      }
  }
  if (UnpAudioBlock)
    for (I=0;I<UnpChannels;I++)
      MakeDecodeTables(&Table[I*MC],(struct Decode *)MDPtr[I],MC);
  else
  {
    MakeDecodeTables(&Table[0],(struct Decode *)&LD,NC);
    MakeDecodeTables(&Table[NC],(struct Decode *)&DD,DC);
    MakeDecodeTables(&Table[NC+DC],(struct Decode *)&RD,RC);
  }
  memcpy(UnpOldTable,Table,sizeof(UnpOldTable));
}


void CRarArchive::ReadLastTables()
//--------------------------------
{
  if (ReadTop>=InAddr+5)
    if (UnpAudioBlock)
    {
      DecodeNumber((struct Decode *)MDPtr[CurChannel]);
      if (Number==256)
        ReadTables();
    }
    else
    {
      DecodeNumber((struct Decode *)&LD);
      if (Number==269)
        ReadTables();
    }
}


static void MakeDecodeTables(unsigned char *LenTab,struct Decode *Dec,int Size)
{
  int LenCount[16],TmpPos[16],I;
  long M,N;
  memset(LenCount,0,sizeof(LenCount));
  for (I=0;I<Size;I++)
    LenCount[LenTab[I] & 0xF]++;

  LenCount[0]=0;
  for (TmpPos[0]=Dec->DecodePos[0]=Dec->DecodeLen[0]=0,N=0,I=1;I<16;I++)
  {
    N=2*(N+LenCount[I]);
    M=N<<(15-I);
    if (M>0xFFFF)
      M=0xFFFF;
    Dec->DecodeLen[I]=(unsigned int)M;
    TmpPos[I]=Dec->DecodePos[I]=Dec->DecodePos[I-1]+LenCount[I-1];
  }

  for (I=0;I<Size;I++)
    if (LenTab[I]!=0)
      Dec->DecodeNum[TmpPos[LenTab[I] & 0xF]++]=I;
  Dec->MaxNum=Size;
}


static void DecodeNumber(struct Decode *Dec)
{
  unsigned int I;
  register unsigned int N;
  GetBits();
  N=BitField & 0xFFFE;
  if (N<Dec->DecodeLen[8])
    if (N<Dec->DecodeLen[4])
      if (N<Dec->DecodeLen[2])
        if (N<Dec->DecodeLen[1])
          I=1;
        else
          I=2;
      else
        if (N<Dec->DecodeLen[3])
          I=3;
        else
          I=4;
    else
      if (N<Dec->DecodeLen[6])
        if (N<Dec->DecodeLen[5])
          I=5;
        else
          I=6;
      else
        if (N<Dec->DecodeLen[7])
          I=7;
        else
          I=8;
  else
    if (N<Dec->DecodeLen[12])
      if (N<Dec->DecodeLen[10])
        if (N<Dec->DecodeLen[9])
          I=9;
        else
          I=10;
      else
        if (N<Dec->DecodeLen[11])
          I=11;
        else
          I=12;
    else
      if (N<Dec->DecodeLen[14])
        if (N<Dec->DecodeLen[13])
          I=13;
        else
          I=14;
      else
        I=15;

  AddBits(I);
  if ((N=Dec->DecodePos[I]+((N-Dec->DecodeLen[I-1])>>(16-I)))>=Dec->MaxNum)
    N=0;
  Number=Dec->DecodeNum[N];
}


void CopyString(void)
{
  LastDist=OldDist[OldDistPtr++ & 3]=Distance;
  DestUnpSize-=(LastLength=Length);
  while (Length--)
  {
    UnpBuf[UnpPtr]=UnpBuf[(UnpPtr-Distance) & MAXWINMASK];
    UnpPtr=(UnpPtr+1) & MAXWINMASK;
  }
}


void UnpInitData(int Solid)
{
  InAddr=InBit=0;
  if (!Solid)
  {
    ChannelDelta=CurChannel=0;
    memset(AudV,0,sizeof(AudV));
    memset(OldDist,0,sizeof(OldDist));
    OldDistPtr=0;
    LastDist=LastLength=0;
    memset(UnpBuf,0,MAXWINSIZE);
    memset(UnpOldTable,0,sizeof(UnpOldTable));
    UnpPtr=WrPtr=0;
  }
}


UBYTE DecodeAudio(int Delta)
{
  struct AudioVariables *V;
  unsigned int Ch;
  unsigned int NumMinDif,MinDif;
  int PCh,I;

  V=&AudV[CurChannel];
  V->ByteCount++;
  V->D4=V->D3;
  V->D3=V->D2;
  V->D2=V->LastDelta-V->D1;
  V->D1=V->LastDelta;
  PCh=8*V->LastChar+V->K1*V->D1+V->K2*V->D2+V->K3*V->D3+V->K4*V->D4+V->K5*ChannelDelta;
  PCh=(PCh>>3) & 0xFF;

  Ch=PCh-Delta;

  I=((signed char)Delta)<<3;

  V->Dif[0]+=abs(I);
  V->Dif[1]+=abs(I-V->D1);
  V->Dif[2]+=abs(I+V->D1);
  V->Dif[3]+=abs(I-V->D2);
  V->Dif[4]+=abs(I+V->D2);
  V->Dif[5]+=abs(I-V->D3);
  V->Dif[6]+=abs(I+V->D3);
  V->Dif[7]+=abs(I-V->D4);
  V->Dif[8]+=abs(I+V->D4);
  V->Dif[9]+=abs(I-ChannelDelta);
  V->Dif[10]+=abs(I+ChannelDelta);

  ChannelDelta=V->LastDelta=(signed char)(Ch-V->LastChar);
  V->LastChar=Ch;

  if ((V->ByteCount & 0x1F)==0)
  {
    MinDif=V->Dif[0];
    NumMinDif=0;
    V->Dif[0]=0;
    for (I=1;I<sizeof(V->Dif)/sizeof(V->Dif[0]);I++)
    {
      if (V->Dif[I]<MinDif)
      {
        MinDif=V->Dif[I];
        NumMinDif=I;
      }
      V->Dif[I]=0;
    }
    switch(NumMinDif)
    {
      case 1:
        if (V->K1>=-16)
          V->K1--;
        break;
      case 2:
        if (V->K1<16)
          V->K1++;
        break;
      case 3:
        if (V->K2>=-16)
          V->K2--;
        break;
      case 4:
        if (V->K2<16)
          V->K2++;
        break;
      case 5:
        if (V->K3>=-16)
          V->K3--;
        break;
      case 6:
        if (V->K3<16)
          V->K3++;
        break;
      case 7:
        if (V->K4>=-16)
          V->K4--;
        break;
      case 8:
        if (V->K4<16)
          V->K4++;
        break;
      case 9:
        if (V->K5>=-16)
          V->K5--;
        break;
      case 10:
        if (V->K5<16)
          V->K5++;
        break;
    }
  }
  return((UBYTE)Ch);
}

