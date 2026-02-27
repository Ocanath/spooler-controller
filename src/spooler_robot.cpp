#include "spooler_robot.h"
#include "dartt_init.h"
#include "dartt.h"
#include "dartt_sync.h"
#include <cstdio>
#include <cstring>
#include "SDL.h"

static constexpr double THETA_SCALE = 180.0 / ((double)(1 << 14) * 3.14159265);

void SpoolerRobot::add_motor(unsigned char addr, const char* ip, uint16_t port)
{
    motors.emplace_back(addr);
    Motor& m = motors.back();
    snprintf(m.socket.ip, sizeof(m.socket.ip), "%s", ip);
    m.socket.port = port;
    udp_connect(&m.socket);

    int n = (int)motors.size();
    p.conservativeResize(n); 
	iq.conservativeResize(n);  
	t.conservativeResize(n);
	dp.conservativeResize(n);
    p[n-1] = 0.0;  
	iq[n-1] = 0.0f;
	t[n-1] = 0.0;
	dp[n-1] = 0.0;
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
		dp[i] = (float)motors[i].dp_periph.dtheta_fixedpoint_rad_p_sec / 16.f;
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

bool SpoolerRobot::write_zero_offsets()
{
	bool pass = true;
	for(int i = 0; i < (int)motors.size(); i++)
	{
		bool res = motors[i].write_zero_offset();
		if(res == false)
		{
			pass = false;
		}
	}
	return pass;
}

void SpoolerRobot::oscillate(float time)
{
	float elapsed_time = time - prev_time;
	if(elapsed_time > 3)
	{
		prev_time = time;
		if(targ > -10000)
		{
			targ = -20000;
		}
		else
		{
			targ = -2000;
		}
	}
}

float time_sec(void)
{
	return (float)(((double)SDL_GetTicks64())/1000.);	
}

float abs_f(float input)
{
	if(input < 0)
	{
		return -input;
	}
	else
	{
		return input;
	}
}

void SpoolerRobot::calibrate(void)
{
	printf("Starting calibration...\n");
	float start = time_sec();
	bool speed_triggered = false;
	while(time_sec() - start < 10)
	{
		read();
		t[0] = 400.f;
		t[1] = 100.f;
		printf("t = (%f, %f)\n",iq[0], iq[1]);
		float velocity = (dp[0] - dp[1]);
		if(velocity > 100 && speed_triggered == false)
		{
			speed_triggered = true;
		}
		if(speed_triggered && abs_f(velocity) < 1)
		{
			printf("writing zero\n");
			for(int i = 0; i < 1000; i++)
			{
				bool rc = motors[0].write_zero_offset();
				if(rc == true)
				{
					i = 1000;
					continue;
				}
				else
				{
					printf("Fail to write motor0 attempt: %d\n", i);
				}
			}
			printf("Done writing zero\n");
			break;
		}
		write();

		SDL_Delay(10);
	}
	printf("...Stopped\n");

}