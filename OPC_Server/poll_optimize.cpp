#include "main.h"

#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <string> 
#include <iostream>
#include <vector>
#include <unistd.h>
#include <map>
#include <mutex>

#define _BSD_SOURCE //for <sys/time.h>

#include <time.h>
#include <sys/time.h>
#include <bits/stdc++.h> 



std::vector<std::vector<int>> splitRegs(std::vector<int>& regs)
{	
	std::vector<int> split;
	std::vector <std::vector<int>> split_out;

	if (regs.size() > 1)
	{
		sort(regs.begin(), regs.end());

		if ((regs.back() - regs.front()) < 125)
		{
			if (regs.front() == 0) split.push_back(regs.front());
			else split.push_back(regs.front() - 1);

			if (regs.back() == 0) split.push_back(regs.back());
			else split.push_back(regs.back() - 1);

			split_out.push_back(split);
		}
		else
		{ 			
			
			for (int i = 0; i < regs.size() - 1; i++)
			{
				//Проверка на количество регистров в запросе (ограничение протокола modbus)
				if ((regs[i + 1] - regs[i]) < 125)
				{
					split.push_back(regs[i] - 1);
					split.push_back(regs[i + 1] - 1);
					split_out.push_back(split);
					split.clear();
				}
				else
				{
					if (regs.size() == 2)
					{
						split.push_back(regs[i] - 1);
						split_out.push_back(split);
						split.clear();
						split.push_back(regs[i + 1] - 1);
					}
					else
					{
						split.push_back(regs[i + 1] - 1);
					}

					split_out.push_back(split);
					split.clear();
				}
			}				

		}
	}
	else if (regs.size() == 1)
	{

		split.push_back(regs[0] - 1);
		split_out.push_back(split);

	}

	return split_out;
};


std::vector<Optimize> reorganizeNodeIntoPolls(Node* node)
{
	
	std::vector<Optimize> vector_optimize; //Для формирования пакета запроса
	std::vector<int> regs;	//Для сортировки адресов
	int reg_qty = 0;
	std::vector<uint8_t> message;	
	bool holding_present = false;
	bool input_present = false;
	std::vector <std::vector<int>> pair_request;



	//Проходим по устройствам
	for (int i = 0; i < node->vectorDevice.size(); i++)
	{
		if (node->vectorDevice[i].on == 1)
		{
			Optimize optimize;
			holding_present = false;
			input_present = false;



			//Определяем присутствие функции в тэгах устройства
			for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
			{
				if (node->vectorDevice[i].vectorTag[j].on == 1)
				{
					if (node->vectorDevice[i].vectorTag[j].function == 3) holding_present = true;
					if (node->vectorDevice[i].vectorTag[j].function == 4) input_present = true;
				}
			}


			if (holding_present == true)
			{
				//Проходим по тэгам устройства 
				for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
				{
					if (node->vectorDevice[i].vectorTag[j].on == 1 && node->vectorDevice[i].vectorTag[j].function == 3)
					{
						if (node->vectorDevice[i].vectorTag[j].reg_address != 0)
						{
							regs.push_back(node->vectorDevice[i].vectorTag[j].reg_address);
							node->vectorDevice[i].vectorTag[j].reg_position = j; //Запоминаем номер позиции регистра
						}
						else printf("Warning! %s Register address == 0 \n", node->vectorDevice[i].vectorTag[j].name);
					}
				}

				//Сортируем и разбиваем адреса (общий список) на подзапросы
				pair_request = splitRegs(regs);

				for (int z = 0; z < pair_request.size(); z++)
				{				

					if ((pair_request[z].back() - pair_request[z].front()) > 0)
					{
						//Формируем пакет для запроса
						message.push_back(0x00);														//Идентификатор транзакции
						message.push_back(0x00);
						message.push_back(0x00);														//Идентификатор протокола
						message.push_back(0x00);
						message.push_back(0x00);														//Длина сообщения
						message.push_back(0x06);
						message.push_back(node->vectorDevice[i].device_address);						//Адрес устройства
						message.push_back(0x03);														//Функциональный код 
						message.push_back(pair_request[z].front() >> 8);								//Адрес первого регистра Hi байт
						message.push_back(pair_request[z].front());										//Адрес первого регистра Lo байт			

						reg_qty = pair_request[z].back() - pair_request[z].front() + 1;

						message.push_back(reg_qty >> 8);												//Количество регистров Hi байт
						message.push_back(reg_qty);														//Количество регистров Lo байт

						optimize.device_addr = node->vectorDevice[i].device_address;
						optimize.request.push_back(message);
						
						for (int g = 0; g < regs.size(); g++) optimize.holding_regs.push_back(regs[g]);	//Копирование вектора

					}
					else
					{
						//Формируем пакет для запроса
						message.push_back(0x00);														//Идентификатор транзакции
						message.push_back(0x00);
						message.push_back(0x00);														//Идентификатор протокола
						message.push_back(0x00);
						message.push_back(0x00);														//Длина сообщения
						message.push_back(0x06);
						message.push_back(node->vectorDevice[i].device_address);						//Адрес устройства
						message.push_back(0x03);														//Функциональный код 
						message.push_back(pair_request[z].front() >> 8);								//Адрес первого регистра Hi байт
						message.push_back(pair_request[z].front());										//Адрес первого регистра Lo байт			

						reg_qty = 1;

						message.push_back(reg_qty >> 8);												//Количество регистров Hi байт
						message.push_back(reg_qty);														//Количество регистров Lo байт


						optimize.device_addr = node->vectorDevice[i].device_address;
						optimize.request.push_back(message);

						for (int g = 0; g < regs.size(); g++) optimize.holding_regs.push_back(regs[g]);	//Копирование вектора

					}

					regs.clear();
					message.clear();

				}
			}


			if (input_present == true)
			{
				//Проходим по тэгам устройства 
				for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
				{
					if (node->vectorDevice[i].vectorTag[j].on == 1 && node->vectorDevice[i].vectorTag[j].function == 4)
					{
						if (node->vectorDevice[i].vectorTag[j].reg_address != 0)
						{
							regs.push_back(node->vectorDevice[i].vectorTag[j].reg_address);
							node->vectorDevice[i].vectorTag[j].reg_position = j; //Запоминаем номер позиции регистра
						}
						else printf("Warning! %s Register address == 0 \n", node->vectorDevice[i].vectorTag[j].name);
					}
				}

				//Сортируем и разбиваем адреса (общий список) на подзапросы
				pair_request = splitRegs(regs);

				for (int z = 0; z < pair_request.size(); z++)
				{
					if ((pair_request[z].back() - pair_request[z].front()) > 0)
					{
						//Формируем пакет для запроса
						message.push_back(0x00);														//Идентификатор транзакции
						message.push_back(0x00);
						message.push_back(0x00);														//Идентификатор протокола
						message.push_back(0x00);
						message.push_back(0x00);														//Длина сообщения
						message.push_back(0x06);
						message.push_back(node->vectorDevice[i].device_address);						//Адрес устройства
						message.push_back(0x04);														//Функциональный код 
						message.push_back(pair_request[z].front() >> 8);								//Адрес первого регистра Hi байт
						message.push_back(pair_request[z].front());										//Адрес первого регистра Lo байт			

						reg_qty = pair_request[z].back() - pair_request[z].front() + 1;

						message.push_back(reg_qty >> 8);												//Количество регистров Hi байт
						message.push_back(reg_qty);														//Количество регистров Lo байт


						optimize.device_addr = node->vectorDevice[i].device_address;
						optimize.request.push_back(message);

						for (int g = 0; g < regs.size(); g++) optimize.input_regs.push_back(regs[g]);	//Копирование вектора
					}
					else
					{
						//Формируем пакет для запроса
						message.push_back(0x00);														//Идентификатор транзакции
						message.push_back(0x00);
						message.push_back(0x00);														//Идентификатор протокола
						message.push_back(0x00);
						message.push_back(0x00);														//Длина сообщения
						message.push_back(0x06);
						message.push_back(node->vectorDevice[i].device_address);						//Адрес устройства
						message.push_back(0x04);														//Функциональный код 
						message.push_back(pair_request[z].front() >> 8);								//Адрес первого регистра Hi байт
						message.push_back(pair_request[z].front());										//Адрес первого регистра Lo байт			

						reg_qty = 1;

						message.push_back(reg_qty >> 8);												//Количество регистров Hi байт
						message.push_back(reg_qty);														//Количество регистров Lo байт


						optimize.device_addr = node->vectorDevice[i].device_address;
						optimize.request.push_back(message);
						
						for (int g = 0; g < regs.size(); g++) optimize.input_regs.push_back(regs[g]);	//Копирование вектора
					}

					regs.clear();
					message.clear();
				}
			}

			vector_optimize.push_back(optimize);
		}	
	}	

	return vector_optimize;
}


