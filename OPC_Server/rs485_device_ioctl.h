#pragma once

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/ioctl.h>

typedef struct 
{
    /* data */
    size_t tx_count;     //count byte to transmite
    const char* tx_buf;  //data to transmit 
    size_t rx_size;     //size rx_buffer
    size_t rx_count;    //count recive byte
	size_t rx_expected; //ATTENTION: rx_expected must be set for correct driver reception.
    char* rx_buf;       //recived data
}rs485_device_ioctl_t;

typedef struct 
{
    /* data */
    uint32_t reg_adr;
    uint32_t reg_value;     
}rs485_uart_reg_t;

#define MY_MACIG 'r'
#define RS485_SEND_PACKED       _IOWR(MY_MACIG, 1, rs485_device_ioctl_t*)
#define RS485_CONFIG_UART       _IOWR(MY_MACIG, 2, rs485_uart_config_t*)
#define RS485_READ_CONFIG_UART  _IOWR(MY_MACIG, 3, rs485_uart_config_t*)
#define RS485_READ_REG          _IOWR(MY_MACIG, 4, rs485_uart_reg_t*)
#define RS485_WRITE_REG         _IOWR(MY_MACIG, 5, rs485_uart_reg_t*)
#define RS485_INIT              _IOWR(MY_MACIG, 6, rs485_uart_reg_t*)

typedef enum{
RS485_BITS_5 = 5,
RS485_BITS_6,
RS485_BITS_7,
RS485_BITS_8,
RS485_BITS_9,
}rs485_bits_t;

typedef enum{
RS485_STOP_BIT_0 = 0,
RS485_STOP_BIT_1,
RS485_STOP_BIT_2,
}rs485_stop_bit_t;

typedef enum{
RS485_PARITY_NONE = 0,
RS485_PARITY_ODD,
RS485_PARITY_EVEN,
}rs485_parity_t;

typedef struct 
{
    /* data */
    size_t baudrate;
    rs485_bits_t bits;
    rs485_stop_bit_t stop_bit;
    rs485_parity_t parity;
}rs485_uart_config_t;

/* 
{
//@example write/read data from port. 
//sending and waiting for a response
{
	char tx[10];
	char rx[300];
    int F_ID_x = open("/dev/rs485_6", O_RDWR);
	if (F_ID_x < 0)
	{
		return -1;
	}		

	//create request
	tx[0] = 1;
	tx[2] = 2;
	//....

	//transaction preparation	
	rs485_uart_config_t config;
	config.tx_buf = tx;
	config.tx_count = 2;
	config.rx_buf = rx;
	config.rx_size = sizeof(rx);
	config.rx_expected = 2; 		//ATTENTION: rx_expected must be set for correct driver reception.

	// The driver will add a transaction to the queue.
	// When the queue comes up,
	// If (config.tx_count > 0)
	// 	{
	// 		The driver enable transiver.
	// 		Clear internal rx buffer. 
	// 		Wait idle line.
	// 		Send data from config.tx_buf to port. 
	// 		Guaranted to send config.tx_count or more. max config.tx_count + 32.
	// 		Disable transiver.
	//	}
	// If (config.tx_count == 0) The driver not enable transiver and not clear internal buffer.
	// The driver waits until it recieves the config.rx_expected or the time required to receive.
	// The driver set config.rx_count recived number, in range from 0 to config.rx_size. 
	ioctl(F_ID_x, RS485_SEND_PACKED, &config);

	cout << received " << config.rx_count  << " bytes."<< endl;

	int count = config.rx_count;
	cout << "rx: ";
	while(count < config.rx_count)
	{
		cout << " 0x" << hex << rx[count++];
	}
	cout << endl;	
	close(F_ID_x);
	exit(0);
}

//@example only read data from port. 
//print all incoming data to the port
//Only works if the synchronization and sampling protocol is turned off
{
	char rx[4096];
    int F_ID_x = open("/dev/rs485_6", O_RDWR);
	if (F_ID_x < 0)
	{
		return -1;
	}		

	while(!stop())
	{
		//transaction preparation	
		rs485_uart_config_t config;
		config.tx_buf = NULL;
		config.tx_count = 0;
		config.rx_buf = rx;
		config.rx_size = sizeof(rx);
		config.rx_expected = sizeof(rx); 		//ATTENTION: rx_expected must be set for correct driver reception.

		// The driver will add a transaction to the queue.
		// When the queue comes up,
		// If (config.tx_count == 0) The driver not enable transiver and not clear internal buffer.
		// The driver waits until it recieves the config.rx_expected or the time required to receive.
		// The driver set config.rx_count recived number, in range from 0 to config.rx_size. 
		ioctl(F_ID_x, RS485_SEND_PACKED, &config);

		cout << received " << config.rx_count  << " bytes."<< endl;

		int count = config.rx_count;
		cout << "rx: ";
		while(count < config.rx_count)
		{
			cout << " 0x" << hex << rx[count++];
		}
		cout << endl;	
	}
	close(F_ID_x);
	exit(0);
}


//@example config rs485:
{
	int F_ID_x = open("/dev/rs485_6", O_RDWR);
	if (F_ID_x < 0)
	{
		return -1;
	}
	rs485_device_ioctl_t config;
	config.baudrate = 	3000000;
	config.bits 	= 	RS485_BITS_8;
	config.stop_bit = 	RS485_STOP_BIT_1;
	config.parity 	=	RS485_PARITY_NONE;

	ioctl(F_ID_x, RS485_CONFIG_UART, &config);

	ioctl(F_ID_x, RS485_READ_CONFIG_UART, &config);

	cout << "config.baudrate = " << config.baudrate << endl;
	cout << "config.bits = " 	<< config.bits << endl;
	cout << "config.parity = " 	<< config.parity << endl;
	cout << "config.stop_bit = " << config.stop_bit << endl;
	
	close(F_ID_x);
	exit(0);
}

//@example read/write register not use/ need for develope:
{
    int F_ID_x = open("/dev/rs485_6", O_RDWR);
	if (F_ID_x < 0)
	{
		return -1;
	}
	rs485_uart_reg_t tmp;
	tmp.reg_adr = UCR1;
	tmp.reg_value = 0;
	int ioret = ioctl(F_ID_x, RS485_READ_REG, &tmp);
	////ioctl(F_ID_x, RS485_INIT, tmp);
	cout << "UCR1(" << hex << tmp.reg_adr << ") = "<< hex << tmp.reg_value << endl;
		
    //reset uart
    tmp.reg_adr = UCR2;
	tmp.reg_value = 0;
    ioctl(fd, RS485_WRITE_REG, tmp);

	close(F_ID_x);
	exit(0);
}

*/