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

std::mutex mutex_lock;

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
	
	Device* device = (Device*)args;

	int result = 0;	
	//char write_buffer[] = { 00, 00, 00, 00, 00, 06, 01, 03, 00, 00, 00, 01 };
	char write_buffer[12];
	char read_buffer[255];

	extern UA_Server *server;
	UA_Variant value;
	int modbus_value = 0;

	struct timeval timeout;
	
	fd_set set;	
	int trial = 0;

	signal(SIGPIPE, SIG_IGN);
	
	while (1)
	{
	

		//Проходим по тэгам устройства 
		for (int j = 0; j < device->vectorTag.size(); j++)
		{
			if (device->vectorTag[j].on == 1)
			{
				//Формируем пакет для запроса
				write_buffer[4] = 0x00;
				write_buffer[5] = 0x06;
				write_buffer[6] = device->device_address;
				write_buffer[7] = device->vectorTag[j].function;
				write_buffer[8] = device->vectorTag[j].reg_address >> 8;
				write_buffer[9] = device->vectorTag[j].reg_address;
				write_buffer[10] = 0x00;				
				if (device->vectorTag[j].data_type == "int") write_buffer[11] = 0x01;
				if (device->vectorTag[j].data_type == "float") write_buffer[11] = 0x02;

				//Запрос
				result = send(device->device_socket, &write_buffer, sizeof(write_buffer), 0);

				//Ответ
				//result = read(device->device_socket, &read_buffer, sizeof(read_buffer));

				//reinit fd
				FD_ZERO(&set); /* clear the set */
				FD_SET(device->device_socket, &set); /* add our file descriptor to the set */

				//reinit timeout
				timeout.tv_sec = device->poll_timeout / 1000;
				timeout.tv_usec = (device->poll_timeout % 1000) * 1000;

				//mutex_lock.lock();

				result = select(device->device_socket+1, &set, 0, 0, &timeout);			
				
				//mutex_lock.unlock();
				
				if (result > 0 )
				{
					result = read(device->device_socket, &read_buffer, sizeof(read_buffer));
				}
				else if (result == 0)
				{
					printf("Timeout device: %d, poll attempt %d \n", device->device_address, trial);
					
					if (++trial > 3)
					{
						printf("Timeout device: %d, exit thread.\n", device->device_address);
						return 0;
					}
				}

				

				
				if (device->vectorTag[j].data_type == "int")
				{
					device->vectorTag[j].value = ((read_buffer[9] << 8) + read_buffer[10]);

					UA_Int16 opc_value = (UA_Int16)device->vectorTag[j].value;
					UA_Variant_setScalarCopy(&value, &opc_value, &UA_TYPES[UA_TYPES_INT16]);
					UA_Server_writeValue(server, device->vectorTag[j].tagNodeId, value);
				}
				
				if (device->vectorTag[j].data_type == "float") 
				{
					modbus_value = ( (read_buffer[9] << 24) + (read_buffer[10] << 16) + (read_buffer[11] << 8) + read_buffer[12] );
					device->vectorTag[j].value = *reinterpret_cast<float*>(&modbus_value);

					UA_Float opc_value = (UA_Float)device->vectorTag[j].value;
					UA_Variant_setScalarCopy(&value, &opc_value, &UA_TYPES[UA_TYPES_FLOAT]);
					UA_Server_writeValue(server, device->vectorTag[j].tagNodeId, value);
				}				
			}

			printf("%d %s %.00f\n", device->device_address, device->vectorTag[j].name.c_str(), device->vectorTag[j].value);
		}






		//if (device->id_device == 0) printf("%d\n", device->vectorTag[0].value);


		usleep(device->poll_period * 1000);
	}

	

}