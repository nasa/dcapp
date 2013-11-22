#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include "nodes.h"
#include "opengl_draw.h"

extern int CheckCondition(struct node *);

int update_dyn_elements(struct node *);

int update_dyn_elements(struct node *list)
{
    struct node *current;
    size_t nbytes, origbytes, newh, pad, padbytes, offset, offsetbytes;
    uint32_t on = 1, off = 0;
    int updated, update_count = 0;

    for (current = list; current != NULL; current = current->p_next)
    {
        switch (current->info.type)
        {
            case Condition:
                if (CheckCondition(current))
                    update_count += update_dyn_elements(current->object.cond.TrueList);
                else
                    update_count += update_dyn_elements(current->object.cond.FalseList);
                break;
            case Container:
                update_count += update_dyn_elements(current->object.cont.SubList);
                break;
            case PixelStream:
                if (!current->object.pixelstream.fp) current->object.pixelstream.fp = fopen(current->object.pixelstream.filename, "r");
                else if (current->object.pixelstream.shm)
                {
                    updated = 0;
                    if (!(current->object.pixelstream.shm->writing))
                    {
                        memcpy(&(current->object.pixelstream.shm->reading), &on, 4);
                        if (current->object.pixelstream.buffercount != current->object.pixelstream.shm->buffercount)
                        {
                            current->object.pixelstream.buffercount = current->object.pixelstream.shm->buffercount;
                            rewind(current->object.pixelstream.fp);
                            newh = current->object.pixelstream.shm->width * (*(current->info.h)) / (*(current->info.w));
                            if (newh > current->object.pixelstream.shm->height)
                            {
                                origbytes = current->object.pixelstream.shm->width * current->object.pixelstream.shm->height * 3;
                                // overpad the pad by 1 so that no unfilled rows show up
                                pad = ((newh - current->object.pixelstream.shm->height) / 2) + 1;
                                padbytes = current->object.pixelstream.shm->width * pad * 3;
                                nbytes = (size_t)(current->object.pixelstream.shm->width * newh * 3);
                                current->object.pixelstream.pixels = realloc(current->object.pixelstream.pixels, nbytes);
                                bzero(current->object.pixelstream.pixels, padbytes);
                                bzero(current->object.pixelstream.pixels + nbytes - padbytes, padbytes);
                                fread(current->object.pixelstream.pixels + padbytes, 1, origbytes, current->object.pixelstream.fp);
                            }
                            else
                            {
                                nbytes = (size_t)(current->object.pixelstream.shm->width * current->object.pixelstream.shm->height * 3);
                                current->object.pixelstream.pixels = realloc(current->object.pixelstream.pixels, nbytes);
                                fread(current->object.pixelstream.pixels, 1, nbytes, current->object.pixelstream.fp);
                            }
                            updated = 1;
                        }
                        memcpy(&(current->object.pixelstream.shm->reading), &off, 4);

                        if (updated)
                        {
                            if (newh < current->object.pixelstream.shm->height)
                            {
                                offset = (current->object.pixelstream.shm->height - newh) / 2;
                                offsetbytes = current->object.pixelstream.shm->width * offset * 3;
                                set_texture(current->object.pixelstream.textureID, current->object.pixelstream.shm->width, newh, current->object.pixelstream.pixels + offsetbytes);
                            }
                            else
                                set_texture(current->object.pixelstream.textureID, current->object.pixelstream.shm->width, newh, current->object.pixelstream.pixels);
                            update_count++;
                        }   
                    }
                }
                break;
            default:
                break;
        }
    }
    return update_count;
}
