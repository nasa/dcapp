#include <string>
#include "app_data.hh"
#include "values.hh"
#include "basicutils/stringutils.hh"
#include "RenderLib/RenderLib.hh"
#include "commonutils.hh"
#include "pixelstream.hh"

extern appdata AppData;
extern void SetNeedsRedraw(void); // TODO: put in header file

dcPixelStream::dcPixelStream(dcParent *myparent) : dcGeometric(myparent), testpatternID(0x0), psi(0x0), pixels(0x0), memallocation(0), channels(3)
{
    textureID = new tdTexture();
}

dcPixelStream::~dcPixelStream()
{
    if (psi) delete psi;
}

void dcPixelStream::setProtocol(const std::string &protocolstr, const std::string &host, const std::string &port, const std::string &path, const std::string &username, const std::string &password, const std::string &shmemkey, const std::string &filename, const std::string &cameraspec)
{
    PixelStreamData *mypsd = 0x0;
    PixelStreamFile *psf;
    PixelStreamDynamicFile *psdf;
    PixelStreamMjpeg *psm;
    PixelStreamTcp *pst;
    PixelStreamVsm *psv;
    PixelStreamItem *match = 0x0;
    std::list<PixelStreamItem *>::iterator psitem;

    unsigned protocol = PixelStreamFileProtocol;
    if (!protocolstr.empty())
    {
        if (CaseInsensitiveCompare(protocolstr, "MJPEG")) protocol = PixelStreamMjpegProtocol;
        if (CaseInsensitiveCompare(protocolstr, "TCP")) protocol = PixelStreamTcpProtocol;
        if (CaseInsensitiveCompare(protocolstr, "VSM")) protocol = PixelStreamVsmProtocol;
        if (CaseInsensitiveCompare(protocolstr, "DFILE")) protocol = PixelStreamDynamicFileProtocol;
    }

    switch (protocol)
    {
        case PixelStreamFileProtocol:
            psf = new PixelStreamFile;
            if (psf->readerInitialize(filename, StringToInteger(shmemkey, 0)))
            {
                delete psf;
                return;
            }
            mypsd = (PixelStreamData *)psf;
            break;
        case PixelStreamDynamicFileProtocol:
            psdf = new PixelStreamDynamicFile;
            this->channels = 4;
            this->textureID->setPixelSpec(PixelRGBA);
            if (psdf->readerInitialize(shmemkey))
            {
                delete psdf;
                return;
            }
            mypsd = (PixelStreamDynamicFile *)psdf;
            break;
        case PixelStreamMjpegProtocol:
            psm = new PixelStreamMjpeg;
            if (psm->readerInitialize(host, StringToInteger(port, 80), path, username, password))
            {
                delete psm;
                return;
            }
            mypsd = (PixelStreamData *)psm;
            break;
        case PixelStreamTcpProtocol:
            pst = new PixelStreamTcp;
            if (pst->readerInitialize(host, StringToInteger(port, 0)))
            {
                delete pst;
                return;
            }
            mypsd = (PixelStreamData *)pst;
            break;
        case PixelStreamVsmProtocol:
            psv = new PixelStreamVsm;
            if (psv->readerInitialize(host, StringToInteger(port, 80), path, getValue(cameraspec)))
            {
                delete psv;
                return;
            }
            mypsd = (PixelStreamData *)psv;
            break;
        default:
            break;
    }

    if (!mypsd) return;

    for (psitem = AppData.pixelstreams.begin(); psitem != AppData.pixelstreams.end() && !match; psitem++)
    {
        if (*(*psitem)->psd == *mypsd) match = *psitem;
    }

    if (match)
    {
        delete mypsd;
        psi = match;
    }
    else
    {
        psi = new PixelStreamItem;
        psi->psd = (PixelStreamData *)mypsd;
        AppData.pixelstreams.push_back(psi);
    }
}

void dcPixelStream::setTestPattern(const std::string &filename)
{
    if (!filename.empty()) testpatternID = tdLoadTexture(filename);
}

void dcPixelStream::updateStreams(unsigned passcount)
{
    if (!psi) return;

    if (psi->frame_count != passcount)
    {
        if (psi->psd->reader()) SetNeedsRedraw();
        psi->frame_count = passcount;
    }
}

void dcPixelStream::draw(void)
{
    if (!psi) return;

    size_t nbytes, origbytes, newh, pad, padbytes, offset, offsetbytes;

    computeGeometry();
    container_start(refx, refy, delx, dely, 1, 1, rotate->getDecimal());

    if (psi->psd->connected)
    {
        newh = (size_t)((double)(psi->psd->width) * (h->getDecimal()) / (w->getDecimal()));

        if (newh > psi->psd->height)
        {
            origbytes = psi->psd->width * psi->psd->height * this->channels;
            // overpad the pad by 1 so that no unfilled rows show up
            pad = ((newh - psi->psd->height) / 2) + 1;
            padbytes = psi->psd->width * pad * this->channels;
            nbytes = (size_t)(psi->psd->width * newh * this->channels);
            if (nbytes > memallocation)
            {
                pixels = realloc(pixels, nbytes);
                memallocation = nbytes;
            }
            bzero(pixels, padbytes);
            bzero((void *)((size_t)pixels + nbytes - padbytes), padbytes);
            bcopy(psi->psd->pixels, (void *)((size_t)pixels + padbytes), origbytes);
        }
        else
        {
            nbytes = (size_t)(psi->psd->width * psi->psd->height * this->channels);
            if (nbytes > memallocation)
            {
                pixels = realloc(pixels, nbytes);
                memallocation = nbytes;
            }
            bcopy(psi->psd->pixels, pixels, nbytes);
        }

        textureID->width = psi->psd->width;
        textureID->height = newh;
        if (newh < psi->psd->height)
        {
            offset = (psi->psd->height - newh) / 2;
            offsetbytes = psi->psd->width * offset * this->channels;
            textureID->data = (unsigned char *)((size_t)pixels + offsetbytes);
        }
        else
            textureID->data = (unsigned char *)pixels;

        load_texture(textureID);

        if(this->channels == 3)
            draw_image(textureID, width, height);
        else if(this->channels == 4)
            draw_image_flipped(textureID, width, height);
    }
    else if (testpatternID) draw_image(testpatternID, width, height);

    container_end();
}
