// generated 1999/12/18 23:45:52 CST by temporal@temporal.
// using glademm V0.5.5
//
// newer (non customized) versions of this file go to window1.cc_glade

// This file is for your program, I won't touch it again!

#include "configwin.h"
#include "../../modplugxmms.h"

CConfigureWindow::CConfigureWindow()
{
	const ModProperties& lModProps = CModplugXMMS::GetModProps();

	mModProps.mSurround       = lModProps.mSurround;
	mModProps.mOversamp       = lModProps.mOversamp;
	mModProps.mMegabass       = lModProps.mMegabass;
	mModProps.mNoiseReduction = lModProps.mNoiseReduction;
	mModProps.mVolumeRamp     = lModProps.mVolumeRamp;
	mModProps.mReverb         = lModProps.mReverb;
	mModProps.mFadeout        = lModProps.mFadeout;
	mModProps.mFastinfo       = lModProps.mFastinfo;

	mModProps.mChannels       = lModProps.mChannels;
	mModProps.mBits           = lModProps.mBits;
	mModProps.mFrequency      = lModProps.mFrequency;

	mModProps.mReverbDepth    = lModProps.mReverbDepth;
	mModProps.mReverbDelay    = lModProps.mReverbDelay;
	mModProps.mBassAmount     = lModProps.mBassAmount;
	mModProps.mBassRange      = lModProps.mBassRange;
	mModProps.mSurroundDepth  = lModProps.mSurroundDepth;
	mModProps.mSurroundDelay  = lModProps.mSurroundDelay;
	mModProps.mFadeTime       = lModProps.mFadeTime;

	fxsurroundon.set_active(mModProps.mSurround);
	fxoversamp.set_active(mModProps.mOversamp);
	fxbasson.set_active(mModProps.mMegabass);
	fxnr.set_active(mModProps.mNoiseReduction);
	fxvolramp.set_active(mModProps.mVolumeRamp);
	fxreverbon.set_active(mModProps.mReverb);
	fxfadeon.set_active(mModProps.mFadeout);
	fxfastinfo.set_active(mModProps.mFastinfo);

	switch(mModProps.mChannels)
	{
	case 1:
		mono->set_active(true);
		break;
	case 2:
	default:
		stereo->set_active(true);
		break;
	}
	switch(mModProps.mBits)
	{
	case 8:
		bit8->set_active(true);
		break;
	case 16:
	default:
		bit16->set_active(true);
		break;
	}
	switch(mModProps.mFrequency)
	{
	case 11025:
		samp11->set_active(true);
		break;
	case 22050:
		samp22->set_active(true);
		break;
	case 44100:
	default:
		samp44->set_active(true);
		break;
	}

	fxreverbdepth_adj->set_value(mModProps.mReverbDepth);
	fxreverbdelay_adj->set_value(mModProps.mReverbDelay);
	fxbassamount_adj->set_value(mModProps.mBassAmount);
	fxbassrange_adj->set_value(mModProps.mBassRange);
	fxsurrounddepth_adj->set_value(mModProps.mSurroundDepth);
	fxsurrounddelay_adj->set_value(mModProps.mSurroundDelay);
	fxfadetime_adj->set_value(mModProps.mFadeTime);
}

void CConfigureWindow::on_bit16_toggled()
{
	if(bit16->get_active())
		mModProps.mBits = 16;
}

void CConfigureWindow::on_bit8_toggled()
{
	if(bit8->get_active())
		mModProps.mBits = 8;
}

void CConfigureWindow::on_stereo_toggled()
{
	if(stereo->get_active())
		mModProps.mChannels = 2;
}

void CConfigureWindow::on_mono_toggled()
{
	if(mono->get_active())
		mModProps.mChannels = 1;
}

void CConfigureWindow::on_samp44_toggled()
{
	if(samp44->get_active())
		mModProps.mFrequency = 44100;
}

void CConfigureWindow::on_samp22_toggled()
{
	if(samp22->get_active())
		mModProps.mFrequency = 22050;
}

void CConfigureWindow::on_samp11_toggled()
{
	if(samp11->get_active())
		mModProps.mFrequency = 11025;
}

void CConfigureWindow::on_fxOversamp_toggled()
{
	mModProps.mOversamp = fxoversamp.get_active();
}

void CConfigureWindow::on_fxNR_toggled()
{
	mModProps.mNoiseReduction = fxnr.get_active();
}

void CConfigureWindow::on_fxVolRamp_toggled()
{
	mModProps.mVolumeRamp = fxvolramp.get_active();
}

void CConfigureWindow::on_fxFastinfo_toggled()
{
	mModProps.mFastinfo = fxfastinfo.get_active();
}

void CConfigureWindow::on_fxReverbOn_toggled()
{
	mModProps.mReverb = fxreverbon.get_active();
}

void CConfigureWindow::on_fxReverbDepth_drag_motion()
{
}

void CConfigureWindow::on_fxReverbDelay_drag_motion()
{
}

void CConfigureWindow::on_fxBassOn_toggled()
{
	mModProps.mMegabass = fxbasson.get_active();
}

void CConfigureWindow::on_fxBassAmount_drag_motion()
{
}

void CConfigureWindow::on_fxBassRange_drag_motion()
{
}

void CConfigureWindow::on_fxSurroundOn_toggled()
{
	mModProps.mSurround = fxsurroundon.get_active();
}

void CConfigureWindow::on_fxSurroundDepth_drag_motion()
{
}

void CConfigureWindow::on_fxSurroundDelay_drag_motion()
{
}

void CConfigureWindow::on_fxFadeOn_toggled()
{
	mModProps.mFadeout = fxfadeon.get_active();
}

void CConfigureWindow::on_fxFadeTime_drag_motion()
{
}

void CConfigureWindow::on_okBtn_clicked()
{
	mModProps.mReverbDepth = (uint32)fxreverbdepth_adj->get_value();
	mModProps.mReverbDelay = (uint32)fxreverbdelay_adj->get_value();
	mModProps.mBassAmount = (uint32)fxbassamount_adj->get_value();
	mModProps.mBassRange = (uint32)fxbassrange_adj->get_value();
	mModProps.mSurroundDepth = (uint32)fxsurrounddepth_adj->get_value();
	mModProps.mSurroundDelay = (uint32)fxsurrounddelay_adj->get_value();
	mModProps.mFadeTime = (uint32)fxfadetime_adj->get_value();

	CModplugXMMS::SetModProps(mModProps);

//	delete_self();        //why does this segfault?
	hide();
}

void CConfigureWindow::on_applyBtn_clicked()
{
	mModProps.mReverbDepth = (uint32)fxreverbdepth_adj->get_value();
	mModProps.mReverbDelay = (uint32)fxreverbdelay_adj->get_value();
	mModProps.mBassAmount = (uint32)fxbassamount_adj->get_value();
	mModProps.mBassRange = (uint32)fxbassrange_adj->get_value();
	mModProps.mSurroundDepth = (uint32)fxsurrounddepth_adj->get_value();
	mModProps.mSurroundDelay = (uint32)fxsurrounddelay_adj->get_value();
	mModProps.mFadeTime = (uint32)fxfadetime_adj->get_value();

	CModplugXMMS::SetModProps(mModProps);
}

void CConfigureWindow::on_cancelBtn_clicked()
{
//	delete_self();        //why does this segfault?
	hide();
}

