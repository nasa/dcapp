#ifndef _VSCOMM_CONSTANTS_HH_
#define _VSCOMM_CONSTANTS_HH_

#define VS_DEFAULT_HOST       (NULL)
#define VS_DEFAULT_PORT       (7000)
#define VS_DEFAULT_SAMPLERATE "1.0"

#define VS_SUCCESS            (0)
#define VS_ERROR              (-1)
#define VS_NO_NEW_DATA        (-2)
#define VS_INVALID_CONNECTION (-3)
#define VS_MANGLED_BUFFER     (-4)
#define VS_PARTIAL_BUFFER     (-5)

#define VS_UNKNOWN_TYPE       (0)
#define VS_STRING             (1)
#define VS_FLOAT              (2)
#define VS_INTEGER            (3)

#endif