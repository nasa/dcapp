#include <sys/shm.h>
#include "nodes.hh"

// This is a safe size.  The shmget call will fail if it's too small or too large.  It only needs
// to be large enough to house the elements of the associated data structure.
#define SHM_SIZE 1024

extern appdata AppData;

static void *FindShm(key_t);

void *LoadShm(key_t shm_key)
{
    void *shm;
    int shmid;
    struct node *data;

    // check if this shm has already been created
    shm = FindShm(shm_key);

    // if not, create a new shm node
    if (!shm)
    {
        data = NewNode(NULL, &(AppData.ShMemList));

        if ((shmid = shmget(shm_key, SHM_SIZE, IPC_CREAT | 0666)) < 0)
        {
            perror("LoadShm shmget");
            return 0;
        }
        if ((shm = shmat(shmid, NULL, 0)) < 0)
        {
            perror("LoadShm shmat");
            return 0;
        }

        data->object.shmem.shm_key = shm_key;
        data->object.shmem.shm = shm;
    }

    return shm;
}

/*********************************************************************************
 *
 * This function will determine if a shm has already been created.
 *
 *********************************************************************************/
static void *FindShm(key_t shm_key)
{
    struct node *current;

    // Traverse the list to find the shm key
    for (current = AppData.ShMemList; current != NULL; current = current->p_next)
    {
        // If we find it, return the shm
        if (current->object.shmem.shm_key == shm_key) return (current->object.shmem.shm);
    }

    // If we made it here, we didn't find the shm.
    return 0;
}
