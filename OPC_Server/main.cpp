#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <string> 
#include <iostream>
#include <vector>
#include <unistd.h>

#include "open62541.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include<netdb.h>

#include "main.h"



using namespace rapidjson;


UA_Server *server;
pthread_t server_thread;
pthread_t modbus_thread;







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




void* pollingDevice(void *args)
{
	int result = 0;
	char write_buffer[] = { 00, 00, 00, 00, 00, 06, 01, 03, 00, 00, 00, 01 };		
	char read_buffer[255];
	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	const char* address = "192.168.0.146";
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	
	Controller* thread_controller = (Controller*) args;

	if (thread_controller != NULL)
	{
		

		if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
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
				

		serv_addr.sin_addr.s_addr = inet_addr(address);		
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(8080);
				
		result = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

		if (result < 0)
		{
			printf("\nConnection Failed: %d\n", result);			
		}

		
		
		while(1)
		{ 
			result = send(sock, &write_buffer, sizeof(write_buffer), 0);		
			result = read(sock, &read_buffer, sizeof(read_buffer));

			thread_controller->vectorNode[1].vectorDevice[1].vectorTag[0].value = float((read_buffer[9]<<8)  + read_buffer[10]);

			UA_Variant value;
			UA_Int32 myInteger = (UA_Int32) int(thread_controller->vectorNode[1].vectorDevice[1].vectorTag[0].value);
			UA_Variant_setScalarCopy(&value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
			UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 1), value);


			//for (int i = 0; i < result; i++)
			//{
			//	printf("%02x ", read_buffer[i]);				
			//}
			printf("%.00f\n", thread_controller->vectorNode[1].vectorDevice[1].vectorTag[0].value);
			//printf("%d", thread_controller->vectorNode[1].vectorDevice[1].vectorTag[1].attribute);
			//printf("\n");

			
			

			sleep(1);
		}

	}

}



void* poll(void *args)
{

	Controller* controller = (Controller*)args;

	
	//Узел (Node/Coms)
	for (int i = 0; i < controller->vectorNode.size(); i++)
	{
		printf("%s\n", controller->vectorNode[i].name.c_str());

		pthread_t modbus_thread[controller->vectorNode[i].vectorDevice.size()];

		//Устройство (Device)
		for (int j = 0; j < controller->vectorNode[i].vectorDevice.size(); j++)
		{
			printf("    %s\n", controller->vectorNode[i].vectorDevice[j].name.c_str());

			//Запуск опроса устройства
			if (controller->vectorNode[i].vectorDevice[j].vectorTag.size() > 0)
			{				
				pthread_create(&modbus_thread[j], NULL, pollingDevice, controller);
			}


		}
	}

	//pthread_create(&modbus_thread[0], NULL, pollingDevice, &controller);
}


