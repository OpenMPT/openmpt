/*---------------------------------------------------------------------------

  unshrink.c

  Shrinking is a Dynamic Lempel-Ziv-Welch compression algorithm with partial
  clearing.

  ---------------------------------------------------------------------------*/



/*************************************/
/*  UnShrink Defines, Globals, etc.  */
/*************************************/

extern DWORD mask_bits[33];

#define READBIT(nbits,zdest) {if(nbits>bits_left) FillBitBuffer();\
  zdest=(int)(bitbuf&mask_bits[nbits]); bitbuf>>=nbits; bits_left-=nbits;}

#define INIT_BITS       9
#define FIRST_ENT       257
#define CLEAR           256
#define GetCode(dest)   READBIT(codesize,dest)


/*************************/
/*  Function unShrink()  */
/*************************/

void CZipArchive::unShrink()
//--------------------------
{
    register int code;
    register int stackp;
    int finchar;
    int oldcode;
    int incode;


    /* decompress the file */
    codesize = INIT_BITS;
    maxcode = (1 << codesize) - 1;
    maxcodemax = HSIZE;         /* (1 << MAX_BITS) */
    free_ent = FIRST_ENT;

    for (code = maxcodemax; code > 255; code--)
        prefix_of[code] = -1;

    for (code = 255; code >= 0; code--) {
        prefix_of[code] = 0;
        suffix_of[code] = code;
    }

    GetCode(oldcode);
    if (zipeof)
        return;
    finchar = oldcode;

    OUTB(finchar);

    stackp = HSIZE;

    while (!zipeof) {
        GetCode(code);
        if (zipeof)
            return;

        while (code == CLEAR) {
            GetCode(code);
            switch (code) {

            case 1:{
                    codesize++;
                    if (codesize == MAX_BITS)
                        maxcode = maxcodemax;
                    else
                        maxcode = (1 << codesize) - 1;
                }
                break;

            case 2:
                partial_clear();
                break;
            }

            GetCode(code);
            if (zipeof)
                return;
        }


        /* special case for KwKwK string */
        incode = code;
        if (prefix_of[code] == -1) 
		{
            stack[--stackp] = finchar;
            code = oldcode;
        }
        /* generate output characters in reverse order */
        while (code >= FIRST_ENT) {
            if (prefix_of[code] == -1) {
                stack[--stackp] = finchar;
                code = oldcode;
            } else {
                stack[--stackp] = suffix_of[code];
                code = prefix_of[code];
            }
        }

        finchar = suffix_of[code];
        stack[--stackp] = finchar;


        /* and put them out in forward order, block copy */
        if ((HSIZE - stackp + outcnt) < OUTBUFSIZ) {
            memcpy(outptr, &stack[stackp], HSIZE - stackp);
            outptr += HSIZE - stackp;
            outcnt += HSIZE - stackp;
            stackp = HSIZE;
        }
        /* output byte by byte if we can't go by blocks */
        else
            while (stackp < HSIZE)
                OUTB(stack[stackp++]);


        /* generate new entry */
        code = free_ent;
        if (code < maxcodemax) {
            prefix_of[code] = oldcode;
            suffix_of[code] = finchar;

            do
                code++;
            while ((code < maxcodemax) && (prefix_of[code] != -1));

            free_ent = code;
        }
        /* remember previous code */
        oldcode = incode;
    }
}


/******************************/
/*  Function partial_clear()  */
/******************************/

void CZipArchive::partial_clear()
//-------------------------------
{
    int pr;
    int cd;

    /* mark all nodes as potentially unused */
    for (cd = FIRST_ENT; cd < free_ent; cd++)
        prefix_of[cd] |= 0x8000;

    /* unmark those that are used by other nodes */
    for (cd = FIRST_ENT; cd < free_ent; cd++) {
        pr = prefix_of[cd] & 0x7fff;    /* reference to another node? */
        if (pr >= FIRST_ENT)    /* flag node as referenced */
            prefix_of[pr] &= 0x7fff;
    }

    /* clear the ones that are still marked */
    for (cd = FIRST_ENT; cd < free_ent; cd++)
        if ((prefix_of[cd] & 0x8000) != 0)
            prefix_of[cd] = -1;

    /* find first cleared node as next free_ent */
    cd = FIRST_ENT;
    while ((cd < maxcodemax) && (prefix_of[cd] != -1))
        cd++;
    free_ent = cd;
}
