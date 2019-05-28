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
#include "crc.h"
#include "poll_optimize.h"


char* pathToPort(int node_port)
{
	switch (node_port)
	{
	case 0:
		return "/dev/rs485_0";
	case 1:
		return "/dev/rs485_1";
	case 2:
		return "/dev/rs485_2";
	case 3:
		return "/dev/rs485_3";
	case 4:
		return "/dev/rs485_4";
	case 5:
		return "/dev/rs485_5";
	case 6:
		return "/dev/rs485_6";
	case 7:
		return "/dev/rs485_7";
	
	
	default:
		printf("Warning! Wrong RS-485 port parameter. Default value: /dev/rs485_3\n");
		return "/dev/rs485_3";
	}
};




void* pollingDeviceRS485(void *args)
{	
	
	
	int16_t result = 0;
	char write_buffer[] = { 0x01, 0x04, 0x03, 0xDD, 0x00, 0x01, 0xA1, 0xB4 };
	char read_buffer[255];





	Node* node = (Node*)args;


	if (node != NULL)
	{
		char* port = pathToPort(node->port);

		node->f_id = open(port, O_RDWR);
		

		if (node->f_id < 0)
		{
			printf("\nConnection Failed: %d\n", node->f_id);
			//return -1;
		}
	}
	


	std::vector<Optimize> vector_optimize = reorganizeNodeIntoPolls(node);


	//transaction preparation			
	rs485_device_ioctl_t config;
	   
	while (1)
	{

		//Проходим по устройствам
		for (int i = 0; i < node->vectorDevice.size(); i++)
		{
			if (node->vectorDevice[i].on == 1)
			{

				//Отправляем запросы и принимаем ответы по порядку
				for (int y = 0; y < vector_optimize[i].request.size(); y++)
				{
					config.tx_buf = (const char*) &vector_optimize[i].request[y][0];
					config.tx_count = vector_optimize[i].request[y].size();
					
					config.rx_buf = read_buffer;
					config.rx_size = sizeof(read_buffer);
					config.rx_expected = 100; 		//ATTENTION: rx_expected must be set for correct driver reception.
					
					ioctl(node->f_id, RS485_SEND_PACKED, &config);
					//result = send(node->vectorDevice[i].device_socket, &vector_optimize[i].request[y][0], vector_optimize[i].request[y].size(), 0);

					printf("--->");
					for (int w = 0; w < 8; w++)
					{
						printf("%X ", vector_optimize[i].request[y][w]);					
					}
					printf("\n <---");
					for (int w = 0; w < 20; w++)
					{					
						printf("%X ", read_buffer[w]);
					}
					printf("\n");

					
				}
			}
		}





		//config.tx_buf = write_buffer;
		//config.tx_count = sizeof(write_buffer);
		//config.rx_buf = read_buffer;
		//config.rx_size = sizeof(read_buffer);
		//config.rx_expected = 10; 		//ATTENTION: rx_expected must be set for correct driver reception.

		//

		//ioctl(F_ID_x, RS485_SEND_PACKED, &config);

		//for (int i = 0; i < 8; i++)
		//{
		//	printf("%d ", read_buffer[i]);
		//}
		//printf("\n");

		//printf("F_ID = %d, rx_count = %d \n", node->f_id, config.rx_count);

		//for (int i = 0; i < sizeof(read_buffer); i++) read_buffer[i] = 0;

		//int crc = calculate_crc((uint8_t*)&buf, 6);
		//printf("crc = %d \n", crc);

		sleep(1);
	}


}
