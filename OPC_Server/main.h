
#ifndef MAIN_H
#define MAIN_H

#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <string> 
#include <iostream>
#include <vector>
#include <unistd.h>

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
	int attribute;

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
	//std::string address;
	//std::string port;

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
	uint port = 0;

	int devtype = 0;
	std::string ip;

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

#endif