/**
 * \file ccsds_udp_io.c
 *
 * Read CCSDS data messages from UDP socket using the udpRX and ccsds_xlate
 * functionality from dcapp.
 *
 *
 * **************************************************************************
 *
 * \copyright \{
 * Copyright Statement
 *
 * Copyright (C) 2014 Odyssey Space Research LLC (OSR)
 * Houston, TX. All rights reserved.
 *
 * Copyrighted by Odyssey Space Research LLC (OSR) and proprietary to it. Any
 * unauthorized use of these models including source code, object code
 * or executables is strictly prohibited and OSR assumes no liability for
 * such actions or results thereof.
 *
 * This work has been developed under U. S. Government Contract (NASA)
 * (RPOC contract NNJ12HB20C) and access to it may be granted for U. S.
 * Government work by the following contact:
 *
 * Contact: Allan L. DuPont, EG4
 *          Aeroscience and Flight Mechanics Division
 *          NASA - Johnson Space Center, Houston, TX
 * \}
 * **************************************************************************
 */

#ifdef CCSDSUDPACTIVE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "msg.hh"
#include "ccsds_udp_io.hh"
#include "udpRX.h"
#include "ccsds_msg_headers.h"
#include "ccsds_xlate.h"

#define SecondsElapsed(a,b) ((float)((b).tv_sec - (a).tv_sec) + (0.000001 * (float)((b).tv_usec - (a).tv_usec)))
#define TIDY(a) if (a != NULL) { free(a); a=NULL; }

static struct timeval udpRX_timer;

//
// FIXME: The hardcoded data here should be moved into a data file for runtime config.
// (Preferably dcapp's xml file schema and parser.)
//

//  TESTING:
// listen on
char DCAPP_UDPRX_ADDR[IPBUFLEN] = "127.0.0.1\0";	// FIXME hardcoded (see .h)
uint8_t DCAPP_UDPRX_ADDRLEN = 10;					// FIXME hardcoded (see .h)
// the connection port to listen on
uint32_t DCAPP_UDPRX_PORT = 0x9c41U; /* 40001 decimal */	// FIXME hardcoded (see .h)

// send to (send to self for testing)
char DCAPP_UDPSEND_ADDR[IPBUFLEN] = "127.0.0.1\0";	// FIXME hardcoded (see .h)
uint8_t DCAPP_UDPSEND_ADDRLEN = 10;					// FIXME hardcoded (see .h)
// the connection port to send to
uint32_t DCAPP_UDPSEND_PORT = 0x9c41U; /* 40001 decimal */	// FIXME hardcoded (see .h)

// F.F fdfsim1
// listen on
//char DCAPP_UDPRX_ADDR[IPBUFLEN] = "192.168.3.202\0";	// FIXME hardcoded (see .h)
//uint8_t DCAPP_UDPRX_ADDRLEN = 14;					// FIXME hardcoded (see .h)
// the connection port to listen on
//uint32_t DCAPP_UDPRX_PORT = 0x9c41U; /* 40001 decimal */	// FIXME hardcoded (see .h)

// send to
//char DCAPP_UDPSEND_ADDR[IPBUFLEN] = "192.168.3.101\0";	// FIXME hardcoded (see .h)
//uint8_t DCAPP_UDPSEND_ADDRLEN = 14;					// FIXME hardcoded (see .h)
// the connection port to send to
//uint32_t DCAPP_UDPSEND_PORT = 0x9c43U; /* 40003 decimal */	// FIXME hardcoded (see .h)

// the definition of the external declarations:
udpRX_thread_results_t udpRXResults;
CCSDS_PriHdr_t ccsdsPriHdr;
CCSDS_TlmSecHdr_t ccsdsTlmHdr;
char ccsdsPayload[CCDSS_UDP_MAXPAYLOADLEN];
int32_t ccsdsPayloadLength;
uint32_t numTlmCCSDSReceived;
udpRX_thread_data_t udpRXThread;
int sockfdsend = -1;
// socket address for sending commands
struct sockaddr_in si_sendother;

//
// local variables
//

// UDP reader thread control struct, local copy
udpRX_thread_ctrl_t udpRXCtrl;

