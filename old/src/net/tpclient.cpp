/*
 * $Id: tpclient.c,v 1.10 2005/08/17 11:12:12 rlonn Exp $
 * $Source: /cvsroot/tptest/engine/tpclient.c,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpclient.c - test client support functions
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


#include "tpclient.h"
#include "tpengine.h"

void RecalculatePPSSZ(TPEngine *);
int AdvanceTest(TPEngine *, int, int, int);

#ifdef UNIX
double min(double a, double b) { return a < b ? a : b; }
#endif

/*
// Recalculate good PPS and Packetsize values after the user has changed
// the desired data rate. Most modern PCs can output several thousand UDP packets
// per second without stalling due to CPU shortage so I have changed the old
// behaviour somewhat: this program increases the packetsize up to 1400
// bytes then starts increasing the packet rate until it reaches 3000 pps.
// It doesn't continue increasing the packet size before reaching 3000 pps.
// This function also decreases packet rate and size until the data rate
// matches the desired data rate as closely as possible.
//
*/

void RecalculatePPSSZ(TPEngine *engp)
{
	if (engp->packetSize < MIN_PKT_SIZE)
		engp->packetSize = MIN_PKT_SIZE;
	if (engp->bitsPerSecond > 
		(engp->packetsPerSecond * engp->packetSize * 8)) {
		while (engp->bitsPerSecond > (engp->packetsPerSecond * engp->packetSize * 8)) {
			while (engp->packetsPerSecond < 20) {
				engp->packetsPerSecond++;
				continue;
			}
			if (engp->packetSize < 1400) {
				engp->packetSize++;
				continue;
			}
			if (engp->packetsPerSecond < 3000) {
				engp->packetsPerSecond++;
				continue;
			}
			if (engp->packetSize < 32000) {
				engp->packetSize++;
				continue;
			}
			if (engp->packetsPerSecond < 6000) {
				engp->packetsPerSecond++;
				continue;
			}
			if (engp->packetSize < 65000) {
				engp->packetSize++;
				continue;
			}
			engp->packetsPerSecond++;
		}
	}
	else if (engp->bitsPerSecond < (engp->packetsPerSecond * engp->packetSize * 8)) {
		while (engp->bitsPerSecond < (engp->packetsPerSecond * engp->packetSize * 8)) {
			if (engp->packetsPerSecond > 6000) {
				engp->packetsPerSecond--;
				continue;
			}
			if (engp->packetSize > 32000) {
				engp->packetSize--;
				continue;
			}
			if (engp->packetsPerSecond > 3000) {
				engp->packetsPerSecond--;
				continue;
			}
			if (engp->packetSize > 1400) {
				engp->packetSize--;
				continue;
			}
			if (engp->packetsPerSecond > 20) {
				engp->packetsPerSecond--;
				continue;
			}
			if (engp->packetSize > MIN_PKT_SIZE) {
				engp->packetSize--;
				continue;
			}
			engp->packetsPerSecond--;
		}
	}
	// Lower value so we don't *exceed* selected datarate
	while ((engp->packetsPerSecond * engp->packetSize * 8) > engp->bitsPerSecond)
	{
		if (engp->packetsPerSecond > 10 || engp->packetSize == MIN_PKT_SIZE)
			engp->packetsPerSecond--;
		else
			engp->packetSize--;
	}

	engp->nPackets = 0;
}



