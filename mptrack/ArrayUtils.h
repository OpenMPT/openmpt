/*
 * ArrayUtils.h
 * ------------
 * Purpose: Some tools for handling CArrays.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

template <class Type>
class CArrayUtils
{
public:
	 static void Merge(CArray<Type,Type>& Dest, CArray<Type,Type>& Src);
     static void RemoveDuplicates(CArray<Type,Type>& Src);
	 static void Sort( CArray<Type,Type>& Src, bool Ascend=true);
     static int Compare(const void*A, const void*B);
	 static bool m_Ascend;
};
