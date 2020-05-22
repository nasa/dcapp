#ifdef IDF

#include <cstdlib>
#include <cstring>
#include "idf/UsbHagstromKEUSB36FS.hh"
#include "basicutils/timer.hh"
#include "basicutils/msg.hh"
#include "Hagstrom.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.5

extern void HandleBezelPress(int);
extern void HandleBezelRelease(int);

HagstromDevice::HagstromDevice() : valid(false)
{
    user_msg("Connecting Hagstrom...");
    this->hagstrom = new idf::UsbHagstromKEUSB36FS;
    this->currkey = (int *)calloc(this->hagstrom->getInputs().size(), sizeof(int));
    this->prevkey = (int *)calloc(this->hagstrom->getInputs().size(), sizeof(int));
    if (this->currkey && this->prevkey) this->valid = true;
}

HagstromDevice::~HagstromDevice()
{
    if (this->currkey) free(this->currkey);
    if (this->prevkey) free(this->prevkey);
    delete this->hagstrom;
}

void HagstromDevice::setSerialNumber(const char *serialnum)
{
    if (serialnum)
    {
        const size_t numchars = strlen(serialnum) + 1;
        std::wstring wideserialnum(numchars, L'#');
        mbstowcs(&wideserialnum[0], serialnum, numchars);
        this->hagstrom->setSerialNumber(wideserialnum);
    }
}

void HagstromDevice::read(void)
{
    if (!(this->valid)) return;

    if (this->hagstrom->isOpen() || this->last_connect_attempt.getSeconds() > CONNECT_ATTEMPT_INTERVAL)
    {
        const std::vector<idf::SingleInput *>& inputs = this->hagstrom->getInputs();
        std::vector<idf::SingleInput *>::const_iterator key;
        unsigned i;

        try
        {
            while (this->hagstrom->updateOnce())
            {
                for (key = inputs.begin(), i = 0; key != inputs.end(); ++key, ++i)
                {
                    this->currkey[i] = (int)((*key)->getNormalizedValue());
                    if (this->currkey[i] != this->prevkey[i])
                    {
                        switch (this->currkey[i])
                        {
                            case 1:
                                user_msg("Key " << i+1 << " pressed");
                                HandleBezelPress(i+1);
                                break;
                            case -1:
                                user_msg("Key " << i+1 << " released");
                                HandleBezelRelease(i+1);
                                break;
                            default:
                                user_msg("Key " << i+1 << ", strange value: " << this->currkey[i]);
                                break;
                        }
                        this->prevkey[i] = this->currkey[i];
                    }
                }
            }
        }
        catch(...) { this->last_connect_attempt.restart(); }
    }
}

#else

#include "basicutils/msg.hh"
#include "Hagstrom.hh"

HagstromDevice::HagstromDevice()
{
    warning_msg("Hagstrom device requested, but IDF isn't properly built or installed...");
}

#endif
