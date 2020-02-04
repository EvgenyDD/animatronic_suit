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

#define MASTER_HB_TIMEOUT 600
#define ACK_TIMEOUT_MS 20

#ifdef AIR_PROTOCOL_MASTER
#pragma message("Device is AIR PROTOCOL Master")
#else
#pragma message("Device is AIR PROTOCOL Slave")
#endif
typedef enum
{
#ifdef AIR_PROTOCOL_MASTER
    AIR_PROTOCOL_STATE_NODE_SACK,
#endif
    AIR_PROTOCOL_STATE_TX,
#ifdef AIR_PROTOCOL_MASTER
    AIR_PROTOCOL_STATE_NODE_SLS,
#endif
    AIR_PROTOCOL_STATE_RX,
} AIR_PROTOCOL_STATE;

#ifdef AIR_PROTOCOL_MASTER
uint32_t current_process_node = 0;
static AIR_PROTOCOL_STATE state = AIR_PROTOCOL_STATE_NODE_SACK;
#else
uint32_t to_counter_master = 0;
static AIR_PROTOCOL_STATE state = AIR_PROTOCOL_STATE_RX;
#endif

static uint32_t timeout_await_to_tx = 0;
static uint32_t timeout_await_other_node_tx_to = 0;

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
    while(HAL_GetTick() - now <= ACK_TIMEOUT_MS)
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

/// true - NACKed, false - ACKed
static bool trx_send_ack(uint8_t node_id, const uint8_t *payload, uint8_t payload_length)
{
    for(uint32_t i = 0; i < 3; i++)
    {
        if(_trx_send_ack(node_id, payload, payload_length) == false)
        {
            debug("A%d<%d(%d)\n", node_id, payload[0], payload_length);
            return false;
        }
        debug("a%d<%d(%d)\n", node_id, payload[0], payload_length);
    }
    return true;
}

static bool trx_send_ack_single(uint8_t node_id, const uint8_t *payload, uint8_t payload_length)
{
    if(_trx_send_ack(node_id, payload, payload_length) == false)
    {
        debug("A%d<%d(%d)\n", node_id, payload[0], payload_length);
        return false;
    }
    debug("a%d<%d(%d)\n", node_id, payload[0], payload_length);
    return true;
}

#ifdef AIR_PROTOCOL_MASTER
static bool send_nop(uint8_t dest_id)
{
    static uint8_t nop_data[] = {RFM_NET_CMD_NOP, OWN_ID};
    bool state_tx = trx_send_ack_single(dest_id, nop_data, sizeof(nop_data));
    hb_tracker_update_state(dest_id, state_tx);
    return state_tx;
}
static bool send_sls(uint8_t dest_id)
{
    static uint8_t sls_data[] = {RFM_NET_CMD_SLS, OWN_ID};
    bool state_tx = trx_send_ack(dest_id, sls_data, sizeof(sls_data));
    hb_tracker_update_state(dest_id, state_tx);
    return state_tx;
}
#endif
#ifndef AIR_PROTOCOL_MASTER
static bool send_sle(uint8_t dest_id)
{
    static uint8_t sle_data[] = {RFM_NET_CMD_SLE, OWN_ID};
    bool state_tx = trx_send_ack(dest_id, sle_data, sizeof(sle_data));
    hb_tracker_update_state(dest_id, state_tx);
    return state_tx;
}
#endif
// static void regenerate_timeout_await_to_tx(void)
// {
//     timeout_await_to_tx = HAL_GetTick() + (get_random() % RAND_WAIT_MS_LIMIT);
//     change_state(AIR_PROTOCOL_STATE_AWAIT_RANDOM_TO_TX);
// }

// inline static void regenerate_timeout_await_other_node_tx_to(void) { timeout_await_other_node_tx_to = HAL_GetTick() + TIMEOUT_RX_TRANSMISSION_END_CMD; }
// inline static void regenerate_hb_interval(void) { timeout_send_own_hb = HAL_GetTick() + (get_random() % 20) + HB_INTERVAL; }

