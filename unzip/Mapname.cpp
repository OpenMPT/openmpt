/*---------------------------------------------------------------------------

  mapname.c

  This routine changes DEC-20, VAX/VMS, and DOS-style filenames into normal
  Unix names

  ------------------------------------------------------------------------- */



/************************/
/*  Function mapname()  */
/************************/

int CZipArchive::MapName()   /* return 0 if no error, 1 if caution (filename */
//------------------------
					       /*  truncated), 2 if warning (skip file because */
{                          /*  dir doesn't exist), 3 if error (skip file) */
    char name[FILNAMSIZ];       /* file name buffer */
    char *pp, *cp;			/* character pointers */
    char delim = '\0';          /* directory delimiter */
    int quote = FALSE;          /* flags */
    int indir = FALSE;
    int done = FALSE;
    int workch;        /* hold the character being tested */


//	Initialize various pointers and counters and stuff.
    pp = name;                  /* Point to translation buffer */
    *name = '\0';               /* Initialize buffer */

//	Begin main loop through characters in filename.
    for (cp = filename; (workch = *cp++) != 0  &&  !done;) 
	{
        if (quote) 
		{            /* If this char quoted... */
            *pp++ = workch;     /*  include it literally. */
            quote = FALSE;
        } else if (indir) 
		{     /* If in directory name... */
            if (workch == delim)
                indir = FALSE;  /*  look for end delimiter. */
        } else
            switch (workch) 
			{
            case '<':           /* Discard DEC-20 directory name */
                indir = TRUE;
                delim = '>';
                break;
            case '[':           /* Discard VMS directory name */
                indir = TRUE;
                delim = ']';
                break;
            case '/':           /* Discard Unix path name... */
            case '\\':          /*  or MS-DOS path name...
                                 *  IF -j flag was given. */
                /*
                 * Special processing case:  if -j flag was not specified on
                 * command line and create_dirs is TRUE, create any necessary
                 * directories included in the pathname.  Creation of dirs is
                 * straightforward on BSD and MS-DOS machines but requires use
                 * of the system() command on SysV systems (or any others which
                 * don't have mkdir()).  The stat() check is necessary with
                 * MSC because it doesn't have an EEXIST errno, and it saves
                 * the overhead of multiple system() calls on SysV machines.
                 */

                pp = name;
                break;
            case ':':
                *pp++ = '_';      /*  NOT have stored drive/node names!!) */
             /* pp = name;  (OLD) discard DEC dev: or node:: name */
                break;
            case '.':
                *pp++ = workch;
                break;
            case ';':                   /* VMS generation or DEC-20 attrib */
										/* If requested, save VMS ";##" */
										/*  version info; else discard */
										/*  everything starting with */
                done = TRUE;			/*  semicolon.  (Worry about */
                break;                  /*  DEC-20 later.) */
            case '\026':                /* Control-V quote for special chars */
                quote = TRUE;           /* Set flag for next time. */
                break;
            case ' ':
                *pp++ = workch;         /* otherwise, leave as spaces */
                break;
            default:
				if ((workch & 0xFF) > ' ')	// other printable, just keep
                    *pp++ = workch;
            }                   /* end switch */
    }                           /* end for loop */
    *pp = '\0';                 /* done with name:  terminate it */

/*---------------------------------------------------------------------------
    We COULD check for existing names right now, create a "unique" name, etc.
    At present, we do this in extract_or_test_files() (immediately after we
    return from here).  If conversion went bad, the name'll either be nulled
    out (in which case we'll return non-0), or following procedures won't be
    able to create the extracted file and other error msgs will result.
  ---------------------------------------------------------------------------*/

    if (*name == '\0')
	{
        // mapname:  conversion of [%s] failed
        return 3;
    }

	lstrcpy(filename, name); /* copy converted name into global */
    return 0;
}
