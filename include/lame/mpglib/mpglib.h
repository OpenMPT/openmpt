/*
 * Copyright (C) 1999-2010 The L.A.M.E. project
 *
 * Initially written by Michael Hipp, see also AUTHORS and README.
 *  
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef LAME_MPGLIB_H
#define LAME_MPGLIB_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "lame.h"

#ifdef HAVE_MPG123
#include <mpg123.h>
#ifndef MPG123_API_VERSION
#error "Seems like you got the wrong mpg123 header. No MPG123_API_VERSION defined."
#endif
#if (MPG123_API_VERSION < 45)
#error "Need mpg123 API >= 45."
#endif
#endif

#ifndef plotting_data_defined
#define plotting_data_defined
struct plotting_data;
typedef struct plotting_data plotting_data;
#endif


extern void lame_report_fnc(lame_report_function f, const char *format, ...);

typedef struct mpstr_tag {
#ifdef HAVE_MPG123
    mpg123_handle *mh;
    struct mpg123_moreinfo mi;
#endif
    plotting_data *pinfo;

    lame_report_function report_msg;
    lame_report_function report_dbg;
    lame_report_function report_err;
} MPSTR, *PMPSTR;


#endif /* LAME_MPGLIB_H */