static void poll_rx(void)
{
    if(rfm12b_rx_complete())
    {
        if(rfm12b_is_crc_pass())
        {
            if(rfm12b_get_dest_id() == RFM_NET_ID_BROADCAST)
            {
                // __disable_irq();
                // volatile uint8_t *data = rfm12b_get_data();
                // uint8_t data_len = rfm12b_get_data_len();
                // __enable_irq();
            }
            else if(rfm12b_get_dest_id() == OWN_ID)
            {
                debug("$");
                __disable_irq();

                bool precessed = false;
                if(rfm12b_get_data_len() >= 2)
                {
#ifdef AIR_PROTOCOL_MASTER
                    if(rfm12b_get_data()[0] == RFM_NET_CMD_SLE)
                    {
                        debug("==>SLE\n");
#warning "or hb"
                        change_state(AIR_PROTOCOL_STATE_NODE_SACK);
                        // regenerate_timeout_await_other_node_tx_to();
                        precessed = true;
                    }
#endif

#ifndef AIR_PROTOCOL_MASTER
                    if(rfm12b_get_data()[0] == RFM_NET_CMD_SLS)
                    {
                        debug("==>SLS\n");
                        change_state(AIR_PROTOCOL_STATE_TX);
                        precessed = true;
                    }
                    if(rfm12b_get_data()[0] == RFM_NET_CMD_NOP)
                    {
                        precessed = true;
                    }
#endif
                }

                if(precessed == false) process_data(rfm12b_get_sender_id(), rfm12b_get_data(), rfm12b_get_data_len());

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
    switch(state)
    {
#ifdef AIR_PROTOCOL_MASTER
    case AIR_PROTOCOL_STATE_NODE_SACK:
    {
        if(++current_process_node >= NODES_COUNT) current_process_node = 0;
        if(send_nop(storage[current_process_node].dest_id))
        {
#warning "TODO: HB"
            break;
        }
        change_state(AIR_PROTOCOL_STATE_TX);
    }
    break;

    case AIR_PROTOCOL_STATE_TX:
    {
        if(storage[current_process_node].p_push != storage[current_process_node].p_pull)
        {
            storage[current_process_node].p_pull++;
            if(storage[current_process_node].p_pull >= MSG_COUNT) storage[current_process_node].p_pull = 0;

            if(storage[current_process_node].dest_id == RFM_NET_ID_BROADCAST)
                trx_send_nack(storage[current_process_node].dest_id,
                              storage[current_process_node].data[storage[current_process_node].p_pull],
                              storage[current_process_node].data_len[storage[current_process_node].p_pull]);
            else
            {
                hb_tracker_update_state(storage[current_process_node].dest_id,
                                        trx_send_ack(storage[current_process_node].dest_id,
                                                     storage[current_process_node].data[storage[current_process_node].p_pull],
                                                     storage[current_process_node].data_len[storage[current_process_node].p_pull]));
            }
            return;
        }

        // tx_pending == true
        {
            // send_sls(RFM_NET_ID_MASTER);
            // regenerate_hb_interval();
            // regenerate_timeout_await_to_tx();
            // tx_pending = false;
            change_state(AIR_PROTOCOL_STATE_NODE_SLS);
        }
    }
    break;

    case AIR_PROTOCOL_STATE_NODE_SLS:
    {
        debug("SSLS %d\n",storage[current_process_node].dest_id);
        if(send_sls(storage[current_process_node].dest_id))
        {
#warning "TODO: HB"
            debug("RB\n");
            change_state(AIR_PROTOCOL_STATE_NODE_SACK);
            break;
        }
        change_state(AIR_PROTOCOL_STATE_RX);
    }
    break;

    case AIR_PROTOCOL_STATE_RX:
    {
        poll_rx();
#warning "add TO"
    }
    break;
#else
    case AIR_PROTOCOL_STATE_RX:
    {
        poll_rx();
        if(to_counter_master + MASTER_HB_TIMEOUT < HAL_GetTick())
        {
            hb_tracker_update_state(RFM_NET_ID_MASTER, true);
#warning "Add master ACK & request monitor "
        }
    }
    break;

    case AIR_PROTOCOL_STATE_TX:
    {
        if(storage[0].p_push != storage[0].p_pull)
        {
            storage[0].p_pull++;
            if(storage[0].p_pull >= MSG_COUNT) storage[0].p_pull = 0;

            if(storage[0].dest_id == RFM_NET_ID_BROADCAST)
                trx_send_nack(storage[0].dest_id, storage[0].data[storage[0].p_pull], storage[0].data_len[storage[0].p_pull]);
            else
            {
                hb_tracker_update_state(storage[0].dest_id, trx_send_ack(storage[0].dest_id, storage[0].data[storage[0].p_pull], storage[0].data_len[storage[0].p_pull]));
            }
            return;
        }

        // tx_pending == true
        {
            send_sle(RFM_NET_ID_MASTER);
            // regenerate_hb_interval();
            // regenerate_timeout_await_to_tx();
            to_counter_master = HAL_GetTick();
            tx_pending = false;
            change_state(AIR_PROTOCOL_STATE_RX);
        }
    }
    break;
#endif

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

    rfm12b_init(RFM_NET_GATEWAY);
    rfm12b_encrypt(rfm_net_key, RFM_NET_KEY_LENGTH);
    // rfm12b_sleep();
    // regenerate_hb_interval();
    // regenerate_timeout_await_to_tx();
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
    case AIR_PROTOCOL_STATE_RX:
    {
        // regenerate_timeout_await_to_tx();
    }
    break;

    default:
        break;
    }
}