/**
 * \file ccsds_udp_io.h
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

#ifndef CCSDS_UDP_IO_H_
#define CCSDS_UDP_IO_H_

#ifdef CCSDSUDPACTIVE
#include <stdbool.h>
#include <stdint.h>

#include "udpRX.h"
#include "ccsds_xlate.h"
#include "ccsds_msg_headers.h"
#else
typedef unsigned CCSDS_INFO_RESULT;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CCSDSUDPACTIVE
// The max length of the raw payload buffer
#define CCDSS_UDP_MAXPAYLOADLEN 255

//
// FIXME: The hardcoded data here should be moved into a data file for runtime config.
// (Preferably dcapp's xml file schema and parser.)
//

// The thread ID
#define DCAPP_UDPRX_THREADID 1						// FIXME hardcoded

// The IPv4 address string to listen on
extern char DCAPP_UDPRX_ADDR[IPBUFLEN];				// FIXME hardcoded (see .c)
extern uint8_t DCAPP_UDPRX_ADDRLEN;					// FIXME hardcoded (see .c)

// the connection port to listen on
extern uint32_t DCAPP_UDPRX_PORT;					// FIXME hardcoded (see .c)

// The IPv4 address string to send to
extern char DCAPP_UDPSEND_ADDR[IPBUFLEN];			// FIXME hardcoded (see .c)
extern uint8_t DCAPP_UDPSEND_ADDRLEN;				// FIXME hardcoded (see .c)

// the connection port to send to
extern uint32_t DCAPP_UDPSEND_PORT;					// FIXME hardcoded (see .c)

// The rate at which dcapp queries the reader thread
#define DCAPP_UDPRX_SPECRATE 0.5F /* sec */			// FIXME hardcoded

// Determines how the CCSDS header should be interpreted, T=LE, F=NBO
#define DCAPP_CCSDS_NETWORK_LITTLEEND false			// FIXME hardcoded

// Logs update latency metrics
#define DCAPP_CCSDS_UPDATE_METRICS 1				// FIXME hardcoded

// The reader thread working data, don't alter outside the API!
extern udpRX_thread_data_t udpRXThread;

// reader thread results
extern udpRX_thread_results_t udpRXResults;

// decoded CCSDS primary header
extern CCSDS_PriHdr_t ccsdsPriHdr;

// decoded CCSDS secondary telemetry header
extern CCSDS_TlmSecHdr_t ccsdsTlmHdr;

// CCSDS raw payload buffer
extern char ccsdsPayload[CCDSS_UDP_MAXPAYLOADLEN];

// CCSDS raw payload buffer length
extern int32_t ccsdsPayloadLength;

// wrapping counter of the # of CCSDS Tlm Packets seen
extern uint32_t numTlmCCSDSReceived;

// sending socket fd
extern int sockfdsend;

// socket address for sending commands
extern struct sockaddr_in si_sendother;
#endif

typedef enum dcapp_ccsds_udp_io {
	CCSDSIO_SUCCESS = 0,		// fcn successfully completed
	CCSDSIO_INIT_ERROR = 1,		// init() failed
	CCSDSIO_IO_ERROR = 2,		// unrecoverable i/o error
	CCSDSIO_PARSE_ERROR = 3,	// logic error while parsing packet (shouldn't occur)
	CCSDSIO_NO_NEW_DATA = 4,	// no new data as of last ccsds_udp_readsimdata()
	CCSDSIO_NOT_CCSDS = 5,		// a udp message was received, but it wasn't CCSDS
	CCSDSIO_WRONG_CCSDS = 6		// A CCSDS message was received, but it wasn't what we wanted
} dcapp_ccsds_udp_io;

/**
 * Initializes the UDP CCSDS Reader thread and related structs.
 *
 * Currently, the thread ID, port, and update rate are hardcoded.
 * @return success/failure result
 */
dcapp_ccsds_udp_io ccsds_udp_finish_initialization();

/**
 * Initializes for sending out data over a UDP socket.
 *
 * Currently the address and port are hardcoded
 * @return zero for success, non-zero for failure
 */
int ccsds_udp_finish_send_initialization();

/**
 * Collects message updates from the UDP CCSDS Reader thread and
 * attempts to determine if it is a valid CCSDS packet with a telemetry
 * secondary header.
 *
 * @param *errorcause The CCSDS_INFO_RESULT that may help explain a read error.
 * @return success/failure result
 */
dcapp_ccsds_udp_io ccsds_udp_readtlmmsg(CCSDS_INFO_RESULT *errrorcause);

/**
 * Close down the reader thread.
 */
void ccsds_udp_term(void);

#ifdef __cplusplus
}
#endif

#endif  /* CCSDS_UDP_IO_H_ */
