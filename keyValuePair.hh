#ifndef _KEYVALUEPAIR_HH_
#define _KEYVALUEPAIR_HH_

class KeyValuePair
{
    public:
        KeyValuePair();
        virtual ~KeyValuePair();

        int setKeyAndValue(const char *, const char *);
        char *getKey(void);
        char *getValue(void);

    private:
        char *key;
        char *value;
};

#endif
