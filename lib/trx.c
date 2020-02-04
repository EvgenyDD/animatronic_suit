#include "trx.h"
#include "debug.h"
#include "hb_tracker.h"
#include "main.h"
#include "rfm12b.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len);
extern uint32_t get_random(void);

#define RAND_WAIT_MS_LIMIT 15
#define TIMEOUT_RX_TRANSMISSION_END_CMD 300
#define HB_INTERVAL 500

typedef enum
{
    AIR_PROTOCOL_STATE_AWAIT_RANDOM_TO_TX,
    AIR_PROTOCOL_STATE_TX,
    AIR_PROTOCOL_STATE_OTHER_NODE_TX
} AIR_PROTOCOL_STATE;

static AIR_PROTOCOL_STATE state = AIR_PROTOCOL_STATE_AWAIT_RANDOM_TO_TX;
static uint32_t timeout_await_to_tx = 0;
static uint32_t timeout_await_other_node_tx_to = 0;
static uint32_t timeout_send_own_hb = 0;

typedef struct
{
    uint8_t data[MSG_COUNT][RF12_MAXDATA];
    uint8_t data_len[MSG_COUNT];
    uint32_t p_push;
    uint32_t p_pull;
    uint8_t dest_id;
} storage_t;

static storage_t storage[NODES_COUNT];
static bool tx_pending = false;

inline static void change_state(AIR_PROTOCOL_STATE new)
{
    if(state != new)
    {
        debug(">%d\n", new);
        state = new;
    }
}

static bool waitForAck(uint8_t dest_node_id)
{
    uint32_t now = HAL_GetTick();
    while(HAL_GetTick() - now <= 20 /* ms */)
        if(rfm12b_is_ack_received(dest_node_id))
            return true;
    return false;
}

static void trx_send_nack(uint8_t node_id, uint8_t *payload, uint8_t payload_length)
{
    // rfm12b_wakeup();
    debug("n%d<%d\n", node_id, payload[0]);
    rfm12b_send(node_id, payload, payload_length, false, false);
}

static void send_tx_start(void)
{
    static uint8_t data[] = {RFM_NET_CMD_NODE_START_TX, OWN_ID};
    trx_send_nack(RFM_NET_ID_BROADCAST, data, sizeof(data));
}

static void send_tx_end(void)
{
    static uint8_t data[] = {RFM_NET_CMD_NODE_STOP_TX, OWN_ID};
    trx_send_nack(RFM_NET_ID_BROADCAST, data, sizeof(data));
}

