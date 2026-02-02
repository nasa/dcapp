#ifndef _DC_PIXELSTREAM_SHMEM_
#define _DC_PIXELSTREAM_SHMEM_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct _DcPsShmemHandle {
    uint8_t _index;
} DcPsShmemHandle;

#ifdef __cplusplus
extern "C" {
#endif

// global funcs
void dc_ps_shmem_init(void);
void dc_ps_shmem_update(void);
void dc_ps_shmem_cleanup(void);

// for specific handles
DcPsShmemHandle dc_ps_shmem_add_source(const char *filepath);
void            dc_ps_shmem_remove_source(DcPsShmemHandle handle);
bool            dc_ps_shmem_is_connected(DcPsShmemHandle handle);
bool            dc_ps_shmem_has_new_data(DcPsShmemHandle handle);
void            dc_ps_shmem_get_data(DcPsShmemHandle handle, unsigned char *out_data, size_t out_data_size, size_t *out_size);
uint32_t        dc_ps_shmem_get_width(DcPsShmemHandle handle);
uint32_t        dc_ps_shmem_get_height(DcPsShmemHandle handle);

#ifdef __cplusplus
}
#endif

#endif
