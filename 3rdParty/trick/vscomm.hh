#ifdef TRICKACTIVE

#ifndef _VSCOMM_HH_
#define _VSCOMM_HH_

#include <string>
#include <list>
#ifdef TRICK16PLUS
#include "trick/tc.h"
#include "trick/tc_proto.h"
#else
#include "trick_utils/comm/include/tc.h"
#include "trick_utils/comm/include/tc_proto.h"
#endif

#define VS_SUCCESS            (0)
#define VS_ERROR              (-1)
#define VS_NO_NEW_DATA        (-2)
#define VS_INVALID_CONNECTION (-3)
#define VS_MANGLED_BUFFER     (-4)
#define VS_PARTIAL_BUFFER     (-5)

#define VS_STRING             (1)
#define VS_DECIMAL            (2)
#define VS_INTEGER            (3)

class ValueData
{
    public:
        ValueData();
        virtual ~ValueData();

        void setType(int);
        void setValue(const char *, unsigned);
        
        double decval;
        int intval;
        std::string strval;

    private:
        int type;
};

class ParamData
{
    public:
        ParamData(const char *, const char *, int);
        virtual ~ParamData();

        std::string label;
        std::string units;
        ValueData value;
};

class VariableServerComm
{
    public:
        VariableServerComm();
        virtual ~VariableServerComm();

        ValueData *add_var(const char *, const char *, int);
        void remove_var(const char *);
        int activate(const char *, int, const char *, char *);
        int get(void);
        int putMethod(const char *);
        int putValue(const char *, int, const char *);
        int putValue(const char *, double, const char *);
        int putValue(const char *, std::string, const char *);

    private:
        std::list<ParamData> paramlist;
        TCDevice connection;
        char *databuf;
        char *prevbuf;
        bool databuf_complete;
        int databuf_size;

        int putGeneric(const char *, std::string, const char *);
        void sim_read(void);
        void sim_write(const char *);
        int update_data(const char *);
        size_t count_tokens(const char *, char);
        int find_next_token(const char *, char);
        int find_last_token(const char *, char);
};

#endif

#endif
