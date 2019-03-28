#include "main.h"

#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <string> 
#include <iostream>
#include <vector>
#include <unistd.h>

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
			optimize.request[7] = 0x03;														//Функциональный код !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			optimize.request[8] = regs.front() >> 8;										//Адрес первого регистра Hi байт
			optimize.request[9] = regs.front();												//Адрес первого регистра Lo байт			
			optimize.request[10] = regs.back() >> 8;										//Количество регистров Hi байт
			optimize.request[11] = regs.back();												//Количество регистров Lo байт

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
			//Проходим по тэгам устройства 
			for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
			{
				if (node->vectorDevice[i].vectorTag[j].on == 1)
				{
					//Проходим по регистрам вектора с ответом
					for (int k = 0; k < vector_response[i].regs.size(); k++)
					{
						if (vector_response[i].regs[k] == node->vectorDevice[i].vectorTag[j].reg_address)
						{
							//node->vectorDevice[i].vectorTag[j].value = ((read_buffer[9] << 8) + read_buffer[10]);
							//node->vectorDevice[i].vectorTag[j].value = ((read_buffer[9] << 8) + read_buffer[10]);
							//vector_response[i].regs[k]
							//printf("vector: %d, tag: %d\n", vector_response[i].regs[k], node->vectorDevice[i].vectorTag[j].reg_address);
						}
					}
				}
			}
		}
	}

}