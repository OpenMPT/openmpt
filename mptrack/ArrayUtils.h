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
