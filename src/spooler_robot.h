#ifndef SPOOLER_ROBOT_H
#define SPOOLER_ROBOT_H

#include <vector>
#include <cstdint>
#include <Eigen/Dense>
#include "motor.h"

class SpoolerRobot
{
public:
    std::vector<Motor> motors;

    Eigen::VectorXd p;   // angular positions (degrees), size = motors.size()
    Eigen::VectorXf iq;  // q-axis currents (float — plotter pointer compat)
    Eigen::VectorXd t;   // tension commands (set by controller before write())
	Eigen::VectorXf dp;	//angular velocity

	float k;
	float kd;
	float x;
	float tmax;
	float targ;


	bool do_oscillation;
	float prev_time;


    SpoolerRobot() = default;
    SpoolerRobot(const SpoolerRobot&) = delete;
    SpoolerRobot& operator=(const SpoolerRobot&) = delete;

    // Add motor, connect immediately; resizes p/iq/t
    void add_motor(unsigned char addr, const char* ip, uint16_t port);

    // Read all motors via dartt_read_multi; convert fixed-point → p, iq.
    // Returns true if all reads succeeded.
    bool read();

    // Convert t → command_word (int32_t) for each motor; write via dartt_write_multi.
    void write();

	//send one-time fixed theta offset to motors
	bool write_zero_offset();

	void oscillate(float time);
};

#endif
