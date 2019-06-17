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
#include <mutex>

#include "rs485_device_ioctl.h"
#include "rs485.h"
#include "utils.h"
#include <signal.h>
#include <syslog.h> 
#include <sys/syscall.h>
#include <time.h>
#include <memory>

 
static UA_Server* server;

UA_Server* getServer(void)
{
	return server;
}


void* workerOPC(void *args)
{

	UA_Int16 value_int16;
	UA_UInt16 value_uint16;
	UA_Int32 value_int32;
	UA_UInt32 value_uint32;
	UA_Float value_float;

	//Controller * controller = (Controller*) calloc(1, sizeof(Controller)); // Выделяем память 
	//controller = (Controller*)args;

	Controller* controller = (Controller*) args;
		
	UA_Boolean running = true;

	

	UA_ServerConfig* config = UA_ServerConfig_new_default();	
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

				if (controller->vectorNode[i].vectorDevice[j].vectorTag[k].enum_data_type == Data_type::int16)
				{
					value_int16 = 0;
					UA_Variant_setScalar(&statusAttr3.value, &value_int16, &UA_TYPES[UA_TYPES_INT16]);
					statusAttr3.displayName = UA_LOCALIZEDTEXT("en-US", (char*)id_tag.c_str());
					UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceId,
						UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
						UA_QUALIFIEDNAME(1, (char*)id_tag.c_str()),
						UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr3, NULL, &tagNodeId);
				}
				else if (controller->vectorNode[i].vectorDevice[j].vectorTag[k].enum_data_type == Data_type::uint16)
				{
					value_uint16 = 0;
					UA_Variant_setScalar(&statusAttr3.value, &value_uint16, &UA_TYPES[UA_TYPES_UINT16]);
					statusAttr3.displayName = UA_LOCALIZEDTEXT("en-US", (char*)id_tag.c_str());
					UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceId,
						UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
						UA_QUALIFIEDNAME(1, (char*)id_tag.c_str()),
						UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr3, NULL, &tagNodeId);
				}
				else if (controller->vectorNode[i].vectorDevice[j].vectorTag[k].enum_data_type == Data_type::int32)
				{
					value_int32 = 0;
					UA_Variant_setScalar(&statusAttr3.value, &value_int32, &UA_TYPES[UA_TYPES_INT32]);
					statusAttr3.displayName = UA_LOCALIZEDTEXT("en-US", (char*)id_tag.c_str());
					UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceId,
						UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
						UA_QUALIFIEDNAME(1, (char*)id_tag.c_str()),
						UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr3, NULL, &tagNodeId);
				}
				else if (controller->vectorNode[i].vectorDevice[j].vectorTag[k].enum_data_type == Data_type::uint32)
				{
					value_uint32 = 0;
					UA_Variant_setScalar(&statusAttr3.value, &value_uint32, &UA_TYPES[UA_TYPES_UINT32]);
					statusAttr3.displayName = UA_LOCALIZEDTEXT("en-US", (char*)id_tag.c_str());
					UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceId,
						UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
						UA_QUALIFIEDNAME(1, (char*)id_tag.c_str()),
						UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr3, NULL, &tagNodeId);
				}

				if ( (controller->vectorNode[i].vectorDevice[j].vectorTag[k].enum_data_type == Data_type::float_BE) ||
					(controller->vectorNode[i].vectorDevice[j].vectorTag[k].enum_data_type == Data_type::float_BE_swap) ||
					(controller->vectorNode[i].vectorDevice[j].vectorTag[k].enum_data_type == Data_type::float_LE) ||
					(controller->vectorNode[i].vectorDevice[j].vectorTag[k].enum_data_type == Data_type::float_LE_swap) )
				{
					value_float = 0;
					UA_Variant_setScalar(&statusAttr3.value, &value_float, &UA_TYPES[UA_TYPES_FLOAT]);
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
	
	free(controller);
	
	pthread_exit(0);
}








