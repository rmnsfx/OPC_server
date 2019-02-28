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
#include <iostream>
#include "rapidjson/filereadstream.h"

using namespace rapidjson;


class Controller;
class Node;
class Device;
class Tag;

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


/////////////////////Абстрактная фабрика для производства сервера, узлов, устройств, тэгов
//class iFactory
//{
//public:
//
//	virtual Controller* createController() = 0;
//	virtual Node* createNode() = 0;
//	virtual Device* createDevice() = 0;
//	virtual Tag* createTag() = 0;
//	virtual ~iFactory() {}
//};
//
//class Factory : public iFactory
//{
//public:
//	Controller* createController()
//	{
//		return new Controller;
//	}
//
//	Node* createNode()
//	{
//		return new Node;
//	}
//
//	Device* createDevice()
//	{
//		return new Device;
//	}
//
//	Tag* createTag()
//	{
//		return new Tag;
//	}
//};


int main()
{

	char readBuffer[65536];
	
	FILE* fp = fopen("/root/projects/OPC_Server/opc_json.json", "r"); 	
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	fclose(fp);

	Document doc;
	doc.ParseStream(is);

	const Value& Coms = doc["Coms"];
	const Value& Tags = doc["Tags"];
		
		
	
	if (doc.IsObject() == true)
	{

		Controller controller;		
	
		controller.name = doc["name"].GetString();
		controller.type = doc["type"].GetUint();
		controller.description = doc["comment"].GetString();

		if (Coms.Size() > 0)
		{
			Node node;			

			for (SizeType i = 0; i < Coms.Size(); i++)
			{
				//printf("Coms %s\n", Coms[i]["name"].GetString());
								
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
						//printf("%s\n", Coms[i]["Devs"][j]["name"].GetString());					

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
								//printf("%s\n", Coms[i]["Devs"][j]["Tags"][k]["name"].GetString());					

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



	
	//for (Value::ConstMemberIterator itr = department.MemberBegin(); itr != department.MemberEnd(); ++itr)
	//{
	//	std::cout << itr->name.GetString() << " : ";
	//	std::cout << itr->value.GetInt() << "\r\n";
	//	printf("%s\n", itr->name.GetString());
	//}

	//for (Value::ConstValueIterator itr = Coms.Begin(); itr != Coms.End(); ++itr)
	//	printf("%s\n", itr->GetString());
	
	

	sleep(15);

	return 0;
}