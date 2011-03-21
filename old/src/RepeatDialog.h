#pragma once
#include "includes.h"
#include <wx/gbsizer.h>

#include "AppConfig.h"

class RepeatDialog : public wxDialog
{
public:
	RepeatDialog(wxWindow *parent);
	~RepeatDialog(void);

	void OnOk( wxCommandEvent& event );

private:

	wxGridBagSizer	*m_SizerMain;

	wxCheckBox	*m_ctrlEnable;
	wxChoice	*m_ctrlTime;
	wxChoice	*m_ctrlWait;
	wxButton	*m_btnOk;

	DECLARE_EVENT_TABLE()

};
