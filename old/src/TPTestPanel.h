#pragma once
#include "includes.h"
#include "main.h"
#include "TestMarshall.h"
#include "MainFrame.h"
#include "ResultLog.h"
#include "TestPanel.h"

#include <wx/gbsizer.h>

enum {	DIRECTION_INVALID,
	DIRECTION_UP,
	DIRECTION_DOWN,
	DIRECTION_FD };

class TPTestPanel : public TestPanel
{
public:
	TPTestPanel( wxWindow *parent );
	~TPTestPanel(void);

	void OnStartTCPTest(wxCommandEvent& event);
	void OnStartUDPTest(wxCommandEvent& event);
	static void OnTestCompleted_Wrapper(void* obj, bool arg);
	void OnTestCompleted(bool abort);
	void ReportStats(void);
	void ClearTextWindow(void);
	void Report(char *out);
	void OnDirectionBox(wxCommandEvent& event);
	void RecalculateUDPFields(wxCommandEvent& event);
	void UpdateServerList(wxCommandEvent& event);

	bool ExecuteTest(void);
	static bool ExecuteTest_Wrapper(void* obj);
	void RefreshSettingsList(void);
	void OnItemSelected(wxListEvent& event);

private:
	wxGridBagSizer	*m_SizerMain;

	wxStaticBoxSizer	*m_StaticSizerTCPTest;
	wxStaticBoxSizer	*m_StaticSizerUDPTest;

	wxGridBagSizer  *m_SizerGlobal;
	wxGridBagSizer	*m_SizerTCPTest;
	wxGridBagSizer	*m_SizerUDPTest;


	wxTextCtrl	*m_TextOut;
	wxTextCtrl	*m_TPTEST3_ServerURL;
	wxTextCtrl	*m_TPTEST3_ServerPort;

	int			m_SelectedDirection;
	int			m_SelectedMode;

	bool		m_SkipUDPTextEvent;

	ServerList	m_ServerList;
	wxListCtrl	*m_ServerListCtrl;

	DECLARE_EVENT_TABLE()
};
