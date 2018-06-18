#include "nodes.hh"
#include "varlist.hh"
#include "string_utils.hh"
#include "opengl_draw.hh"
#include "loadUtils.hh"
#include "pixelstream.hh"

extern appdata AppData;
extern void SetNeedsRedraw(void); // TODO: put in header file

dcPixelStream::dcPixelStream(dcParent *myparent) : dcGeometric(myparent), testpatternID(-1), psi(0x0), pixels(0x0), memallocation(0)
{
    init_texture(&textureID);
}

dcPixelStream::~dcPixelStream()
{
    if (psi) delete psi;
}

void dcPixelStream::setProtocol(const char *protocolstr, const char *host, const char *port, const char *path, const char *username, const char *password, const char *shmemkey, const char *filename, const char *cameraspec)
{
    PixelStreamData *mypsd = 0x0;
    PixelStreamFile *psf;
    PixelStreamMjpeg *psm;
    PixelStreamTcp *pst;
    PixelStreamVsm *psv;
    PixelStreamItem *match = 0x0;
    std::list<PixelStreamItem *>::iterator psitem;

    unsigned protocol = PixelStreamFileProtocol;
    if (protocolstr)
    {
        if (!strcasecmp(protocolstr, "MJPEG")) protocol = PixelStreamMjpegProtocol;
        if (!strcasecmp(protocolstr, "TCP")) protocol = PixelStreamTcpProtocol;
        if (!strcasecmp(protocolstr, "VSM")) protocol = PixelStreamVsmProtocol;
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
            if (psv->readerInitialize(host, StringToInteger(port, 80), getStringPointer(cameraspec)))
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

void dcPixelStream::setTestPattern(const char *filename)
{
    if (filename) testpatternID = dcLoadTexture(filename);
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
    container_start(refx, refy, delx, dely, 1, 1, *rotate);

    if (psi->psd->connected)
    {
        newh = (size_t)((double)(psi->psd->width) * (*h) / (*w));

        if (newh > psi->psd->height)
        {
            origbytes = psi->psd->width * psi->psd->height * 3;
            // overpad the pad by 1 so that no unfilled rows show up
            pad = ((newh - psi->psd->height) / 2) + 1;
            padbytes = psi->psd->width * pad * 3;
            nbytes = (size_t)(psi->psd->width * newh * 3);
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
            nbytes = (size_t)(psi->psd->width * psi->psd->height * 3);
            if (nbytes > memallocation)
            {
                pixels = realloc(pixels, nbytes);
                memallocation = nbytes;
            }
            bcopy(psi->psd->pixels, pixels, nbytes);
        }

        if (newh < psi->psd->height)
        {
            offset = (psi->psd->height - newh) / 2;
            offsetbytes = psi->psd->width * offset * 3;
            set_texture(textureID, psi->psd->width, newh, (void *)((size_t)pixels + offsetbytes));
        }
        else
            set_texture(textureID, psi->psd->width, newh, pixels);

        draw_image(textureID, width, height);
    }
    else draw_image(testpatternID, width, height);

    container_end();
}
