#pragma once
#include "main.h"

#include "TestPanel.h"

class ThroughputModePanel : public TestPanel
{
public:
	ThroughputModePanel(wxWindow *parent);
	~ThroughputModePanel(void);

	static void OnTestCompleted_Wrapper(void* obj, bool arg);
	void OnTestCompleted(bool abort);

	void RefreshResultList(void);
	void RefreshSettingsList(void);
	void RefreshGUI(void);
	void OnSettingsListItemSelected(wxListEvent& event);
	void OnCheckBoxEnable(wxCommandEvent &event);
	void OnCheckBoxTPTest(wxCommandEvent &event);
	void OnCheckBoxHTTP(wxCommandEvent &event);
	void OnCheckBoxFTP(wxCommandEvent &event);
	void FocusList(void);
	void OnRemoveResult(wxCommandEvent& event);

	bool ExecuteTest(void);
	static bool ExecuteTest_Wrapper(void* obj);
	void ThroughputModePanel::OnResultDetail(wxListEvent& event);


private:
	wxNotebook			*m_BookResult;
	wxNotebookPage		*m_PageSettings;
	wxNotebookPage		*m_PageTextResult;
	wxNotebookPage		*m_PageGraphTPTestDownResult;
	wxNotebookPage		*m_PageGraphTPTestUpResult;
	wxNotebookPage		*m_PageGraphHTTPResult;
	wxNotebookPage		*m_PageGraphFTPResult;
	wxBoxSizer			*m_SizerPageSettingsEdit;
	wxBoxSizer			*m_SizerPageSettings;
	wxBoxSizer			*m_SizerPageTextResult;
	wxBoxSizer			*m_SizerPageGraphTPTestDownResult;
	wxBoxSizer			*m_SizerPageGraphTPTestUpResult;
	wxBoxSizer			*m_SizerPageGraphHTTPResult;
	wxBoxSizer			*m_SizerPageGraphFTPResult;
	wxCheckBox			*m_CheckBoxEnable;
	wxCheckBox			*m_CheckBoxEnableTPTest;
	wxCheckBox			*m_CheckBoxEnableHTTP;
	wxCheckBox			*m_CheckBoxEnableFTP;


	Server				*m_SelectedSettingsServer;
	int					m_SelectedSettingsServerIndex;

	ServerList			*m_ServerList;
	bool				m_Initialized;

	DECLARE_EVENT_TABLE()

public:
	void OnPageChanged(wxNotebookEvent& event);

};
