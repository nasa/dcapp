#include "PixelStream.hh"
#include "PixelStreamData.hh"

PixelStreamData::PixelStreamData()
:
protocol(PixelStreamFileProtocol),
pixels(0x0),
width(0),
height(0)
{
}

PixelStreamData::~PixelStreamData()
{
}

int PixelStreamData::reader(void)
{
    return 0;
}

int PixelStreamData::writer(void)
{
    return 0;
}
