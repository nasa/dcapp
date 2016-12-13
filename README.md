# dcapp

**dcapp** (pronounced “dee see app”) is a displays and controls software package designed for UNIX platforms,
specifically MacOS and Linux.

### Prerequisites

* OpenGL
* libxml2
* FreeType2

Note that these packages are standard on most MacOS and Linux installations.

### Building

To build, `cd` to the top level of the dcapp package (`$DCAPP_HOME`) and type `make`.

### Activation

To enable command line activation, add the return value of `$DCAPP_HOME/bin/dcapp-config --exepath` to the `$PATH`
environment variable.

To activate dcapp for a given spec file (`file.xml`) via the command line:

```
dcapp file.xml [const=value...]
```

...where as many of the optional `const=value` constructs as needed may be used to customize constants defined in the
spec file.

Note that on MacOS, the dcapp.app file may be double-clicked to activate dcapp.  This will bring up a GUI for providing
the necessary information to run dcapp.

---

_For more information on dcapp and the content and structure of a dcapp specfile, please see the documentation included
in the docs directory._
