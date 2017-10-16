#include <cstring>
#include "PixelStreamData.hh"
#include "PixelStreamFile.hh"
#include "PixelStreamMjpeg.hh"
#include "PixelStreamTcp.hh"

PixelStreamData::PixelStreamData()
:
protocol(PixelStreamUnknownProtocol),
connected(false),
pixels(0x0),
width(0),
height(0)
{
}

PixelStreamData::~PixelStreamData()
{
}

bool PixelStreamData::operator == (const PixelStreamData &that)
{
    if (this->protocol == that.protocol)
    {
        switch (this->protocol)
        {
        case PixelStreamFileProtocol:
            return (*(PixelStreamFile *)this == *(PixelStreamFile *)&that);
            break;
        case PixelStreamMjpegProtocol:
            return (*(PixelStreamMjpeg *)this == *(PixelStreamMjpeg *)&that);
            break;
        case PixelStreamTcpProtocol:
            return (*(PixelStreamTcp *)this == *(PixelStreamTcp *)&that);
            break;
        default:
            break;
        }
    }
    return false;
}

bool PixelStreamData::operator != (const PixelStreamData &that)
{
    return !(*this == that);
}

int PixelStreamData::reader(void)
{
    return 0;
}

int PixelStreamData::writer(void)
{
    return 0;
}

bool PixelStreamData::writeRequested(void)
{
    return true;
}
