#include "parent.hh"
#include "function.hh"
#include "basicutils/msg.hh"

#include <map>

static std::map<std::string, void (*)()> functions;

// fetch the function being referenced by <name>
voidfunc getFunction(const std::string &name) {
    if (functions.find(name) != functions.end()){
        return functions[name];
    }

    return NULL;
}

// register a function <func> with <name>
voidfunc registerFunction(const std::string &name, void(*func)()) {

    // check for errors
    if (name.empty()) {
        error_msg("Attempting to create a function without a name");
        return NULL;
    }
    if (name[0] == '@' || name[0] == '#') {
        error_msg("Creating a function with a variable or constant name");
        return NULL;
    }
    if (!func) {
        error_msg("Registering non-existent function: " + name);
        return NULL;
    }

    functions[name] = func;
    return func;
}

dcFunction::dcFunction(dcParent *myparent, void(*func)()) : func(NULL) {
    myparent->addChild(this);
    this->func = func;
}

void dcFunction::draw(void) {
    if (this->func) {
        this->func();
    }
}
