/*
 * tuningbase.h
 * ------------
 * Purpose: Alternative sample tuning.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#include <string>
#include <vector>
#include <cmath>
#include <iosfwd>
#include <limits>
#include "../common/typedefs.h"


OPENMPT_NAMESPACE_BEGIN


enum class TuningSerializationResult : int {
	Success = 1,
	NoMagic = 0,
	Failure = -1
};


//Tuning baseclass; basic functionality is to map note to ratio.
class CTuningBase
//===============
{
	//NOTEINDEXTYPE: Some signed integer-type.
	//UNOTEINDEXTYPE: Unsigned NOTEINDEXTYPE
	//RATIOTYPE: Some 'real figure' type able to present ratios.
	//STEPINDEXTYPE: Counter of steps between notes. If there is no 'finetune'(finestepcount == 0),
			//then 'step difference' between notes is the
			//same as differences in NOTEINDEXTYPE. In a way similar to ticks and rows in pattern -
			//ticks <-> STEPINDEX, rows <-> NOTEINDEX

public:
//BEGIN TYPEDEFS:
	typedef int16 NOTEINDEXTYPE;
	typedef uint16 UNOTEINDEXTYPE;
	typedef float32 RATIOTYPE; //If changing RATIOTYPE, serialization methods may need modifications.
	typedef int32 STEPINDEXTYPE;
	typedef uint32 USTEPINDEXTYPE;

	//Validity Range PAIR.
	typedef std::pair<NOTEINDEXTYPE, NOTEINDEXTYPE> VRPAIR;

	typedef uint16 TUNINGTYPE;

//END TYPEDEFS


//BEGIN PUBLIC STATICS

	static const char s_FileExtension[5];

	static const TUNINGTYPE TT_GENERAL;
	static const TUNINGTYPE TT_GROUPGEOMETRIC;
	static const TUNINGTYPE TT_GEOMETRIC;

//END PUBLIC STATICS

};

#define NOTEINDEXTYPE_MIN (std::numeric_limits<NOTEINDEXTYPE>::min)()
#define NOTEINDEXTYPE_MAX (std::numeric_limits<NOTEINDEXTYPE>::max)()
#define UNOTEINDEXTYPE_MAX (std::numeric_limits<UNOTEINDEXTYPE>::max)()
#define STEPINDEXTYPE_MIN (std::numeric_limits<STEPINDEXTYPE>::min)()
#define STEPINDEXTYPE_MAX (std::numeric_limits<STEPINDEXTYPE>::max)()
#define USTEPINDEXTYPE_MAX (std::numeric_limits<USTEPINDEXTYPE>::max)()


OPENMPT_NAMESPACE_END
