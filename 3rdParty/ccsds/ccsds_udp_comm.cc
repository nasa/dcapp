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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include "basicutils/msg.hh"
#include "basicutils/timer.hh"
#include "basicutils/tidy.hh"
#include "ccsds_udp_comm.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.0

CcsdsUdpCommModule::CcsdsUdpCommModule()
#ifdef CCSDSUDPACTIVE
:
read_active(false),
write_active(false),
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
    this->last_read_try = new Timer;
    this->last_write_try = new Timer;
    this->udpRX_timer = new Timer;
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

    delete this->last_read_try;
    delete this->last_write_try;
    delete this->udpRX_timer;
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
                this->read_active = false; // stop reading
                return this->Fail;
            case (CCSDSIO_NO_NEW_DATA):
            case (CCSDSIO_NOT_CCSDS):
            case (CCSDSIO_WRONG_CCSDS):
                return this->None;
            case (CCSDSIO_SUCCESS):
                return this->Success;
            default:
                error_msg("ccsds reader switch status logic error, status=" << readstatus);
                this->read_active = false; // stop reading
                return this->Fail;
        }
    }
    else
    {
        if (this->last_read_try->getSeconds() > CONNECT_ATTEMPT_INTERVAL)
        {
            this->read_connect();
            this->last_read_try->restart();
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
        if (this->last_write_try->getSeconds() > CONNECT_ATTEMPT_INTERVAL)
        {
            this->write_connect();
            this->last_write_try->restart();
        }
    }

    return this->None;
#else
    return this->Inactive;
#endif
}


#ifdef CCSDSUDPACTIVE
void CcsdsUdpCommModule::read_initialize(char *hostspec, int portspec, int threadspec, float ratespec, bool littleendianspec)
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
        error_msg("udpRx_reader_init() failed with status " << st);
        debug_msg("\taddress=" << this->DCAPP_UDPRX_ADDR);
        debug_msg("\taddress=" << this->DCAPP_UDPRX_PORT);
        error_msg("ccsds_udp failed to open socket for reading telemetry");
        return;
    }
    else
    {
        debug_msg("udpRx_reader_init() opened socket");
        debug_msg("\taddress=" << this->DCAPP_UDPRX_ADDR);
        debug_msg("\taddress=" << this->DCAPP_UDPRX_PORT);
    }
    this->udpRX_timer->restart();
    debug_msg("ccsds_udp opened socket for reading telemetry");
    this->read_active = true;
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
        error_msg("Failed to obtain port " << this->DCAPP_UDPSEND_PORT);
        error_msg("ccsds_udp failed to open socket for sending commands");
        return;
    }

    debug_msg("ccsds_udp opened socket for sending commands");
    this->write_active = true;
}


CcsdsUdpCommModule::dcapp_ccsds_udp_io CcsdsUdpCommModule::read_tlm_msg(void)
{
    // only check the reader based on the set update_rate
    if (this->udpRX_timer->getSeconds() < this->DCAPP_UDPRX_SPECRATE)
    {
        return CCSDSIO_NO_NEW_DATA;
    }

    // # of packets received (wrapping counter) by the reader thread
    // Capture before sync to see if we have a new packet.
    uint32_t numpkts = udpRXResults.numpkts;

    udpRX_parent_synchdata(&udpRXThread, &udpRXResults, &udpRXCtrl);

    if (udpRXResults.status != UDPRX_NOERROR)
    {
        error_msg("ccsds_udp_readsimdata() had error=" << CCSDSIO_IO_ERROR);
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

    this->udpRX_timer->restart();

    switch (res)
    {
        case (INVALID_BADARG):
        case (INVALID_BUFSIZE):
        case (INVALID_BADCCSDSVER):
        case (INVALID_BADPKTLENGTH):
        case (INVALID_BADTLMPAYLOADLENGTH):
        case (INVALID_BADCMDPAYLOADLENGTH):
        case (INVALID_BADPAYLOADLENGTH):
            debug_msg("ccsds_udp_readsimdata() received non-CCSDS packet. status=" << CCSDSIO_NOT_CCSDS);
            debug_msg("\t\t(error cause ID=" << res << ")");
            return CCSDSIO_NOT_CCSDS;
        case (VALID_NOSECHDR):
        case (INVALID_UNEXPTLMSECHDR):
        case (INVALID_UNEXPCMDSECHDR):
        case (VALID_CMDSECHDR):
            debug_msg("ccsds_udp_readsimdata() received different CCSDS packet");
            debug_msg("\t\t(error cause ID=" << res << ")");
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


void CcsdsUdpCommModule::read_initialize(char *hostspec, int portspec, int threadspec, float ratespec, bool littleendianspec)
{
}


void CcsdsUdpCommModule::write_initialize(char *hostspec, int portspec)
{
}
#endif


bool CcsdsUdpCommModule::isActive(void)
{
#ifdef CCSDSUDPACTIVE
    if (this->read_active || this->write_active) return true;
    else return false;
#else
    return false;
#endif
}
