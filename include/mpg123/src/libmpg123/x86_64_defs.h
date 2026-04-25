/* SPDX-License-Identifier: LGPL-2.1
 *
 * 86_64_defs.h: Support macros for the x86-64 architectural feature
 *
 * include at the top of asm files to define GPU property section    
 */

#ifndef SRC_LIBMPG123_X86_64_DEFS_H_
#define SRC_LIBMPG123_X86_64_DEFS_H_

/* Could adapt for x86 trivially, but do we bother? */
#ifdef __x86_64__

#if defined(__CET__) && ((__CET__ & 1) != 0)
  #define GNU_PROPERTY_X86_64_IBT 1 /* bit 0 GNU Notes is for IBT support */
#else
  #define GNU_PROPERTY_X86_64_IBT 0
#endif

#if defined(__CET__) && ((__CET__ & 2) != 0)
  #define GNU_PROPERTY_X86_64_SHSTK 2 /* bit 1 GNU Notes is for PAC support */
#else
  #define GNU_PROPERTY_X86_64_SHSTK 0
#endif

/* Add the BTI support to GNU Notes section */
#if defined(__ASSEMBLER__) && defined(__ELF__)
#if GNU_PROPERTY_X86_64_IBT != 0 || GNU_PROPERTY_X86_64_SHSTK != 0
    .pushsection .note.gnu.property, "a"; /* Start a new allocatable section */
    .balign 8; /* align it on a byte boundry */
    .long 4; /* size of "GNU\0" */
    .long 0x10; /* size of descriptor */
    .long 0x5; /* NT_GNU_PROPERTY_TYPE_0 */
    .asciz "GNU";
    .long 0xc0000002; /* GNU_PROPERTY_X86_FEATURE_1_AND */
    .long 4; /* Four bytes of data */
    .long (GNU_PROPERTY_X86_64_IBT|GNU_PROPERTY_X86_64_SHSTK); /* IBT or SHSTK is enabled */
    .long 0; /* padding for 8 byte alignment */
    .popsection; /* end the section */
#endif /* GNU Notes additions */
#endif /* if __ASSEMBLER__ and __ELF__ */

#endif /* __x86_64__ */

#endif /* SRC_LIBMPG123_X86_64_DEFS_H_ */
