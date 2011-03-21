#pragma once

#include "includes.h"
#include <wx/gbsizer.h>

class ResultLog;

class TestDetailDialog : public wxDialog
{
public:
	TestDetailDialog(wxWindow* parent, 
			 wxWindowID id, 
			 const wxString title, 
			 const wxPoint& pos = wxDefaultPosition, 
			 const wxSize& size = wxDefaultSize, 
			 long style = wxDEFAULT_DIALOG_STYLE, 
			 const wxString name = wxT("dialogBox"));
	~TestDetailDialog(void);
	virtual void RefreshList(int row) = 0;
protected:
	wxGridBagSizer		*m_SizerMain;
	wxListCtrl			*m_List;
	ResultLog			*m_rl;
};
