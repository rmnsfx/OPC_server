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

std::vector<Optimize> reorganizeNodeIntoPolls(Node* node)
{
	Optimize optimize;	
	std::vector<Optimize> vector_optimize; //Для формирования пакета запроса
	std::vector<int> regs;	//Для сортировки адресов
	int reg_qty = 0;
	

	//Проходим по устройствам
	for (int i = 0; i < node->vectorDevice.size(); i++)
	{
		if (node->vectorDevice[i].on == 1)
		{
			//Проходим по тэгам устройства 
			for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
			{
				if (node->vectorDevice[i].vectorTag[j].on == 1)
				{
					regs.push_back(node->vectorDevice[i].vectorTag[j].reg_address);
					node->vectorDevice[i].vectorTag[j].reg_position = j; //Запоминаем номер позиции регистра
				}
			}

			sort(regs.begin(), regs.end());

			//Формируем пакет для запроса
			optimize.request[0] = 0x00;														//Идентификатор транзакции
			optimize.request[1] = 0x00;
			optimize.request[2] = 0x00;														//Идентификатор протокола
			optimize.request[3] = 0x00;
			optimize.request[4] = 0x00;														//Длина сообщения
			optimize.request[5] = 0x06;														
			optimize.request[6] = node->vectorDevice[i].device_address;						//Адрес устройства
			optimize.request[7] = 0x03;														//Функциональный код !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Доделать !!!!!!!!!!!!!!!!!!!!!!!!!!!
			optimize.request[8] = regs.front() >> 8;										//Адрес первого регистра Hi байт
			optimize.request[9] = regs.front();												//Адрес первого регистра Lo байт			
						
			reg_qty = regs.back() - regs.front() + 1;

			optimize.request[10] = reg_qty >> 8;										//Количество регистров Hi байт
			optimize.request[11] = reg_qty;												//Количество регистров Lo байт

			optimize.device_addr = node->vectorDevice[i].device_address;						
			optimize.regs.swap(regs);
			vector_optimize.push_back(optimize);	

			regs.clear();
		}	

	}	

	return vector_optimize;
}


void distributeResponse(Node* node, std::vector<Optimize> vector_response)
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

				//Проходим по байтам вектора с ответом															
				for (int k = 9, t = node->vectorDevice[i].vectorTag[0].reg_address; k < (9 + vector_response[i].response[8]); k+=2, t++)
				{					
					if (t == node->vectorDevice[i].vectorTag[j].reg_address)
					{
						value_array.push_back((vector_response[i].response[k] << 8) + vector_response[i].response[k + 1]);
					}
				}
								
				node->vectorDevice[i].vectorTag[j].value = value_array[j];

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