int AdvanceTest(TPEngine * engp, int SelMode, int Cur, int LastRet)
{
	double BytesPerSecondRecv;

	static double LastBytesPerSecondRecv = 0;
	static double bestTCPSendRate = 0;
	static double bestTCPRecvRate = 0;
	static double bestUDPSendRate = 0;
	static double bestUDPRecvRate = 0;

	int msRecv;

	if (Cur != CLM_NONE) {
		msRecv = ( engp->stats.StopRecv.tv_sec - engp->stats.StartRecv.tv_sec ) * 1000;
		msRecv += ( engp->stats.StopRecv.tv_usec - engp->stats.StartRecv.tv_usec ) / 1000;
		
		if( msRecv != 0 )
			BytesPerSecondRecv = ( (double)(engp->stats.BytesRecvd) * 1000.0 )
								/ (double)(msRecv);
		else
			BytesPerSecondRecv = 0.0;
	}
	else {
		LastBytesPerSecondRecv = 0;
		bestTCPSendRate = bestTCPRecvRate = bestUDPSendRate = bestUDPRecvRate = 0.0;
	}

	switch (SelMode) {
	
		case CLM_AUTO:
			switch (Cur) {
				case CLM_NONE:
					engp->tcpBytes = engp->start_tcpsend_bytes;
					engp->sessionMaxTime = 60;
					LastBytesPerSecondRecv = 0.0;
					return CLM_TCP_SEND;
				case CLM_TCP_SEND:
					if (msRecv < 18000 && LastRet == 0) {
						// aim for 20 secs if last receive time was > 1 sec
                        // The *5 multiplication can work badly for connections
                        // with high, but very fluctuating bandwidth
                        if (msRecv > 1000) 
						    engp->tcpBytes = (UINT32)
							    min(	(float)(engp->tcpBytes) * 5.0, 
									((float)(engp->tcpBytes) * (20000.0 / (float)msRecv))
								    );
                        else
						    engp->tcpBytes = (UINT32)(engp->tcpBytes * 1.8);
						return Cur;
					}
					if (BytesPerSecondRecv > bestTCPSendRate) { 
						bestTCPSendRate = BytesPerSecondRecv;
					}
					if (BytesPerSecondRecv > engp->bestTCPSendRate) {
						engp->bestTCPSendRate = BytesPerSecondRecv;
					}
					LastBytesPerSecondRecv = 0.0;
					engp->tcpBytes = engp->start_tcprecv_bytes;
					engp->sessionMaxTime = 60;
					return CLM_TCP_RECV;
				case CLM_TCP_RECV:
					if (msRecv < 18000 && LastRet == 0) {
						// aim for 20 secs if last receive time was > 1 sec
                        // The *5 multiplication can work badly for connections
                        // with high, but very fluctuating bandwidth
                        if (msRecv > 1000) 
						    engp->tcpBytes = (UINT32)
							    min(	(float)(engp->tcpBytes) * 5.0, 
									((float)(engp->tcpBytes) * (20000.0 / (float)msRecv))
								    );
                        else
						    engp->tcpBytes = (UINT32)(engp->tcpBytes * 1.8);
						return Cur;
					}
					if (BytesPerSecondRecv > bestTCPRecvRate) { 
						bestTCPRecvRate = BytesPerSecondRecv;
					}
					if (BytesPerSecondRecv > engp->bestTCPRecvRate) {
						engp->bestTCPRecvRate = BytesPerSecondRecv;
					}
					LastBytesPerSecondRecv = 0.0;
					if ((bestTCPSendRate * 8) < 20000.0)
						engp->bitsPerSecond = 20000;
					else
						engp->bitsPerSecond = (UINT32)((bestTCPSendRate*8)*0.75);
					engp->sessionTime = 5;
					RecalculatePPSSZ(engp);
					return CLM_UDP_SEND;
				case CLM_UDP_SEND:
					if (engp->stats.PktsRecvd > ((engp->nPackets / 2) + 1) && LastRet == 0) {
						if (BytesPerSecondRecv > bestUDPSendRate) { 
							bestUDPSendRate = BytesPerSecondRecv;
						}
						if (BytesPerSecondRecv > engp->bestUDPSendRate) {
							engp->bestUDPSendRate = BytesPerSecondRecv;
						}
						if (BytesPerSecondRecv > (LastBytesPerSecondRecv * 1.1)) {
							engp->bitsPerSecond = (int)((double)(engp->bitsPerSecond) * 1.5);
							RecalculatePPSSZ(engp);
							LastBytesPerSecondRecv = BytesPerSecondRecv;
							return Cur;
						}
					}
					LastBytesPerSecondRecv = 0.0;
					if ((bestTCPRecvRate * 8) < 20000.0)
						engp->bitsPerSecond = 20000;
					else
						engp->bitsPerSecond = (UINT32)((bestTCPRecvRate*8)*0.75);
					engp->sessionTime = 5;
					RecalculatePPSSZ(engp);
					return CLM_UDP_RECV;
				case CLM_UDP_RECV: /// ***
					if (engp->stats.PktsRecvd > ((engp->nPackets / 2) + 1) && LastRet == 0) {
						if (BytesPerSecondRecv > bestUDPRecvRate) { 
							bestUDPRecvRate = BytesPerSecondRecv;
						}
						if (BytesPerSecondRecv > engp->bestUDPRecvRate) {
							engp->bestUDPRecvRate = BytesPerSecondRecv;
						}
						if (BytesPerSecondRecv > (LastBytesPerSecondRecv * 1.1)) {
							engp->bitsPerSecond = (int)((double)(engp->bitsPerSecond) * 1.5);
							RecalculatePPSSZ(engp);
							LastBytesPerSecondRecv = BytesPerSecondRecv;
							return Cur;
						}
					}
					return CLM_NONE;
					
				default: // not reached
					return CLM_NONE;
			}
			// not reached
		
		case CLM_AUTO_TCP:
			if (Cur == M_NONE) {
				engp->tcpBytes = engp->start_tcpsend_bytes;
				engp->sessionMaxTime = 60;
				return CLM_TCP_SEND;
			}
			if (msRecv < 18000 && LastRet == 0) {
				// aim for 20 secs if last receive time was > 1 sec
                		// The *5 multiplication can work badly for connections
                		// with high, but very fluctuating bandwidth
                		if (msRecv > 1000) 
   				    engp->tcpBytes = (UINT32)
					    min(	(float)(engp->tcpBytes) * 5.0, 
							((float)(engp->tcpBytes) * (20000.0 / (float)msRecv))
    					    );
                		else
				    engp->tcpBytes = (UINT32)(engp->tcpBytes * 1.8);
			    	return Cur;
			}
			if (Cur == M_TCP_SEND) {
				if (BytesPerSecondRecv > engp->bestTCPSendRate) { 
					engp->bestTCPSendRate = BytesPerSecondRecv;
				}
				if (BytesPerSecondRecv > bestTCPSendRate) {
					bestTCPSendRate = BytesPerSecondRecv;
				}
				engp->tcpBytes = engp->start_tcprecv_bytes;
				engp->sessionMaxTime = 60;
				return CLM_TCP_RECV;
			}
			else {
				if (BytesPerSecondRecv > engp->bestTCPRecvRate) { 
					engp->bestTCPRecvRate = BytesPerSecondRecv;
				}
				if (BytesPerSecondRecv > bestTCPRecvRate) {
					bestTCPRecvRate = BytesPerSecondRecv;
				}
			}
			return CLM_NONE;
	
		case CLM_AUTO_TCP_SEND:
		case CLM_AUTO_TCP_RECV:
			if (Cur == M_NONE) {
				engp->sessionMaxTime = 60;
				if (SelMode == CLM_AUTO_TCP_SEND) {
					engp->tcpBytes = engp->start_tcpsend_bytes;
					return CLM_TCP_SEND;
				}
				else {
					engp->tcpBytes = engp->start_tcprecv_bytes;
					return CLM_TCP_RECV;
				}
			}
			if (msRecv < 18000 && LastRet == 0) {
				// aim for 20 secs if last receive time was > 1 sec
                		// The *5 multiplication can work badly for connections
                		// with high, but very fluctuating bandwidth
                		if (msRecv > 1000) 
   				    engp->tcpBytes = (UINT32)
					    min(	(float)(engp->tcpBytes) * 5.0, 
							((float)(engp->tcpBytes) * (20000.0 / (float)msRecv))
    					    );
                		else
				    engp->tcpBytes = (UINT32)(engp->tcpBytes * 1.8);
			    	return Cur;
			}
			if (Cur == M_TCP_SEND) {
				if (BytesPerSecondRecv > engp->bestTCPSendRate) { 
					engp->bestTCPSendRate = BytesPerSecondRecv;
				}
				if (BytesPerSecondRecv > bestTCPSendRate) {
					bestTCPSendRate = BytesPerSecondRecv;
				}
			}
			else {
				if (BytesPerSecondRecv > engp->bestTCPRecvRate) { 
					engp->bestTCPRecvRate = BytesPerSecondRecv;
				}
				if (BytesPerSecondRecv > bestTCPRecvRate) {
					bestTCPRecvRate = BytesPerSecondRecv;
				}
			}
			return CLM_NONE;
			
		case CLM_AUTO_UDP_SEND:
		case CLM_AUTO_UDP_RECV:
			if (Cur == M_NONE) {
				engp->sessionTime = 5;
				if (! engp->flags & TPENGINE_FLAGS_NOINITRATE) {
					engp->bitsPerSecond = 30000;
					RecalculatePPSSZ(engp);
				}
				if (SelMode == CLM_AUTO_UDP_SEND)
					return CLM_UDP_SEND;
				else
					return CLM_UDP_RECV;
			}
			if (engp->stats.PktsRecvd > ((engp->nPackets / 2) + 1) && LastRet == 0) {
				if (Cur == M_UDP_SEND) {
					if (BytesPerSecondRecv > engp->bestUDPSendRate) { 
						engp->bestUDPSendRate = BytesPerSecondRecv;
					}
					if (BytesPerSecondRecv > bestUDPSendRate) {
						bestUDPSendRate = BytesPerSecondRecv;
					}
				}
				else {
					if (BytesPerSecondRecv > engp->bestUDPRecvRate) { 
						engp->bestUDPRecvRate = BytesPerSecondRecv;
					}
					if (BytesPerSecondRecv > bestUDPRecvRate) {
						bestUDPRecvRate = BytesPerSecondRecv;
					}
				}
				if (BytesPerSecondRecv > (LastBytesPerSecondRecv * 1.1)) {
					engp->bitsPerSecond = (int)((double)(engp->bitsPerSecond) * 1.5);
					engp->sessionTime = 5;
					RecalculatePPSSZ(engp);
					LastBytesPerSecondRecv = BytesPerSecondRecv;
					return Cur;
				}
			}
			return CLM_NONE;
			
		case CLM_AUTO_SEND:
			switch (Cur) {
				case CLM_NONE:
					engp->tcpBytes = engp->start_tcpsend_bytes;
					engp->sessionMaxTime = 60;
					return CLM_TCP_SEND;
				case CLM_TCP_SEND:
					if (msRecv < 18000 && LastRet == 0) {
						// aim for 20 secs if last receive time was > 1 sec
                        // The *5 multiplication can work badly for connections
                        // with high, but very fluctuating bandwidth
                        if (msRecv > 1000) 
						    engp->tcpBytes = (UINT32)
							    min(	(float)(engp->tcpBytes) * 5.0, 
									((float)(engp->tcpBytes) * (20000.0 / (float)msRecv))
								    );
                        else
						    engp->tcpBytes = (UINT32)(engp->tcpBytes * 1.8);
						return Cur;
					}
					if (BytesPerSecondRecv > engp->bestTCPSendRate) { 
						engp->bestTCPSendRate = BytesPerSecondRecv;
					}
					if (BytesPerSecondRecv > bestTCPSendRate) {
						bestTCPSendRate = BytesPerSecondRecv;
					}
					LastBytesPerSecondRecv = 0.0;
					if ((engp->bestTCPSendRate * 8) < 20000.0)
						engp->bitsPerSecond = 20000;
					else
						engp->bitsPerSecond = (UINT32)((bestTCPSendRate * 8)*0.75);
					engp->sessionTime = 5;
					RecalculatePPSSZ(engp);
					return CLM_UDP_SEND;
				case CLM_UDP_SEND:

					if (engp->stats.PktsRecvd > ((engp->nPackets / 2) + 1) && LastRet == 0) {
						if (BytesPerSecondRecv > engp->bestUDPSendRate) { 
							engp->bestUDPSendRate = BytesPerSecondRecv;
						}
						if (BytesPerSecondRecv > bestUDPSendRate) {
							bestUDPSendRate = BytesPerSecondRecv;
						}
						if (BytesPerSecondRecv > (LastBytesPerSecondRecv * 1.1)) {
							engp->bitsPerSecond = (int)((double)(engp->bitsPerSecond) * 1.5);
							engp->sessionTime = 5;
							RecalculatePPSSZ(engp);
							LastBytesPerSecondRecv = BytesPerSecondRecv;
							return Cur;
						}
					}
					return CLM_NONE;
			}
			return CLM_NONE;
			
		case CLM_AUTO_RECV:
			switch (Cur) {
				case CLM_NONE:
					engp->tcpBytes = engp->start_tcprecv_bytes;
					engp->sessionMaxTime = 60;
					return CLM_TCP_RECV;
					
				case CLM_TCP_RECV:
					if (msRecv < 18000 && LastRet == 0) {
						// aim for 20 secs if last receive time was > 1 sec
                        // The *5 multiplication can work badly for connections
                        // with high, but very fluctuating bandwidth
                        if (msRecv > 1000) 
						    engp->tcpBytes = (UINT32)
							    min(	(float)(engp->tcpBytes) * 5.0, 
									((float)(engp->tcpBytes) * (20000.0 / (float)msRecv))
								    );
                        else
						    engp->tcpBytes = (UINT32)(engp->tcpBytes * 1.8);
						return Cur;
					}
					if (BytesPerSecondRecv > engp->bestTCPRecvRate) { 
						engp->bestTCPRecvRate = BytesPerSecondRecv;
					}
					if (BytesPerSecondRecv > bestTCPRecvRate) {
						bestTCPRecvRate = BytesPerSecondRecv;
					}
					LastBytesPerSecondRecv = 0.0;
					if ((engp->bestTCPRecvRate * 8) < 20000.0)
						engp->bitsPerSecond = 20000;
					else
						engp->bitsPerSecond = (UINT32)((bestTCPRecvRate * 8)*0.75);
					engp->sessionTime = 5;
					RecalculatePPSSZ(engp);					
					return CLM_UDP_RECV;
					
				case CLM_UDP_RECV:
					if (engp->stats.PktsRecvd > ((engp->nPackets / 2) + 1) && LastRet == 0) {
						if (BytesPerSecondRecv > engp->bestUDPRecvRate) { 
							engp->bestUDPRecvRate = BytesPerSecondRecv;
						}
						if (BytesPerSecondRecv > bestUDPRecvRate) {
							bestUDPRecvRate = BytesPerSecondRecv;
						}
						if (BytesPerSecondRecv > (LastBytesPerSecondRecv * 1.1)) {
							engp->bitsPerSecond = (int)((double)(engp->bitsPerSecond) * 1.5);
							engp->sessionTime = 5;
							RecalculatePPSSZ(engp);
							LastBytesPerSecondRecv = BytesPerSecondRecv;
							return Cur;
						}
					}
					return CLM_NONE;
			}
			return CLM_NONE;
		
			
		case CLM_TCP_SEND:
			if (Cur == CLM_NONE) return SelMode;
			if (msRecv >= 18000 && LastRet == 0) {
				if (BytesPerSecondRecv > engp->bestTCPSendRate) { 
					engp->bestTCPSendRate = BytesPerSecondRecv;
				}
			}
			return CLM_NONE;
			
		case CLM_TCP_RECV:
			if (Cur == CLM_NONE) return SelMode;
			if (msRecv >= 18000 && LastRet == 0) {
				if (BytesPerSecondRecv > engp->bestTCPRecvRate) { 
					engp->bestTCPRecvRate = BytesPerSecondRecv;
				}
			}
			return CLM_NONE;

		case CLM_UDP_SEND:
			if (Cur == CLM_NONE) return SelMode;
			if (engp->stats.PktsRecvd > ((engp->nPackets / 2) + 1) &&
				LastRet == 0) {
				if (BytesPerSecondRecv > engp->bestUDPSendRate) { 
					engp->bestUDPSendRate = BytesPerSecondRecv;
				}
			}
			return CLM_NONE;

		case CLM_UDP_RECV:
			if (Cur == CLM_NONE) return SelMode;
			if (engp->stats.PktsRecvd > ((engp->nPackets / 2) + 1) &&
				LastRet == 0) {
				if (BytesPerSecondRecv > engp->bestUDPRecvRate) { 
					engp->bestUDPRecvRate = BytesPerSecondRecv;
				}
			}
			return CLM_NONE;

		case CLM_UDP_FDX:
			if (Cur == CLM_NONE) return SelMode;
			return CLM_NONE;

		case CLM_QUERY_MASTER:
			if (Cur == CLM_NONE) return SelMode;
			return CLM_NONE;

		case CLM_NAME_LOOKUP:
			if (Cur == CLM_NONE) return SelMode;
			return CLM_NONE;

			// not reached
	}
	// not reached
	return 0;
}
