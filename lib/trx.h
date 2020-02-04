#ifndef TRX_H
#define TRX_H

#include "rfm_net.h"
#include <stdbool.h>
#include <stdint.h>

void trx_init(void);
void trx_init_node(uint8_t iterator, uint8_t node_id);

void trx_poll(void);

void trx_send_async(uint8_t iterator, const uint8_t *payload, uint8_t payload_length);

#endif // TRX_H