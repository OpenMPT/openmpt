/* inflate.c -- put in the public domain by Mark Adler
   version c14v, 8 November 1995 */


/*
   Inflate deflated (PKZIP's method 8 compressed) data.  The compression
   method searches for as much of the current string of bytes (up to a
   length of 258) in the previous 32K bytes.  If it doesn't find any
   matches (of at least length 3), it codes the next byte.  Otherwise, it
   codes the length of the matched string and its distance backwards from
   the current position.  There is a single Huffman code that codes both
   single bytes (called "literals") and match lengths.  A second Huffman
   code codes the distance information, which follows a length code.  Each
   length or distance code actually represents a base value and a number
   of "extra" (sometimes zero) bits to get to add to the base value.  At
   the end of each deflated block is a special end-of-block (EOB) literal/
   length code.  The decoding process is basically: get a literal/length
   code; if EOB then done; if a literal, emit the decoded byte; if a
   length then get the distance and emit the referred-to bytes from the
   sliding window of previously emitted data.

   There are (currently) three kinds of inflate blocks: stored, fixed, and
   dynamic.  The compressor outputs a chunk of data at a time and decides
   which method to use on a chunk-by-chunk basis.  A chunk might typically
   be 32K to 64K, uncompressed.  If the chunk is uncompressible, then the
   "stored" method is used.  In this case, the bytes are simply stored as
   is, eight bits per byte, with none of the above coding.  The bytes are
   preceded by a count, since there is no longer an EOB code.

   If the data are compressible, then either the fixed or dynamic methods
   are used.  In the dynamic method, the compressed data are preceded by
   an encoding of the literal/length and distance Huffman codes that are
   to be used to decode this block.  The representation is itself Huffman
   coded, and so is preceded by a description of that code.  These code
   descriptions take up a little space, and so for small blocks, there is
   a predefined set of codes, called the fixed codes.  The fixed method is
   used if the block ends up smaller that way (usually for quite small
   chunks); otherwise the dynamic method is used.  In the latter case, the
   codes are customized to the probabilities in the current block and so
   can code it much better than the pre-determined fixed codes can.

   The Huffman codes themselves are decoded using a multi-level table
   lookup, in order to maximize the speed of decoding plus the speed of
   building the decoding tables.  See the comments below that precede the
   lbits and dbits tuning parameters.

   GRR:  return values(?)
           0  OK
           1  incomplete table
           2  bad input
           3  not enough memory
 */


/*
   Notes beyond the 1.93a appnote.txt:

   1. Distance pointers never point before the beginning of the output
      stream.
   2. Distance pointers can point back across blocks, up to 32k away.
   3. There is an implied maximum of 7 bits for the bit length table and
      15 bits for the actual data.
   4. If only one code exists, then it is encoded using one bit.  (Zero
      would be more efficient, but perhaps a little confusing.)  If two
      codes exist, they are coded using one bit each (0 and 1).
   5. There is no way of sending zero distance codes--a dummy must be
      sent if there are none.  (History: a pre 2.0 version of PKZIP would
      store blocks with no distance codes, but this was discovered to be
      too harsh a criterion.)  Valid only for 1.93a.  2.04c does allow
      zero distance codes, which is sent as one code of zero bits in
      length.
   6. There are up to 286 literal/length codes.  Code 256 represents the
      end-of-block.  Note however that the static length tree defines
      288 codes just to fill out the Huffman codes.  Codes 286 and 287
      cannot be used though, since there is no length base or extra bits
      defined for them.  Similarily, there are up to 30 distance codes.
      However, static trees define 32 codes (all 5 bits) to fill out the
      Huffman codes, but the last two had better not show up in the data.
   7. Unzip can check dynamic Huffman blocks for complete code sets.
      The exception is that a single code would not be complete (see #4).
   8. The five bits following the block type is really the number of
      literal codes sent minus 257.
   9. Length codes 8,16,16 are interpreted as 13 length codes of 8 bits
      (1+6+6).  Therefore, to output three times the length, you output
      three codes (1+1+1), whereas to output four times the same length,
      you only need two codes (1+3).  Hmm.
  10. In the tree reconstruction algorithm, Code = Code + Increment
      only if BitLength(i) is not zero.  (Pretty obvious.)
  11. Correction: 4 Bits: # of Bit Length codes - 4     (4 - 19)
  12. Note: length code 284 can represent 227-258, but length code 285
      really is 258.  The last length deserves its own, short code
      since it gets used a lot in very redundant files.  The length
      258 is special since 258 - 3 (the min match length) is 255.
  13. The literal/length and distance code bit lengths are read as a
      single stream of lengths.  It is possible (and advantageous) for
      a repeat code (16, 17, or 18) to go across the boundary between
      the two sets of lengths.
 */