void* pollingEngine(void *args)
{
	Controller* controller = (Controller*)args;

	std::vector<pthread_t> modbus_thread;
	

	//Узел (Node/Coms)
	for (int i = 0; i < controller->vectorNode.size(); i++)
	{
		//printf("%s\n", controller->vectorNode[i].name.c_str());

		
		//Создание сокета и подключение к устройству
		if (controller->vectorNode[i].enum_interface_type == Interface_type::tcp)
		{
			connectDeviceTCP(&controller->vectorNode[i]);


			//Устройство (Device)
			for (int j = 0; j < controller->vectorNode[i].vectorDevice.size(); j++)
			{
				//printf("    %s\n", controller->vectorNode[i].vectorDevice[j].name.c_str());

				controller->vectorNode[i].vectorDevice[j].id_device = j;
				controller->vectorNode[i].vectorDevice[j].device_socket = controller->vectorNode[i].socket;
			}

			//Запускаем опрос по TCP
			if (controller->vectorNode[i].on == 1)
			{
				pthread_create(&modbus_thread[i], NULL, pollingDeviceTCP, &controller->vectorNode[i]);
			}
		}

		//Подключение к устройству RS-485
		if (controller->vectorNode[i].enum_interface_type == Interface_type::rs485)
		{
			//connectDeviceRS485(&controller->vectorNode[i]);


			//Запускаем опрос по RS-485
			if (controller->vectorNode[i].on == 1)
			{				
				pthread_t thr;
				
				modbus_thread.push_back(thr);

				pthread_create(&thr, NULL, pollingDeviceRS485, &controller->vectorNode[i]);					
				
			}			

		}
		
	}	


	
	for (int i = 0; i < modbus_thread.size(); i++)
	{
		pthread_join((pthread_t)&modbus_thread[i], NULL);
		pthread_detach((pthread_t)&modbus_thread[i]);
	}
	
	
	modbus_thread.clear();
	
	return 0;
}


Data_type type_converter(const std::string &str)
{
	if (str == "int16") return Data_type::int16;	
	else if (str == "uint16") return Data_type::uint16;
	else if (str == "int32") return Data_type::int32;
	else if (str == "uint32") return Data_type::uint32;
	else if (str == "float_BE") return Data_type::float_BE;
	else if (str == "float_BE_swap") return Data_type::float_BE_swap;
	else if (str == "float_LE") return Data_type::float_LE;
	else if (str == "float_LE_swap") return Data_type::float_LE_swap;
};

Interface_type interface_converter(const std::string &str)
{
	if (str == "TCP") return Interface_type::tcp;
	else if (str == "RS-485") return Interface_type::rs485;
};


void sig_handler(int signum)
{
	printf("\nReceived signal %d. \n", signum);

	
	if (signum == SIGTERM || signum == SIGSTOP || signum == SIGINT || signum == SIGQUIT || signum == SIGTSTP)
	{		
		//pthread_exit(&server_thread);
		exit(0);
		//kill(main_pid, SIGSTOP);
	};

	if (signum == SIGSEGV) //Segmentation fault
	{		
		write_text_to_log_file("Segmentation fault");
	};

};



int main(int argc, char** argv)
{

	pthread_t server_thread;

	int status;
	pid_t main_pid;
	pid_t th1_pid;

	
	



	signal(SIGINT, sig_handler);

	main_pid = getpid();

	char* path_to_json;

	if (argc > 1)
	{
		path_to_json = argv[1];
	}
	else
	{
		path_to_json = "/usr/httpserv/opc.json";
	};


	printf("Start OPC server...\n");
	printf("Path to json: %s\n", path_to_json);
		
	write_text_to_log_file("Start OPC server...\n");

		
	Controller controller;
	
	controller = serializeFromJSON(path_to_json);
	
	pthread_create(&server_thread, NULL, workerOPC, &controller); //Запуск OPC сервера 		
	sleep(1);


	pollingEngine(&controller);	//Запуск опроса	(MODBUS)

	pthread_join(server_thread, (void**)&status);
	pthread_detach(server_thread);


	return 0;
}