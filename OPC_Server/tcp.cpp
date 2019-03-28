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
#include<netdb.h>

#include "open62541.h"
#include <mutex>

#define _BSD_SOURCE //for <sys/time.h>

#include <time.h>
#include <sys/time.h>


struct timespec start, stop, duration;


void* connectDeviceTCP(void *args)
{
	Node* node = (Node*)args;

	struct sockaddr_in serv_addr;

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	int result = 0;

	if (node != NULL)
	{

		//Создаем сокет
		if ((node->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("\n Socket creation error \n");
		}
		else
		{
			printf("\nSocket create: %d\n", node->socket);
		}

		//Таймаут
		if (setsockopt(node->socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		{
			printf("\n setsockopt failed \n");
		}
		if (setsockopt(node->socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		{
			printf("\n setsockopt failed \n");
		}

		//IP и Порт
		serv_addr.sin_addr.s_addr = inet_addr(node->address.c_str());
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(node->port);

		//Подключаем
		result = connect(node->socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

		if (result < 0)
		{
			printf("\nConnection Failed: %d\n", result);
		}
	}
}



void* pollingDeviceTCP(void *args)
{
	
	
	Node* node = (Node*)args;

	int result = 0;	
	//char write_buffer[] = { 00, 00, 00, 00, 00, 06, 01, 03, 00, 00, 00, 01 };
	char write_buffer[12];
	char read_buffer[255];

	extern UA_Server *server;
	UA_Variant value;
	int modbus_value = 0;

	struct timeval timeout;
	
	fd_set set;	
	int trial[node->vectorDevice.size()]; //Хранилище флагов о попытках опроса

	int dur_ms = 0;

	//signal(SIGPIPE, SIG_IGN);

	
	while (1)
	{		
		clock_gettime(CLOCK_REALTIME, &start);

		//Проходим по устройствам
		for (int i = 0; i < node->vectorDevice.size(); i++)
		{
			if (node->vectorDevice[i].on == 1)
			//Проходим по тэгам устройства 
			for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
			{
				if (node->vectorDevice[i].vectorTag[j].on == 1)
				{
					//Формируем пакет для запроса
					write_buffer[4] = 0x00;
					write_buffer[5] = 0x06;
					write_buffer[6] = node->vectorDevice[i].device_address;
					write_buffer[7] = node->vectorDevice[i].vectorTag[j].function;
					write_buffer[8] = node->vectorDevice[i].vectorTag[j].reg_address >> 8;
					write_buffer[9] = node->vectorDevice[i].vectorTag[j].reg_address;
					write_buffer[10] = 0x00;
					if (node->vectorDevice[i].vectorTag[j].data_type == "int") write_buffer[11] = 0x01;
					if (node->vectorDevice[i].vectorTag[j].data_type == "float") write_buffer[11] = 0x02;

					//Запрос
					result = send(node->vectorDevice[i].device_socket, &write_buffer, sizeof(write_buffer), 0);

					//Ответ
					//result = read(device->device_socket, &read_buffer, sizeof(read_buffer));

					//reinit fd
					FD_ZERO(&set); /* clear the set */
					FD_SET(node->vectorDevice[i].device_socket, &set); /* add our file descriptor to the set */

					//reinit timeout
					timeout.tv_sec = node->vectorDevice[i].poll_timeout / 1000;
					timeout.tv_usec = (node->vectorDevice[i].poll_timeout % 1000) * 1000;

					//mutex_lock.lock();

					result = select(node->vectorDevice[i].device_socket + 1, &set, 0, 0, &timeout);

					//mutex_lock.unlock();

					if (result > 0)
					{
						result = read(node->vectorDevice[i].device_socket, &read_buffer, sizeof(read_buffer));
					}
					else if (result == 0)
					{
						printf("Timeout device: %d, poll attempt %d \n", node->vectorDevice[i].device_address, trial[i]);

						//reset buffer to zero when poll timed out
						for (int h = 0; h < sizeof(read_buffer); h++) read_buffer[h] = 0;

						if (++trial[i] > 3)
						{
							printf("Timeout device: %d, switch off device.\n", node->vectorDevice[i].device_address);							

							//Выключаем устройство
							node->vectorDevice[i].on = 0;
						}
					}




					if (node->vectorDevice[i].vectorTag[j].data_type == "int")
					{
						node->vectorDevice[i].vectorTag[j].value = ((read_buffer[9] << 8) + read_buffer[10]);

						UA_Int16 opc_value = (UA_Int16)node->vectorDevice[i].vectorTag[j].value;
						UA_Variant_setScalarCopy(&value, &opc_value, &UA_TYPES[UA_TYPES_INT16]);
						UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
					}

					if (node->vectorDevice[i].vectorTag[j].data_type == "float")
					{
						modbus_value = ((read_buffer[9] << 24) + (read_buffer[10] << 16) + (read_buffer[11] << 8) + read_buffer[12]);
						node->vectorDevice[i].vectorTag[j].value = *reinterpret_cast<float*>(&modbus_value);

						UA_Float opc_value = (UA_Float)node->vectorDevice[i].vectorTag[j].value;
						UA_Variant_setScalarCopy(&value, &opc_value, &UA_TYPES[UA_TYPES_FLOAT]);
						UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
					}

					
					
				}

				printf("%d %s %.00f\n", node->vectorDevice[i].device_address, node->vectorDevice[i].vectorTag[j].name.c_str(), node->vectorDevice[i].vectorTag[j].value);
			}			

		}


		
		clock_gettime(CLOCK_REALTIME, &stop);
		duration.tv_sec = stop.tv_sec - start.tv_sec;
		duration.tv_nsec = stop.tv_nsec - start.tv_nsec;
		dur_ms = duration.tv_sec * 100 + (duration.tv_nsec / 1000000);
		//printf("Time %d", duration.tv_sec*100 + (duration.tv_nsec / 1000000) );

		if (dur_ms < node->vectorDevice[0].poll_period)
		{
			usleep( (node->vectorDevice[0].poll_period - dur_ms) *1000);
			
			//printf("Time %d ", (node->vectorDevice[0].poll_period - dur_ms));
		}
	}

	

}