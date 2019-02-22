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
	
	std::vector<Node*> vectorNode;
};

//Класс узел
class Node : public Controller
{
public:

	Node()
	{
		name = "Node";		
	}

	std::string com_port;
	int baud_rate = 0;
	int word_lenght = 0;
	int parity = 0;
	int stop_bit = 0;		

	std::vector<Device*> vectorDevice;
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
	int poll_priod = 0;
	int poll_timeout = 0;

	std::vector<Tag*> vectorTag;
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
class iFactory
{
public:

	virtual Controller* createController() = 0;
	virtual Node* createNode() = 0;
	virtual Device* createDevice() = 0;
	virtual Tag* createTag() = 0;
	virtual ~iFactory() {}
};

class Factory : public iFactory
{
public:
	Controller* createController()
	{
		return new Controller;
	}

	Node* createNode()
	{
		return new Node;
	}

	Device* createDevice()
	{
		return new Device;
	}

	Tag* createTag()
	{
		return new Tag;
	}
};


int main()
{
	//std::vector<Controller*> vs;

	//Factory nodefactory;

	//vs.push_back(nodefactory.createController());
	//vs.push_back(nodefactory.createTag());
	//vs.push_back(nodefactory.createNode());

	//printf("%s\n", vs[0]->name.c_str());
	//
	//printf("%s\n", vs[1]->name.c_str());

	Controller controller;
	Node node;
	Device device;
	Tag tag;
	
	controller.vectorNode.push_back( &node );
	node.vectorDevice.push_back( &device );

	

	printf("%s\n", controller.name.c_str());
	printf("%s\n", controller.vectorNode[0]->name.c_str());
	
	printf("%s\n", node.name.c_str());
	printf("%s\n", node.vectorDevice[0]->name.c_str());

	
	//char readBuffer[65536];

	//FILE* fp = fopen("opc_json.json", "r");	
	//FileReadStream is(fp, readBuffer, sizeof(readBuffer));

	//Document d;
	//d.ParseStream(is);
	//fclose(fp);

//////////

	FILE* fp = fopen("/root/projects/OPC_Server/opc_json.json", "r"); 

	char readBuffer[90000];
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));

	Document d;
	d.ParseStream(is);
	fclose(fp);



	const Value& name = d["name"];
	


	//for (Value::ConstMemberIterator itr = department.MemberBegin(); itr != department.MemberEnd(); ++itr)
	//{
	//	std::cout << itr->name.GetString() << " : ";
	//	std::cout << itr->value.GetInt() << "\r\n";
	//}

	
	printf("%s\n", name.GetString() );

	sleep(5);

	return 0;
}