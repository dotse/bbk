#pragma once

#include "main.h"

#include "TestPanel.h"

class AvailabilityModePanel : public TestPanel
{
public:
	AvailabilityModePanel(wxWindow *parent );
	~AvailabilityModePanel(void);

	static void OnTestCompleted_Wrapper(void* obj, bool arg);
	void OnTestCompleted(bool abort);

	void FocusList(void);

private:
	wxNotebook			*m_BookResult;
	wxNotebookPage		*m_PageSettings;
	wxNotebookPage		*m_PageTextResult;
	wxNotebookPage		*m_PageGraphResult;
	wxBoxSizer			*m_SizerPageSettingsEdit;
	wxBoxSizer			*m_SizerPageSettings;
	wxBoxSizer			*m_SizerPageTextResult;
	wxBoxSizer			*m_SizerPageGraphResult;
	wxCheckBox			*m_CheckBoxEnable;
	wxCheckBox			*m_CheckBoxEnableTCP;
	wxCheckBox			*m_CheckBoxEnableICMP;

	Server				*m_SelectedSettingsServer;
	int					m_SelectedSettingsServerIndex;
	ServerList			*m_ServerList;
	DECLARE_EVENT_TABLE()

public:
	void RefreshSettingsList(void);
	void OnSettingsListItemSelected(wxListEvent& event);
	void OnCheckBoxEnable(wxCommandEvent &event);
	void OnCheckBoxTCP(wxCommandEvent &event);
	void OnCheckBoxICMP(wxCommandEvent &event);
	void OnRemoveResult(wxCommandEvent& event);
	void OnResultDetail(wxListEvent& event);
	void RefreshResultList(void);

	bool ExecuteTest(void);
	static bool ExecuteTest_Wrapper(void* obj);
};