#define PKZIP_BUG_WORKAROUND    /* PKZIP 1.93a problem--live with it */

/*
    inflate.h must supply the uch slide[WSIZE] array, the zvoid typedef
    (void if (void *) is accepted, else char) and the NEXTBYTE,
    FLUSH() and memzero macros.  If the window size is not 32K, it
    should also define WSIZE.  If INFMOD is defined, it can include
    compiled functions to support the NEXTBYTE and/or FLUSH() macros.
    There are defaults for NEXTBYTE and FLUSH() below for use as
    examples of what those functions need to do.  Normally, you would
    also want FLUSH() to compute a crc on the data.  inflate.h also
    needs to provide these typedefs:

        typedef unsigned char uch;
        typedef unsigned short ush;
        typedef unsigned long ulg;

 */


#define EOF			(-1)
#define wsize		WSIZE       // wsize is a constant
#define FLUSH(n)	Write(redirSlide, n)  // return value not used


/* The inflate algorithm uses a sliding 32K byte window on the uncompressed
   stream to find repeated byte strings.  This is implemented here as a
   circular buffer.  The index is updated simply by incrementing and then
   and'ing with 0x7fff (32K-1). */
/* It is left to other modules to supply the 32K area.  It is assumed
   to be usable as if it were declared "uch slide[32768];" or as just
   "uch *slide;" and then malloc'ed in the latter case.  The definition
   must be in unzip.h, included above. */


/* Tables for deflate from PKZIP's appnote.txt. */
static unsigned border[] = {    /* Order of the bit length code lengths */
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
static WORD cplens[] = {         /* Copy lengths for literal codes 257..285 */
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
        /* note: see note #13 above about the 258 in this list. */
static WORD cplext[] = {         /* Extra bits for literal codes 257..285 */
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99}; /* 99==invalid */
static WORD cpdist[] = {         /* Copy offsets for distance codes 0..29 */
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577};
static WORD cpdext[] = {         /* Extra bits for distance codes */
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
        12, 12, 13, 13};


// And'ing with mask_bits[n] masks the lower n bits
extern DWORD mask_bits[33];

/* Macros for inflate() bit peeking and grabbing.
   The usage is:

        NEEDBITS(j)
        x = b & mask_bits[j];
        DUMPBITS(j)

   where NEEDBITS makes sure that b has at least j bits in it, and
   DUMPBITS removes the bits from b.  The macros use the variable k
   for the number of bits in b.  Normally, b and k are register
   variables for speed and are initialized at the begining of a
   routine that uses these macros from a global bit buffer and count.

   In order to not ask for more bits than there are in the compressed
   stream, the Huffman tables are constructed to only ask for just
   enough bits to make up the end-of-block code (value 256).  Then no
   bytes need to be "returned" to the buffer at the end of the last
   block.  See the huft_build() routine.
 */

#define NEEDBITS(n) {while(k<(n)){int c=NEXTBYTE();if(c==EOF)return 1;\
    b|=((DWORD)c)<<k;k+=8;}}

#define DUMPBITS(n) {b>>=(n);k-=(n);}


