#ifndef TRX_H
#define TRX_H

#include <stdbool.h>
#include <stdint.h>
#include "rfm_net.h"

void trx_init(uint8_t own_id);

bool trx_send_ack(uint8_t node_id, uint8_t *payload, uint8_t payload_length);
void trx_send_nack(uint8_t node_id, uint8_t *payload, uint8_t payload_length);

void trx_poll_rx(void);
void trx_poll_tx_hb(uint32_t period_tx_ms, bool nodes_timeout, int nodes_count, ...);

#endif // TRX_H