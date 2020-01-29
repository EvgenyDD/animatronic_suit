#ifndef ASUIT_PROTOCOL_H
#define ASUIT_PROTOCOL_H

#include "rapi/parsers/abstract_parser.h"
#include <numeric>
#include <math.h>       /* sqrt */
#include "tools/helper.h"
#include "../lib/serial_suit_protocol.h"
#include "tools/queue_threadsafe.h"


class ASuitProtocol : public AbstractParser
{
public:
    ASuitProtocol(int iface_id, std::shared_timed_mutex &p_impl_mutex, std::queue_threadsafe<char> &q) :
        AbstractParser(iface_id, p_impl_mutex),
        q_log(q)
    {

    }

    virtual ~ASuitProtocol()
    {
    }

    ErrorCodes tx_data(std::vector<uint8_t> vector)
    {
        std::shared_lock<std::shared_timed_mutex> lock(if_impl_mutex);
        return tx_method(vector);
    }


private:
    std::queue_threadsafe<char> &q_log;

    ErrorCodes rx_data(const std::vector<uint8_t> &bytes)
    {
        std::shared_lock<std::shared_timed_mutex> lock(if_impl_mutex);

        if(bytes.size() < 1) return ErrorCodes::Ok;
        switch(bytes[0])
        {
        case SSP_CMD_DEBUG:
//            PRINT_MSG_("RX " << std::string((char*)&bytes[1], bytes.size()-1));
            for(auto i=1U; i<bytes.size()-1; i++)
                q_log.push(static_cast<char>(bytes[i]));
            break;

        default:
            PRINT_MSG("Unknown CMD: " << std::to_string(bytes[0]) << " len: " << std::to_string(bytes.size()-1));
            break;
        }

        return ErrorCodes::Ok;
    }

};

#endif // ASUIT_PROTOCOL_H
