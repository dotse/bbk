/*
 * $Id: tpcommon.cpp,v 1.1 2007/01/31 07:45:40 danron Exp $
 * $Source: /cvsroot-fuse/tptest/tptest5/src/net/tpcommon.cpp,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpcommon.c - common TPTEST functions
 *
 * Written by
 *  Ragnar LÅˆnn <prl@gatorhole.com>
 *  Hans Green <hg@3tag.com>
 *
 * Based on earlier work by
 *  Hans NÅ‰stÅÈn
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


#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef UNIX
#include <sys/time.h>
#endif

#include "tpengine.h"
#include "tpcommon.h"
#include "tpio.h"



/*
 * Convert a int value to a string formatted as
 * "64.25 kbit/s" or "10.16 Mbit/s" or "103 bit/s".
 */

char *Int32ToString( int iVal )
{
        static char sBuf[ 256 ];

        if ( iVal >= TP_1MBPS ) {
                sprintf( sBuf, "%.2f Mbit/s", (double)(iVal) / (double)(TP_1MBPS) );
        } else if( iVal > TP_1KBPS ) {
                sprintf( sBuf, "%.2f kbit/s", (double)(iVal) / (double)(TP_1KBPS ) );
        } else {
                sprintf( sBuf, "%d bit/s", iVal );
        }
        return( sBuf );
}



/*
 * Convert a ulong value to a string formatted as
 * "64.25 Kbyte" or "10.16 Mbyte" or "103 byte".
 */
char *UInt32ToString( UINT32 lVal )
{
  static char sBuf[ 256 ];

  if ( lVal >= 1024*1024 ) {
    sprintf( sBuf, "%.2f Mbyte",
	      (double)(lVal) / ( 1024.0 * 1024.0 ) );
  }
  else if( lVal > 1024 ) {
    sprintf( sBuf, "%.2f Kbyte", (double)(lVal) / 1024.0 );
  }
  else {
    sprintf( sBuf, "%lu byte", lVal );
  }
  return( sBuf );

}





/* ---------------------------------------------------------- SameTag ---- *\

        Description:    Case independent tag name compare

        Input:          s1 - string one
                                s2 - string two

        Return:         1 - same tag, 0 - not same
        
\* ----------------------------------------------------------------------- */

int SameTag(char *s1, char *s2)
{
        char     c1, c2;
        
        for (;;) {
                c1 = *s1++ & 255; 
                c2 = *s2++ & 255;
                
                if (c1 == 0 || c2 == 0) return (c1 == c2);
                if (c1 >= 'a') c1 -= ('a' -'A');
                if (c2 >= 'a') c2 -= ('a' -'A');
                if (c1 !=  c2) return 0;
        }
}


/* ----------------------------------------------------- CopyTagField ---- *\

        Description:    Extract value field in 'x1=yy;x2=yy;'-type string

        Input:          destp           Ptr to dest area
                                destSize        Size of dest area (including NUL byte)
                                srcp            Ptr to data (terminated by char < ' ')
                                pname           Tag to look for

        Return:         1 if tag found
                                0 if tag not found

\* ----------------------------------------------------------------------- */

int CopyTagField(
        char    *destp,
        int             destSize,
        char    *srcp,
        char    *pname)
{
        char            *cp, *dp;
        char            *savep, *delp;
        char            *valp;
        int                     len, cnt;
        char            idBuf[20];

        cp = srcp;
		while (*cp) {
                savep = cp;
                valp = strchr(savep, '=');
                if (valp == 0) goto done;                       /* No more assigns */

                delp = strchr(savep, ';');
                if (delp && delp < valp) {                      /* Skip some leading junk */
                        cp = delp + 1;
                        continue;
                }
                len = (int)(valp - savep);
                if (len < sizeof(idBuf)) {
                        memcpy(idBuf, savep, len);
                        idBuf[len] = 0;

                        if (SameTag(idBuf, pname)) {            /* Found the tag */                         
                                valp += 1;
                                for (cnt = 0, dp = destp ; *valp ; valp++) {
                                        if (*valp == ';' || (*valp & 255) < ' ') {
                                                break;
                                        }
                                        if (cnt < destSize - 1) {
                                                *dp++ = *valp;
                                                cnt += 1;
                                        }
                                }
                                *dp = 0;
                                return 1;
                        }
                }
                
                /* Skip until next field */
                /* ===================== */
                
                for (cp = valp ; *cp != ';' ; cp++) if (*cp == 0) goto done;

                cp += 1;                                                        /* Bypass delimiter 
*/
        }
done:
        return 0;                                                               /* Not found */
}


