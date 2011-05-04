#ifndef __DIALOGABOUT_H
#define __DIALOGABOUT_H

#include "main.h"

#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/msgdlg.h>
#include <wx/dcclient.h>
#include <wx/gbsizer.h>

class DialogAbout : public wxDialog
{
public:
	DialogAbout(wxWindow *parent);

	~DialogAbout(void);

	void OnPaint( wxPaintEvent &WXUNUSED(event) );
	void OnLeftDown( wxMouseEvent &event );

private:
	wxGridBagSizer	*m_Sizer;
	wxBitmap		*m_ghnLogo;
	wxStaticText	*m_stxtAbout;
	wxButton		*m_btnOk;
	DECLARE_EVENT_TABLE()
};

#endif
