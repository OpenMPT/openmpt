/*
 *
 * Reporting.h
 * -----------
 * Purpose: Header file for reporting class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */


#pragma once
#ifndef REPORTING_H
#define REPORTING_H

//=============
class Reporting
//=============
{

public:

	static UINT Notification(CString text, UINT flags = MB_OK, HWND parent = NULL);
	static UINT Notification(CString text, CString caption, UINT flags = MB_OK, HWND parent = NULL);

};

#endif // REPORTING_H
