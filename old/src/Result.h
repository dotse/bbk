#pragma once
#include "includes.h"
#include "Lists.h"

class Result
{
public:
	Result(void);
	~Result(void);
private:
	StringValueList		*m_Header;
	RowList				*m_Rows;
public:
	bool				AddColumn(wxString &name);
	bool				AddRow(StringValueList &row);
	StringValueList*	GetRow(int row);
	int					GetRowCount(void);
	int					GetColumnCount(void);
	StringValueList*	GetColumn(wxString& name);
	bool				DeleteRow(int index);
	StringValueList*	GetHeader(void);
};
