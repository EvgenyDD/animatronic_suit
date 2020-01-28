#include "hb_tracker.h"
#include "main.h"

#define MAX_NODES 2

static hbt_node_t nodes[MAX_NODES];
static uint32_t nodes_count = 0;

hbt_node_t *hb_tracker_init(uint8_t node_id, uint32_t timeout_ms)
{
    if(nodes_count >= MAX_NODES) return NULL;

    nodes[nodes_count].node_id = node_id;
    nodes[nodes_count].timeout_ms = timeout_ms;
    nodes[nodes_count].last_hb_ms = 0;

    nodes_count++;

    return &nodes[nodes_count - 1];
}

bool hb_tracker_update(uint8_t node_id)
{
    for(uint32_t i = 0; i < nodes_count; i++)
    {
        if(node_id == nodes[i].node_id)
        {
            nodes[i].last_hb_ms = HAL_GetTick();
            return false;
        }
    }
    return true;
}

bool hb_tracker_is_timeout(const hbt_node_t *node) { return HAL_GetTick() > node->timeout_ms + node->last_hb_ms; }
uint32_t hb_tracker_get_timeout(const hbt_node_t *node) { return HAL_GetTick() - node->last_hb_ms; }