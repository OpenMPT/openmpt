// generated 1999/12/23 3:23:41 CST by temporal@temporal.
// using glademm V0.5.5
//
// newer (non customized) versions of this file go to CModinfoWindow.hh_glade

// you might replace
//    class Foo : public Foo_Glade { ... };
// by
//    typedef Foo_Glade Foo;
// if you didn't make any modifications to the widget

#ifndef _CMODINFOWINDOW_HH
#define _CMODINFOWINDOW_HH

#include"modinfowin_glade.h"

class CModinfoWindow : public CModinfoWindow_Glade
{
private:
	friend class CModinfoWindow_Glade;
	void on_closeBtn_clicked();

public:
	void LoadModInfo(const string& aFilename);
};

#endif
