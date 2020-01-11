/*
 * ResizableDialog.cpp
 * -------------------
 * Purpose: A wrapper for resizable MFC dialogs that fixes the dialog's minimum size
 *          (as MFC does not scale controls properly if the user makes the dialog smaller than it originally was)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "ResizableDialog.h"
#include "resource.h"

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(ResizableDialog, CDialog)
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

ResizableDialog::ResizableDialog(UINT nIDTemplate, CWnd *pParentWnd)
	: CDialog(nIDTemplate, pParentWnd)
{ }


BOOL ResizableDialog::OnInitDialog()
{
	CRect rect;
	GetWindowRect(rect);
	m_minSize = rect.Size();
	
	HICON icon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MODULETYPE));
	SetIcon(icon, FALSE);
	SetIcon(icon, TRUE);

	return CDialog::OnInitDialog();
}


void ResizableDialog::OnGetMinMaxInfo(MINMAXINFO *mmi)
{
	mmi->ptMinTrackSize = m_minSize;
	CDialog::OnGetMinMaxInfo(mmi);
}

OPENMPT_NAMESPACE_END
