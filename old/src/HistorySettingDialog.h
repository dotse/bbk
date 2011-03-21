#pragma once
#include "includes.h"
#include <wx/gbsizer.h>

#include "AppConfig.h"

class HistorySettingDialog : public wxDialog
{
public:
	HistorySettingDialog(wxWindow *parent);
	~HistorySettingDialog(void);

	void OnOk( wxCommandEvent& event );

private:

	wxGridBagSizer	*m_SizerMain;

	wxChoice	*m_ctrlCount;
	wxButton	*m_btnOk;

	DECLARE_EVENT_TABLE()
};
