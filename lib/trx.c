#include "trx.h"
#include "main.h"
#include "rfm12b.h"
#include "debug.h"
#include <stdio.h>
#include <stdarg.h>

extern void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len);

static bool waitForAck(uint8_t dest_node_id)
{
    uint32_t now = HAL_GetTick();
    while(HAL_GetTick() - now <= 50 /* ms */)
        if(rfm12b_is_ack_received(dest_node_id))
            return true;
    return false;
}

/**
 * @brief 
 * 
 * @param node_id 
 * @param payload 
 * @param payload_length 
 * @return true ACK not received
 * @return false ACK received
 */
static bool _trx_send_ack(uint8_t node_id, uint8_t *payload, uint8_t payload_length)
{
    // rfm12b_wakeup();
    rfm12b_send(node_id, payload, payload_length, true, true);

    if(waitForAck(node_id))
        return false;
    else
        return true;
}

bool trx_send_ack(uint8_t node_id, uint8_t *payload, uint8_t payload_length)
{
    for(uint32_t i=0; i<3; i++)
    {
        if(_trx_send_ack(node_id, payload, payload_length) == false) return false;
    }
    return true;
}

void trx_send_nack(uint8_t node_id, uint8_t *payload, uint8_t payload_length)
{
    // rfm12b_wakeup();
    rfm12b_send(node_id, payload, payload_length, false, false);
}

void trx_init(uint8_t own_id)
{
    rfm12b_init(RFM_NET_GATEWAY, own_id);
    rfm12b_encrypt(rfm_net_key, RFM_NET_KEY_LENGTH);
    // rfm12b_sleep();
}

void trx_poll_rx(void)
{
    if(rfm12b_rx_complete())
    {
        if(rfm12b_is_crc_pass())
        {
            if(rfm12b_is_ack_requested())
            {
                rfm12b_sendACK("", 0, false);
                // debug(" - ACK sent for %d\n", rfm12b_get_data_len());
            }
            
            // debug("[%d] Size: %d\n", rfm12b_get_sender(), rfm12b_get_data_len());
            // for(uint32_t i = 0; i < rfm12b_get_data_len(); i++) //can also use radio.GetDataLen() if you don't like pointers
            //     debug(" > %c\n", (char)rfm12b_get_data()[i]);
            process_data(rfm12b_get_sender(), rfm12b_get_data(), rfm12b_get_data_len());
        }
        else
            debug("BAD-CRC\n");

        // debug("\n");
    }
}

void trx_poll_tx_hb(uint32_t period_tx_ms, int nodes_count, ...)
{
    static uint32_t fire_timestamp = 0;
    if(fire_timestamp < HAL_GetTick())
    {
        fire_timestamp = HAL_GetTick() + period_tx_ms;

        // rfm12b_sleep();

        uint8_t hb[] = {RFM_NET_CMD_HB};

        va_list args;
        va_start(args, nodes_count);
        
        for (int i = 0; i < nodes_count; i++) 
        {
            trx_send_nack(va_arg(args, int), hb, sizeof(hb));
        }

        va_end(args);
    }
}