/*
   Huffman code decoding is performed using a multi-level table lookup.
   The fastest way to decode is to simply build a lookup table whose
   size is determined by the longest code.  However, the time it takes
   to build this table can also be a factor if the data being decoded
   are not very long.  The most common codes are necessarily the
   shortest codes, so those codes dominate the decoding time, and hence
   the speed.  The idea is you can have a shorter table that decodes the
   shorter, more probable codes, and then point to subsidiary tables for
   the longer codes.  The time it costs to decode the longer codes is
   then traded against the time it takes to make longer tables.

   This results of this trade are in the variables lbits and dbits
   below.  lbits is the number of bits the first level table for literal/
   length codes can decode in one step, and dbits is the same thing for
   the distance codes.  Subsequent tables are also less than or equal to
   those sizes.  These values may be adjusted either when all of the
   codes are shorter than that, in which case the longest code length in
   bits is used, or when the shortest code is *longer* than the requested
   table size, in which case the length of the shortest code in bits is
   used.

   There are two different values for the two tables, since they code a
   different number of possibilities each.  The literal/length table
   codes 286 possible values, or in a flat code, a little over eight
   bits.  The distance table codes 30 possible values, or a little less
   than five bits, flat.  The optimum values for speed end up being
   about one bit more than those, so lbits is 8+1 and dbits is 5+1.
   The optimum values may differ though from machine to machine, and
   possibly even between compilers.  Your mileage may vary.
 */


// bits in base literal/length lookup table
#define lbits	9
// bits in base distance lookup table
#define dbits	6


