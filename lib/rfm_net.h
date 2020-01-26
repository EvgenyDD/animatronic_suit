#ifndef RFM_NET_H
#define RFM_NET_H

#define RFM_NET_GATEWAY 0x6A

#define RFM_NET_ID_CTRL 50
#define RFM_NET_ID_HEAD 51
#define RFM_NET_ID_TAIL 52

#define RFM_NET_KEY_LENGTH 16
extern uint8_t rfm_net_key[RFM_NET_KEY_LENGTH + 1];

enum RFM_NET_PROTOCOL
{
    RFM_NET_CMD_HB = 0,
    RFM_NET_CMD_LIGHT = 1,

    RFM_NET_CMD_DEBUG = 40,
};

#endif // RFM_NET_H