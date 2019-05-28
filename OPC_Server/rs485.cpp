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
#include "utils.h"


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
	//char write_buffer[] = { 0x01, 0x04, 0x03, 0xDD, 0x00, 0x01, 0xA1, 0xB4 };
	uint8_t read_buffer[255];
	int16_t dur_ms = 0;
	int16_t common_dur_ms = 0;
	struct timespec start, stop, duration, stop2, common_duration;




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
		clock_gettime(CLOCK_REALTIME, &start);


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
					
					config.rx_buf = (char*) &read_buffer;
					config.rx_size = sizeof(read_buffer);
					config.rx_expected = ((vector_optimize[i].request[y][4] << 8) + vector_optimize[i].request[y][5]); 		//ATTENTION: rx_expected must be set for correct driver reception.
					
					printf("expected size: %d\n", ((vector_optimize[i].request[y][4] << 8) + vector_optimize[i].request[y][5]));

					ioctl(node->f_id, RS485_SEND_PACKED, &config);
					

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







		clock_gettime(CLOCK_REALTIME, &stop);
		duration = time_diff(start, stop);
		dur_ms = duration.tv_sec * 100 + (duration.tv_nsec / 1000000);
		//printf("Time %d", duration.tv_sec*100 + (duration.tv_nsec / 1000000) );

		if (dur_ms < node->poll_period)
		{
			usleep((node->poll_period - dur_ms) * 1000);
			//printf("Time %d ", (node->poll_period - dur_ms));
		}

		clock_gettime(CLOCK_REALTIME, &stop2);
		common_duration = time_diff(start, stop2);
		//printf("\nCommon Duration Time %d \n\n", common_duration.tv_sec*100 + (common_duration.tv_nsec / 1000000) );
	}


}
