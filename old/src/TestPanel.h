#pragma once
#include "main.h"


class TestPanel : public wxPanel
{
public:
	TestPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
	~TestPanel(void);

	virtual bool ExecuteTest(void) = 0;
	void PostTest(bool abort = false, bool hidewindow = false);
	void PreStartTest(void);
	void OnStartTest( wxCommandEvent& event );
	void SetSBToolTip( wxString label );

	void GetListColumn( wxListCtrl *list, int index, int icol, wxString &out );
	void CopyToClipboard( wxString &text );
	void GetListAsText( wxListCtrl *list, wxString &text );
	void OnResultListKeyDown(wxListEvent& event);
	virtual void RefreshSettingsList(void) = 0;

protected:
	wxButton	*m_StartButton;
	wxBoxSizer	*m_SizerMain;
	wxListCtrl	*m_ListResult;
	wxListCtrl	*m_ListSettings;
};
