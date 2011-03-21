#ifndef __REPORTDIALOG_H
#define __REPORTDIALOG_H

#include "includes.h"
#include <wx/gbsizer.h>
#include <wx/textctrl.h>

#include "TestMarshall.h"
#include "Downloader.h"
#include <wx/process.h>


#define WAIT_STRING wxT("Var god vänta...")

class ReportDialog : public wxDialog
{
public:
	ReportDialog(wxWindow *parent);
	~ReportDialog(void);

	void OnButtonClose(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnButtonSave(wxCommandEvent& event);
	void OnTimer( wxTimerEvent& event );
	void Init(void);
	void Deinit(void);
	bool IsDone(void);
	bool Run(int step = 0);
	bool NextStep(void);
private:
	void OnEndProc( wxProcessEvent &event );
	bool OutputBuffer();
	bool m_proc_running;

	wxGridBagSizer	 *m_SizerMain;
	wxTextCtrl		 *m_Output;
	wxProgressDialog *m_dlgProgress;
	wxButton		 *m_ButtonSaveToFile;

	wxTimer		*m_timer;
	wxProcess			*m_proc;
	wxInputStream		*m_procIn;

	bool		m_Done;
	bool        m_Closed;
	bool        m_TPTestInitialized;
	int		    m_dummy;
	int		    m_step;
	bool		m_stepDone;
	bool		m_stepError;
	wxChar		*m_szTracerouteServer;

	DECLARE_EVENT_TABLE()
};


#endif

