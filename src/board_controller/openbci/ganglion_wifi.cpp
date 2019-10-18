#include "ganglion_wifi.h"
#include "custom_cast.h"
#include "timestamp.h"

#define START_BYTE 0xA0
#define END_BYTE 0xC0


void GanglionWifi::read_thread ()
{
    /*  Only for Ganglion + WIFI shield, plain Ganglion has another format
        Byte 1: 0xA0
        Byte 2: Sample Number
        Bytes 3-5: Data value for EEG channel 1
        Bytes 6-8: Data value for EEG channel 2
        Bytes 9-11: Data value for EEG channel 3
        Bytes 12-14: Data value for EEG channel 4
        Bytes 15-26: zeroes
        Aux Data Bytes 27-32: 6 bytes of data
        Byte 33: 0xCX where X is 0-F in hex
    */
    int res;
    unsigned char b[32];
    while (keep_alive)
    {
        // check start byte
        res = server_socket->recv (b, 1);
        if (res != 1)
        {
            continue;
        }
        if (b[0] != START_BYTE)
        {
            continue;
        }

        res = server_socket->recv (b, 32);
        if (res != 32)
        {
            continue;
        }
        // check end byte
        if (b[res - 1] != END_BYTE)
        {
            safe_logger (
                spdlog::level::warn, "Wrong end byte, found {}, required {}", b[res - 1], END_BYTE);
            continue;
        }

        double package[12];
        // package num
        package[0] = (double)b[0];
        // eeg
        for (int i = 0; i < 4; i++)
        {
            package[i + 1] = eeg_scale * cast_24bit_to_int32 (b + 1 + 3 * i);
        }
        // accel
        package[5] = accel_scale * cast_16bit_to_int32 (b + 25);
        package[6] = accel_scale * cast_16bit_to_int32 (b + 27);
        package[7] = accel_scale * cast_16bit_to_int32 (b + 29);

        db->add_data (get_timestamp (), package);
    }
}
