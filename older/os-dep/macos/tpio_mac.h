/*
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpio_mac.f - Header file for Mac I/O stuff
 *
 * Written by
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

#ifndef __TPIO_MAC__H__
#define __TPIO_MAC__H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

OSStatus TPOTSetup(void);
void TPOTShutDown(void);
int Init_gettimeofday(void);
void SendStatMessage(TPEngine *engp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