static void append_hb(void)
{
    static uint8_t hb[] = {RFM_NET_CMD_HB, OWN_ID};
    trx_send_async(0, hb, sizeof(hb));
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

static bool trx_send_ack(uint8_t node_id, uint8_t *payload, uint8_t payload_length)
{
    for(uint32_t i = 0; i < 3; i++)
    {
        if(_trx_send_ack(node_id, payload, payload_length) == false)
        {
            debug("A%d<%d\n", node_id, payload[0]);
            return false;
        }
        debug("a%d<%d\n", node_id, payload[0]);
    }
    return true;
}

static void regenerate_timeout_await_to_tx(void)
{
    timeout_await_to_tx = HAL_GetTick() + (get_random() % RAND_WAIT_MS_LIMIT);
    change_state(AIR_PROTOCOL_STATE_AWAIT_RANDOM_TO_TX);
}

inline static void regenerate_timeout_await_other_node_tx_to(void) { timeout_await_other_node_tx_to = HAL_GetTick() + TIMEOUT_RX_TRANSMISSION_END_CMD; }
inline static void regenerate_hb_interval(void) { timeout_send_own_hb = HAL_GetTick() + (get_random() % 20) + HB_INTERVAL; }

static void poll_tx(void)
{
    for(uint32_t i = 0; i < NODES_COUNT; i++)
    {
        if(storage[i].p_push != storage[i].p_pull)
        {
            storage[i].p_pull++;
            if(storage[i].p_pull >= MSG_COUNT) storage[i].p_pull = 0;

            if(storage[i].dest_id == RFM_NET_ID_BROADCAST)
                trx_send_nack(storage[i].dest_id, storage[i].data[storage[i].p_pull], storage[i].data_len[storage[i].p_pull]);
            else
                trx_send_ack(storage[i].dest_id, storage[i].data[storage[i].p_pull], storage[i].data_len[storage[i].p_pull]);
            return;
        }
    }

    // tx_pending == true
    {
        send_tx_end();
        regenerate_hb_interval();
        regenerate_timeout_await_to_tx();
        tx_pending = false;
    }
}

static void poll_rx(void)
{
    if(rfm12b_rx_complete())
    {
        if(rfm12b_is_crc_pass())
        {
            if(rfm12b_get_dest_id() == RFM_NET_ID_BROADCAST)
            {
                __disable_irq();
                volatile uint8_t *data = rfm12b_get_data();
                uint8_t data_len = rfm12b_get_data_len();
                if(data_len >= 2)
                {
                    if(data[0] == RFM_NET_CMD_NODE_START_TX)
                    {
                        debug("==>\n");
                        change_state(AIR_PROTOCOL_STATE_OTHER_NODE_TX);
                        regenerate_timeout_await_other_node_tx_to();
                    }
                    else if(data[0] == RFM_NET_CMD_NODE_STOP_TX)
                    {
                        debug("<==\n");
                        regenerate_timeout_await_to_tx();
                    }
                    else if(data[0] == RFM_NET_CMD_HB)
                    {
                        debug("<>\n");
                        hb_tracker_update(rfm12b_get_sender_id());
                    }
                }
                __enable_irq();
            }
            else if(rfm12b_get_dest_id() == OWN_ID)
            {
                debug("$");
                __disable_irq();
                process_data(rfm12b_get_sender_id(), rfm12b_get_data(), rfm12b_get_data_len());
                __enable_irq();

                if(rfm12b_is_ack_requested())
                {
                    rfm12b_sendACK("", 0, false);
                    // debug(" - ACK sent for %d\n", rfm12b_get_data_len());
                }
            }
            else
            {
                // listen other node - somebody is transmitting
                debug(":");
                regenerate_timeout_await_other_node_tx_to();
            }

            // debug("[%d] Size: %d\n", rfm12b_get_sender_id(), rfm12b_get_data_len());
            // for(uint32_t i = 0; i < rfm12b_get_data_len(); i++) //can also use radio.GetDataLen() if you don't like pointers
            //     debug(" > %c\n", (char)rfm12b_get_data()[i]);
        }
        // else
        //     debug("BAD-CRC\n");

        // debug("\n");
    }
}

void trx_poll(void)
{
    if(tx_pending == false)
    {
        if(timeout_send_own_hb < HAL_GetTick())
        {
            append_hb();
        }
    }

    switch(state)
    {
    case AIR_PROTOCOL_STATE_AWAIT_RANDOM_TO_TX:
    {
        poll_rx();
        if(timeout_await_to_tx < HAL_GetTick())
        {
            if(tx_pending)
            {
                change_state(AIR_PROTOCOL_STATE_TX);
                send_tx_start();
            }
        }
    }
    break;

    case AIR_PROTOCOL_STATE_TX:
    {
        poll_tx();
    }
    break;

    case AIR_PROTOCOL_STATE_OTHER_NODE_TX:
    {
        poll_rx();
        if(timeout_await_other_node_tx_to < HAL_GetTick())
        {
            regenerate_timeout_await_to_tx();
        }
    }
    break;

    default:
        break;
    }
}

void trx_send_async(uint8_t iterator, const uint8_t *payload, uint8_t payload_length)
{
    if(iterator < NODES_COUNT && payload_length < RF12_MAXDATA)
    {
        uint8_t next_push = storage[iterator].p_push + 1;
        if(next_push >= MSG_COUNT) next_push = 0;
        if(next_push != storage[iterator].p_pull)
        {
            memcpy(&storage[iterator].data[next_push], payload, payload_length);
            storage[iterator].data_len[next_push] = payload_length;
            storage[iterator].p_push = next_push;
            tx_pending = true;
        }
    }
}

void trx_init(void)
{
    memset(storage, 0, sizeof(storage));

    rfm12b_init(RFM_NET_GATEWAY, OWN_ID);
    rfm12b_encrypt(rfm_net_key, RFM_NET_KEY_LENGTH);
    trx_init_node(0, RFM_NET_ID_BROADCAST);
    // rfm12b_sleep();
    regenerate_hb_interval();
    regenerate_timeout_await_to_tx();
}

void trx_init_node(uint8_t iterator, uint8_t node_id)
{
    if(iterator < NODES_COUNT)
    {
        storage[iterator].dest_id = node_id;
    }
}

void something_receiving(void)
{
    switch(state)
    {
    case AIR_PROTOCOL_STATE_AWAIT_RANDOM_TO_TX:
    {
        regenerate_timeout_await_to_tx();
    }
    break;

    case AIR_PROTOCOL_STATE_OTHER_NODE_TX:
    {
        regenerate_timeout_await_other_node_tx_to();
    }
    break;

    default:
        break;
    }
}