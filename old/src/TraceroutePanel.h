#pragma once
#include "includes.h"
#include <wx/process.h>
#include "main.h"

class TraceroutePanel : public wxPanel
{
public:
	TraceroutePanel( wxWindow *parent );
	~TraceroutePanel(void);

	void OnStartTest(wxCommandEvent& event);
	void OnTimer( wxTimerEvent& event );

private:
	void OnEndProc( wxProcessEvent &event );
	bool OutputBuffer();

	wxBoxSizer			*m_SizerMain;
	wxBoxSizer			*m_SizerSettings;
	wxTextCtrl			*m_TextOut;
	wxTextCtrl			*m_Host;

	wxTimer				*m_timer;
	wxProcess			*m_proc;
	wxInputStream		*m_procIn;
	bool				m_die;
	bool				m_running;

	DECLARE_EVENT_TABLE()
};
