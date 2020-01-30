#ifndef ASUIT_PROTOCOL_H
#define ASUIT_PROTOCOL_H

#include "rapi/parsers/abstract_parser.h"
#include <numeric>
#include <math.h>       /* sqrt */
#include "tools/helper.h"
#include "../lib/serial_suit_protocol.h"
#include "tools/queue_threadsafe.h"
#include "tools/thread_pool.h"
#include <string.h>


class ASuitProtocol : public AbstractParser
{
private:
    typedef struct
    {
        bool received;
        uint8_t id;
        uint8_t ret_code;
        uint32_t address;
    }flash_response_t;

public:
    ASuitProtocol(int iface_id, std::shared_timed_mutex &p_impl_mutex, std::queue_threadsafe<char> &q) :
        AbstractParser(iface_id, p_impl_mutex),
        q_log(q)
    {
        memset(&flash_response, 0, sizeof(flash_response_t));
    }

    virtual ~ASuitProtocol()
    {
    }

    ErrorCodes tx_data(std::vector<uint8_t> vector)
    {
        std::shared_lock<std::shared_timed_mutex> lock(if_impl_mutex);
        return tx_method(vector);
    }

    void reset(uint8_t id)
    {
        AnyContainer c;
        c.append<uint8_t>(SSP_CMD_REBOOT);
        c.append<uint8_t>(id);
        tx_data(c.data);
    }

    /**
     * @brief flash
     * @param id
     * @param addr
     * @param bytes
     * @return true if error
     */
    bool flash(uint8_t id, uint32_t addr, const std::vector<uint8_t> &bytes)
    {
        AnyContainer c;
        c.append<uint8_t>(SSP_CMD_FLASH);
        c.append<uint8_t>(id);
        c.append<uint32_t>(addr);
        for(const auto &i : bytes)
        {
            c.append<uint8_t>(i);
        }
        flash_response.received = false;

        tx_data(c.data);

        uint32_t timeout = 0;
        while(flash_response.received == false)
        {
#define SSP_TO_FLASH_MS 1
            std::this_thread::sleep_for(std::chrono::milliseconds(SSP_TO_FLASH_MS));
            timeout += SSP_TO_FLASH_MS;
            if(timeout > 1000)
            {
                debug("Timeout!\n");
                return true;
            }
        }

        if(flash_response.ret_code != 0)
        {
            debug("Error code: " + std::to_string(flash_response.ret_code) + "\n");
            return true;
        }

        if(flash_response.id != id)
        {
            debug("Wrong ID!\n");
            return true;
        }

        if(flash_response.address != addr)
        {
            debug("Wrong address!\n");
            return true;
        }

        return false;
    }


private:
    std::queue_threadsafe<char> &q_log;

    flash_response_t flash_response;

    ErrorCodes rx_data(const std::vector<uint8_t> &bytes)
    {
        std::shared_lock<std::shared_timed_mutex> lock(if_impl_mutex);

        if(bytes.size() < 1) return ErrorCodes::Ok;
        switch(bytes[0])
        {
        case SSP_CMD_DEBUG:
//            PRINT_MSG_("RX " << std::string((char*)&bytes[1], bytes.size()-1));
            q_log.push('$');
            q_log.push(' ');
            for(auto i=1U; i<bytes.size()-1; i++)
                q_log.push(static_cast<char>(bytes[i]));
            break;

        case SSP_CMD_FLASH:
        {
//            for(uint32_t i=0; i<bytes.size(); i++)
//            {
//                PRINT_MSG("#\t" << (int)bytes[i]);
//            }
             if(bytes.size() >= 3)
             {
                flash_response.id = bytes[1];
                flash_response.ret_code = bytes[2];
             }

             if(bytes.size() == 3+4)
             {
                 memcpy(&flash_response.address, &bytes[3], sizeof(uint32_t));
             }
             else
             {
                 flash_response.address = 0xFFFFFFFF;
             }
             flash_response.received = true;
        }
        break;

        default:
            PRINT_MSG("Unknown CMD: " << std::to_string((int)bytes[0]) << " len: " << std::to_string(bytes.size()-1));
//            for(uint32_t i=0; i<bytes.size(); i++)
//            {
//                PRINT_MSG("\t" << (int)bytes[i]);
//            }
            break;
        }

        return ErrorCodes::Ok;
    }

    void debug(std::string s)
    {
        for(auto &i : s) {q_log.push(i);}
    }
};

class Flasher
{
public:
    Flasher(std::queue_threadsafe<char> &q_log) : q_log(q_log)
    {}
    void set_protocol(ASuitProtocol *proto) {protocol = proto;}
    void start(uint8_t _id, std::vector<uint8_t> &_fw)
    {
        if(flashing_active)
        {
            debug("Can't start flashing! Still active!");
            return;
        }

        flashing_active = true;
        id = _id;
        fw = _fw;

        tf.start(std::bind(&Flasher::flash_process, this));
    }

    int flash_process();

    void debug(std::string s)
    {
        for(auto &i : s) {q_log.push(i);}
    }

private:
    ASuitProtocol *protocol{nullptr};
    std::queue_threadsafe<char> &q_log;
    bool flashing_active{false};
    basic_thread tf;

    uint8_t id;
    std::vector<uint8_t> fw;
};

#endif // ASUIT_PROTOCOL_H
