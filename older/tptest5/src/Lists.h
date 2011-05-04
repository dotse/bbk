#pragma once

#include <wx/list.h>
#include <wx/listimpl.cpp>
#include <wx/hashmap.h>
#include <wx/string.h>

#include "Server.h"

WX_DECLARE_LIST( Server, ServerList );

WX_DECLARE_LIST( wxString, StringValueList );
WX_DECLARE_LIST( StringValueList, RowList );

