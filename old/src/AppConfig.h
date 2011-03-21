#pragma once

#include "includes.h"
#include "Server.h"

#include "Downloader.h"

//WX_DECLARE_STRING_HASH_MAP( wxString, ConfigHash );

class AppConfig
{
public:
	AppConfig(void);
	~AppConfig(void);
	bool LoadServerList(void);
	bool SaveServerList(void);

	bool RemoteLoadServerList(void);

	static AppConfig* GetInstance(void);

	bool SetValue( wxString key, wxString &val );
	bool GetValue( wxString key, wxString &val );
	void SaveConfig(void);

private:
	static AppConfig	*s_instance;
	
	wxFileConfig		*m_fileconfig;

//	ConfigHash			m_config;
};
