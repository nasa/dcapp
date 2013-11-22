#include <string.h>
#include "imgload/imgload.h"
#include "nodes.h"

extern appdata AppData;

static unsigned int FindTexture(char *);

int LoadTexture(char *filename)
{
    unsigned int texture;
    struct node *data;

    // check if this texture has already been loaded
    texture = FindTexture(filename);

    // if not, create a new texture node and load the image
    if (texture == -1)
    {
        data = NewNode(NULL, &(AppData.TextureList));
        texture = imgload(filename);
        data->object.textures.textureFile = strdup(filename);
        data->object.textures.textureID = texture;
    }

    return texture;
}

/*********************************************************************************
 *
 * This function will determine if a texture file has already been loaded.
 *
 *********************************************************************************/
static unsigned int FindTexture(char *textureFile)
{
    struct node *current;

    // Traverse the list to find the texture file name
    for (current = AppData.TextureList; current != NULL; current = current->p_next)
    {
        // If we find it, return the textureID
        if (!strcmp(current->object.textures.textureFile, textureFile)) return (current->object.textures.textureID);
    }

    // If we made it here, we didn't find the texture.
    return (-1);
}
