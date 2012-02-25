/*
 * ArrayUtils.cpp
 * --------------
 * Purpose: Some tools for handling CArrays.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "afxtempl.h"
#include "arrayutils.h"

template <class Type>
static void CArrayUtils<Type>::Merge(CArray<Type,Type>& Dest, CArray<Type,Type>& Src)
//----------------------------------------------------------------------------------
{
	Dest.Append(Src);
	RemoveDuplicates(Dest);
}

template <class Type>
static void CArrayUtils<Type>::RemoveDuplicates(CArray<Type,Type>& Src)
//-------------------------------------------------------
{
	Sort(Src, true);
	
	for (int i=0; i<Src.GetSize(); ++i) {
		if(Src[i]==Src[i+1]) {
			Src.RemoveAt(i);
		}
	} 
}

template <class Type>
static void CArrayUtils<Type>::Sort(CArray<Type,Type>& Src, bool Ascend=true)
//----------------------------------------------------------------
{
    m_Ascend = Ascend;
    qsort( Src.GetData(), Src.GetSize(), sizeof(Src[0]), Compare );
}

template <class Type>
static int CArrayUtils<Type>::Compare(const void*A, const void*B)
//--------------------------------------------------
{
    Type* RealA;
    Type* RealB;
    RealA = (Type*)A;
    RealB = (Type*)B;
    if( *RealA < *RealB )
        return( m_Ascend? -1 : 1 );
    if( *RealA > *RealB )
        return( m_Ascend? 1 : -1 );
    return(0);
}