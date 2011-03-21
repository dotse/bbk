#pragma once

#include "main.h"

#include "TestMarshall.h"
#include "ResultLog.h"
#include "GraphPanel.h"
#include "TestPanel.h"


class SimpleModePanel : public TestPanel
{
public:
	SimpleModePanel( wxWindow *parent );
	~SimpleModePanel(void);

private:
	wxNotebook		*m_BookResult;
	wxNotebookPage		*m_PageTextResult;
	wxNotebookPage		*m_PageGraphResult;
	wxBoxSizer		*m_SizerPageTextResult;
	wxBoxSizer		*m_SizerPageGraphResult;

	DECLARE_EVENT_TABLE()

public:
	bool ExecuteTest(void);
	static bool ExecuteTest_Wrapper(void* obj);
	
	void OnPaintGraphPanel( wxPaintEvent &WXUNUSED(event) );

	static void OnTestCompleted_Wrapper(void* obj, bool arg);
	void OnTestCompleted(bool abort);

	bool RefreshResultList(void);
	void OnRemoveResult(wxCommandEvent& event);
	void OnResultListItemSelected(wxListEvent& event);
	void RefreshSettingsList(void);
};
