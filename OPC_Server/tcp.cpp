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
#include "poll_optimize.h"




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

	struct timespec start, stop, duration;

	//signal(SIGPIPE, SIG_IGN);

	//Оптимизируем запрос
	Optimize optimize;
	std::vector<Optimize> vector_optimize;
	vector_optimize = reorganizeNodeIntoPolls(node);
	if (vector_optimize.size() != node->vectorDevice.size()) printf("Warning! Vector != Device. Break.");	
	

	
	while (1)
	{		
		clock_gettime(CLOCK_REALTIME, &start);

		

		//Проходим по устройствам
		for (int i = 0; i < node->vectorDevice.size(); i++)
		{
			if (node->vectorDevice[i].on == 1)
			{				
				result = send(node->vectorDevice[i].device_socket, &vector_optimize[i].request, sizeof(vector_optimize[i].request), 0);

				//reinit fd
				FD_ZERO(&set); /* clear the set */
				FD_SET(node->vectorDevice[i].device_socket, &set); /* add our file descriptor to the set */

				//reinit timeout
				timeout.tv_sec = node->vectorDevice[i].poll_timeout / 1000;
				timeout.tv_usec = (node->vectorDevice[i].poll_timeout % 1000) * 1000;

				result = select(node->vectorDevice[i].device_socket + 1, &set, 0, 0, &timeout);

				if (result > 0)
				{
					result = read(node->vectorDevice[i].device_socket, &vector_optimize[i].response, sizeof(vector_optimize[i].response));
				}
				else if (result == 0)
				{
					printf("Timeout device: %d, poll attempt %d \n", node->vectorDevice[i].device_address, trial[i]);

					//reset buffer to zero when poll timed out
					//for (int h = 0; h < sizeof(vector_optimize[i].request); h++) vector_optimize[i].request[h] = 0;

					if (++trial[i] > 3)
					{
						printf("Timeout device: %d, switch off device.\n", node->vectorDevice[i].device_address);

						//Выключаем устройство
						node->vectorDevice[i].on = 0;
					}
				}



				//Проходим по тэгам устройства 
				for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
				{
					if (node->vectorDevice[i].vectorTag[j].on == 1)
					{

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

		}


		
		clock_gettime(CLOCK_REALTIME, &stop);
		duration.tv_sec = stop.tv_sec - start.tv_sec;
		duration.tv_nsec = stop.tv_nsec - start.tv_nsec;
		dur_ms = duration.tv_sec * 100 + (duration.tv_nsec / 1000000);
		//printf("Time %d", duration.tv_sec*100 + (duration.tv_nsec / 1000000) );

		if (dur_ms < node->poll_period)
		{
			usleep( (node->poll_period - dur_ms) * 1000);
			
			//printf("Time %d ", (node->vectorDevice[0].poll_period - dur_ms));
		}
	}

	

}