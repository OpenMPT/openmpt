/*
	calctables: compute fixed decoder table values

	copyright ?-2021 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp (as tabinit.c, and parts of layer2.c and layer3.c),
	then printout added by Thomas Orgis

	This is supposed to compute the supposedly fixed tables that used to be computed
	live on library startup in mpg123_init().
*/

#define FORCE_FIXED
#include "mpg123lib_intern.h"
#include "debug.h"

#define ASIZE(a) (sizeof(a)/sizeof(*a))

// generic cos tables
static double cos64[16],cos32[8],cos16[4],cos8[2],cos4[1];
double *cpnts[] = { cos64,cos32,cos16,cos8,cos4 };

static void compute_costabs(void)
{
  int i,k,kr,divv;
  double *costab;

  for(i=0;i<5;i++)
  {
    kr=0x10>>i; divv=0x40>>i;
    costab = cpnts[i];
    for(k=0;k<kr;k++)
      costab[k] = 1.0 / (2.0 * cos(M_PI * ((double) k * 2.0 + 1.0) / (double) divv));
  }
}

// layer I+II tables
static double layer12_table[27][64];
static const double mulmul[27] =
{
	0.0 , -2.0/3.0 , 2.0/3.0 ,
	2.0/7.0 , 2.0/15.0 , 2.0/31.0, 2.0/63.0 , 2.0/127.0 , 2.0/255.0 ,
	2.0/511.0 , 2.0/1023.0 , 2.0/2047.0 , 2.0/4095.0 , 2.0/8191.0 ,
	2.0/16383.0 , 2.0/32767.0 , 2.0/65535.0 ,
	-4.0/5.0 , -2.0/5.0 , 2.0/5.0, 4.0/5.0 ,
	-8.0/9.0 , -4.0/9.0 , -2.0/9.0 , 2.0/9.0 , 4.0/9.0 , 8.0/9.0
};

// Storing only values 0 to 26, so char is fine.
// The size of those might be reduced ... 
static char grp_3tab[32 * 3] = { 0, };   /* used: 27 */
static char grp_5tab[128 * 3] = { 0, };  /* used: 125 */
static char grp_9tab[1024 * 3] = { 0, }; /* used: 729 */

static void compute_layer12(void)
{
	// void init_layer12_stuff()
	// real* init_layer12_table()
	for(int k=0;k<27;k++)
	{
		int i,j;
		double *table = layer12_table[k];
		for(j=3,i=0;i<63;i++,j--)
			*table++ = mulmul[k] * pow(2.0,(double) j / 3.0);
		*table++ = 0.0;
	}

	// void init_layer12()
	const char base[3][9] =
	{
		{ 1 , 0, 2 , } ,
		{ 17, 18, 0 , 19, 20 , } ,
		{ 21, 1, 22, 23, 0, 24, 25, 2, 26 }
	};
	int i,j,k,l,len;
	const int tablen[3] = { 3 , 5 , 9 };
	char *itable;
	char *tables[3] = { grp_3tab , grp_5tab , grp_9tab };

	for(i=0;i<3;i++)
	{
		itable = tables[i];
		len = tablen[i];
		for(j=0;j<len;j++)
		for(k=0;k<len;k++)
		for(l=0;l<len;l++)
		{
			*itable++ = base[i][l];
			*itable++ = base[i][k];
			*itable++ = base[i][j];
		}
	}
}

// layer III

static double ispow[8207]; // scale with SCALE_POW43
static double aa_ca[8],aa_cs[8];
static double win[4][36];
static double win1[4][36];
double COS9[9]; /* dct36_3dnow wants to use that */
static double COS6_1,COS6_2;
double tfcos36[9]; /* dct36_3dnow wants to use that */
static double tfcos12[3];
static double cos9[3],cos18[3];

