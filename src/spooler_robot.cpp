#include "spooler_robot.h"
#include "dartt_init.h"
#include "dartt.h"
#include "dartt_sync.h"
#include <cstdio>
#include <cstring>

static constexpr double THETA_SCALE = 180.0 / ((double)(1 << 14) * 3.14159265);

void SpoolerRobot::add_motor(unsigned char addr, const char* ip, uint16_t port)
{
    motors.emplace_back(addr);
    Motor& m = motors.back();
    snprintf(m.socket.ip, sizeof(m.socket.ip), "%s", ip);
    m.socket.port = port;
    udp_connect(&m.socket);

    int n = (int)motors.size();
    p.conservativeResize(n);  iq.conservativeResize(n);  t.conservativeResize(n);
    p[n-1] = 0.0;  iq[n-1] = 0.0f;  t[n-1] = 0.0;
}

bool SpoolerRobot::read()
{
    bool ok = true;
    for (int i = 0; i < (int)motors.size(); i++)
    {
        dartt_buffer_t r = {
            .buf  = motors[i].ds.ctl_base.buf,
            .size = sizeof(uint32_t) * 4,
            .len  = sizeof(uint32_t) * 4
        };
        if (dartt_read_multi(&r, &motors[i].ds) != DARTT_PROTOCOL_SUCCESS)
            ok = false;

        p[i]  = motors[i].dp_periph.theta_rem_m * THETA_SCALE;
        iq[i] = (float)motors[i].dp_periph.iq;
    }
    return ok;
}

void SpoolerRobot::write()
{
    for (int i = 0; i < (int)motors.size(); i++)
    {
        motors[i].dp_ctl.command_word = (int32_t)t[i];
        dartt_buffer_t w = {
            .buf  = motors[i].ds.ctl_base.buf,
            .size = sizeof(uint32_t),
            .len  = sizeof(uint32_t)
        };
        if (dartt_write_multi(&w, &motors[i].ds) != 0)
            printf("write failure motor %d\r\n", i);
    }
}
