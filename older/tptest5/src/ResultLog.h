#pragma once

#include "main.h"

#include "Result.h"

WX_DECLARE_STRING_HASH_MAP( Result*, ResultMap );

class ResultLog
{
public:
	ResultLog(void);
	~ResultLog(void);
	bool Load(wxString name);
	static ResultLog* GetInstance(void);

private:
	static ResultLog	*s_instance;
	ResultMap			*m_Results;
	wxString			m_SaveDelimiter;

	void FreeResults( wxString name );

public:
	bool LoadResults(void);
	Result* GetResults(wxString name);
	bool AddResult(wxString resultname, StringValueList &row);
	StringValueList* GetColumn(wxString resultname, wxString& name);
	bool SaveResults(bool _export = false);

	bool SaveStandardResults( wxString filename, bool _export = false );
	bool SaveAvailabilityResults( wxString filename, bool _export = false );
	bool SaveRTTPacketlossResults( wxString filename, bool _export = false );
	bool SaveThroughputResults( wxString filename, bool _export = false );
};
