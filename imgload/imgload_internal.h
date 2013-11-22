enum ncol { NCOL_NONE, NCOL_BW, NCOL_IA, NCOL_RGB, NCOL_RGBA };

typedef struct
{
  int target;          /* specifies the target texture GL_TEXTURE_2D | GL_PROXY_TEXTURE_2D */
  int level;           /* specifies the level of detail number, level 0 is the base image level */
  int internalFormat;  /* specifies the number of color components (1 | 2 | 3 | 4) */
  int width;           /* specifies the width of the texture image, must be 2n+2(border) */
  int height;          /* specifies the height of the texture image, must be 2m+2(border) */
  int border;          /* specifies the width of the border (0 | 1) */
  int format;          /* specifies the format of the pixel data */
  int type;            /* specifies the data type of the pixel data */
  int packing;         /* specifies the pixel storage modes */
  unsigned char *data; /* specifies a pointer to the image data in memory */
} ImageStruct;