void distributeResponse(Node* node, std::vector<Optimize> vector_optimize, int sequence_number)
{		


	//Проходим по устройствам
	for (int i = 0; i < node->vectorDevice.size(); i++)
	{
		if (node->vectorDevice[i].on == 1)
		{

			//Передаем лямбду (11-ый стандарт) в функцию сортировки из STL
			std::sort(node->vectorDevice[i].vectorTag.begin(), node->vectorDevice[i].vectorTag.end(), [](const Tag& tag1, const Tag& tag2) -> bool 			
			{
				return tag1.reg_address < tag2.reg_address;
			});


			std::vector<int> value_array;

			//Проходим по тэгам устройства 
			for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
			{				
				int size = (9 + vector_optimize[i].response[j][8]);
				for (int k = 9; k < size; k += 2)
				{
					//printf("%d \n", (vector_optimize[i].response[j][k] << 8) + vector_optimize[i].response[j][k+1]);
					value_array.push_back((vector_optimize[i].response[j][k] << 8) + vector_optimize[i].response[j][k + 1]);
				}
				
				
				
				////Проходим по байтам вектора с ответом															
				//for (int k = 9, t = node->vectorDevice[i].vectorTag[j].reg_address; k < (9 + vector_response[i].response[j][8]); k+=2, t++)
				//{					
				//	if (t == node->vectorDevice[i].vectorTag[j].reg_address)
				//	{
				//		value_array.push_back((vector_response[i].response[k] << 8) + vector_response[i].response[k + 1]);
				//	}
				//}
								
				//node->vectorDevice[i].vectorTag[j].value = value_array[j];

				//printf("%d ", vector_response[i].response[k]);				
				
			}


			//Передаем лямбду (11-ый стандарт) в функцию сортировки из STL
			std::sort(node->vectorDevice[i].vectorTag.begin(), node->vectorDevice[i].vectorTag.end(), [](const Tag& tag1, const Tag& tag2) -> bool
			{
				return tag1.reg_position < tag2.reg_position;
			});

		}
	}

}