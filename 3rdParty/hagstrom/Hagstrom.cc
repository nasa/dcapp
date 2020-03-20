#include "basicutils/msg.hh"
#ifdef IDF
#include "basicutils/timer.hh"
#include "idf/UsbHagstromKEUSB36FS.hh"

#define CONNECT_ATTEMPT_INTERVAL 2.5

extern void HandleBezelPress(int);
extern void HandleBezelRelease(int);

static idf::UsbHagstromKEUSB36FS hagstrom;
static const std::vector<idf::SingleInput *>& inputs = hagstrom.getInputs();
static int *currkey = 0x0, *prevkey = 0x0;
static Timer last_connect_attempt;
static bool requested = false;
static bool connected = false;

void Hagstrom_init(const char *serialnum)
{
    user_msg("Connecting Hagstrom...");
    if (serialnum)
    {
        const size_t numchars = strlen(serialnum) + 1;
        std::wstring wideserialnum(numchars, L'#');
        mbstowcs(&wideserialnum[0], serialnum, numchars);
        hagstrom.setSerialNumber(wideserialnum);
    }

    currkey = (int *)calloc(inputs.size(), sizeof(int));
    prevkey = (int *)calloc(inputs.size(), sizeof(int));

    requested = true;
}

void Hagstrom_read(void)
{
    if (!requested || !currkey || !prevkey) return;

    if (connected || last_connect_attempt.getSeconds() > CONNECT_ATTEMPT_INTERVAL)
    {
        if (hagstrom.isConnected())
        {
            std::vector<idf::SingleInput *>::const_iterator key;
            unsigned i;

            connected = true;

            while (hagstrom.updateOnce())
            {
                for (key = inputs.begin(), i = 0; key != inputs.end(); ++key, ++i)
                {
                    currkey[i] = (int)((*key)->getNormalizedValue());
                    if (currkey[i] != prevkey[i])
                    {
                        switch (currkey[i])
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
                                user_msg("Key " << i+1 << ", strange value: " << currkey[i]);
                                break;
                        }
                        prevkey[i] = currkey[i];
                    }
                }
            }
        }
        else
        {
            connected = false;
            last_connect_attempt.restart();
        }
    }
}

void Hagstrom_term(void)
{
}
#else
void Hagstrom_init(const char *)
{
    warning_msg("Hagstrom device requested, but IDF_HOME not identified...");
}

void Hagstrom_read(void)
{
}

void Hagstrom_term(void)
{
    if (currkey) free(currkey);
    if (prevkey) free(prevkey);
}
#endif
