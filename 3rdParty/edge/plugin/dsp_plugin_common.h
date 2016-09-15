#include <cstdio>

static void DSF_RegisterPluginVersion(DSS_PLUGIN *A, const char *B)
{
    printf("registering plugin %s\n", B);
}

#define DSF_LogWarningQ printf
#define DSF_LogStatusQ printf
