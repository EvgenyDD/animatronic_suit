#include "adc.h"
#include "debug.h"
#include "main.h"
#include "rfm12b.h"

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

// extern CRC_HandleTypeDef hcrc;

// extern RNG_HandleTypeDef hrng;

extern UART_HandleTypeDef huart1;

extern uint32_t rx_cnt;

uint8_t KEY[] = "ABCDABCDABCDABCD";
char payload[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890~!@#$%^&*(){}[]`|<>?+=:;,.";
#define NETWORKID 0xD4 //the network ID we are on
#define GATEWAYID 2    //the node ID we're sending to
#define NODEID 1       //network ID used for this unit
#define ACK_TIME 50    // # of ms to wait for an ack

void init(void)
{
    __HAL_UART_ENABLE(&huart1);

    rfm12b_init(NETWORKID, NODEID);
    rfm12b_encrypt(KEY, 16);

    rfm12b_sleep();

    adc_init();
}

uint8_t buff[200];

// wait a few milliseconds for proper ACK, return true if received
static bool waitForAck(void)
{
    uint32_t now = HAL_GetTick();
    while(HAL_GetTick() - now <= ACK_TIME)
        if(rfm12b_is_ack_received(GATEWAYID))
            return true;
    return false;
}

void loop(void)
{
    static uint32_t prev_tick = 0;

    static uint8_t prev_sts = 0xff;
    uint8_t stsx = 0; //rfm12b_trx_1b(0);
    if(stsx != prev_sts)
    {
        prev_sts = stsx;
        // debug("STS Change: %d @%d\n", prev_sts, HAL_GetTick());
    }

    if(prev_tick < HAL_GetTick())
    {
        prev_tick = HAL_GetTick() + 500;
        LED0_GPIO_Port->ODR ^= LED0_Pin;
        LED1_GPIO_Port->ODR ^= LED1_Pin;
        //     LED2_GPIO_Port->ODR ^= LED2_Pin;
        //     LED3_GPIO_Port->ODR ^= LED3_Pin;
        //     LED4_GPIO_Port->ODR ^= LED4_Pin;
        //     LED5_GPIO_Port->ODR ^= LED5_Pin;
        //     LED6_GPIO_Port->ODR ^= LED6_Pin;
        //     LED7_GPIO_Port->ODR ^= LED7_Pin;
        static uint32_t cnt = 0;
        // debug(">>> %d\n", cnt++);
        if(0)
        {
            static uint8_t sendSize = 0;
            static bool request_ack = false;

            debug("Sending[%d]\n", sendSize + 1);
            // for(uint32_t i = 0; i < sendSize + 1U; i++)
            //     debug(" > %c\n", (char)payload[i]);

            request_ack = !(sendSize % 3); //request ACK every 3rd xmission

            rfm12b_wakeup();

            rfm12b_send(GATEWAYID, payload, sendSize + 1, request_ack, true);

            if(request_ack)
            {
                debug(" - waiting for ACK...");
                if(waitForAck())
                    debug("ok!\n");
                else
                    debug("nothing...\n");
            }

            rfm12b_sleep();

            sendSize = (sendSize + 1) % 88;

            debug("\n");
        }

        int16_t sts = 777;
        // if((sts = radio_rcv(buff, 200)) > 0) //try to receive some data
        // {
        //     // if(buff[0] == 'a' && buff[1] == 'b' && buff[2] == 'c')
        //     // {
        //     //     SET(LED_GREEN);            //indicate that the packet is received
        //     //     _delay_ms(180);            //wait for some (shorter) time
        //     //     CLEAR(LED_GREEN);        //and turn the LED off
        //     // }
        //     debug("FUCK YES!");
        // }
        // else
        {
            // debug("Fail %d >\n", sts);
        }

        // rfm_test_tx();
        //     static float i = 0;
        //     debug("Fuckyou %.2f %d\n", i, rx_cnt);
        //     i += 0.5f;
    }

    if(1)
    {
        if(rfm12b_rx_complete())
        {
            if(rfm12b_is_crc_pass())
            {
                debug("[%d] Size: %d\n", rfm12b_get_sender(), rfm12b_get_data_len());
                // for(uint32_t i = 0; i < rfm12b_get_data_len(); i++) //can also use radio.GetDataLen() if you don't like pointers
                //     debug(" > %c\n", (char)rfm12b_get_data()[i]);

                if(rfm12b_is_ack_requested())
                {
                    rfm12b_sendACK("", 0, false);
                    debug(" - ACK sent\n");
                }
            }
            else
                debug("BAD-CRC\n");

            debug("\n");
        }
    }

    // rx();
}