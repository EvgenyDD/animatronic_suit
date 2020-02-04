#ifndef RFM_NET_H
#define RFM_NET_H

#include <stdint.h>

#define RFM_NET_GATEWAY 0x6A

#define RFM_NET_ID_BROADCAST 0
#define RFM_NET_ID_CTRL 50
#define RFM_NET_ID_HEAD 51
#define RFM_NET_ID_TAIL 52

#define RFM_NET_KEY_LENGTH 16
extern uint8_t rfm_net_key[RFM_NET_KEY_LENGTH + 1];

enum RFM_NET_PROTOCOL
{
    RFM_NET_CMD_NOP = 0, /// NOP cmd
    RFM_NET_CMD_SLS, /// Slave listen enable
    RFM_NET_CMD_SLE, /// Slave listen disable

    RFM_NET_CMD_STS_HEAD = 10,
    RFM_NET_CMD_LIGHT,

    RFM_NET_CMD_DEBUG = 40,
};

#endif // RFM_NET_H