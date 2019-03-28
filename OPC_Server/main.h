
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

#include "open62541.h"





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
	std::vector<Node> vectorNode;

private:
	std::vector<Tag> vectorControllerTag;
	
};

//Класс узел
class Node : public Controller
{

public:	
	std::vector<Device> vectorDevice;

	int on = 0;
	int baud_rate = 9600;
	int word_lenght = 8;
	int parity = 0;
	int stop_bit = 1;

	std::string intertype;
	std::string address; //IP or RS485 device
	int port = 8080;
	int tcp_wait_connection = 5;

	int socket = 0;
	int poll_period = 1000;
	
	

private:
	std::vector<Tag> vectorNodeTag;

};


//Класс устройство
class Device : public Node
{

public:
	std::vector<Tag> vectorTag;

	int device_address = 0;	
	int poll_timeout = 1000;

	int devtype = 0;
	int id_device = 0;
	int device_socket = 0;

private:
	std::vector<Tag> vectorDeviceTag;

};


//Класс тэг
class Tag : public Device
{

public:
	int reg_address = 0;
	int function = 0;
	std::string data_type;
	float coef_A = 1;
	float coef_B = 0;
	float value = 0;
	
	UA_NodeId tagNodeId;
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

//Структура для хранения оптимизированных запросов каждого из устройств
struct Optimize
{
	int device_addr;
	uint8_t request[12];
	uint8_t response[255];
};


#endif