int CZipArchive::inflate_codes(struct huft *tl, struct huft *td, int bl, int bd)
//------------------------------------------------------------------------------
//struct huft *tl, *td;   /* literal/length and distance decoder tables */
//int bl, bd;             /* number of bits decoded by tl[] and td[] */
/* inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. */
{
  unsigned e;			/* table entry flag/number of extra bits */
  unsigned n, d;        /* length and index for copy */
  unsigned w;           /* current window position */
  struct huft *t;       /* pointer to table entry */
  unsigned ml, md;      /* masks for bl and bd bits */
  DWORD b;				/* bit buffer */
  unsigned k;			/* number of bits in bit buffer */


  /* make local copies of globals */
  b = bb;                       /* initialize bit buffer */
  k = bk;
  w = wp;                       /* initialize window position */


  /* inflate the coded data */
  ml = mask_bits[bl];           /* precompute masks for speed */
  md = mask_bits[bd];
  while (1)                     /* do until end of block */
  {
    NEEDBITS((unsigned)bl)
    if ((e = (t = tl + ((unsigned)b & ml))->e) > 16)
      do {
        if (e == 99)
          return 1;
        DUMPBITS(t->b)
        e -= 16;
        NEEDBITS(e)
      } while ((e = (t = t->v.t + ((unsigned)b & mask_bits[e]))->e) > 16);
    DUMPBITS(t->b)
    if (e == 16)                /* then it's a literal */
    {
      redirSlide[w++] = (BYTE)t->v.n;
      if (w == wsize)
      {
        FLUSH(w);
        w = 0;
      }
    }
    else                        /* it's an EOB or a length */
    {
      /* exit if end of block */
      if (e == 15)
        break;

      /* get length of block to copy */
      NEEDBITS(e)
      n = t->v.n + ((unsigned)b & mask_bits[e]);
      DUMPBITS(e);

      /* decode distance of block to copy */
      NEEDBITS((unsigned)bd)
      if ((e = (t = td + ((unsigned)b & md))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((unsigned)b & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      NEEDBITS(e)
      d = w - t->v.n - ((unsigned)b & mask_bits[e]);
      DUMPBITS(e)

      /* do the copy */
      do {
          n -= (e = (e = wsize - ((d &= wsize-1) > w ? d : w)) > n ? n : e);
        if (w - d >= e)         /* (this test assumes unsigned comparison) */
        {
          memcpy(redirSlide + w, redirSlide + d, e);
          w += e;
          d += e;
        }
        else                    /* do it slowly to avoid memcpy() overlap */
          do {
            redirSlide[w++] = redirSlide[d++];
          } while (--e);
        if (w == wsize)
        {
          FLUSH(w);
          w = 0;
        }
      } while (n);
    }
  }


  /* restore the globals from the locals */
  wp = w;                       /* restore global window pointer */
  bb = b;                       /* restore global bit buffer */
  bk = k;

  /* done */
  return 0;
}


int CZipArchive::inflate_stored()
/* "decompress" an inflated type 0 (stored) block. */
{
  unsigned n;           /* number of bytes in block */
  unsigned w;           /* current window position */
  register DWORD b;       /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */


  /* make local copies of globals */
  b = bb;                       /* initialize bit buffer */
  k = bk;
  w = wp;                       /* initialize window position */


  /* go to byte boundary */
  n = k & 7;
  DUMPBITS(n);

  /* get the length and its complement */
  NEEDBITS(16)
  n = ((unsigned)b & 0xffff);
  DUMPBITS(16)
  NEEDBITS(16)
  // error in compressed data
  if (n != (unsigned)((~b) & 0xffff)) return 1;
  DUMPBITS(16)

  // read and output the compressed data
  while (n--)
  {
    NEEDBITS(8)
    redirSlide[w++] = (BYTE)b;
    if (w == wsize)
    {
      FLUSH(w);
      w = 0;
    }
    DUMPBITS(8)
  }


  /* restore the globals from the locals */
  wp = w;                       /* restore global window pointer */
  bb = b;                       /* restore global bit buffer */
  bk = k;
  return 0;
}


/* Globals for literal tables (built once) */
/* Moved to globals.h                      */

int CZipArchive::inflate_fixed()
/* decompress an inflated type 1 (fixed Huffman codes) block.  We should
   either replace this with a custom decoder, or at least precompute the
   Huffman tables. */
{
  /* if first time, set up tables for fixed blocks */
  if (fixed_tl == NULL)
  {
    int i;                /* temporary variable */
    unsigned l[288];      /* length list for huft_build */

    /* literal table */
    for (i = 0; i < 144; i++)
      l[i] = 8;
    for (; i < 256; i++)
      l[i] = 9;
    for (; i < 280; i++)
      l[i] = 7;
    for (; i < 288; i++)          /* make a complete, but wrong code set */
      l[i] = 8;
    fixed_bl = 7;
    if ((i = huft_build(l, 288, 257, cplens, cplext, &fixed_tl, &fixed_bl)) != 0)
    {
      fixed_tl = NULL;
      return i;
    }

    // distance table
    for (i = 0; i < 30; i++)      /* make an incomplete code set */
      l[i] = 5;
    fixed_bd = 5;
    if ((i = huft_build(l, 30, 0, cpdist, cpdext, &fixed_td, &fixed_bd)) > 1)
    {
      huft_free(fixed_tl);
      fixed_tl = NULL;
      return i;
    }
  }

  /* decompress until an end-of-block code */
  return inflate_codes(fixed_tl, fixed_td, fixed_bl, fixed_bd) != 0;
}



int CZipArchive::inflate_dynamic()
/* decompress an inflated type 2 (dynamic Huffman codes) block. */
{
  int i;                /* temporary variables */
  unsigned j;
  unsigned l;           /* last length */
  unsigned m;           /* mask for bit lengths table */
  unsigned n;           /* number of lengths to get */
  struct huft *tl;      /* literal/length code table */
  struct huft *td;      /* distance code table */
  int bl;               /* lookup bits for tl */
  int bd;               /* lookup bits for td */
  unsigned nb;          /* number of bit length codes */
  unsigned nl;          /* number of literal/length codes */
  unsigned nd;          /* number of distance codes */
#ifdef PKZIP_BUG_WORKAROUND
  unsigned ll[288+32]; /* literal/length and distance code lengths */
#else
  unsigned ll[286+30]; /* literal/length and distance code lengths */
#endif
  DWORD b;       /* bit buffer */
  unsigned k;  /* number of bits in bit buffer */


  /* make local bit buffer */
  b = bb;
  k = bk;


  /* read in table lengths */
  NEEDBITS(5)
  nl = 257 + ((unsigned)b & 0x1f);      /* number of literal/length codes */
  DUMPBITS(5)
  NEEDBITS(5)
  nd = 1 + ((unsigned)b & 0x1f);        /* number of distance codes */
  DUMPBITS(5)
  NEEDBITS(4)
  nb = 4 + ((unsigned)b & 0xf);         /* number of bit length codes */
  DUMPBITS(4)
#ifdef PKZIP_BUG_WORKAROUND
  if (nl > 288 || nd > 32)
#else
  if (nl > 286 || nd > 30)
#endif
    return 1;                   /* bad lengths */


  /* read in bit-length-code lengths */
  for (j = 0; j < nb; j++)
  {
    NEEDBITS(3)
    ll[border[j]] = (unsigned)b & 7;
    DUMPBITS(3)
  }
  for (; j < 19; j++)
    ll[border[j]] = 0;


  /* build decoding table for trees--single level, 7 bit lookup */
  bl = 7;
  if ((i = huft_build(ll, 19, 19, NULL, NULL, &tl, &bl)) != 0)
  {
    if (i == 1)
      huft_free(tl);
    return i;                   /* incomplete code set */
  }


  /* read in literal and distance code lengths */
  n = nl + nd;
  m = mask_bits[bl];
  i = l = 0;
  while ((unsigned)i < n)
  {
    NEEDBITS((unsigned)bl)
    j = (td = tl + ((unsigned)b & m))->b;
    DUMPBITS(j)
    j = td->v.n;
    if (j < 16)                 /* length of code in bits (0..15) */
      ll[i++] = l = j;          /* save last length in l */
    else if (j == 16)           /* repeat last length 3 to 6 times */
    {
      NEEDBITS(2)
      j = 3 + ((unsigned)b & 3);
      DUMPBITS(2)
      if ((unsigned)i + j > n)
        return 1;
      while (j--)
        ll[i++] = l;
    }
    else if (j == 17)           /* 3 to 10 zero length codes */
    {
      NEEDBITS(3)
      j = 3 + ((unsigned)b & 7);
      DUMPBITS(3)
      if ((unsigned)i + j > n)
        return 1;
      while (j--)
        ll[i++] = 0;
      l = 0;
    }
    else                        /* j == 18: 11 to 138 zero length codes */
    {
      NEEDBITS(7)
      j = 11 + ((unsigned)b & 0x7f);
      DUMPBITS(7)
      if ((unsigned)i + j > n) return 1;
      while (j--)
        ll[i++] = 0;
      l = 0;
    }
  }


  /* free decoding table for trees */
  huft_free(tl);

  /* restore the global bit buffer */
  bb = b;
  bk = k;

  /* build the decoding tables for literal/length and distance codes */
  bl = lbits;
  if ((i = huft_build(ll, nl, 257, cplens, cplext, &tl, &bl)) != 0)
  {
    if (i == 1) // && !qflag)
	{
      huft_free(tl);
    }
    return i;                   /* incomplete code set */
  }
  bd = dbits;
  if ((i = huft_build(ll + nl, nd, 0, cpdist, cpdext, &td, &bd)) != 0)
  {
    if (i == 1) // && !qflag) 
	{
#ifdef PKZIP_BUG_WORKAROUND
      i = 0;
#else
      huft_free(td);
#endif
    }
#ifndef PKZIP_BUG_WORKAROUND
    huft_free(tl);
    return i;                   /* incomplete code set */
#endif
  }


  /* decompress until an end-of-block code */
  if (inflate_codes(tl, td, bl, bd)) return 1;

  /* free the decoding tables, return */
  huft_free(tl);
  huft_free(td);
  return 0;
}



int CZipArchive::inflate_block(int *e)
//  int *e;               /* last block flag */
/* decompress an inflated block */
{
  unsigned t;        /* block type */
  DWORD b;       /* bit buffer */
  unsigned k;  /* number of bits in bit buffer */

  /* make local bit buffer */
  b = bb;
  k = bk;

  /* read in last block bit */
  NEEDBITS(1)
  *e = (int)b & 1;
  DUMPBITS(1)

  /* read in block type */
  NEEDBITS(2)
  t = (unsigned)b & 3;
  DUMPBITS(2)

  /* restore the global bit buffer */
  bb = b;
  bk = k;

  /* inflate that block type */
  if (t == 2)
    return inflate_dynamic();
  if (t == 0)
    return inflate_stored();
  if (t == 1)
    return inflate_fixed();

  /* bad block type */
  return 2;
}



int CZipArchive::inflate()
/* decompress an inflated entry */
{
  int e;                /* last block flag */
  int r;                /* result code */
  unsigned h;           /* maximum struct huft's malloc'ed */

  redirSlide = new BYTE[WSIZE];
  if (!redirSlide) return 3;
  /* initialize window, bit buffer */
  wp = 0;
  bk = 0;
  bb = 0;


  /* decompress until the last block */
  h = 0;
  do 
  {
    hufts = 0;
    if ((r = inflate_block(&e)) != 0)
	{
		delete redirSlide;
		redirSlide = NULL;
		return r;
	}
    if (hufts > h)
      h = hufts;
  } while (!e);


  /* flush out redirSlide */
  FLUSH(wp);


  /* return success */
  delete redirSlide;
  redirSlide = NULL;
  return 0;
}



int CZipArchive::inflate_free()
//-----------------------------
{
  if (fixed_tl != NULL)
  {
    huft_free(fixed_td);
    huft_free(fixed_tl);
    fixed_td = fixed_tl = NULL;
  }
  return 0;
}

/*
 * GRR:  moved huft_build() and huft_free() down here; used by explode()
 *       and fUnZip regardless of whether USE_ZLIB defined or not
 */


/* If BMAX needs to be larger than 16, then h and x[] should be ulg. */
#define BMAX 16         /* maximum bit length of any code (16 for explode) */
#define N_MAX 288       /* maximum number of codes in any set */


int CZipArchive::huft_build(unsigned *b, unsigned n, unsigned s, WORD *d, WORD *e, struct huft **t, int *m)
//unsigned *b;            /* code lengths in bits (all assumed <= BMAX) */
//unsigned n;             /* number of codes (assumed <= N_MAX) */
//unsigned s;             /* number of simple-valued codes (0..s-1) */
//ush *d;                 /* list of base values for non-simple codes */
//ush *e;                 /* list of extra bits for non-simple codes */
//struct huft **t;        /* result: starting table */
//int *m;                 /* maximum lookup bits, returns actual */
/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return zero on success, one if
   the given code set is incomplete (the tables are still built in this
   case), two if the input is invalid (all zero length codes or an
   oversubscribed set of lengths), and three if not enough memory.
   The code with value 256 is special, and the tables are constructed
   so that no bits beyond that code are fetched when that code is
   decoded. */
{
  unsigned a;                   /* counter for codes of length k */
  unsigned c[BMAX+1];           /* bit length count table */
  unsigned el;                  /* length of EOB code (value 256) */
  unsigned f;                   /* i repeats in table every f entries */
  int g;                        /* maximum code length */
  int h;                        /* table level */
  unsigned i;					/* counter, current code */
  unsigned j;					/* counter */
  int k;						/* number of bits in current code */
  int lx[BMAX+1];               /* memory for l[-1..BMAX-1] */
  int *l = lx+1;                /* stack of bits per table */
  register unsigned *p;         /* pointer into c[], b[], or v[] */
  register struct huft *q;      /* points to current table */
  struct huft r;                /* table entry for structure assignment */
  struct huft *u[BMAX];         /* table stack */
  unsigned v[N_MAX];            /* values in order of bit length */
  register int w;               /* bits before this table == (l * h) */
  unsigned x[BMAX+1];           /* bit offsets, then code stack */
  unsigned *xp;                 /* pointer into x */
  int y;                        /* number of dummy codes added */
  unsigned z;                   /* number of entries in current table */


  /* Generate counts for each bit length */
  el = n > 256 ? b[256] : BMAX; /* set length of EOB code, if any */
  memset((char *)c, 0, sizeof(c));
  p = b;  i = n;
  do {
    c[*p]++; p++;               /* assume all entries <= BMAX */
  } while (--i);
  if (c[0] == n)                /* null input--all zero length codes */
  {
    *t = (struct huft *)NULL;
    *m = 0;
    return 0;
  }


  /* Find minimum and maximum length, bound *m by those */
  for (j = 1; j <= BMAX; j++)
    if (c[j])
      break;
  k = j;                        /* minimum code length */
  if ((unsigned)*m < j)
    *m = j;
  for (i = BMAX; i; i--)
    if (c[i])
      break;
  g = i;                        /* maximum code length */
  if ((unsigned)*m > i)
    *m = i;


  /* Adjust last length count to fill out codes, if needed */
  for (y = 1 << j; j < i; j++, y <<= 1)
    if ((y -= c[j]) < 0)
      return 2;                 /* bad input: more codes than bits */
  if ((y -= c[i]) < 0)
    return 2;
  c[i] += y;


  /* Generate starting offsets into the value table for each length */
  x[1] = j = 0;
  p = c + 1;  xp = x + 2;
  while (--i) {                 /* note that i == g from above */
    *xp++ = (j += *p++);
  }


  /* Make a table of values in order of bit lengths */
  p = b;  i = 0;
  do {
    if ((j = *p++) != 0)
      v[x[j]++] = i;
  } while (++i < n);


  /* Generate the Huffman codes and for each, make the table entries */
  x[0] = i = 0;                 /* first Huffman code is zero */
  p = v;                        /* grab values in bit order */
  h = -1;                       /* no tables yet--level -1 */
  w = l[-1] = 0;                /* no bits decoded yet */
  u[0] = (struct huft *)NULL;   /* just to keep compilers happy */
  q = (struct huft *)NULL;      /* ditto */
  z = 0;                        /* ditto */

  /* go through the bit lengths (k already is bits in shortest code) */
  for (; k <= g; k++)
  {
    a = c[k];
    while (a--)
    {
      /* here i is the Huffman code of length k bits for value *p */
      /* make tables up to required level */
      while (k > w + l[h])
      {
        w += l[h++];            /* add bits already decoded */

        /* compute minimum size table less than or equal to *m bits */
        z = (z = g - w) > (unsigned)*m ? *m : z;        /* upper limit */
        if ((f = 1 << (j = k - w)) > a + 1)     /* try a k-w bit table */
        {                       /* too few codes for k-w bit table */
          f -= a + 1;           /* deduct codes from patterns left */
          xp = c + k;
          while (++j < z)       /* try smaller tables up to z bits */
          {
            if ((f <<= 1) <= *++xp)
              break;            /* enough codes to use up j bits */
            f -= *xp;           /* else deduct codes from patterns */
          }
        }
        if ((unsigned)w + j > el && (unsigned)w < el)
          j = el - w;           /* make EOB code end at table */
        z = 1 << j;             /* table entries for j-bit table */
        l[h] = j;               /* set table size in stack */

        /* allocate and link in new table */
        if ((q = new struct huft[z + 1]) == NULL)
        {
          if (h)
            huft_free(u[0]);
          return 3;             /* not enough memory */
        }
        hufts += z + 1;         /* track memory usage */
        *t = q + 1;             /* link to list for huft_free() */
        *(t = &(q->v.t)) = (struct huft *)NULL;
        u[h] = ++q;             /* table starts after link */

        /* connect to last table, if there is one */
        if (h)
        {
          x[h] = i;             /* save pattern for backing up */
          r.b = (BYTE)l[h-1];    /* bits to dump before this table */
          r.e = (BYTE)(16 + j);  /* bits in this table */
          r.v.t = q;            /* pointer to this table */
          j = (i & ((1 << w) - 1)) >> (w - l[h-1]);
          u[h-1][j] = r;        /* connect to last table */
        }
      }

      /* set up table entry in r */
      r.b = (BYTE)(k - w);
      if (p >= v + n)
        r.e = 99;               /* out of values--invalid code */
      else if (*p < s)
      {
        r.e = (BYTE)(*p < 256 ? 16 : 15);  /* 256 is end-of-block code */
        r.v.n = (WORD)*p++;                /* simple code is just the value */
      }
      else
      {
        r.e = (BYTE)e[*p - s];   /* non-simple--look up in lists */
        r.v.n = d[*p++ - s];
      }

      /* fill code-like entries with r */
      f = 1 << (k - w);
      for (j = i >> w; j < z; j += f)
        q[j] = r;

      /* backwards increment the k-bit code i */
      for (j = 1 << (k - 1); i & j; j >>= 1)
        i ^= j;
      i ^= j;

      /* backup over finished tables */
      while ((i & ((1 << w) - 1)) != x[h])
        w -= l[--h];            /* don't need to update q */
    }
  }


  /* return actual size of base table */
  *m = l[0];


  /* Return true (1) if we were given an incomplete table */
  return y != 0 && g != 1;
}



int CZipArchive::huft_free(struct huft *t) // table to free
/* Free the malloc'ed tables built by huft_build(), which makes a linked
   list of the tables it made, with the links in a dummy first entry of
   each table. */
{
  struct huft *p, *q;


  /* Go through linked list, freeing from the malloced (t[-1]) address. */
  p = t;
  while (p != NULL)
  {
    q = (--p)->v.t;
    delete p;
    p = q;
  }
  return 0;
}

