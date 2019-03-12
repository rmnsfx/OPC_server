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

using namespace rapidjson;


class Controller;
class Node;
class Device;
class Tag;

char readBuffer[65536];

//Абстрактный базовый класс
class iServerTree
{
public:

	virtual ~iServerTree() {};
		
	int type;
	std::string name;
	std::string label;
	std::string description;
		
};

//Класс контроллер
class Controller : public iServerTree
{
public:
	
	Controller()
	{
		name = "Controller";		
	}	
	
	std::vector<Tag> vectorControllerTag;
	std::vector<Node> vectorNode;	
};

//Класс узел
class Node : public Controller
{
public:

	Node()
	{
		name = "Node";		
	}
		
	int baud_rate = 0;
	int word_lenght = 0;
	int parity = 0;
	int stop_bit = 0;		

	std::string intertype;
	std::string address;
	std::string port;

	std::vector<Tag> vectorNodeTag;
	std::vector<Device> vectorDevice;	
};


//Класс устройство
class Device : public Node
{
public:

	Device()
	{
		name = "Device";
	}

	int modbus_address = 0;
	int poll_period = 0;
	int poll_timeout = 0;
	
	int devtype = 0;	
	std::string address;

	std::vector<Tag> vectorDeviceTag;
	std::vector<Tag> vectorTag;	
};


//Класс тэг
class Tag : public Device
{
public:

	Tag()
	{
		name = "Tag";
	}

	int on = 0;
	int function = 0;
	int type = 0;
	double coef_A = 0;
	double coef_B = 0;
	double value = 0;

	std::string tagtype;
	std::string string;
	std::string address;
};

//Класс группа
class Group : public iServerTree
{
public:

	Group()
	{
		name = "Group";		
	}
};



void pollingDevice(Controller* controller)
{
	int result = 0;
	char buffer[] = { 00, 00, 00, 00, 00, 06, 01, 03, 00, 00, 00, 01 };
	
	//struct sockaddr_in address;
	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	const char* address = "192.168.0.146";
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	

	if (controller != NULL)
	{
		

		if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		{
			printf("\n Socket creation error \n");
			//return -1;
		}
		else
		{
			printf("\n Socket create %d\n", sock);
		}

		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		{
			printf("\n setsockopt failed \n");
		}		

		if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		{
			printf("\n setsockopt failed \n");
		}



		memset(&serv_addr, 0, sizeof(serv_addr));
		
		bzero((char *)&serv_addr, sizeof(serv_addr));

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
			result = send(sock, buffer, sizeof(buffer), 0);		
			sleep(2);
		}
		
		//result = write(sock, buffer, 11);
		printf("Hello message sent %d\n", result);


		

		//printf("\n...Start polling\n\n");
	}

}




int main()
{

	printf("Start OPC server...\n\n");
	
	
	FILE* fp = fopen("/root/projects/OPC_Server/opc.json", "r"); 	
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	
	Document doc;
	doc.ParseStream(is);
	fclose(fp);

	printf("Parsing json complete.\n\n");



	const Value& Coms = doc["Coms"];
	const Value& Tags = doc["Tags"];		
		
	
	if (doc.IsObject() == true)
	{

		Controller controller;		
	
		controller.name = doc["name"].GetString();
		controller.type = doc["type"].GetUint();
		controller.description = doc["comment"].GetString();

		printf("%s:\n", doc["name"].GetString());

		if (Coms.Size() > 0)
		{
			Node node;			

			for (SizeType i = 0; i < Coms.Size(); i++)
			{
				printf("    Coms: %s\n", Coms[i]["name"].GetString());
								
				node.name = Coms[i]["name"].GetString();
				node.type = Coms[i]["type"].GetUint();
				node.intertype = Coms[i]["intertype"].GetString();
				node.port = Coms[i]["port"].GetString();
				node.address = Coms[i]["address"].GetString();
				node.description = Coms[i]["comment"].GetString();

				

				if (Coms[i]["Devs"].Size() > 0)
				{
					Device device;

					for (SizeType j = 0; j < Coms[i]["Devs"].Size(); j++)
					{
						printf("         Devs: %s\n", Coms[i]["Devs"][j]["name"].GetString());					

						device.name = Coms[i]["Devs"][j]["name"].GetString();
						device.type = Coms[i]["Devs"][j]["type"].GetUint();
						device.devtype = Coms[i]["Devs"][j]["devtype"].GetUint();
						device.address = Coms[i]["Devs"][j]["address"].GetString();
						device.description = Coms[i]["Devs"][j]["comment"].GetString();



						if (Coms[i]["Devs"][j]["Tags"].Size() > 0)
						{
							Tag tags;

							for (SizeType k = 0; k < Coms[i]["Devs"][j]["Tags"].Size(); k++)
							{
								printf("            Tag: %s\n", Coms[i]["Devs"][j]["Tags"][k]["name"].GetString());					

								tags.name = Coms[i]["Devs"][j]["Tags"][k]["name"].GetString();
								tags.type = Coms[i]["Devs"][j]["Tags"][k]["type"].GetUint();
								tags.tagtype = Coms[i]["Devs"][j]["Tags"][k]["type"].GetUint();
								tags.string = Coms[i]["Devs"][j]["Tags"][k]["string"].GetString();
								tags.address = Coms[i]["Devs"][j]["Tags"][k]["address"].GetString();
								tags.description = Coms[i]["Devs"][j]["Tags"][k]["comment"].GetString();

								device.vectorTag.push_back(tags);
							}
						}

						node.vectorDevice.push_back(device);
					}
				}
				
				controller.vectorNode.push_back(node);



				if (Coms[i]["Tags"].Size() > 0)
				{
					Tag nodeTags;

					for (SizeType j = 0; j < Coms[i]["Tags"].Size(); j++)
					{
						printf("Tags: %s\n", Tags[i]["Tags"][j]["name"].GetString());

						nodeTags.name = Tags[i]["Tags"][j]["name"].GetString();
						nodeTags.type = Tags[i]["Tags"][j]["type"].GetUint();
						nodeTags.tagtype = Tags[i]["Tags"][j]["tagtype"].GetString();
						nodeTags.string = Tags[i]["Tags"][j]["string"].GetString();
						nodeTags.address = Tags[i]["Tags"][j]["address"].GetString();
						nodeTags.description = Tags[i]["Tags"][j]["comment"].GetString();

						node.vectorNodeTag.push_back(nodeTags);
					}
				}


			} //For Coms/Node end

			
			pollingDevice(&controller); //Запуск опроса устройства
			

		} //Coms/Node end


		if (Tags.Size() > 0)
		{
			Tag controllerTags;

			for (SizeType i = 0; i < Tags.Size(); i++)
			{
				//printf("Tags: %s\n", Tags[i]["name"].GetString());

				controllerTags.name = Tags[i]["name"].GetString();
				controllerTags.type = Tags[i]["type"].GetUint();
				controllerTags.tagtype = Tags[i]["tagtype"].GetString();
				controllerTags.string = Tags[i]["string"].GetString();
				controllerTags.address = Tags[i]["address"].GetString();
				controllerTags.description = Tags[i]["comment"].GetString();

				controller.vectorControllerTag.push_back(controllerTags);
			}		
		} 
		
	} //Server/Controller end



	

	

	sleep(15);

	return 0;
}