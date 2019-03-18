#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <string> 
#include <iostream>
#include <vector>
#include <unistd.h>

#include "open62541.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include<netdb.h>

#include "main.h"
#include "serialize.h"
#include "tcp.h"



UA_Server *server;
pthread_t server_thread;
//pthread_t modbus_thread;
pthread_t* modbus_thread;
int status;


static void addCounterSensorVariable(UA_Server * server) {

	UA_NodeId counterNodeId = UA_NODEID_NUMERIC(1, 1);

	UA_QualifiedName counterName = UA_QUALIFIEDNAME(1, "Piece Counter[pieces]");

	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.description = UA_LOCALIZEDTEXT("en_US", "Piece Counter (units:pieces)");
	attr.displayName = UA_LOCALIZEDTEXT("en_US", "Piece Counter");
	attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

	UA_Int32 counterValue = 0;
	UA_Variant_setScalarCopy(&attr.value, &counterValue, &UA_TYPES[UA_TYPES_INT32]);

	UA_Server_addVariableNode(server, counterNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), counterName, UA_NODEID_NULL, attr, NULL, NULL);
}




void* workerOPC(void *args)
{
	UA_Boolean running = true;


	UA_ServerConfig *config = UA_ServerConfig_new_default();
	
	server = UA_Server_new(config);

	addCounterSensorVariable(server);

	UA_StatusCode retval = UA_Server_run(server, &running);
	UA_Server_delete(server);
	UA_ServerConfig_delete(config);
}








void* pollingEngine(void *args)
{
	Controller* controller = (Controller*)args;
	
	

	//Узел (Node/Coms)
	for (int i = 0; i < controller->vectorNode.size(); i++)
	{
		//printf("%s\n", controller->vectorNode[i].name.c_str());

		pthread_t modbus_thread[controller->vectorNode[i].vectorDevice.size()];

		//Создание сокета и подключение к устройству
		if (controller->vectorNode[i].intertype == "TCP")
		{
			connectDeviceTCP(&controller->vectorNode[i]);
		}
			


		//Устройство (Device)
		for (int j = 0; j < controller->vectorNode[i].vectorDevice.size(); j++)
		{
			//printf("    %s\n", controller->vectorNode[i].vectorDevice[j].name.c_str());

			controller->vectorNode[i].vectorDevice[j].id_device = j;
			controller->vectorNode[i].vectorDevice[j].device_socket = controller->vectorNode[i].socket;
			
			if (controller->vectorNode[i].vectorDevice[j].on = 1)
			{

				if (controller->vectorNode[i].intertype == "TCP")
				{
					pthread_create(&modbus_thread[j], NULL, pollingDeviceTCP, &controller->vectorNode[i].vectorDevice[j]);
				}

				//if (controller->vectorNode[i].intertype == "RS485")
				//{
					//pthread_create(&modbus_thread[j], NULL, pollingDevice, &controller->vectorNode[i]);
				//}
			}
			
		}


	}	
}





int main()
{
	

	printf("Start OPC server...\n\n");
	
	
	Controller controller;


	controller = serializeFromJSON("/root/projects/OPC_Server/opc.json");
	

	pthread_create(&server_thread, NULL, workerOPC, NULL); //Запуск OPC сервера 
	
	
	pollingEngine(&controller);
	
	
	pthread_join(server_thread, (void**)&status);
	
	//pthread_join(modbus_thread[0], (void**)&status);
	

	sleep(15);

	return 0;
}