#ifndef _ICMP_PING_H_
#define _ICMP_PING_H_

#ifdef UNIX
#include <sys/time.h>
#include <netinet/in.h>
#endif
#include "valuestruct.h"

struct icmp_ping_arg_struct * new_icmp_ping_arg_struct();
void delete_icmp_ping_arg_struct(struct icmp_ping_arg_struct *p);
void * icmp_ping_executor(void *arg);

#pragma pack(1)

struct icmp_ping_arg_struct {
  /* parameters */
  unsigned int num_packets;
  unsigned int packetsize;
  unsigned int packet_interval_ms;
  unsigned int packet_timeout_ms;
  unsigned int number_hosts;
  in_addr *hostlist;
  /* general results */
  unsigned int responding_hosts;
  unsigned int packets_sent;
  unsigned int packets_received;
  double rtt_max;
  double rtt_min;
  double rtt_avg;
  /* individual results */
  /* values is an array, number_hosts * num_packets in size */
  struct valuestruct *values;
  unsigned int no_values;
  void *userdata;
};

#define ICMP_ECHOREPLY	0
#define ICMP_ECHOREQ	8

// IP Header -- RFC 791
typedef struct tagIPHDR
{
	u_char  VIHL;			// Version and IHL
	u_char	TOS;			// Type Of Service
	short	TotLen;			// Total Length
	short	ID;				// Identification
	short	FlagOff;		// Flags and Fragment Offset
	u_char	TTL;			// Time To Live
	u_char	Protocol;		// Protocol
	u_short	Checksum;		// Checksum
	struct	in_addr iaSrc;	// Internet Address - Source
	struct	in_addr iaDst;	// Internet Address - Destination
}IPHDR, *PIPHDR;


// ICMP Header - RFC 792
typedef struct tagICMPHDR
{
	u_char	Type;			// Type
	u_char	Code;			// Code
	u_short	Checksum;		// Checksum
	u_short	ID;				// Identification
	u_short	Seq;			// Sequence
	char	Data;			// Data
}ICMPHDR, *PICMPHDR;


// ICMP Echo Request
typedef struct tagECHOREQUEST
{
	ICMPHDR icmpHdr;
	struct timeval	sendTime;
	char	cData[1];
}ECHOREQUEST, *PECHOREQUEST;


// ICMP Echo Reply
typedef struct tagECHOREPLY
{
	IPHDR	ipHdr;
	ECHOREQUEST	echoRequest;
	char    cFiller[256];
}ECHOREPLY, *PECHOREPLY;


#pragma pack()

#endif



