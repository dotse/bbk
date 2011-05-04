#include "Result.h"

Result::Result(void)
{
	m_Header = new StringValueList();
	m_Rows = new RowList();

	m_Header->DeleteContents(true);
	m_Rows->DeleteContents(true);
}

Result::~Result(void)
{
	m_Header->Clear();
	m_Rows->Clear();
	delete m_Header;
	delete m_Rows;
}

bool Result::AddColumn(wxString &name)
{
	m_Header->Append( &name );
	return true;
}


bool Result::AddRow(StringValueList &row)
{
	if( row.GetCount() == m_Header->GetCount() )
	{
		row.DeleteContents(true);
		m_Rows->Append( &row );
		return true;
	}
	else
	{
		return false;
	}
}


StringValueList* Result::GetRow(int row)
{
	return m_Rows->Item(row)->GetData();
}


int Result::GetRowCount(void)
{
	return (int)m_Rows->GetCount();
}

int Result::GetColumnCount(void)
{
	return (int)m_Header->GetCount();
}

StringValueList* Result::GetColumn(wxString& name)
{
	StringValueList *ret = new StringValueList();
	wxString Column;
	int	Index = 0;

	for ( StringValueList::Node *node = m_Header->GetFirst(); node; node = node->GetNext() )
    {
		if( *node->GetData() == name )
		{
			// We found our column
			break;
		}
		Index++;
	}

	for ( RowList::Node *node = m_Rows->GetFirst(); node; node = node->GetNext() )
    {
		wxString *colval = node->GetData()->Item( Index )->GetData();
		ret->Append( colval );
	}

	return ret;
}

bool Result::DeleteRow(int index)
{
	wxRowListNode *n = m_Rows->Item(index);
	m_Rows->DeleteNode( n );
	return false;
}

StringValueList* Result::GetHeader(void)
{
	return this->m_Header;
}
