#pragma once	
#include "includes.h"
#include <wx/process.h>
#include "main.h"

class PingPanel : public wxPanel
{
public:
	PingPanel( wxWindow *parent );
	~PingPanel(void);

	void OnStartTest(wxCommandEvent& event);
	void OnTimer( wxTimerEvent& event );

private:
	void OnEndProc( wxProcessEvent &event );
	bool OutputBuffer();

	wxGridBagSizer		*m_SizerMain;

	wxTextCtrl			*m_TextOut;
	wxTextCtrl			*m_Host;
	wxTextCtrl			*m_Packetsize;
	wxTextCtrl			*m_TTL;
	wxTextCtrl			*m_Delay;
	wxTextCtrl			*m_Count;

	wxTimer				*m_timer;
	wxProcess			*m_proc;
	wxInputStream		*m_procIn;
	bool				m_die;
	bool				m_running;

	DECLARE_EVENT_TABLE()
};
