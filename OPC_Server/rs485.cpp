#include "main.h"

#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <string> 
#include <iostream>
#include <vector>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <netdb.h>

#include "rs485.h"
#include "rs485_device_ioctl.h"

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <sys/ioctl.h>

#include <fcntl.h>




void* connectDeviceRS485(void *args)
{
	Node* node = (Node*)args;


	if (node != NULL)
	{
		int F_ID_x = open("/dev/rs485_6", O_RDWR);
		if (F_ID_x < 0)
		{
			//return -1;
		}

		
	}
}


void* pollingDeviceRS485(void *args)
{
	Node* node = (Node*)args;

	int F_ID_x = open("/dev/rs485_7", O_RDWR);

	char write_buffer[] = { 0x01, 0x04, 0x03, 0xDD, 0x00, 0x01, 0xA1, 0xB4 };
	char read_buffer[255];

	//transaction preparation			
	rs485_device_ioctl_t config;

	while (1)
	{
		config.tx_buf = write_buffer;
		config.tx_count = sizeof(write_buffer);
		config.rx_buf = read_buffer;
		config.rx_size = sizeof(read_buffer);
		config.rx_expected = 8; 		//ATTENTION: rx_expected must be set for correct driver reception.

		

		ioctl(F_ID_x, RS485_SEND_PACKED, &config);

		for (int i = 0; i < 8; i++)
		{
			printf("%d ", read_buffer[i]);
		}
		printf("\n");

		printf("F_ID = %d, rx_count = %d \n", F_ID_x, config.rx_count);

		//for (int i = 0; i < sizeof(read_buffer); i++) read_buffer[i] = 0;

		sleep(1);
	}


}
