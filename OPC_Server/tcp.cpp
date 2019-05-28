#include "main.h"

#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <string> 
#include <iostream>
#include <vector>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include<netdb.h>

#include "open62541.h"
#include <mutex>

#define _BSD_SOURCE //for <sys/time.h>

#include <time.h>
#include <sys/time.h>
#include "poll_optimize.h"



timespec time_diff(timespec start, timespec end)
{
	timespec temp;

	if ((end.tv_nsec - start.tv_nsec) < 0)
	{
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	}
	else
	{
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	return temp;
}


void* connectDeviceTCP(void *args)
{
	Node* node = (Node*)args;

	struct sockaddr_in serv_addr;

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	int result = 0;

	if (node != NULL)
	{

		//Создаем сокет
		if ((node->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("\n Socket creation error \n");
		}
		else
		{
			printf("\nSocket create: %d\n", node->socket);
		}

		//Таймаут
		if (setsockopt(node->socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		{
			printf("\n setsockopt failed \n");
		}
		if (setsockopt(node->socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		{
			printf("\n setsockopt failed \n");
		}

		//IP и Порт
		serv_addr.sin_addr.s_addr = inet_addr(node->address.c_str());
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(node->port);

		//Подключаем
		result = connect(node->socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

		if (result < 0)
		{
			printf("\nConnection Failed: %d\n", result);
		}

	}
}



void* pollingDeviceTCP(void *args)
{
	
	
	Node* node = (Node*)args;

	int16_t result = 0;	
	//char write_buffer[] = { 00, 00, 00, 00, 00, 06, 01, 03, 00, 00, 00, 01 };	
	uint8_t read_buffer[255];
	std::vector<uint8_t> read_buffer_vector;

	extern UA_Server *server;
	UA_Variant value;
	int16_t modbus_value = 0;

	struct timeval timeout;
	
	fd_set set;	
	int16_t trial[node->vectorDevice.size()]; //Хранилище флагов о попытках опроса

	int16_t dur_ms = 0;
	int16_t common_dur_ms = 0;
	struct timespec start, stop, duration, stop2, common_duration;

	//signal(SIGPIPE, SIG_IGN);

	int16_t pos = 0;

	std::vector<Optimize> vector_optimize = reorganizeNodeIntoPolls(node);	
	if (vector_optimize.size() != node->vectorDevice.size()) printf("Warning, vector != device quantity.");	
	
	
	
	
	
	while (1)
	{		
		clock_gettime(CLOCK_REALTIME, &start);
		

		//Проходим по устройствам
		for (int i = 0; i < node->vectorDevice.size(); i++)
		{
			if (node->vectorDevice[i].on == 1)
			{					

				//Отправляем запросы и принимаем ответы по порядку
				for (int y = 0; y < vector_optimize[i].request.size(); y++)
				{

					result = send(node->vectorDevice[i].device_socket, &vector_optimize[i].request[y][0], vector_optimize[i].request[y].size(), 0);


					//reinit fd
					FD_ZERO(&set); /* clear the set */
					FD_SET(node->vectorDevice[i].device_socket, &set); /* add our file descriptor to the set */

					//reinit timeout
					timeout.tv_sec = node->vectorDevice[i].poll_timeout / 1000;
					timeout.tv_usec = (node->vectorDevice[i].poll_timeout % 1000) * 1000;

					result = select(node->vectorDevice[i].device_socket + 1, &set, 0, 0, &timeout);

					if (result > 0)
					{
						result = read(node->vectorDevice[i].device_socket, &read_buffer, sizeof(read_buffer));

						if (result > 0)
						{
							for (int a = 0; a < result; a++)
							{
								read_buffer_vector.push_back(read_buffer[a]);
							}

							vector_optimize[i].response.push_back(read_buffer_vector);


							//Разбираем (распределяем значения по регистрам) ответ
							//Определяем начальный адрес
							int start_address = (vector_optimize[i].request[y][8] << 8) + vector_optimize[i].request[y][9];
							
							
							//Проходим по вектору с ответами
							for (int v = 9, addr = start_address+1; v < vector_optimize[i].response[y].size(); v+=2, addr++)
							{			

								//Перебираем holding
								if (vector_optimize[i].response[y][7] == 0x03)
								{	
									for (int s = 0; s < vector_optimize[i].holding_regs.size()+1; s++)
									{
										if (vector_optimize[i].holding_regs[s] == addr)
										{
											for (int c = 0; c < node->vectorDevice[i].vectorTag.size(); c++)
											{
												if (addr == node->vectorDevice[i].vectorTag[c].reg_address)
												{
													pos = node->vectorDevice[i].vectorTag[c].reg_position;

													if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::int16)
													{														
														node->vectorDevice[i].vectorTag[pos].value = (int16_t) (vector_optimize[i].response[y][v] << 8) + vector_optimize[i].response[y][v + 1];
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::uint16)
													{
														node->vectorDevice[i].vectorTag[pos].value = (uint16_t) (vector_optimize[i].response[y][v] << 8) + vector_optimize[i].response[y][v + 1];
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::int32) //1.2.3.4 (LE)
													{
														node->vectorDevice[i].vectorTag[pos].value = (int32_t) (vector_optimize[i].response[y][v] << 24) + (vector_optimize[i].response[y][v + 1] << 16) + (vector_optimize[i].response[y][v + 2] << 8) + (vector_optimize[i].response[y][v + 3]);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::uint32) //1.2.3.4 (LE)
													{
														node->vectorDevice[i].vectorTag[pos].value = (uint32_t)(vector_optimize[i].response[y][v] << 24) + (vector_optimize[i].response[y][v + 1] << 16) + (vector_optimize[i].response[y][v + 2] << 8) + (vector_optimize[i].response[y][v + 3]);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_BE) //4.3.2.1
													{
														int32_t int_val = ((vector_optimize[i].response[y][v + 3] << 24) + (vector_optimize[i].response[y][v + 2] << 16)) + ((vector_optimize[i].response[y][v + 1] << 8) + vector_optimize[i].response[y][v + 0]);														
														
														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_BE_swap)  //3.4.1.2
													{
														int32_t int_val = ((vector_optimize[i].response[y][v + 2] << 24) + (vector_optimize[i].response[y][v + 3] << 16)) + ((vector_optimize[i].response[y][v] << 8) + vector_optimize[i].response[y][v + 1]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_LE)  //1.2.3.4
													{
														int32_t int_val = ((vector_optimize[i].response[y][v] << 24) + (vector_optimize[i].response[y][v + 1] << 16)) + ((vector_optimize[i].response[y][v + 2] << 8) + vector_optimize[i].response[y][v + 3]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_LE_swap)  //2.1.4.3
													{
														int32_t int_val = ((vector_optimize[i].response[y][v + 1] << 24) + (vector_optimize[i].response[y][v] << 16)) + ((vector_optimize[i].response[y][v + 3] << 8) + vector_optimize[i].response[y][v + 2]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}
												}
											}																				
										}										
									}
								}

								//Перебираем input
								if (vector_optimize[i].response[y][7] == 0x04)							
								{
									for (int s = 0; s < vector_optimize[i].input_regs.size() + 1; s++)
									{
										if (vector_optimize[i].input_regs[s] == addr) 
										{
											for (int c = 0; c < node->vectorDevice[i].vectorTag.size(); c++)
											{
												if (addr == node->vectorDevice[i].vectorTag[c].reg_address) 
												{
													pos = node->vectorDevice[i].vectorTag[c].reg_position;

													if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::int16)
													{
														node->vectorDevice[i].vectorTag[pos].value = (int16_t)(vector_optimize[i].response[y][v] << 8) + vector_optimize[i].response[y][v + 1];
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::uint16)
													{
														node->vectorDevice[i].vectorTag[pos].value = (uint16_t)(vector_optimize[i].response[y][v] << 8) + vector_optimize[i].response[y][v + 1];
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::int32) //1.2.3.4 (LE)
													{
														node->vectorDevice[i].vectorTag[pos].value = (int32_t)(vector_optimize[i].response[y][v] << 24) + (vector_optimize[i].response[y][v + 1] << 16) + (vector_optimize[i].response[y][v + 2] << 8) + (vector_optimize[i].response[y][v + 3]);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::uint32) //1.2.3.4 (LE)
													{
														node->vectorDevice[i].vectorTag[pos].value = (uint32_t)(vector_optimize[i].response[y][v] << 24) + (vector_optimize[i].response[y][v + 1] << 16) + (vector_optimize[i].response[y][v + 2] << 8) + (vector_optimize[i].response[y][v + 3]);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_BE) //4.3.2.1
													{
														int32_t int_val = ((vector_optimize[i].response[y][v + 3] << 24) + (vector_optimize[i].response[y][v + 2] << 16)) + ((vector_optimize[i].response[y][v + 1] << 8) + vector_optimize[i].response[y][v + 0]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_BE_swap)  //3.4.1.2
													{
														int32_t int_val = ((vector_optimize[i].response[y][v + 2] << 24) + (vector_optimize[i].response[y][v + 3] << 16)) + ((vector_optimize[i].response[y][v] << 8) + vector_optimize[i].response[y][v + 1]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_LE)  //1.2.3.4
													{
														int32_t int_val = ((vector_optimize[i].response[y][v] << 24) + (vector_optimize[i].response[y][v + 1] << 16)) + ((vector_optimize[i].response[y][v + 2] << 8) + vector_optimize[i].response[y][v + 3]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_LE_swap)  //2.1.4.3
													{
														int32_t int_val = ((vector_optimize[i].response[y][v + 1] << 24) + (vector_optimize[i].response[y][v] << 16)) + ((vector_optimize[i].response[y][v + 3] << 8) + vector_optimize[i].response[y][v + 2]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}
												}
											}
										}
									}
								}


							}

							if (vector_optimize[i].response[y][7] == 0x83 || vector_optimize[i].response[y][7] == 0x84)
							{
								printf("Warning! No response from: device = %d, start reg address = %d \n", node->vectorDevice[i].device_address, start_address);
							}

							read_buffer_vector.clear();
							
						}
					}

					if (result == 0)
					{
						printf("Timeout device: %d, poll attempt %d \n", node->vectorDevice[i].device_address, trial[i]);

						//set buffer to zero when poll timed out
						//for (int h = 0; h < sizeof(vector_optimize[i].request); h++) vector_optimize[i].request[h] = 0;

						if (++trial[i] > 3)
						{
							printf("Timeout device: %d, switch off device.\n", node->vectorDevice[i].device_address);

							//Выключаем устройство
							node->vectorDevice[i].on = 0;
						}
					}

					if (result == -1)
					{
						printf("Warning! Error read, result: %d\n", result);
					}				


				}//Закрывашка "отправляем запросы и принимаем ответы по порядку"

				vector_optimize[i].response.clear();




				//Проходим по тэгам устройства (OPC)
				for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
				{
					if (node->vectorDevice[i].vectorTag[j].on == 1)
					{

						if (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::int16)
						{
							//node->vectorDevice[i].vectorTag[j].value = ((read_buffer[9] << 8) + read_buffer[10]);

							UA_Int16 opc_value = (UA_Int16)node->vectorDevice[i].vectorTag[j].value;
							UA_Variant_setScalarCopy(&value, &opc_value, &UA_TYPES[UA_TYPES_INT16]);
							UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
						}

						if (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::float_LE)
						{
							//modbus_value = ((read_buffer[9] << 24) + (read_buffer[10] << 16) + (read_buffer[11] << 8) + read_buffer[12]);
							//node->vectorDevice[i].vectorTag[j].value = *reinterpret_cast<float*>(&modbus_value);

							UA_Float opc_value = (UA_Float)node->vectorDevice[i].vectorTag[j].value;
							UA_Variant_setScalarCopy(&value, &opc_value, &UA_TYPES[UA_TYPES_FLOAT]);
							UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
						}

					}

					printf("%d %s %g\n", node->vectorDevice[i].device_address, node->vectorDevice[i].vectorTag[j].name.c_str(), node->vectorDevice[i].vectorTag[j].value);
				}


				
			}

		}


		
		clock_gettime(CLOCK_REALTIME, &stop);
		duration = time_diff(start, stop);
		dur_ms = duration.tv_sec * 100 + (duration.tv_nsec / 1000000);		
		//printf("Time %d", duration.tv_sec*100 + (duration.tv_nsec / 1000000) );

		if (dur_ms < node->poll_period)
		{
			usleep( (node->poll_period - dur_ms) * 1000);			
			//printf("Time %d ", (node->vectorDevice[0].poll_period - dur_ms));
		}

		clock_gettime(CLOCK_REALTIME, &stop2);
		common_duration = time_diff(start, stop2);		
		//printf("\nCommon Duration Time %d \n\n", common_duration.tv_sec*100 + (common_duration.tv_nsec / 1000000) );
	}

	

}