dcapp_ccsds_udp_io ccsds_udp_finish_initialization()
{
	memset(&ccsdsPayload,0,CCDSS_UDP_MAXPAYLOADLEN);
	ccsdsPayloadLength = 0;
	numTlmCCSDSReceived = 0;

	udpRX_thread_status st = udpRx_reader_init(DCAPP_UDPRX_THREADID,
    										   DCAPP_UDPRX_ADDR, DCAPP_UDPRX_ADDRLEN,
    										   DCAPP_UDPRX_PORT,
    										   &udpRXThread,
    										   &udpRXResults,
    										   &udpRXCtrl);

	if (st != UDPRX_NOERROR) {
    	error_msg("udpRx_reader_init() failed with status %d",st);
    	debug_msg("\taddress=%s",DCAPP_UDPRX_ADDR);
    	debug_msg("\taddress=%d",DCAPP_UDPRX_PORT);
    	return CCSDSIO_INIT_ERROR;
    }
	else {
		debug_msg("udpRx_reader_init() opened socket");
    	debug_msg("\taddress=%s",DCAPP_UDPRX_ADDR);
    	debug_msg("\taddress=%d",DCAPP_UDPRX_PORT);
	}
    gettimeofday(&udpRX_timer, NULL);
	return CCSDSIO_SUCCESS;
}

int ccsds_udp_finish_send_initialization()
{
	if ((sockfdsend = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		if (sockfdsend == -1) {
			error_msg("Failed to grab socket for writing");
			return sockfdsend;
		}
	}

	memset((char *) &si_sendother, 0, sizeof(si_sendother));
	si_sendother.sin_family = AF_INET;
	int porth = DCAPP_UDPSEND_PORT;
	si_sendother.sin_port = htons(porth); // single sender
	if (inet_aton(DCAPP_UDPSEND_ADDR, &si_sendother.sin_addr) == 0) {
		error_msg("Failed to obtain port %d.", porth);
		close(sockfdsend);
		return -1;
	}

	return 0;
}


dcapp_ccsds_udp_io ccsds_udp_readtlmmsg(CCSDS_INFO_RESULT *errrorcause)
{
	struct timeval now;

    gettimeofday(&now, NULL);

    // only check the reader based on the set update_rate
    if (SecondsElapsed(udpRX_timer, now) < DCAPP_UDPRX_SPECRATE) {
    	return CCSDSIO_NO_NEW_DATA;
    }

    // # of packets received (wrapping counter) by the reader thread
    // Capture before sync to see if we have a new packet.
    uint32_t numpkts = udpRXResults.numpkts;

    udpRX_parent_synchdata(&udpRXThread, &udpRXResults, &udpRXCtrl);

    if (udpRXResults.status != UDPRX_NOERROR) {
    	return CCSDSIO_IO_ERROR;
    }

    if (udpRXResults.numpkts == numpkts) {
    	return CCSDSIO_NO_NEW_DATA;
    }

    // Attempts to convert the received char buffer into a CCSDS packet
    // in host layout.
    CCSDS_INFO_RESULT res = get_CCSDS_Info(&(udpRXResults.rxbuf[0]),
    		udpRXResults.rxbufbytesused,
    		DCAPP_CCSDS_NETWORK_LITTLEEND,
    		&ccsdsPriHdr,
    		&ccsdsTlmHdr,
    		0,
    		&(ccsdsPayload[0]),
    		&ccsdsPayloadLength
    		);

    udpRX_timer = now;

    *errrorcause = res;

	switch (res) {
	case (INVALID_BADARG):
	case (INVALID_BUFSIZE):
	case (INVALID_BADCCSDSVER):
	case (INVALID_BADPKTLENGTH):
	case (INVALID_BADTLMPAYLOADLENGTH):
	case (INVALID_BADCMDPAYLOADLENGTH):
	case (INVALID_BADPAYLOADLENGTH):
		return CCSDSIO_NOT_CCSDS;
	case (VALID_NOSECHDR):
		return CCSDSIO_WRONG_CCSDS; // missing 2nd hdr!
	case (VALID_TLMSECHDR):
		// got it!
		++numTlmCCSDSReceived;
		break;
	case(INVALID_UNEXPTLMSECHDR):
	case(INVALID_UNEXPCMDSECHDR):
	case (VALID_CMDSECHDR):
		return CCSDSIO_WRONG_CCSDS; // cmd not tlm
	case (INVALID_LOGICERROR):
	default:
		return CCSDSIO_PARSE_ERROR; // wtf?
	}

	return CCSDSIO_SUCCESS;
}


void ccsds_udp_term(void)
{
	// close down the reader thread
	udpRX_reader_close(&udpRXThread, &udpRXResults, &udpRXCtrl);

	// close the writer socket
	if (sockfdsend != -1) {
		close(sockfdsend);
	}
}

#else

#include "ccsds_udp_io.hh"

dcapp_ccsds_udp_io ccsds_udp_finish_initialization()
{
	return CCSDSIO_SUCCESS;
}

int ccsds_udp_finish_send_initialization()
{
	return 0;
}

dcapp_ccsds_udp_io ccsds_udp_readtlmmsg(CCSDS_INFO_RESULT *errrorcause)
{
	return CCSDSIO_NO_NEW_DATA;
}

void ccsds_udp_term(void)
{
}

#endif
