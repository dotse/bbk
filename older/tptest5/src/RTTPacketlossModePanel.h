#pragma once

#include "main.h"

#include "TestPanel.h"

class RTTPacketlossModePanel : public TestPanel
{
public:
	RTTPacketlossModePanel(wxWindow *parent);
	~RTTPacketlossModePanel(void);

	static void OnTestCompleted_Wrapper(void* obj, bool arg);
	void OnTestCompleted(bool abort);
	void RefreshResultList(void);
	void RefreshSettingsList(void);
	void RefreshGUI(void);
	void OnSettingsListItemSelected(wxListEvent& event);
	void OnCheckBoxEnable(wxCommandEvent &event);
	void OnResultDetail(wxListEvent& event);
	void OnRemoveResult(wxCommandEvent& event);
	void FocusList(void);

	bool ExecuteTest(void);
	static bool ExecuteTest_Wrapper(void* obj);

private:
	wxNotebook			*m_BookResult;
	wxNotebookPage		*m_PageSettings;
	wxNotebookPage		*m_PageTextResult;
	wxNotebookPage		*m_PageGraphRTTResult;
	wxNotebookPage		*m_PageGraphPLResult;
	wxNotebookPage		*m_PageGraphJitterResult;
	wxBoxSizer			*m_SizerPageSettingsEdit;
	wxBoxSizer			*m_SizerPageSettings;
	wxBoxSizer			*m_SizerPageTextResult;
	wxBoxSizer			*m_SizerPageGraphRTTResult;
	wxBoxSizer			*m_SizerPageGraphPLResult;
	wxBoxSizer			*m_SizerPageGraphJitterResult;
	wxCheckBox			*m_CheckBoxEnable;

	Server				*m_SelectedSettingsServer;
	int					m_SelectedSettingsServerIndex;

	ServerList			*m_ServerList;
	bool				m_Initialized;

	DECLARE_EVENT_TABLE()

public:
	void OnPageChanged(wxNotebookEvent& event);
};
