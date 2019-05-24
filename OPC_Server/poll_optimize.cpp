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
	std::vector<int> split(2); // создали и указали число ячеек	
	std::vector<std::vector<int>> split_out;

	if (regs.size() > 1)
	{
		sort(regs.begin(), regs.end());

		
		split[0] = regs[0] -1; //Адрес регистра = Номер регистра - 1

		for (int i = 1; i < regs.size(); i++)
		{

			if ((regs[i] - split[0]) < 125)
			{
				split[1] = regs[i] -1;
			}
			else
			{
				split_out.push_back(split);
				split.clear();
				split.push_back(regs[i] -1);
				split.push_back(0);
			}

			if (i == (regs.size() - 1))
			{
				split_out.push_back(split);
				split.clear();
			}
		
		}
	}
	else if (regs.size() == 1)
	{
		split.push_back(regs[0] -1);
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

						reg_qty = pair_request[z].back() - pair_request[z].front() +1;

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

						reg_qty = pair_request[z].back() - pair_request[z].front() +1;

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


