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

		//Соединяем
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
	
	char write_buffer[] = { 00, 00, 00, 00, 00, 06, 01, 03, 00, 00, 00, 01 };
	char read_buffer[255];
	

	while (1)
	{
	

		//Проходим по тэгам устройства 
		for (int j = 0; j < device->vectorTag.size(); j++)
		{
			if (device->vectorTag[j].on == 1)
			{
				//Формируем пакет для запроса
				write_buffer[6] = device->device_address;
				write_buffer[7] = device->vectorTag[j].function;
				write_buffer[8] = device->vectorTag[j].reg_address >> 8;
				write_buffer[9] = device->vectorTag[j].reg_address;
				write_buffer[10] = 0x00;
				write_buffer[11] = 0x01;

				//Запрос
				result = send(device->device_socket, &write_buffer, sizeof(write_buffer), 0);

				//Ответ
				result = read(device->device_socket, &read_buffer, sizeof(read_buffer));

				device->vectorTag[j].value = ((read_buffer[9] << 8) + read_buffer[10]);

				//printf("%.00f\n", device->vectorTag[j].value);
			}
		}






		//result = send(sock, &write_buffer, sizeof(write_buffer), 0);
		//result = read(sock, &read_buffer, sizeof(read_buffer));

		//node->vectorDevice[1].vectorTag[0].value = float((read_buffer[9] << 8) + read_buffer[10]);

		//UA_Variant value;
		//UA_Int32 myInteger = (UA_Int32) int(node->vectorDevice[1].vectorTag[0].value);
		//UA_Variant_setScalarCopy(&value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
		//UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 1), value);


		if (device->id_device == 1) printf("%.00f\n", device->vectorTag[0].value);


		sleep(1);
	}

	

}