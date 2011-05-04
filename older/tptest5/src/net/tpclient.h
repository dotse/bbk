/*
 * $Id: tpclient.h,v 1.1 2007/01/31 07:45:40 danron Exp $
 * $Source: /cvsroot-fuse/tptest/tptest5/src/net/tpclient.h,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpclient.h - header file
 *
 * Written by
 *  Ragnar Lönn <prl@gatorhole.com>
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

#ifndef _TPCLIENT_H_
#define _TPCLIENT_H_

#include "tpengine.h"

/*
* Modes recognized by tpengine.c and tpclient.c
*/
#define CLM_NONE		M_NONE
#define CLM_UDP_FDX		M_UDP_FDX
#define CLM_UDP_SEND		M_UDP_SEND
#define CLM_UDP_RECV		M_UDP_RECV
#define CLM_TCP_SEND		M_TCP_SEND
#define CLM_TCP_RECV		M_TCP_RECV
#define CLM_QUERY_MASTER	M_QUERY_MASTER
#define CLM_NAME_LOOKUP		M_NAME_LOOKUP

/*
* Modes used exclusively by tpclient.c
*/
#define	CLM_SERVER_MODE		301
#define CLM_AUTO		302
#define	CLM_AUTO_TCP_SEND	303
#define	CLM_AUTO_TCP_RECV	304
#define CLM_AUTO_UDP_SEND	305
#define CLM_AUTO_UDP_RECV	306
#define	CLM_AUTO_SEND		307
#define	CLM_AUTO_RECV		308
#define CLM_AUTO_TCP		309

// Client defaults

#define DEFAULT_TCPBYTES	(START_TCP_BYTES * 2)
#define DEFAULT_TESTTIME	10

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void RecalculatePPSSZ(TPEngine *);
int AdvanceTest(TPEngine *, int, int, int);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // _TPCLIENT_H_