// Fill a tpStats structure with the contents from a STATS line
int GetStatsFromLine(char *line, TPStats *s)
{
	char valBuf[30];

	if (strncmp(line, "STATS ", 6) != 0)
		return -1;
	memset(valBuf, 0, 30);

	if (CopyTagField(valBuf, 29, line+6, "majorv"))
		s->MajorVersion = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "minorv"))
		s->MinorVersion = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "pktssent"))
		s->PktsSent = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "pktsunsent"))
		s->PktsUnSent = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "pktsrcvd"))
		s->PktsRecvd = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "bytessent"))
		sscanf(valBuf, "%" LONG_LONG_PREFIX "d", &(s->BytesSent));
	if (CopyTagField(valBuf, 29, line+6, "bytesrcvd"))
		sscanf(valBuf, "%" LONG_LONG_PREFIX "d", &(s->BytesRecvd));
	if (CopyTagField(valBuf, 29, line+6, "maxrtt"))
		s->MaxRoundtrip = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "minrtt"))
		s->MinRoundtrip = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "oocount"))
		s->ooCount = atoi(valBuf);

	if (CopyTagField(valBuf, 29, line+6, "txstart_s"))
		s->StartSend.tv_sec = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "txstart_us"))
		s->StartSend.tv_usec = atoi(valBuf);

	if (CopyTagField(valBuf, 29, line+6, "txstop_s"))
		s->StopSend.tv_sec = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "txstop_us"))
		s->StopSend.tv_usec = atoi(valBuf);

	if (CopyTagField(valBuf, 29, line+6, "rxstart_s"))
		s->StartRecv.tv_sec = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "rxstart_us"))
		s->StartRecv.tv_usec = atoi(valBuf);

	if (CopyTagField(valBuf, 29, line+6, "rxstop_s"))
		s->StopRecv.tv_sec = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "rxstop_us"))
		s->StopRecv.tv_usec = atoi(valBuf);

	if (CopyTagField(valBuf, 29, line+6, "totrtt"))
		s->TotalRoundtrip = atoi(valBuf);
	if (CopyTagField(valBuf, 29, line+6, "nortt"))
		s->nRoundtrips = atoi(valBuf);

	if (CopyTagField(valBuf, 101, line + 6, "email"))
		strcpy(s->email, valBuf);

	if (CopyTagField(valBuf, 101, line + 6, "pwd"))
		strcpy(s->pwd, valBuf);

	return 0;

}



// Create a STATS line from a tpStats structure
char * CreateLineFromStats(TPStats *s, char *destp)
{
	sprintf(destp, "STATS majorv=%u;minorv=%u;pktssent=%lu;pktsunsent=%lu;pktsrcvd=%lu;"
		"bytessent=%" LONG_LONG_PREFIX "d;bytesrcvd=%" LONG_LONG_PREFIX "d;"
		"maxrtt=%lu;minrtt=%lu;totrtt=%lu;nortt=%lu;oocount=%lu;txstart_s=%ld;txstart_us=%ld;"
		"txstop_s=%ld;txstop_us=%ld;rxstart_s=%ld;rxstart_us=%ld;"
		"rxstop_s=%ld;rxstop_us=%ld;email=%s;pwd=%s",
		s->MajorVersion, s->MinorVersion, s->PktsSent, s->PktsUnSent,
		s->PktsRecvd, s->BytesSent, s->BytesRecvd, s->MaxRoundtrip,
		s->MinRoundtrip, s->TotalRoundtrip, s->nRoundtrips, s->ooCount,
		(s->StartSend.tv_sec), (s->StartSend.tv_usec),
		(s->StopSend.tv_sec), (s->StopSend.tv_usec),
		(s->StartRecv.tv_sec), (s->StartRecv.tv_usec),
		(s->StopRecv.tv_sec), (s->StopRecv.tv_usec), s->email, s->pwd );
	return destp;
}



// Get 3-digit reply code from a reply string
int ReplyCode(char * str)
{
	int ret;
	
	ret = atoi(str);
	if (ret < 1000 && ret > 99) return ret;
	return 0;
}



