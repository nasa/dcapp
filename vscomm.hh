#ifndef _VSCOMM_HH_
#define _VSCOMM_HH_

#include "trick_utils/comm/include/tc.h"
#include "trick_utils/comm/include/tc_proto.h"

#define VS_DEFAULT_PORT       (7000)
#define VS_DEFAULT_SAMPLERATE "1.0"

#define VS_SUCCESS            (0)
#define VS_ERROR              (-1)
#define VS_NO_NEW_DATA        (-2)
#define VS_INVALID_CONNECTION (-3)
#define VS_MANGLED_BUFFER     (-4)
#define VS_PARTIAL_BUFFER     (-5)

#define VS_STRING             (1)
#define VS_FLOAT              (2)
#define VS_INTEGER            (3)

class VariableServerComm
{
    public:
        VariableServerComm();
        virtual ~VariableServerComm();

        void *add_var(const char *, const char *, int, int);
        int remove_var(const char *);
        int activate(const char *, int, const char *, char *);
        int get(void);
        int put(const char *, int, void *, const char *);

    private:
        typedef struct paramarray
        {
            struct paramarray *next;
            char *param;
            char *units;
            int type;
            int nelem;
            void *value;
        } ParamArray;

        TCDevice connection;
        ParamArray *parray;
        char *databuf;
        char *prevbuf;
        int databuf_complete;
        int databuf_size;
        int paramcount;

        void sim_read(void);
        void sim_write_const(const char *);
        void sim_write(char *);
        int update_data(char *);
        int count_tokens(const char *, char);
        int find_next_token(const char *, char);
};

#endif
