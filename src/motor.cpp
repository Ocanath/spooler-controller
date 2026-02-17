#include "motor.h"


Motor::Motor(unsigned char addr)
{

	//iniialize the motor
	ds.address = addr;	//must be mapped
	
	/*TODO: consider allocating heap? */
	ds.ctl_base.buf = (unsigned char *)(&dp_ctl);	//must be assigned
	ds.ctl_base.size = sizeof(dartt_mctl_params_t);
	ds.periph_base.buf = (unsigned char *)(&dp_periph);	//must be assigned
	ds.periph_base.size = sizeof(dartt_mctl_params_t);

	ds.msg_type = TYPE_SERIAL_MESSAGE;

	ds.tx_buf.buf = new unsigned char[SERIAL_BUFFER_SIZE];
	ds.tx_buf.size = SERIAL_BUFFER_SIZE - NUM_BYTES_COBS_OVERHEAD;		//DO NOT CHANGE. This is for a good reason. See above note
	ds.tx_buf.len = 0;
	ds.rx_buf.buf = new unsigned char[SERIAL_BUFFER_SIZE];
	ds.rx_buf.size = SERIAL_BUFFER_SIZE - NUM_BYTES_COBS_OVERHEAD;	//DO NOT CHANGE. This is for a good reason. See above note
	ds.rx_buf.len = 0;
	ds.blocking_tx_callback = &tx_blocking;	//todo - figure something out here, cus we can't use the same socket...
	ds.blocking_rx_callback = &rx_blocking;
	ds.timeout_ms = 10;
}


Motor::~Motor()
{
	delete[] ds.tx_buf.buf;
	delete[] ds.rx_buf.buf;
}