// scale with SCALE_15
static double tan1_1[16],tan2_1[16],tan1_2[16],tan2_2[16];
// This used to be [2][32] initially, but already Taihei noticed that
// _apparently_ only [2][16] is used for the integer tables. But
// this is misleading: Accesses beyond that happen, at least in pathologic
// cases. Taihei's fixed-point decoding introduced read-only buffer
// overflows:-/ Those are rather harmless, though, as only bad numbers
// enter calculation. Nothing about pointers or code.
static double pow1_1[2][32],pow2_1[2][32],pow1_2[2][32],pow2_2[2][32];

#include "l3bandgain.h"

// That needs special handling for storing the pointers
// in map and mapend.
//    map[j][N] = mapbufN[j];
// mapend[j][N] = ... varies let's use
// mapbufN[j]+(mapend[j][N]-map[j][N])
static short mapbuf0[9][152];
static short mapbuf1[9][156];
static short mapbuf2[9][44];
static short *map[9][3];
static short *mapend[9][3];

static unsigned short n_slen2[512]; /* MPEG 2.0 slen for 'normal' mode */
static unsigned short i_slen2[256]; /* MPEG 2.0 slen for intensity stereo */

static real gainpow2[256+118+4];

// init tables for layer-3 ... specific with the downsampling...
void init_layer3(void)
{
	int i,j,k,l;

	for(i=0;i<8207;i++)
	ispow[i] = pow((double)i,(double)4.0/3.0);

	for(i=0;i<8;i++)
	{
		const double Ci[8] = {-0.6,-0.535,-0.33,-0.185,-0.095,-0.041,-0.0142,-0.0037};
		double sq = sqrt(1.0+Ci[i]*Ci[i]);
		aa_cs[i] = 1.0/sq;
		aa_ca[i] = Ci[i]/sq;
	}

	for(i=0;i<18;i++)
	{
		win[0][i]    = win[1][i]    =
			0.5*sin(M_PI/72.0 * (double)(2*(i+0) +1)) / cos(M_PI * (double)(2*(i+0) +19) / 72.0);
		win[0][i+18] = win[3][i+18] =
			0.5*sin(M_PI/72.0 * (double)(2*(i+18)+1)) / cos(M_PI * (double)(2*(i+18)+19) / 72.0);
	}
	for(i=0;i<6;i++)
	{
		win[1][i+18] = 0.5 / cos ( M_PI * (double) (2*(i+18)+19) / 72.0 );
		win[3][i+12] = 0.5 / cos ( M_PI * (double) (2*(i+12)+19) / 72.0 );
		win[1][i+24] = 0.5 * sin( M_PI / 24.0 * (double) (2*i+13) ) / cos ( M_PI * (double) (2*(i+24)+19) / 72.0 );
		win[1][i+30] = win[3][i] = 0.0;
		win[3][i+6 ] = 0.5 * sin( M_PI / 24.0 * (double) (2*i+1 ) ) / cos ( M_PI * (double) (2*(i+6 )+19) / 72.0 );
	}

	for(i=0;i<9;i++)
	COS9[i] = cos( M_PI / 18.0 * (double) i);

	for(i=0;i<9;i++)
	tfcos36[i] = 0.5 / cos ( M_PI * (double) (i*2+1) / 36.0 );

	for(i=0;i<3;i++)
	tfcos12[i] = 0.5 / cos ( M_PI * (double) (i*2+1) / 12.0 );

	COS6_1 = cos( M_PI / 6.0 * (double) 1);
	COS6_2 = cos( M_PI / 6.0 * (double) 2);

	cos9[0]  = cos(1.0*M_PI/9.0);
	cos9[1]  = cos(5.0*M_PI/9.0);
	cos9[2]  = cos(7.0*M_PI/9.0);
	cos18[0] = cos(1.0*M_PI/18.0);
	cos18[1] = cos(11.0*M_PI/18.0);
	cos18[2] = cos(13.0*M_PI/18.0);

	for(i=0;i<12;i++)
	{
		win[2][i] = 0.5 * sin( M_PI / 24.0 * (double) (2*i+1) ) / cos ( M_PI * (double) (2*i+7) / 24.0 );
	}

	for(i=0;i<16;i++)
	{
		// Special-casing possibly troublesome values where t=inf or
		// t=-1 in theory. In practice, this never caused issues, but there might
		// be a system with enough precision in M_PI to raise an exception.
		// Actually, the special values are not excluded from use in the code, but
		// in practice, they even have no effect in the compliance tests.
		if(i > 11) // It's periodic!
		{
			tan1_1[i] = tan1_1[i-12];
			tan2_1[i] = tan2_1[i-12];
			tan1_2[i] = tan1_2[i-12];
			tan2_2[i] = tan2_2[i-12];
		} else if(i == 6) // t=inf
		{
			tan1_1[i] = 1.0;
			tan2_1[i] = 0.0;
			tan1_2[i] = M_SQRT2;
			tan2_2[i] = 0.0;
		} else if(i == 9) // t=-1
		{
			tan1_1[i] = -HUGE_VAL;
			tan2_1[i] = HUGE_VAL;
			tan1_2[i] = -HUGE_VAL;
			tan2_2[i] = HUGE_VAL;
		} else
		{
			double t = tan( (double) i * M_PI / 12.0 );
			tan1_1[i] = t / (1.0+t);
			tan2_1[i] = 1.0 / (1.0 + t);
			tan1_2[i] = M_SQRT2 * t / (1.0+t);
			tan2_2[i] = M_SQRT2 / (1.0 + t);
		}
	}

	for(i=0;i<32;i++)
	{
		for(j=0;j<2;j++)
		{
			double base = pow(2.0,-0.25*(j+1.0));
			double p1=1.0,p2=1.0;
			if(i > 0)
			{
				if( i & 1 ) p1 = pow(base,(i+1.0)*0.5);
				else p2 = pow(base,i*0.5);
			}
			pow1_1[j][i] = p1;
			pow2_1[j][i] = p2;
			pow1_2[j][i] = M_SQRT2 * p1;
			pow2_2[j][i] = M_SQRT2 * p2;
		}
	}

	for(j=0;j<4;j++)
	{
		const int len[4] = { 36,36,12,36 };
		for(i=0;i<len[j];i+=2) win1[j][i] = + win[j][i];

		for(i=1;i<len[j];i+=2) win1[j][i] = - win[j][i];
	}

	for(j=0;j<9;j++)
	{
		const struct bandInfoStruct *bi = &bandInfo[j];
		short *mp;
		short cb,lwin;
		const unsigned char *bdf;
		int switch_idx;

		mp = map[j][0] = mapbuf0[j];
		bdf = bi->longDiff;
		switch_idx = (j < 3) ? 8 : 6;
		for(i=0,cb = 0; cb < switch_idx ; cb++,i+=*bdf++)
		{
			*mp++ = (*bdf) >> 1;
			*mp++ = i;
			*mp++ = 3;
			*mp++ = cb;
		}
		bdf = bi->shortDiff+3;
		for(cb=3;cb<13;cb++)
		{
			int l = (*bdf++) >> 1;
			for(lwin=0;lwin<3;lwin++)
			{
				*mp++ = l;
				*mp++ = i + lwin;
				*mp++ = lwin;
				*mp++ = cb;
			}
			i += 6*l;
		}
		mapend[j][0] = mp;

		mp = map[j][1] = mapbuf1[j];
		bdf = bi->shortDiff+0;
		for(i=0,cb=0;cb<13;cb++)
		{
			int l = (*bdf++) >> 1;
			for(lwin=0;lwin<3;lwin++)
			{
				*mp++ = l;
				*mp++ = i + lwin;
				*mp++ = lwin;
				*mp++ = cb;
			}
			i += 6*l;
		}
		mapend[j][1] = mp;

		mp = map[j][2] = mapbuf2[j];
		bdf = bi->longDiff;
		for(cb = 0; cb < 22 ; cb++)
		{
			*mp++ = (*bdf++) >> 1;
			*mp++ = cb;
		}
		mapend[j][2] = mp;
	}

	/* Now for some serious loopings! */
	for(i=0;i<5;i++)
	for(j=0;j<6;j++)
	for(k=0;k<6;k++)
	{
		int n = k + j * 6 + i * 36;
		i_slen2[n] = i|(j<<3)|(k<<6)|(3<<12);
	}
	for(i=0;i<4;i++)
	for(j=0;j<4;j++)
	for(k=0;k<4;k++)
	{
		int n = k + j * 4 + i * 16;
		i_slen2[n+180] = i|(j<<3)|(k<<6)|(4<<12);
	}
	for(i=0;i<4;i++)
	for(j=0;j<3;j++)
	{
		int n = j + i * 3;
		i_slen2[n+244] = i|(j<<3) | (5<<12);
		n_slen2[n+500] = i|(j<<3) | (2<<12) | (1<<15);
	}
	for(i=0;i<5;i++)
	for(j=0;j<5;j++)
	for(k=0;k<4;k++)
	for(l=0;l<4;l++)
	{
		int n = l + k * 4 + j * 16 + i * 80;
		n_slen2[n] = i|(j<<3)|(k<<6)|(l<<9)|(0<<12);
	}
	for(i=0;i<5;i++)
	for(j=0;j<5;j++)
	for(k=0;k<4;k++)
	{
		int n = k + j * 4 + i * 20;
		n_slen2[n+400] = i|(j<<3)|(k<<6)|(1<<12);
	}

	for(int i=-256;i<118+4;i++)
		gainpow2[i+256] =
			DOUBLE_TO_REAL_SCALE_LAYER3(pow((double)2.0,-0.25 * (double) (i+210)),i+256);
}

