/*
 * tuningbase.cpp
 * --------------
 * Purpose: Alternative sample tuning.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "tuningbase.h"
#include "tuning.h"
#include "../common/mptIO.h"
#include "../common/serialization_utils.h"

#include <cmath>

#include <istream>
#include <ostream>


OPENMPT_NAMESPACE_BEGIN


namespace Tuning {


/*
Version history:
	4->5: Lots of changes, finestep interpretation revamp, fileformat revamp.
	3->4: Changed sizetypes in serialisation from size_t(uint32) to
			smaller types (uint8, USTEPTYPE) (March 2007)
*/



const char CTuning::s_FileExtension[5] = ".tun";



void CTuning::SetNoteName(const NOTEINDEXTYPE& n, const std::string& str)
{
	if(!str.empty())
	{
		m_NoteNameMap[n] = str;
	} else
	{
		const auto iter = m_NoteNameMap.find(n);
		if(iter != m_NoteNameMap.end())
		{
			m_NoteNameMap.erase(iter);
		}
	}
}



bool CTuning::Multiply(const RATIOTYPE& r)
{
	if(r <= 0)
		return true;

	//Note: Multiplying ratios by constant doesn't
	//change, e.g. 'geometricness' status.
	for(auto & ratio : m_RatioTable)
	{
		ratio *= r;
	}
	return false;
}


bool CTuning::ChangeGroupsize(const NOTEINDEXTYPE& s)
{
	if(s < 1)
		return true;

	if(m_TuningType == TT_GROUPGEOMETRIC)
		return CreateGroupGeometric(s, GetGroupRatio(), 0);

	if(m_TuningType == TT_GEOMETRIC)
		return CreateGeometric(s, GetGroupRatio());

	return true;
}



bool CTuning::ChangeGroupRatio(const RATIOTYPE& r)
{
	if(r <= 0)
		return true;

	if(m_TuningType == TT_GROUPGEOMETRIC)
		return CreateGroupGeometric(GetGroupSize(), r, 0);

	if(m_TuningType == TT_GEOMETRIC)
		return CreateGeometric(GetGroupSize(), r);

	return true;
}


} // namespace Tuning


OPENMPT_NAMESPACE_END
