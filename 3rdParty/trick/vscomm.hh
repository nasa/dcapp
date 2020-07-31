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
#include "variables.hh"

#define VS_SUCCESS            (0)
#define VS_ERROR              (-1)
#define VS_NO_NEW_DATA        (-2)
#define VS_INVALID_CONNECTION (-3)
#define VS_MANGLED_BUFFER     (-4)
#define VS_PARTIAL_BUFFER     (-5)

class ParamData
{
    public:
        ParamData(std::string &, std::string &, Variable &);
        virtual ~ParamData();

        std::string label;
        std::string units;
        Variable value;
};

class VariableServerComm
{
    public:
        VariableServerComm();
        virtual ~VariableServerComm();

        Variable *add_var(std::string &, std::string &, Variable &);
        void remove_var(std::string &);
        int activate(std::string &, int, std::string &);
        int get(void);
        int putMethod(std::string &);
        int putValue(std::string &, Variable &, std::string &);

    private:
        std::list<ParamData> paramlist;
        TCDevice connection;
        char *databuf;
        char *prevbuf;
        bool databuf_complete;
        int databuf_size;

        void sim_read(void);
        void sim_write(std::string);
        int update_data(const char *);
        size_t count_tokens(const char *, char);
        int find_next_token(const char *, char);
        int find_last_token(const char *, char);
};

#endif

#endif