// helpers

static void print_char_array( const char *indent, const char *name
,	size_t count, char tab[] )
{
	size_t block = 72/4;
	size_t i = 0;
	if(name)
		printf("static const unsigned char %s[%zu] = \n", name, count);
	printf("%s{\n", indent);
	while(i<count)
	{
		size_t line = block > count-i ? count-i : block;
		printf("%s", indent);
		for(size_t j=0; j<line; ++j, ++i)
			printf("%s%c%3u", i ? "," : "", j ? ' ' : '\t', tab[i]);
		printf("\n");
	}
	printf("%s}%s\n", indent, name ? ";" : "");
}

static void print_value( int fixed, double fixed_scale
,	const char *name, double val )
{
	if(name)
		printf("static const real %s = ", name);
	if(fixed)
		printf("%ld;\n", (long)(DOUBLE_TO_REAL(fixed_scale*val)));
	else
		printf("%15.8e;\n", val);
}

// I feal uneasy about inf appearing as literal.
// Do all C99 implementations support it the same?
// An unreasonably big value should also just work.
static double limit_val(double val)
{
	if(val > 1e38)
		return 1e38;
	if(val < -1e38)
		return -1e38;
	return val;
}

static void print_array( int statick, int fixed, double fixed_scale
,	const char *indent, const char *name
,	size_t count, double tab[] )
{
	size_t block = 72/17;
	size_t i = 0;
	if(name)
		printf( "%sconst%s real %s[%zu] = \n", statick ? "static " : ""
		,	fixed ? "" : " ALIGNED(16)", name, count );
	printf("%s{\n", indent);
	while(i<count)
	{
		size_t line = block > count-i ? count-i : block;
		printf("%s", indent);
		if(fixed) for(size_t j=0; j<line; ++j, ++i)
			printf( "%s%c%11ld", i ? "," : "", j ? ' ' : '\t'
			,	(long)(DOUBLE_TO_REAL(fixed_scale*tab[i])) );
		else for(size_t j=0; j<line; ++j, ++i)
			printf("%s%c%15.8e", i ? "," : "", j ? ' ' : '\t', limit_val(tab[i]));
		printf("\n");
	}
	printf("%s}%s\n", indent, name ? ";" : "");
}

