/**
 * \file ccsds_udp_comm.hh
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

#ifndef _CCSDS_UDP_COMM_HH_
#define _CCSDS_UDP_COMM_HH_

#include <stdbool.h>
#include <stdint.h>
#include "comm.hh"
#ifdef CCSDSUDPACTIVE
#include "udpRX.h"
#include "ccsds_xlate.h"
#include "ccsds_msg_headers.h"
#endif

// The max length of the raw payload buffer
#define CCDSS_UDP_MAXPAYLOADLEN 255
// The thread ID
#define DCAPP_UDPRX_THREADID 1                         // FIXME hardcoded
// The rate at which dcapp queries the reader thread
#define DCAPP_UDPRX_SPECRATE 0.5F /* sec */            // FIXME hardcoded
// Determines how the CCSDS header should be interpreted, T=LE, F=NBO
#define DCAPP_CCSDS_NETWORK_LITTLEEND false            // FIXME hardcoded
// Logs update latency metrics
#define DCAPP_CCSDS_UPDATE_METRICS 1                   // FIXME hardcoded

class CcsdsUdpCommModule : public CommModule
{
    public:
        CcsdsUdpCommModule();
        virtual ~CcsdsUdpCommModule();

        CommModule::CommStatus read(void);
        CommModule::CommStatus write(void);

    private:
#ifdef CCSDSUDPACTIVE
        typedef enum dcapp_ccsds_udp_io {
            CCSDSIO_SUCCESS = 0,        // fcn successfully completed
            CCSDSIO_INIT_ERROR = 1,        // init() failed
            CCSDSIO_IO_ERROR = 2,        // unrecoverable i/o error
            CCSDSIO_PARSE_ERROR = 3,    // logic error while parsing packet (shouldn't occur)
            CCSDSIO_NO_NEW_DATA = 4,    // no new data as of last ccsds_udp_readsimdata()
            CCSDSIO_NOT_CCSDS = 5,        // a udp message was received, but it wasn't CCSDS
            CCSDSIO_WRONG_CCSDS = 6        // A CCSDS message was received, but it wasn't what we wanted
        } dcapp_ccsds_udp_io;

        int read_active;
        int write_active;
        struct timeval last_read_try;
        struct timeval last_write_try;
        struct timeval udpRX_timer;

        // The IPv4 address string to listen on
        char DCAPP_UDPRX_ADDR[IPBUFLEN];                // FIXME hardcoded (see .c)
        uint8_t DCAPP_UDPRX_ADDRLEN;                    // FIXME hardcoded (see .c)

        // the connection port to listen on
        uint32_t DCAPP_UDPRX_PORT;                      // FIXME hardcoded (see .c)

        // The IPv4 address string to send to
        char DCAPP_UDPSEND_ADDR[IPBUFLEN];              // FIXME hardcoded (see .c)
        uint8_t DCAPP_UDPSEND_ADDRLEN;                  // FIXME hardcoded (see .c)

        // the connection port to send to
        uint32_t DCAPP_UDPSEND_PORT;                    // FIXME hardcoded (see .c)

        // The reader thread working data, don't alter outside the API!
        udpRX_thread_data_t udpRXThread;

        // reader thread results
        udpRX_thread_results_t udpRXResults;

        // decoded CCSDS primary header
        CCSDS_PriHdr_t ccsdsPriHdr;

        // decoded CCSDS secondary telemetry header
        CCSDS_TlmSecHdr_t ccsdsTlmHdr;

        // CCSDS raw payload buffer
        char ccsdsPayload[CCDSS_UDP_MAXPAYLOADLEN];

        // CCSDS raw payload buffer length
        int32_t ccsdsPayloadLength;

        // wrapping counter of the # of CCSDS Tlm Packets seen
        uint32_t numTlmCCSDSReceived;

        // sending socket fd
        int sockfdsend;

        // socket address for sending commands
        struct sockaddr_in si_sendother;

        // UDP reader thread control struct, local copy
        udpRX_thread_ctrl_t udpRXCtrl;

        /**
         * Initializes the UDP CCSDS Reader thread and related structs.
         *
         * Currently, the thread ID, port, and update rate are hardcoded.
         * @return success/failure result
         */
        void ccsds_udp_finish_initialization(void);

        /**
         * Initializes for sending out data over a UDP socket.
         *
         * Currently the address and port are hardcoded
         * @return zero for success, non-zero for failure
         */
        void ccsds_udp_finish_send_initialization(void);

        /**
         * Collects message updates from the UDP CCSDS Reader thread and
         * attempts to determine if it is a valid CCSDS packet with a telemetry
         * secondary header.
         *
         * @return success/failure result
         */
        dcapp_ccsds_udp_io ccsds_udp_readtlmmsg(void);
#endif
};

#endif