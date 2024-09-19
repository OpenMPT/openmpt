/*
 * DialogBase.h
 * ------------
 * Purpose: Base class for dialogs that adds functionality on top of CDialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

class DialogBase : public CDialog
{
public:
	using CDialog::CDialog;

	BOOL PreTranslateMessage(MSG *pMsg) override;
};

OPENMPT_NAMESPACE_END
