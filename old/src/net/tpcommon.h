/*
 * $Id: tpcommon.h,v 1.4 2002/09/16 14:10:42 rlonn Exp $
 * $Source: /cvsroot/tptest/engine/tpcommon.h,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpcommon.h - header file
 *
 * Written by
 *  Ragnar Lönn <prl@gatorhole.com>
 *  Hans Green <hg@3tag.com>
 *
 * This file is part of the TPTEST system.
 * See the file LICENSE for copyright notice.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA.
 *
 */

#ifndef _TPCOMMON_H_
#define _TPCOMMON_H_

#include "tpengine.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int SameTag(char *s1, char *s2);
int CopyTagField(char *destp, int destSize, char *srcp, char *pname);
int GetSessionFromLine(char *, TPEngine *);
char * CreateSessionLine(TPEngine *, char *);
int GetStatsFromLine(char *, TPStats *);
char * CreateLineFromStats(TPStats *, char *);
int ReplyCode(char *);
void TVAddUSec(struct timeval *, int);
int	TVCompare(struct timeval *, struct timeval *);
char *Int32ToString( int );
char *UInt32ToString( UINT32 );

#ifdef NO_HTONL
long htonl(long);
#endif

#ifdef NO_NTOHL
long ntohl(long);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif	// _TPCOMMON_H_

