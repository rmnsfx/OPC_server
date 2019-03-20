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
pthread_t* modbus_thread;
int status;



void* workerOPC(void *args)
{
	Controller* controller = (Controller*) args;
	
	UA_Boolean running = true;

	UA_ServerConfig *config = UA_ServerConfig_new_default();
	
	server = UA_Server_new(config);



	UA_NodeId contrId; /* get the nodeid assigned by the server */
	UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
	oAttr.displayName = UA_LOCALIZEDTEXT("en-US", (char*) controller->name.c_str());
	UA_Server_addObjectNode(server, UA_NODEID_NULL,
		UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
		UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
		UA_QUALIFIEDNAME(1, (char*)controller->name.c_str()), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
		oAttr, NULL, &contrId);


	for (int i = 0; i < controller->vectorNode.size(); i++)
	{
		//UA_NodeId nodeId = UA_NODEID_NUMERIC(1, i);
		std::string id_node = controller->vectorNode[i].name.c_str();
		UA_NodeId nodeId;

		UA_VariableAttributes statusAttr = UA_VariableAttributes_default;

		UA_Variant_setScalar(&statusAttr.value, NULL, NULL);
		statusAttr.displayName = UA_LOCALIZEDTEXT("en-US", (char*)id_node.c_str());
		UA_Server_addVariableNode(server, UA_NODEID_NULL, contrId,
			UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
			UA_QUALIFIEDNAME(1, (char*)id_node.c_str()),
			UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr, NULL, &nodeId);

		for (int j = 0; j < controller->vectorNode[i].vectorDevice.size(); j++)
		{
			UA_NodeId deviceId;
			std::string id_device = controller->vectorNode[i].vectorDevice[j].name;

			UA_VariableAttributes statusAttr2 = UA_VariableAttributes_default;
			
			UA_Variant_setScalar(&statusAttr2.value, NULL, NULL);
			statusAttr2.displayName = UA_LOCALIZEDTEXT("en-US", (char*)id_device.c_str());
			UA_Server_addVariableNode(server, UA_NODEID_NULL, nodeId,
				UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
				UA_QUALIFIEDNAME(1, (char*)id_device.c_str()),
				UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr2, NULL, &deviceId);

			for (int k = 0; k < controller->vectorNode[i].vectorDevice[j].vectorTag.size(); k++)
			{
				UA_NodeId tagNodeId;
				std::string id_tag = controller->vectorNode[i].vectorDevice[j].vectorTag[k].name;

				UA_VariableAttributes statusAttr3 = UA_VariableAttributes_default;

				if (controller->vectorNode[i].vectorDevice[j].vectorTag[k].data_type == "int")
				{
					UA_Int16 value = 0;
					UA_Variant_setScalar(&statusAttr3.value, &value, &UA_TYPES[UA_TYPES_INT16]);
					statusAttr3.displayName = UA_LOCALIZEDTEXT("en-US", (char*)id_tag.c_str());
					UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceId,
						UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
						UA_QUALIFIEDNAME(1, (char*)id_tag.c_str()),
						UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr3, NULL, &tagNodeId);
				}

				if (controller->vectorNode[i].vectorDevice[j].vectorTag[k].data_type == "float")
				{
					UA_Float value = 0;
					UA_Variant_setScalar(&statusAttr3.value, &value, &UA_TYPES[UA_TYPES_FLOAT]);
					statusAttr3.displayName = UA_LOCALIZEDTEXT("en-US", (char*)id_tag.c_str());
					UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceId,
						UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
						UA_QUALIFIEDNAME(1, (char*)id_tag.c_str()),
						UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr3, NULL, &tagNodeId);
				}

				controller->vectorNode[i].vectorDevice[j].vectorTag[k].tagNodeId = tagNodeId;
			}
		}

	}




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


	controller = serializeFromJSON("/usr/httpserv/opc.json");
	

	pthread_create(&server_thread, NULL, workerOPC, &controller); //Запуск OPC сервера 
	
	
	pollingEngine(&controller);
	
	
	pthread_join(server_thread, (void**)&status);
	

	sleep(15);

	return 0;
}