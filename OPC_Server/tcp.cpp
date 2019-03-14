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

extern UA_Server *server;

void* pollingDevice(void *args)
{
	
	Device* device = (Device*)args;
	
	int result = 0;
	char write_buffer[] = { 00, 00, 00, 00, 00, 06, 01, 03, 00, 00, 00, 01 };
	char read_buffer[255];
	int sock = 0, valread;
	
	
	struct sockaddr_in serv_addr;
	
	
	//const char* address = "192.168.0.146";
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	

	if (device != NULL)
	{


		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("\n Socket creation error \n");
		}
		else
		{
			printf("\nSocket create: %d\n", sock);
		}

		//Таймаут
		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		{
			printf("\n setsockopt failed \n");
		}
		if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		{
			printf("\n setsockopt failed \n");
		}



		//memset(&serv_addr, 0, sizeof(serv_addr));

		//bzero((char *)&serv_addr, sizeof(serv_addr));

		//// Convert IPv4 and IPv6 addresses from text to binary form 
		//if (inet_pton(AF_INET, address, &serv_addr.sin_addr) <= 0)
		//{
		//	printf("\nInvalid address/ Address not supported \n");
		//	//return -1;
		//}
		//inet_pton(AF_INET, "192.168.0.146", &serv_addr.sin_addr);

		

		serv_addr.sin_addr.s_addr = inet_addr(device->ip.c_str());
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(device->port);

		result = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

		if (result < 0)
		{
			printf("\nConnection Failed: %d\n", result);
		}



		while (1)
		{
			result = send(sock, &write_buffer, sizeof(write_buffer), 0);
			result = read(sock, &read_buffer, sizeof(read_buffer));

			device->vectorTag[0].value = float((read_buffer[9] << 8) + read_buffer[10]);

			UA_Variant value;
			UA_Int32 myInteger = (UA_Int32) int(device->vectorTag[0].value);
			UA_Variant_setScalarCopy(&value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
			UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 1), value);


			
			printf("%.00f\n", device->vectorTag[0].value);


			sleep(1);
		}

	}

}