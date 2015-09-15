#include <stdlib.h>
#include <string.h>
#include "keyValuePair.hh"

KeyValuePair::KeyValuePair()
:
key(0x0),
value(0x0)
{
}

KeyValuePair::~KeyValuePair()
{
    if (this->key) free(this->key);
    if (this->value) free(this->value);
}

int KeyValuePair::setKeyAndValue(const char *inkey, const char *invalue)
{
    this->key = strdup(inkey);
    this->value = strdup(invalue);
    if (!(this->key) || !(this->value)) return (-1);
    return 0;
}

char * KeyValuePair::getKey(void)
{
    return this->key;
}

char * KeyValuePair::getValue(void)
{
    return this->value;
}
