#ifndef _DCAPP_DRAW_NODE_H_
#define _DCAPP_DRAW_NODE_H_

#include "dcapp.h"

void dc_app_draw_node_list(_AppData *app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *node_transform);
void dc_app_draw_node(_AppData *app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
void dc_app_flush_deferred_sets(_AppData *app_data);

#endif
