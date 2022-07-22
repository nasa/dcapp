#include <map>
#include <string>
#include <cstdio>
#include <cstring>
#include "basicutils/msg.hh"
#include "basicutils/stringutils.hh"
#include "values.hh"
#include "functions.hh"

static std::map<std::string, Function> functions;

// create an empty function pointer to potentially be set later
void registerFunction(const std::string &funcname, void(*funcpointer)()) {
    if (funcname.empty()) {
        error_msg("Attempting to create a function without a name");
        return;
    }

    if (funcname[0] == '@' || funcname[0] == '#') {
        error_msg("Creating a function with a variable or constant name");
        return;
    }

    Function *finfo = new Function;
    finfo->setName(funcname);
    finfo->setFunction(funcpointer);

    functions[funcname] = *finfo;
}

// get a function object with name
Function* getFunction(const std::string &label) {
    if (label.empty()) return 0x0;

    std::string funcname = getValue(label)->getString();
    if (functions.find(funcname) != functions.end())
        return &(functions[funcname]);

    warning_msg("Invalid function label: " << label);
    return NULL;
}

Function::Function() : func(NULL) { }
Function::~Function() { }

// set the function pointer inside a function object (setter)
void Function::setFunction(void(*f)()) {
    if (this->func == NULL) {
        this->func = f;
    } else {
        error_msg("Overwriting existing function " << this->name);
    }
}

// set the name of the function (setter)
void Function::setName(const std::string &inval) {
    this->name = inval;
}

// run the function
void Function::runFunction(void) {
    if (this->func != NULL) {
        this->func();
    }
}
