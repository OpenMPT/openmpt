// generated 1999/12/18 23:45:52 CST by temporal@temporal.
// using glademm V0.5.5
//
// newer (non customized) versions of this file go to window1.hh_glade

// you might replace
//    class Foo : public Foo_Glade { ... };
// by
//    typedef Foo_Glade Foo;
// if you didn't make any modifications to the widget

#ifndef __MODPLUGXMMS_CONFIGWIN_H_INCLUDED__
#  include "configwin_glade.h"
#  define __MODPLUGXMMS_CONFIGWIN_H_INCLUDED__
#include "glademm_support.h"

#ifndef __MODPLUGXMMS_MODPROPS_H_INCLUDED__
#include "../../modprops.h"
#endif

class CConfigureWindow : public CConfigureWindow_Glade
{
public:
	CConfigureWindow();

private:
	ModProperties mModProps;

	friend class CConfigureWindow_Glade;
	void on_bit16_toggled();
	void on_bit8_toggled();
	void on_stereo_toggled();
	void on_mono_toggled();
	void on_samp44_toggled();
	void on_samp22_toggled();
	void on_samp11_toggled();
	void on_fxOversamp_toggled();
	void on_fxNR_toggled();
	void on_fxVolRamp_toggled();
	void on_fxFastinfo_toggled();
	void on_fxReverbOn_toggled();
	void on_fxReverbDepth_drag_motion();
	void on_fxReverbDelay_drag_motion();
	void on_fxBassOn_toggled();
	void on_fxBassAmount_drag_motion();
	void on_fxBassRange_drag_motion();
	void on_fxSurroundOn_toggled();
	void on_fxSurroundDepth_drag_motion();
	void on_fxSurroundDelay_drag_motion();
	void on_fxFadeOn_toggled();
	void on_fxFadeTime_drag_motion();
	void on_okBtn_clicked();
	void on_applyBtn_clicked();
	void on_cancelBtn_clicked();
};

#endif