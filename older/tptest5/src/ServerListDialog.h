#ifndef __SERVERLISTDIALOG_H
#define __SERVERLISTDIALOG_H

#include "main.h"
#include "AppConfig.h"

enum { wxID_BUTTON_REFRESH_SERVERLIST = wxID_HIGHEST, wxID_BUTTON_CLOSE };

class ServerListDialog : public wxDialog
{
public:
	ServerListDialog( wxWindow *parent );
	~ServerListDialog(void);

private:
	ServerList	*m_ServerList;
	wxListCtrl	*m_ListCtrl;
	wxTextCtrl	*m_TextCtrlHost;
	wxTextCtrl	*m_TextCtrlTCPPort;
	wxTextCtrl	*m_TextCtrlTPTestPort;
	wxTextCtrl	*m_TextCtrlFTP_Path;
	wxTextCtrl	*m_TextCtrlHTTP_Path;
	wxCheckBox	*m_CheckBoxICMP;
	wxButton	*m_ButtonAdd;
	wxButton	*m_ButtonUpdate;
	wxButton	*m_ButtonDelete;
	wxButton	*m_ButtonRefresh;
	wxButton	*m_ButtonClose;

	wxBoxSizer		*m_SizerMain;
	wxGridBagSizer	*m_SizerEdit;

	int			m_SelectedIndex;

	DECLARE_EVENT_TABLE()

public:
	void RefreshServerList(void);
	void OnItemSelected(wxListEvent& event);
	void OnButtonAdd(wxCommandEvent& event);
	void OnButtonUpdate(wxCommandEvent& event);
	void OnButtonDelete(wxCommandEvent& event);
	void OnButtonRefresh(wxCommandEvent& event);
	void OnButtonClose(wxCommandEvent& event);
	void RefreshMainWindow(void);

};

#endif


