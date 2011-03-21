#pragma once
#include "includes.h"
#include <wx/process.h>
#include "main.h"
#include "TestMarshall.h"

class NetstatPanel : public wxPanel
{
public:
	NetstatPanel( wxWindow *parent );
	~NetstatPanel(void);

	void OnStartTest(wxCommandEvent& event);

private:
	void OnTimer( wxTimerEvent& event );
	void OnEndProc( wxProcessEvent &event );
	bool OutputBuffer();

	wxBoxSizer			*m_SizerMain;
	wxTextCtrl			*m_TextOut;

	wxTimer				*m_timer;
	wxProcess			*m_proc;
	wxInputStream		*m_procIn;
	bool				m_die;
	bool				m_running;

	DECLARE_EVENT_TABLE()
};