#ifdef NO_HTONL
/* ------------------------------------------------------------ htonl ---- *\


\* ----------------------------------------------------------------------- */

long htonl(long l)
{
	long			ti, to;
	unsigned char	*ucp;
	
	ucp = (unsigned char *) &ti;
	ti = 1;
	if (ucp[3] == 1) return l; 			// Running on hi-endian
		
	ti = l;
	to = ucp[3];
	to = (to << 8) + ucp[2];
	to = (to << 8) + ucp[1];
	to = (to << 8) + ucp[0];
	return to;
}
#endif // NO_HTONL



#ifdef NO_NTOHL
/* ------------------------------------------------------------ ntohl ---- *\


\* ----------------------------------------------------------------------- */

long ntohl(long l)
{
	long			ti, to;
	unsigned char	*ucp;
	
	ucp = (unsigned char *) &ti;
	ti = 1;
	if (ucp[3] == 1) return l; 			// Running on hi-endian
		
	ti = l;
	to = ucp[0];
	to = (to << 8) + ucp[1];
	to = (to << 8) + ucp[2];
	to = (to << 8) + ucp[3];
	return to;
}
#endif // NO_NTOHL


/* -------------------------------------------------------- TVAddUSec ---- *\


\* ----------------------------------------------------------------------- */

void	TVAddUSec(struct timeval *tp, int usecs)
{
	tp->tv_usec += usecs;
	if (tp->tv_usec > 1000000) {
		tp->tv_sec += tp->tv_usec / 1000000;
		tp->tv_usec = tp->tv_usec % 1000000;
	}
}

/* -------------------------------------------------------- TVCompare ---- *\


\* ----------------------------------------------------------------------- */

int		TVCompare(struct timeval *tp1, struct timeval *tp2)
{
	if (tp1->tv_sec > tp2->tv_sec) return 1;
	if (tp1->tv_sec < tp2->tv_sec) return -1;
	if (tp1->tv_usec > tp2->tv_usec) return 1;
	if (tp1->tv_usec < tp2->tv_usec) return -1;
	return 0;
}


#ifdef TRCLOG

/* ----------------------------------------------------------- TrcLog ---- *\

	Typical usage in program
	
	if (debugWanted) {
		TrcSetFile("MYDEBUG.LOG");		// Please log to file "MYDEBUG.LOG"
		TrcSetOptions(TRCOPT_STDERR);	// Please log to stderr
		TrcEnable(1);					// Enable bit 1
	}
	
	And the to get conditional runtime log:
	
	TrcLog(1, "Running version %d", version);	// Message without newline
	
\* ----------------------------------------------------------------------- */

static char	logFileName[200];
static FILE *logFp;
static unsigned long logDebugBits;
static unsigned long trcOptions;

void TrcClose(void)
{
	if (logFp == 0) return;
	fclose(logFp);
	logFp = 0;
}
void TrcSetFile(char *fileName)
{
	TrcClose();
	strncpy(logFileName, fileName, sizeof(logFileName));
	logFileName[sizeof(logFileName) -1] = 0;
	return;
}
unsigned long TrcSetOptions(unsigned long options)
{
	trcOptions |= options;
	return trcOptions;
}
int TrcLog(unsigned long theBits, char *format, ...)
{
	char		timeBuf[40];
	time_t		tid;
	struct tm	*tp;
	va_list		argp;

	va_start(argp, format);
	
	if ((theBits & logDebugBits) == 0) return 0;
	
	time(&tid);
	tp = localtime(&tid);
	sprintf(timeBuf, "%02d-%02d:%02d.%02d.%02d", tp->tm_mon, tp->tm_mday,
		tp->tm_hour, tp->tm_min, tp->tm_sec);

	if (logFp == 0 && logFileName[0] != 0) {
		logFp = fopen(logFileName, "a");
	}
	if (logFp) {
		fprintf(logFp, "%s: ", timeBuf);
	 	vfprintf(logFp, format, argp);
		fprintf(logFp, "\n");
	}
	if (trcOptions & TRCOPT_STDERR) {
		fprintf(stderr, "%s: ", timeBuf);
	 	vfprintf(stderr, format, argp) ;
		fprintf(stderr, "\n");
	}
	return 1;
}
unsigned long TrcEnable(unsigned long bits)
{
	logDebugBits |= bits;
	return logDebugBits;
}



#endif    /* TRCLOG */
