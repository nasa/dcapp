/**
 * \file ccsds_udp_comm.cc
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include "msg.hh"
#include "timer.hh"
#include "ccsds_udp_comm.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0
#define TIDY(a) if (a) { free(a); a=0x0; }

CcsdsUdpCommModule::CcsdsUdpCommModule()
#ifdef CCSDSUDPACTIVE
:
read_active(0),
write_active(0),
DCAPP_UDPRX_ADDR(0x0),
DCAPP_UDPRX_PORT(DEFAULT_PORT),
DCAPP_UDPSEND_ADDR(0x0),
DCAPP_UDPSEND_PORT(DEFAULT_PORT),
DCAPP_UDPRX_THREADID(DEFAULT_THREADID),
DCAPP_UDPRX_SPECRATE(DEFAULT_RATE),
DCAPP_CCSDS_NETWORK_LITTLEEND(false),
sockfdsend(-1)
#endif
{
#ifdef CCSDSUDPACTIVE
    StartTimer(&(this->last_read_try));
    StartTimer(&(this->last_write_try));
#endif
}

CcsdsUdpCommModule::~CcsdsUdpCommModule()
{
#ifdef CCSDSUDPACTIVE
    // close down the reader thread
    udpRX_reader_close(&udpRXThread, &udpRXResults, &udpRXCtrl);

    // close the writer socket
    if (sockfdsend != -1) close(sockfdsend);

    TIDY(this->DCAPP_UDPRX_ADDR);
    TIDY(this->DCAPP_UDPSEND_ADDR);
#endif
}

CommModule::CommStatus CcsdsUdpCommModule::read(void)
{
#ifdef CCSDSUDPACTIVE
    if (this->read_active)
    {
        dcapp_ccsds_udp_io readstatus = this->read_tlm_msg();
        switch (readstatus)
        {
            case (CCSDSIO_IO_ERROR):
            case (CCSDSIO_PARSE_ERROR):
                this->read_active = 0; // stop reading
                return this->Fail;
            case (CCSDSIO_NO_NEW_DATA):
            case (CCSDSIO_NOT_CCSDS):
            case (CCSDSIO_WRONG_CCSDS):
                return this->None;
            case (CCSDSIO_SUCCESS):
                return this->Success;
            default:
                error_msg("ccsds reader switch status logic error, status=%d.", readstatus);
                this->read_active = 0; // stop reading
                return this->Fail;
        }
    }
    else
    {
        if (SecondsElapsed(this->last_read_try) > CONNECT_ATTEMPT_INTERVAL)
        {
            this->read_connect();
            StartTimer(&(this->last_read_try));
        }
    }


    return this->None;
#else
    return this->Inactive;
#endif
}

CommModule::CommStatus CcsdsUdpCommModule::write(void)
{
#ifdef CCSDSUDPACTIVE
    if (this->write_active)
    {
// write processing goes here
    }
    else
    {
        if (SecondsElapsed(this->last_write_try) > CONNECT_ATTEMPT_INTERVAL)
        {
            this->write_connect();
            StartTimer(&(this->last_write_try));
        }
    }

    return this->None;
#else
    return this->Inactive;
#endif
}


#ifdef CCSDSUDPACTIVE
void CcsdsUdpCommModule::read_initialize(char *hostspec, int portspec, int threadspec, float ratespec, int littleendianspec)
{
    if (hostspec) this->DCAPP_UDPRX_ADDR = strdup(hostspec);
    this->DCAPP_UDPRX_PORT = (uint32_t)portspec;
    if (threadspec > 0) this->DCAPP_UDPRX_THREADID = threadspec;
    else this->DCAPP_UDPRX_THREADID = DEFAULT_THREADID;
    if (ratespec > 0) this->DCAPP_UDPRX_SPECRATE = ratespec;
    else this->DCAPP_UDPRX_SPECRATE = DEFAULT_RATE;
    if (littleendianspec) this->DCAPP_CCSDS_NETWORK_LITTLEEND = true;
}


void CcsdsUdpCommModule::read_connect(void)
{
    memset(&ccsdsPayload, 0, CCDSS_UDP_MAXPAYLOADLEN);
    ccsdsPayloadLength = 0;
    numTlmCCSDSReceived = 0;

    if (!(this->DCAPP_UDPRX_ADDR)) this->DCAPP_UDPRX_ADDR = strdup(DEFAULT_HOST);
    if (!(this->DCAPP_UDPRX_PORT)) this->DCAPP_UDPRX_PORT = DEFAULT_PORT;

    udpRX_thread_status st = udpRx_reader_init(this->DCAPP_UDPRX_THREADID,
                                               this->DCAPP_UDPRX_ADDR,
                                               (uint8_t)(strlen(this->DCAPP_UDPRX_ADDR)+1),
                                               this->DCAPP_UDPRX_PORT,
                                               &udpRXThread,
                                               &udpRXResults,
                                               &udpRXCtrl);

    if (st != UDPRX_NOERROR)
    {
        error_msg("udpRx_reader_init() failed with status %d", st);
        debug_msg("\taddress=%s", this->DCAPP_UDPRX_ADDR);
        debug_msg("\taddress=%d", this->DCAPP_UDPRX_PORT);
        error_msg("ccsds_udp failed to open socket for reading telemetry");
        return;
    }
    else
    {
        debug_msg("udpRx_reader_init() opened socket");
        debug_msg("\taddress=%s", this->DCAPP_UDPRX_ADDR);
        debug_msg("\taddress=%d", this->DCAPP_UDPRX_PORT);
    }
    StartTimer(&(this->udpRX_timer));
    debug_msg("ccsds_udp opened socket for reading telemetry");
    this->read_active = 1;
}


void CcsdsUdpCommModule::write_initialize(char *hostspec, int portspec)
{
    if (hostspec) this->DCAPP_UDPSEND_ADDR = strdup(hostspec);
    this->DCAPP_UDPSEND_PORT = (uint16_t)portspec;
}


void CcsdsUdpCommModule::write_connect(void)
{
    if ((sockfdsend = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        if (sockfdsend == -1)
        {
            error_msg("Failed to grab socket for writing");
            error_msg("ccsds_udp failed to open socket for sending commands");
            return;
        }
    }

    if (!(this->DCAPP_UDPSEND_ADDR)) this->DCAPP_UDPSEND_ADDR = strdup(DEFAULT_HOST);
    if (!(this->DCAPP_UDPSEND_PORT)) this->DCAPP_UDPSEND_PORT = DEFAULT_PORT;

    memset((char *) &si_sendother, 0, sizeof(si_sendother));
    si_sendother.sin_family = AF_INET;
    si_sendother.sin_port = htons(this->DCAPP_UDPSEND_PORT); // single sender
    if (inet_aton(this->DCAPP_UDPSEND_ADDR, &si_sendother.sin_addr) == 0)
    {
        close(sockfdsend);
        error_msg("Failed to obtain port %d.", this->DCAPP_UDPSEND_PORT);
        error_msg("ccsds_udp failed to open socket for sending commands");
        return;
    }

    debug_msg("ccsds_udp opened socket for sending commands");
    this->write_active = 1;
}


CcsdsUdpCommModule::dcapp_ccsds_udp_io CcsdsUdpCommModule::read_tlm_msg(void)
{
    // only check the reader based on the set update_rate
    if (SecondsElapsed(this->udpRX_timer) < this->DCAPP_UDPRX_SPECRATE)
    {
        return CCSDSIO_NO_NEW_DATA;
    }

    // # of packets received (wrapping counter) by the reader thread
    // Capture before sync to see if we have a new packet.
    uint32_t numpkts = udpRXResults.numpkts;

    udpRX_parent_synchdata(&udpRXThread, &udpRXResults, &udpRXCtrl);

    if (udpRXResults.status != UDPRX_NOERROR)
    {
        error_msg("ccsds_udp_readsimdata() had error= %d.", CCSDSIO_IO_ERROR);
        return CCSDSIO_IO_ERROR;
    }

    if (udpRXResults.numpkts == numpkts)
    {
        return CCSDSIO_NO_NEW_DATA;
    }

    // Attempts to convert the received char buffer into a CCSDS packet
    // in host layout.
    CCSDS_INFO_RESULT res = get_CCSDS_Info(&(udpRXResults.rxbuf[0]),
            udpRXResults.rxbufbytesused,
            this->DCAPP_CCSDS_NETWORK_LITTLEEND,
            &ccsdsPriHdr,
            &ccsdsTlmHdr,
            0,
            &(ccsdsPayload[0]),
            &ccsdsPayloadLength
            );

    StartTimer(&(this->udpRX_timer));

    switch (res)
    {
        case (INVALID_BADARG):
        case (INVALID_BUFSIZE):
        case (INVALID_BADCCSDSVER):
        case (INVALID_BADPKTLENGTH):
        case (INVALID_BADTLMPAYLOADLENGTH):
        case (INVALID_BADCMDPAYLOADLENGTH):
        case (INVALID_BADPAYLOADLENGTH):
            debug_msg("ccsds_udp_readsimdata() received non-CCSDS packet. status=%d", CCSDSIO_NOT_CCSDS);
            debug_msg("\t\t(error cause ID=%d)", res);
            return CCSDSIO_NOT_CCSDS;
        case (VALID_NOSECHDR):
        case (INVALID_UNEXPTLMSECHDR):
        case (INVALID_UNEXPCMDSECHDR):
        case (VALID_CMDSECHDR):
            debug_msg("ccsds_udp_readsimdata() received different CCSDS packet");
            debug_msg("\t\t(error cause ID=%d)", res);
            return CCSDSIO_WRONG_CCSDS; // cmd not tlm
        case (VALID_TLMSECHDR):
            // got it!
            ++numTlmCCSDSReceived;
            break;
        case (INVALID_LOGICERROR):
        default:
            return CCSDSIO_PARSE_ERROR; // wtf?
    }

    return CCSDSIO_SUCCESS;
}
#else


void CcsdsUdpCommModule::read_initialize(char *hostspec, int portspec, int threadspec, float ratespec, int littleendianspec)
{
}


void CcsdsUdpCommModule::write_initialize(char *hostspec, int portspec)
{
}
#endif


int CcsdsUdpCommModule::isActive(void)
{
#ifdef CCSDSUDPACTIVE
    if (this->read_active || this->write_active) return 1;
    else return 0;
#else
    return 0;
#endif
}
