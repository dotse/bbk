#pragma once

#include "main.h"

#include "ServerListDialog.h"
#include "DialogAbout.h"

class AdvancedModePanel;	// forward declarations
class SimpleModePanel;		// forward declarations
class AvailabilityModePanel;	// forward declarations
class RTTPacketlossModePanel;	// forward declarations
class ThroughputModePanel;	// forward declarations


class ReportDialog;

class MainFrame :
	public wxFrame
{
public:
	MainFrame(TheApp *theapp);
	~MainFrame(void);
	
	void OnQuit(wxCommandEvent& WXUNUSED(event));
	void OnAbout(wxCommandEvent& WXUNUSED(event));
	void OnToggleMode(wxCommandEvent& event);
	void OnEditServerList(wxCommandEvent& WXUNUSED(event));
	void OnReport(wxCommandEvent& WXUNUSED(event));
	void OnRepeat(wxCommandEvent& WXUNUSED(event));
	void OnHistorySetting(wxCommandEvent& WXUNUSED(event));

	void OnExportAvailability(wxCommandEvent& WXUNUSED(event));
	void OnExportStandard(wxCommandEvent& WXUNUSED(event));
	void OnExportRTTPacketloss(wxCommandEvent& WXUNUSED(event));
	void OnExportThroughput(wxCommandEvent& WXUNUSED(event));
	void OnManual( wxCommandEvent& WXUNUSED(event) );
 
	void OnRepeatTimer( wxTimerEvent& event );
	void OnTimer( wxTimerEvent& event );
	bool IsRepeatActive(void);
	bool IsPagesLocked(void);
	void AbortRepeatTimer(void);

	void PreStartTest(void* pobj, void (*resultfunc)(void*, bool), bool (*executefunc)(void*));
	void StartTest(void);
	void PostTest();
	
	void EnableMenu( bool enable );

	void RefreshPanels(void);

	static MainFrame* GetInstance(void);

private:
	static MainFrame	*s_Instance;

	wxString	m_StartCwd; // Current work directory on startup

	wxMenu		*m_menuFile;
	wxMenu		*m_menuHelp;
	wxMenu		*m_menuFile_Export;
	wxMenu		*m_menuConf;
	wxMenuBar	*m_menuBar;

	int			m_Mode;
	wxPanel		*m_PanelMain;
	wxSizer		*m_SizerMain;
	wxTimer		*m_timer;
	wxTimer		*m_timerRepeat;
	TheApp		*m_app;
	long		m_RepeatStarted;
	bool		m_RepeatActive;

	// Callback for on-test-completed, object and static method
	void	(*m_resultfunc)(void*,bool);
	bool	(*m_executefunc)(void*);
	void	*m_ResultPageInstance;

	ServerListDialog	*m_dlgServerList;
	DialogAbout		*m_dlgAbout;

	wxProgressDialog	*m_dlgProgress;
	ReportDialog		*m_dlgReport;

	AdvancedModePanel	*m_AdvancedPanel;
	SimpleModePanel		*m_SimplePanel;

	DECLARE_EVENT_TABLE()

};
