#ifndef _XFONTS_HH_
#define _XFONTS_HH_

typedef struct
{
    const char *tdspec;
    const char *xspec;
} tdXFontTable;

tdXFontTable tdXFont[] =
{
    { "Courier" ,                        "*-courier-medium-r-normal--%d-*"                } ,
    { "Courier Bold" ,                   "*-courier-bold-r-normal--%d-*"                  } ,
    { "Helvetica" ,                      "*-helvetica-medium-r-normal--%d-*"              } ,
    { "Helvetica Bold" ,                 "*-helvetica-bold-r-normal--%d-*"                } ,
    { "Century Schoolbook" ,             "*-new century schoolbook-medium-r-normal--%d-*" } ,
    { "Century Schoolbook Bold" ,        "*-new century schoolbook-bold-r-normal--%d-*"   } ,
    { "Century Schoolbook Italic" ,      "*-new century schoolbook-medium-i-normal--%d-*" } ,
    { "Century Schoolbook Bold Italic" , "*-new century schoolbook-bold-i-normal--%d-*"   } ,
    { "Times" ,                          "*-times-medium-r-normal--%d-*"                  } ,
    { "Times Bold" ,                     "*-times-bold-r-normal--%d-*"                    } ,
    { "Times Italic" ,                   "*-times-medium-i-normal--%d-*"                  } ,
    { "Times Bold Italic" ,              "*-times-bold-i-normal--%d-*"                    } ,
    { "Lucida Sans" ,                    "*-lucida-medium-r-normal-sans-%d-*"             } ,
    { "Lucida Sans Demibold Roman" ,     "*-lucida-bold-r-normal-sans-%d-*"               } ,
    { "Lucida Sans Italic" ,             "*-lucida-medium-i-normal-sans-%d-*"             } ,
    { "Lucida Sans Demibold Italic" ,    "*-lucida-bold-i-normal-sans-%d-*"               } ,
    { "Lucida Bright" ,                  "*-lucidabright-medium-r-normal--%d-*"           } ,
    { "Lucida Bright Demibold" ,         "*-lucidabright-demibold-r-normal--%d-*"         } ,
    { "Lucida Bright Italic" ,           "*-lucidabright-medium-i-normal--%d-*"           } ,
    { "Lucida Bright Demibold Italic" ,  "*-lucidabright-demibold-i-normal--%d-*"         } ,
    { "Lucida Sans Typewriter" ,         "*-lucidatypewriter-medium-r-normal-sans-%d-*"   } ,
    { "Lucida Sans Typewriter Bold" ,    "*-lucidatypewriter-bold-r-normal-sans-%d-*"     }
};

//#define tdXFontCount 22
#define tdXFontCount 0

int tdXFontSize[] = { 8, 10, 11, 12, 14, 17, 18, 20, 24, 25, 34 };

#define tdXFontSizeCount 11

#endif