Controller serializeFromJSON(char* path)
{
	
	Controller contr;
	Node node;
	Device device;
	Tag tags;	
	Tag nodeTags;
	Tag controllerTags;

	char readBuffer[65536];

	FILE* fp = fopen(path, "r");
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));

	Document doc;
	doc.ParseStream(is);
	fclose(fp);

	printf("Parsing json complete.\n\n");

	const Value& Coms = doc["Coms"];
	const Value& Tags = doc["Tags"];


	if (doc.IsObject() == true)
	{

		contr.name = doc["name"].GetString();
		contr.type = doc["type"].GetUint();
		contr.description = doc["comment"].GetString();
		contr.attribute = doc["attribute"].GetUint();

		printf("%s:\n", doc["name"].GetString());

		if (Coms.Size() > 0)
		{			

			contr.vectorNode.clear();

			for (SizeType i = 0; i < Coms.Size(); i++)
			{
				//printf("    Coms: %s\n", Coms[i]["name"].GetString());				

				node.name = Coms[i]["name"].GetString();
				node.type = Coms[i]["type"].GetUint();
				node.intertype = Coms[i]["intertype"].GetString();
				node.port = Coms[i]["port"].GetString();
				node.address = Coms[i]["address"].GetString();
				node.description = Coms[i]["comment"].GetString();
				node.attribute = Coms[i]["attribute"].GetUint();



				if (Coms[i]["Devs"].Size() > 0)
				{									
					node.vectorDevice.clear();

					for (SizeType j = 0; j < Coms[i]["Devs"].Size(); j++)
					{						
						//printf("         Devs: %s\n", Coms[i]["Devs"][j]["name"].GetString());						

						device.name = Coms[i]["Devs"][j]["name"].GetString();
						device.type = Coms[i]["Devs"][j]["type"].GetUint();
						device.devtype = Coms[i]["Devs"][j]["devtype"].GetUint();
						device.address = Coms[i]["Devs"][j]["address"].GetString();
						device.description = Coms[i]["Devs"][j]["comment"].GetString();
						device.attribute = Coms[i]["Devs"][j]["attribute"].GetUint();


						if (Coms[i]["Devs"][j]["Tags"].Size() > 0)
						{					
							device.vectorTag.clear();

							for (SizeType k = 0; k < Coms[i]["Devs"][j]["Tags"].Size(); k++)
							{
								//printf("            Tag: %s\n", Coms[i]["Devs"][j]["Tags"][k]["name"].GetString());

								tags.name = Coms[i]["Devs"][j]["Tags"][k]["name"].GetString();
								tags.type = Coms[i]["Devs"][j]["Tags"][k]["type"].GetUint();
								tags.tagtype = Coms[i]["Devs"][j]["Tags"][k]["type"].GetUint();
								tags.string = Coms[i]["Devs"][j]["Tags"][k]["string"].GetString();
								tags.address = Coms[i]["Devs"][j]["Tags"][k]["address"].GetString();
								tags.description = Coms[i]["Devs"][j]["Tags"][k]["comment"].GetString();
								tags.attribute = Coms[i]["Devs"][j]["Tags"][k]["attribute"].GetUint();

								device.vectorTag.push_back(tags);
							}							
						}
						else device.vectorTag.clear();
						
						node.vectorDevice.push_back(device);
					}					
				}
				else node.vectorDevice.clear();
				

				


				if (Coms[i]["Tags"].Size() > 0)
				{

					for (SizeType j = 0; j < Coms[i]["Tags"].Size(); j++)
					{
						//printf("Tags: %s\n", Tags[i]["Tags"][j]["name"].GetString());

						nodeTags.name = Tags[i]["Tags"][j]["name"].GetString();
						nodeTags.type = Tags[i]["Tags"][j]["type"].GetUint();
						nodeTags.tagtype = Tags[i]["Tags"][j]["tagtype"].GetString();
						nodeTags.string = Tags[i]["Tags"][j]["string"].GetString();
						nodeTags.address = Tags[i]["Tags"][j]["address"].GetString();
						nodeTags.description = Tags[i]["Tags"][j]["comment"].GetString();
						nodeTags.attribute = Tags[i]["Tags"][j]["attribute"].GetUint();

						node.vectorNodeTag.push_back(nodeTags);
					}
				}
				else node.vectorNodeTag.clear();

				contr.vectorNode.push_back(node);

			} //ForLoop Coms (Node) end
		} //If Coms(Node) end


		if (Tags.Size() > 0)
		{			

			for (SizeType i = 0; i < Tags.Size(); i++)
			{
				//printf("Tags: %s\n", Tags[i]["name"].GetString());

				controllerTags.name = Tags[i]["name"].GetString();
				controllerTags.type = Tags[i]["type"].GetUint();
				controllerTags.tagtype = Tags[i]["tagtype"].GetString();
				controllerTags.string = Tags[i]["string"].GetString();
				controllerTags.address = Tags[i]["address"].GetString();
				controllerTags.description = Tags[i]["comment"].GetString();
				controllerTags.attribute = Tags[i]["attribute"].GetUint();				
			}

			contr.vectorControllerTag.push_back(controllerTags);
		}
		else contr.vectorControllerTag.clear();

	} //Server/Controller end

	return contr;
}


int main()
{
	int status;

	printf("Start OPC server...\n\n");
	
	
	Controller controller;


	controller = serializeFromJSON("/root/projects/OPC_Server/opc.json");

	

	pthread_create(&server_thread, NULL, workerOPC, NULL); //Запуск OPC сервера 
	
	
	poll(&controller);
	//pthread_create(&modbus_thread, NULL, pollingDevice, &controller); //Запуск опроса устройства
	
	pthread_join(server_thread, (void**)&status);
	pthread_join(modbus_thread, (void**)&status);
	

	sleep(15);

	return 0;
}