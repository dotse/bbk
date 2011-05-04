/*
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * macgui_statwin.h - Header for Mac status window
 *
 * Written by
 *  Hans Green <hg@3tag.com>
 *
 * This file is part of the TPTEST system.
 * See the file LICENSE for copyright notice.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA.
 *
 */

#ifndef __STATWIN_H__
#define  __STATWIN_H__

typedef struct DataRates {
	unsigned long	newDataRate;
	unsigned long	oldDataRate;
	unsigned long	newPacketSize;
	unsigned long	oldPacketSize;
	unsigned long	newPacketRate;
	unsigned long	oldPacketRate;
	int				errorFlag;
	char			message[200];
} DataRates;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int		MakeStatWindow(int isMacOSX);
void	PrintMsg(char *msg);
void	Report(char *str);
void	ClearStatWindow(void);
void	ShowNewBestRates(TPEngine *engp);
void	ShowLoadResults(void);
void	ReportStats(struct TPEngine *engp);
void	ReportToClip(TPEngine *engp, int testType, time_t testStart, time_t testEnd);
char	*Long64ToString( long long lVal );
char	*PutOctets(char *destp, struct in_addr *ip);
char	*Int32ToString(int iVal);
void	ClearServerList(TPEngine *engp);
int		AddServerToList(TPEngine *engp, char * str);
void	ValidateRates(DataRates *drp, TPEngine *engp);
void	RecalculatePPSSZ(struct TPEngine *engp);

int		PutIntoClipGlue(void);			// Is actually in testwindow module

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
