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
    RFM_NET_CMD_HB = 100,

    RFM_NET_CMD_STS_HEAD = 10,
    RFM_NET_CMD_LIGHT,

    RFM_NET_CMD_DEBUG = 40,
};

#endif // RFM_NET_H