#pragma once

#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <string> 
#include <iostream>
#include <vector>
#include <unistd.h>
#include <stdint.h>
#include "open62541.h"


#define GTEST_DEBUG 0 //Работа в режиме модульного тестирования
#define LOG 0 

class Controller;
class Node;
class Device;
class Tag;


//Абстрактный базовый класс
class iServerTree
{
public:

	virtual ~iServerTree() {};
	
	int16_t type = 0;
	std::string name = "";
	std::string label = "";
	std::string description = "";
	int16_t attribute = 0;

};

//Класс контроллер
class Controller : virtual public iServerTree
{

public:
	std::vector<Node> vectorNode;
	

private:
	std::vector<Tag> vectorControllerTag;
	
};

enum class Interface_type { tcp, rs485 };

//Класс узел (порт)
class Node : public Controller
{

public:	
	std::vector<Device> vectorDevice;

	int16_t on = 0;
	int16_t baud_rate = 9600;
	int16_t word_lenght = 8;
	int16_t parity = 0;
	int16_t stop_bit = 1;
		
	Interface_type enum_interface_type;
	std::string address; //IP or RS485 device
	int16_t port = 8080;
	int16_t tcp_wait_connection = 5;
	int16_t socket = 0;	
	int16_t f_id = 0; //Дескриптор для RS-485

	int16_t poll_period = 0;

private:
	std::vector<Tag> vectorNodeTag;
	

};


//Класс устройство
class Device : public Node
{

public:
	std::vector<Tag> vectorTag;

	int16_t device_address = 0;
	int16_t poll_timeout = 1000;

	int16_t devtype = 0;
	int16_t id_device = 0;
	int16_t device_socket = 0;

private:
	std::vector<Tag> vectorDeviceTag;

};

//float_BE = point big endian(4, 3, 2, 1)			
//float_BE_swap = point big endian whit byte swapped(3, 4, 1, 2) 	
//float_LE = point little endian(1, 2, 3, 4)			
//float_LE_swap = point little endian whit byte - swapped(2, 1, 4, 3)

enum class Data_type { int16, uint16, int32, uint32, float_BE, float_BE_swap, float_LE, float_LE_swap, sample };

//Класс тэг
class Tag : public Device
{

public:
	int16_t reg_address = 0;
	int16_t function = 0;
	Data_type enum_data_type;
	float coef_A = 1;
	float coef_B = 0;
	float value = 0;
	
	UA_NodeId tagNodeId;
	int16_t reg_position;
	
};

//Класс группа
class Group : virtual public iServerTree
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
	int16_t device_addr;
	std::vector<int16_t> holding_regs; //список адресов регистров
	std::vector<int16_t> input_regs; //список адресов регистров		
	std::vector<std::vector<uint8_t>> request; //список запросов
	std::vector<std::vector<uint8_t>> response; //список ответов	
	//std::vector<std::vector<uint8_t>> holding_request; //список запросов
	//std::vector<std::vector<uint8_t>> input_request; //список запросов	
	//std::vector<std::vector<uint8_t>> holding_response; //список ответов	
	//std::vector<std::vector<uint8_t>> input_response; //список ответов	
};


Data_type type_converter(const std::string &str);
Interface_type interface_converter(const std::string &str);
UA_Server* getServer(void);
UA_HistoryDataBackend getBackend(void);

void* workerOPC(void *args);
void* pollingEngine(void *args);