// C99 allows passing VLA with the fast dimensions first.
static void print_array2d( int fixed, double fixed_scale
,	const char *name, size_t x, size_t y
, double tab[][y] )
{
	printf( "static const%s real %s[%zu][%zu] = \n{\n", fixed ? "" : " ALIGNED(16)"
	,	name, x, y );
	for(size_t i=0; i<x; ++i)
	{
		if(i)
			printf(",");
		print_array(1, fixed, fixed_scale, "\t", NULL, y, tab[i]);
	}
	printf("};\n");
}

static void print_short_array( const char *indent, const char *name
,	size_t count, short tab[] )
{
	size_t block = 72/8;
	size_t i = 0;
	if(name)
		printf("static const short %s[%zu] = \n", name, count);
	printf("%s{\n", indent);
	while(i<count)
	{
		size_t line = block > count-i ? count-i : block;
		printf("%s", indent);
		for(size_t j=0; j<line; ++j, ++i)
			printf("%s%c%6d", i ? "," : "", j ? ' ' : '\t', tab[i]);
		printf("\n");
	}
	printf("%s}%s\n", indent, name ? ";" : "");
}

static void print_fixed_array( const char *indent, const char *name
,	size_t count, real tab[] )
{
	size_t block = 72/13;
	size_t i = 0;
	if(name)
		printf("static const real %s[%zu] = \n", name, count);
	printf("%s{\n", indent);
	while(i<count)
	{
		size_t line = block > count-i ? count-i : block;
		printf("%s", indent);
		for(size_t j=0; j<line; ++j, ++i)
			printf("%s%c%11ld", i ? "," : "", j ? ' ' : '\t', (long)tab[i]);
		printf("\n");
	}
	printf("%s}%s\n", indent, name ? ";" : "");
}

