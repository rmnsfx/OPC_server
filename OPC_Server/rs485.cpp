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
#include <netdb.h>

#include "rs485.h"
#include "rs485_device_ioctl.h"

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include "crc.h"
#include "poll_optimize.h"
#include "utils.h"


char* pathToPort(int node_port)
{
	switch (node_port)
	{
	case 0:
		return "/dev/rs485_0";
	case 1:
		return "/dev/rs485_1";
	case 2:
		return "/dev/rs485_2";
	case 3:
		return "/dev/rs485_3";
	case 4:
		return "/dev/rs485_4";
	case 5:
		return "/dev/rs485_5";
	case 6:
		return "/dev/rs485_6";
	case 7:
		return "/dev/rs485_7";
	
	
	default:
		printf("Warning! Wrong RS-485 port parameter. Default value: /dev/rs485_3\n");
		return "/dev/rs485_3";
	}
};




void* pollingDeviceRS485(void *args)
{	
	
	
	int16_t result = 0;
	//char write_buffer[] = { 0x01, 0x04, 0x03, 0xDD, 0x00, 0x01, 0xA1, 0xB4 };
	uint8_t read_buffer[255];
	int16_t dur_ms = 0;
	int16_t common_dur_ms = 0;
	struct timespec start, stop, duration, stop2, common_duration;
	std::vector<uint8_t> read_buffer_vector;
	uint8_t expected_size = 0;
	uint16_t current_crc = 0;
	uint16_t expected_crc = 0;
	int16_t pos = 0;
	extern UA_Server *server;
	UA_Variant value;
	UA_Int16 opc_value_int16;
	UA_UInt16 opc_value_uint16;
	UA_Int32 opc_value_int32;
	UA_UInt32 opc_value_uint32;
	UA_Float opc_value_float;




	Node* node = (Node*)args;


	if (node != NULL)
	{
		char* port = pathToPort(node->port);

		node->f_id = open(port, O_RDWR);
		

		if (node->f_id < 0)
		{
			printf("\nConnection Failed: %d\n", node->f_id);
			//return -1;
		}
	}
	


	std::vector<Optimize> vector_optimize = reorganizeNodeIntoPolls(node);


	//transaction preparation			
	rs485_device_ioctl_t config;
	   
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
					config.tx_buf = (const char*) &vector_optimize[i].request[y][0];
					config.tx_count = vector_optimize[i].request[y].size();
					
					expected_size = ((vector_optimize[i].request[y][4] << 8) + vector_optimize[i].request[y][5]) * 2 + 5;

					config.rx_buf = (char*) &read_buffer;
					config.rx_size = sizeof(read_buffer);
					config.rx_expected = expected_size; 		//ATTENTION: rx_expected must be set for correct driver reception.
					
					//Драйвер осуществляет запрос/ответ
					ioctl(node->f_id, RS485_SEND_PACKED, &config);

					
					for (int a = 0; a < expected_size; a++)
					{
						read_buffer_vector.push_back(read_buffer[a]);
					}

					//Проверяем crc
					current_crc = read_buffer_vector[expected_size - 2] + (read_buffer_vector[expected_size - 1] << 8);
					expected_crc = calculate_crc((uint8_t*)&read_buffer_vector[0], read_buffer_vector.size() - 2);

					if (current_crc == expected_crc)
					{
						vector_optimize[i].response.push_back(read_buffer_vector);
						

						//Разбираем (распределяем значения по регистрам) ответ
						//Определяем начальный адрес
						uint16_t start_address = (vector_optimize[i].request[y][2] << 8) + vector_optimize[i].request[y][3];
						//printf("start: %d \n", start_address);

						for (int v = 3, addr = start_address + 1; v < vector_optimize[i].response[y].size(); v += 2, addr++)
						{


							//Перебираем holding
							if (vector_optimize[i].response[y][1] == 0x03)
							{
								for (int s = 0; s < vector_optimize[i].holding_regs.size() + 1; s++)
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

												printf("%d \n", (int)node->vectorDevice[i].vectorTag[pos].value);
											}
										}
									}
								}
							}

							

							//Перебираем input
							if (vector_optimize[i].response[y][1] == 0x04)
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

													//node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													memcpy(&node->vectorDevice[i].vectorTag[pos].value, &int_val, sizeof(float));
												}
												else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_BE_swap)  //3.4.1.2
												{
													int32_t int_val = ((vector_optimize[i].response[y][v + 2] << 24) + (vector_optimize[i].response[y][v + 3] << 16)) + ((vector_optimize[i].response[y][v] << 8) + vector_optimize[i].response[y][v + 1]);

													//node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													memcpy(&node->vectorDevice[i].vectorTag[pos].value, &int_val, sizeof(float));
												}
												else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_LE)  //1.2.3.4
												{
													int32_t int_val = ((vector_optimize[i].response[y][v] << 24) + (vector_optimize[i].response[y][v + 1] << 16)) + ((vector_optimize[i].response[y][v + 2] << 8) + vector_optimize[i].response[y][v + 3]);

													//node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													memcpy(&node->vectorDevice[i].vectorTag[pos].value, &int_val, sizeof(float));
												}
												else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_LE_swap)  //2.1.4.3
												{
													int32_t int_val = ((vector_optimize[i].response[y][v + 1] << 24) + (vector_optimize[i].response[y][v] << 16)) + ((vector_optimize[i].response[y][v + 3] << 8) + vector_optimize[i].response[y][v + 2]);

													//node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													memcpy(&node->vectorDevice[i].vectorTag[pos].value, &int_val, sizeof(float));
												}

												printf("%d \n", (int)node->vectorDevice[i].vectorTag[pos].value);
											}
										}
									}
								}
							}
						}

						

					}
					else printf("Warning! Error CRC. ");

					if (vector_optimize[i].response[y][1] == 0x83 || vector_optimize[i].response[y][1] == 0x84)
					{
						printf("Warning! No response from: device = %d, start reg address = %d \n", node->vectorDevice[i].device_address, (vector_optimize[i].request[y][2] << 8) + vector_optimize[i].request[y][3]);
					}

					read_buffer_vector.clear();


					//printf("--->");
					//for (int w = 0; w < 8; w++)
					//{
					//	printf("%X ", vector_optimize[i].request[y][w]);					
					//}
					//printf("\n <---");
					//for (int w = 0; w < expected_size; w++)
					//{					
					//	printf("%X ", read_buffer[w]);
					//}
					//printf("\n");
				}





				//Проходим по тэгам устройства (OPC)
				for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
				{
					if (node->vectorDevice[i].vectorTag[j].on == 1)
					{

						if (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::int16)
						{
							opc_value_int16 = (UA_Int16)node->vectorDevice[i].vectorTag[j].value;
							UA_Variant_setScalarCopy(&value, &opc_value_int16, &UA_TYPES[UA_TYPES_INT16]);
							UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
						}
						else if (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::uint16)
						{
							opc_value_uint16 = (UA_UInt16)node->vectorDevice[i].vectorTag[j].value;
							UA_Variant_setScalarCopy(&value, &opc_value_uint16, &UA_TYPES[UA_TYPES_UINT16]);
							UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
						}
						else if (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::int32)
						{
							opc_value_int32 = (UA_Int32)node->vectorDevice[i].vectorTag[j].value;
							UA_Variant_setScalarCopy(&value, &opc_value_int32, &UA_TYPES[UA_TYPES_INT32]);
							UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
						}
						else if (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::uint32)
						{
							opc_value_uint32 = (UA_UInt32)node->vectorDevice[i].vectorTag[j].value;
							UA_Variant_setScalarCopy(&value, &opc_value_uint32, &UA_TYPES[UA_TYPES_UINT32]);
							UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
						}
						else if ( (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::float_BE) ||
							(node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::float_BE_swap) ||
							(node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::float_LE) ||
							(node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::float_LE_swap) )
						{
							opc_value_float = (UA_Float)node->vectorDevice[i].vectorTag[j].value;
							UA_Variant_setScalarCopy(&value, &opc_value_float, &UA_TYPES[UA_TYPES_FLOAT]);
							UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
						}

					}

					//printf("%d %s %d\n", node->vectorDevice[i].device_address, node->vectorDevice[i].vectorTag[j].name.c_str(), node->vectorDevice[i].vectorTag[j].value);
				}



			}

			vector_optimize[i].response.clear();
		}

		





		clock_gettime(CLOCK_REALTIME, &stop);
		duration = time_diff(start, stop);
		dur_ms = duration.tv_sec * 100 + (duration.tv_nsec / 1000000);
		//printf("Time %d", duration.tv_sec*100 + (duration.tv_nsec / 1000000) );

		if (dur_ms < node->poll_period)
		{
			usleep((node->poll_period - dur_ms) * 1000);
			//printf("Time %d ", (node->poll_period - dur_ms));
		}

		clock_gettime(CLOCK_REALTIME, &stop2);
		common_duration = time_diff(start, stop2);
		//printf("\nCommon Duration Time %d \n\n", common_duration.tv_sec*100 + (common_duration.tv_nsec / 1000000) );
	}


}
