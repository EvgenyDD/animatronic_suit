#include "asuit_protocol.h"

int Flasher::flash_process()
{
    while(fw.size() % 4) fw.push_back(0xff);
    auto timestamp_start = std::chrono::system_clock::now();
    debug(StringHelper::printf("Flashing ID: %d Size: %d\n", id, fw.size()));

    float percent = 0;
    for(uint32_t i=4; i<fw.size();)
    {
        if(protocol)
        {
            std::vector<uint8_t> data;
            for(uint32_t k = 0; k < 4*14 && k < fw.size()-i; k++) data.push_back(fw[i+k]);
            if(protocol->flash(id, 0x08060000+i, data))
            {
                debug(StringHelper::printf("Flashing ID: %d Failed @%d\n", id, i));
                flashing_active = false;
                return 1;
            }
            i += data.size();
            float percent_now = 100.0f * static_cast<float>(i) / static_cast<float>(fw.size());
            if(percent < percent_now)
            {
                percent += 10.0f;
                debug(StringHelper::printf("\t%.1f\%\t %d / %d", static_cast<double>(percent), i, fw.size()));
            }
        }
    }
    std::vector<uint8_t> data;
    for(uint32_t k = 0; k < 4; k++) data.push_back(fw[k]);
    if(protocol->flash(id, 0x08060000, data))
    {
        debug(StringHelper::printf("Flashing ID: %d Failed @%d\n", id, 0));
        flashing_active = false;
        return 1;
    }

    protocol->reset(id);

//    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    debug(StringHelper::printf("Flashing ID: %d Finished in %.3f sec\n", id, 0.001 * (double)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - timestamp_start).count()));
    flashing_active = false;
    return 1;
}