static void print_ushort_array( const char *indent, const char *name
,	size_t count, unsigned short tab[] )
{
	size_t block = 72/8;
	size_t i = 0;
	if(name)
		printf("static const unsigned short %s[%zu] =\n", name, count);
	printf("%s{\n", indent);
	while(i<count)
	{
		size_t line = block > count-i ? count-i : block;
		printf("%s", indent);
		for(size_t j=0; j<line; ++j, ++i)
			printf("%s%c%8u", i ? "," : "", j ? ' ' : '\t', tab[i]);
		printf("\n");
	}
	printf("%s}%s\n", indent, name ? ";" : "");
}

// C99 allows passing VLA with the fast dimensions first.
static void print_short_array2d( const char *name, size_t x, size_t y
, short tab[][y] )
{
	printf( "static const short %s[%zu][%zu] =\n{\n"
	,	name, x, y );
	for(size_t i=0; i<x; ++i)
	{
		if(i)
			printf(",");
		print_short_array("\t", NULL, y, tab[i]);
	}
	printf("};\n");
}

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		fprintf(stderr, "usage:\n\t%s <cos|l12|l3>\n\n", argv[0]);
		return 1;
	}
	printf("// output of:\n// %s", argv[0]);
	for(int i=1; i<argc; ++i)
		printf(" %s", argv[i]);
	printf("\n\n");

	compute_costabs();
	compute_layer12();
	init_layer3();

	for(int fixed=0; fixed < 2; ++fixed)
	{
		printf("\n#ifdef %s\n\n", fixed ? "REAL_IS_FIXED" : "REAL_IS_FLOAT");
		if(!fixed)
			printf("// aligned to 16 bytes for vector instructions, e.g. AltiVec\n\n");
		if(!strcmp("cos", argv[1]))
		{
			print_array(1, fixed, 1., "", "cos64", ASIZE(cos64), cos64);
			print_array(1, fixed, 1., "", "cos32", ASIZE(cos32), cos32);
			print_array(1, fixed, 1., "", "cos16", ASIZE(cos16), cos16);
			print_array(1, fixed, 1., "", "cos8",  ASIZE(cos8),  cos8 );
			print_array(1, fixed, 1., "", "cos4",  ASIZE(cos4),  cos4 );
		}
		if(!strcmp("l12", argv[1]))
		{
			compute_layer12();
			print_array2d(fixed, SCALE_LAYER12/REAL_FACTOR, "layer12_table", 27, 64, layer12_table);
		}
		if(!strcmp("l3", argv[1]))
		{
			print_array(1, fixed, SCALE_POW43/REAL_FACTOR, "", "ispow"
			,	sizeof(ispow)/sizeof(*ispow), ispow );
			print_array(1, fixed, 1., "", "aa_ca", ASIZE(aa_ca), aa_ca);
			print_array(1, fixed, 1., "", "aa_cs", ASIZE(aa_cs), aa_cs);
			print_array2d(fixed, 1., "win", 4, 36, win);
			print_array2d(fixed, 1., "win1", 4, 36, win1);
			print_array(0, fixed, 1., "", "COS9", ASIZE(COS9), COS9);
			print_value(fixed, 1., "COS6_1", COS6_1);
			print_value(fixed, 1., "COS6_2", COS6_2);
			print_array(0, fixed, 1., "", "tfcos36", ASIZE(tfcos36), tfcos36);
			print_array(1, fixed, 1., "", "tfcos12", ASIZE(tfcos12), tfcos12);
			print_array(1, fixed, 1., "", "cos9", ASIZE(cos9), cos9);
			print_array(1, fixed, 1., "", "cos18", ASIZE(cos18), cos18);
			print_array( 1, fixed, SCALE_15/REAL_FACTOR, ""
			,	"tan1_1", ASIZE(tan1_1), tan1_1 );
			print_array( 1, fixed, SCALE_15/REAL_FACTOR, ""
			,	"tan2_1", ASIZE(tan2_1), tan2_1 );
			print_array( 1, fixed, SCALE_15/REAL_FACTOR, ""
			,	"tan1_2", ASIZE(tan1_2), tan1_2 );
			print_array( 1, fixed, SCALE_15/REAL_FACTOR, ""
			,	"tan2_2", ASIZE(tan2_2), tan2_2 );
			print_array2d( fixed, SCALE_15/REAL_FACTOR
			,	"pow1_1", 2, 32, pow1_1 );
			print_array2d( fixed, SCALE_15/REAL_FACTOR
			,	"pow2_1", 2, 32, pow2_1 );
			print_array2d( fixed, SCALE_15/REAL_FACTOR
			,	"pow1_2", 2, 32, pow1_2 );
			print_array2d( fixed, SCALE_15/REAL_FACTOR
			,	"pow2_2", 2, 32, pow2_2 );
		}
		if(fixed)
			print_fixed_array("", "gainpow2", ASIZE(gainpow2), gainpow2);
		printf("\n#endif\n");
	}
	if(!strcmp("l12", argv[1]))
	{
		printf("\n");
		print_char_array("", "grp_3tab", ASIZE(grp_3tab), grp_3tab);
		printf("\n");
		print_char_array("", "grp_5tab", ASIZE(grp_5tab), grp_5tab);
		printf("\n");
		print_char_array("", "grp_9tab", ASIZE(grp_9tab), grp_9tab);
	}
	if(!strcmp("l3", argv[1]))
	{
		printf("\n");
		print_short_array2d("mapbuf0", 9, 152, mapbuf0);
		print_short_array2d("mapbuf1", 9, 156, mapbuf1);
		print_short_array2d("mapbuf2", 9,  44, mapbuf2);
		printf("static const short *map[9][3] =\n{\n");
		for(int i=0; i<9; ++i)
			printf( "%s\t{ mapbuf0[%d], mapbuf1[%d], mapbuf2[%d] }\n"
			,	(i ? "," : ""), i, i, i );
		printf("};\n");
		printf("static const short *mapend[9][3] =\n{\n");
		for(int i=0; i<9; ++i)
			printf( "%s\t{ mapbuf0[%d]+%d, mapbuf1[%d]+%d, mapbuf2[%d]+%d }\n"
			,	(i ? "," : "")
			,	i, (int)(mapend[i][0]-map[i][0])
			,	i, (int)(mapend[i][1]-map[i][1])
			,	i, (int)(mapend[i][2]-map[i][2]) );
		printf("};\n");
		print_ushort_array("", "n_slen2", ASIZE(n_slen2), n_slen2);
		print_ushort_array("", "i_slen2", ASIZE(i_slen2), i_slen2);
	}

	return 0;
}
