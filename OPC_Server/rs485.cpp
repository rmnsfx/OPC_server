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
#include <memory>

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

int sp_master_read(rs485_buffer_pack_t * block , float * data)
{
	
	//if (data_size / sizeof(float) < channel->points) return -1;
		
	
	uint8_t* start_position = block->pack.data;

	uint8_t* position = start_position;
	uint32_t tmp = 0;
	size_t resolution = block->resolution;
	size_t bits_shift = 8;
	float* data_end = data;	
	size_t point = block->points;
	data_end += point;

	uint8_t old_byte = *position++;
	uint32_t mask = 0xFFFFFFFF >> (32 - resolution);

	while (data < data_end)
	{
		old_byte >>= 8 - bits_shift;
		tmp = old_byte;

		while (bits_shift < resolution)
		{
			old_byte = *position++;
			tmp |= old_byte << bits_shift;
			bits_shift += 8;
		}
		bits_shift -= resolution;
		tmp &= mask;
		*data++ = tmp;
	}


	return point;
}


void* pollingDeviceRS485(void *args)
{	
	
	
	int16_t result = 0;
	//char write_buffer[] = { 0x01, 0x04, 0x03, 0xDD, 0x00, 0x01, 0xA1, 0xB4 };
	
	//std::shared_ptr<unsigned char> read_buffer(new unsigned char[255]);
	//char* read_buffer = new char[255];
	//memset(read_buffer, 0, 255); //Иниц.
	//std::vector<uint8_t> read_buffer(255);

	uint8_t read_buffer[255];
	
	int16_t dur_ms = 0;
	int16_t common_dur_ms = 0;
	
	struct timespec start, stop, duration, stop2, common_duration;
	std::vector<uint8_t> read_buffer_vector;
	
	uint8_t expected_size = 0;
	uint16_t current_crc = 0;
	uint16_t expected_crc = 0;
	int16_t pos = 0;

		
	uint8_t test_val_count = 0;

	std::vector<float> out_float(1024);
	

	Node* node = nullptr;
	if ((Node*)args != nullptr)
	{
		node = (Node*)args;
	}

	UA_Server* server = getServer();
	UA_HistoryDataBackend backend = getBackend();
	
	UA_Variant value;
	UA_Variant sample_value;
	UA_Int16 opc_value_int16 = 0;
	UA_UInt16 opc_value_uint16 = 0;
	UA_Int32 opc_value_int32 = 0;
	UA_UInt32 opc_value_uint32 = 0;
	UA_Float opc_value_float = 0;
	UA_Float opc_value_sample;

	int32_t int_val = 0;
	std::string str = "";

	if (node != nullptr)
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
	config.rx_count = 0;


	//Выборка
	uint8_t sample_buffer[100000];
	struct timespec time;
	
	rs485_channel_read_t ch_read;

	int real_points = 0;
	
	//for (int e = 0; e < 100; e++)
	//{
	//	UA_Variant_init(&sample_value);
	//	//opc_value_sample = (UA_Float)out_float[e];
	//	opc_value_sample = (UA_Float)1.9;
	//	UA_Variant_setScalar(&sample_value, &opc_value_sample, &UA_TYPES[UA_TYPES_FLOAT]);
	//	UA_Server_writeValue(server, node->vectorDevice[0].vectorTag[2].tagNodeId, sample_value);
	//}
	//
	//printf("!!! Точки созданы\n");

	//while (1) sleep(1000);
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
					config.tx_buf = (const char*)&vector_optimize[i].request[y][0];
					config.tx_count = vector_optimize[i].request[y].size();

					expected_size = ((vector_optimize[i].request[y][4] << 8) + vector_optimize[i].request[y][5]) * 2 + 5;

					config.rx_buf = (char*)&read_buffer;
					config.rx_size = sizeof(read_buffer);
					config.rx_expected = expected_size; 		//ATTENTION: rx_expected must be set for correct driver reception.

					//Драйвер осуществляет запрос/ответ
					result = ioctl(node->f_id, RS485_SEND_PACKED, &config);

					#if GTEST_DEBUG == 1						
						read_buffer[0] = 0x00;				
						read_buffer[1] = 0x03;
						read_buffer[2] = 0x14;
						
						read_buffer[3] = 0x00;
						read_buffer[4] = 0x03;
						read_buffer[5] = 0x00;
						read_buffer[6] = 0x04;
						read_buffer[7] = 0x00;
						read_buffer[8] = 0x05;
						read_buffer[9] = 0x00;
						read_buffer[10] = 0x06;
						read_buffer[11] = 0x00;
						read_buffer[12] = 0x07;
						read_buffer[13] = 0x00;
						read_buffer[14] = 0x08;
						read_buffer[15] = 0x23;
						read_buffer[16] = 0x2B;						

						config.rx_count = 17;
						
					#endif

					//printf("Port: %d RX_count: %d \n", node->port, config.rx_count);


					//Проверяем количество полученных байт, если меньше то обрыв или ошибка crc
					if (config.rx_count >= expected_size)
					{
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
														int_val = ((vector_optimize[i].response[y][v + 3] << 24) + (vector_optimize[i].response[y][v + 2] << 16)) + ((vector_optimize[i].response[y][v + 1] << 8) + vector_optimize[i].response[y][v + 0]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_BE_swap)  //3.4.1.2
													{
														int_val = ((vector_optimize[i].response[y][v + 2] << 24) + (vector_optimize[i].response[y][v + 3] << 16)) + ((vector_optimize[i].response[y][v] << 8) + vector_optimize[i].response[y][v + 1]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_LE)  //1.2.3.4
													{
														int_val = ((vector_optimize[i].response[y][v] << 24) + (vector_optimize[i].response[y][v + 1] << 16)) + ((vector_optimize[i].response[y][v + 2] << 8) + vector_optimize[i].response[y][v + 3]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_LE_swap)  //2.1.4.3
													{
														int_val = ((vector_optimize[i].response[y][v + 1] << 24) + (vector_optimize[i].response[y][v] << 16)) + ((vector_optimize[i].response[y][v + 3] << 8) + vector_optimize[i].response[y][v + 2]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
													}

													//Применяем коэфициенты А и B
													node->vectorDevice[i].vectorTag[pos].value = (node->vectorDevice[i].vectorTag[pos].value * node->vectorDevice[i].vectorTag[pos].coef_A) + node->vectorDevice[i].vectorTag[pos].coef_B;

													//printf("%d \n", (int)node->vectorDevice[i].vectorTag[pos].value);
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
														int_val = ((vector_optimize[i].response[y][v + 3] << 24) + (vector_optimize[i].response[y][v + 2] << 16)) + ((vector_optimize[i].response[y][v + 1] << 8) + vector_optimize[i].response[y][v + 0]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
														//memcpy(&node->vectorDevice[i].vectorTag[pos].value, &int_val, sizeof(float));

													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_BE_swap)  //3.4.1.2
													{
														int_val = ((vector_optimize[i].response[y][v + 2] << 24) + (vector_optimize[i].response[y][v + 3] << 16)) + ((vector_optimize[i].response[y][v] << 8) + vector_optimize[i].response[y][v + 1]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
														//memcpy(&node->vectorDevice[i].vectorTag[pos].value, &int_val, sizeof(float));
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_LE)  //1.2.3.4
													{
														int_val = ((vector_optimize[i].response[y][v] << 24) + (vector_optimize[i].response[y][v + 1] << 16)) + ((vector_optimize[i].response[y][v + 2] << 8) + vector_optimize[i].response[y][v + 3]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
														//memcpy(&node->vectorDevice[i].vectorTag[pos].value, &int_val, sizeof(float));
													}
													else if (node->vectorDevice[i].vectorTag[pos].enum_data_type == Data_type::float_LE_swap)  //2.1.4.3
													{
														int_val = ((vector_optimize[i].response[y][v + 1] << 24) + (vector_optimize[i].response[y][v] << 16)) + ((vector_optimize[i].response[y][v + 3] << 8) + vector_optimize[i].response[y][v + 2]);

														node->vectorDevice[i].vectorTag[pos].value = *reinterpret_cast<float*>(&int_val);
														//memcpy(&node->vectorDevice[i].vectorTag[pos].value, &int_val, sizeof(float));
													}

													//Применяем коэфициенты A и B
													node->vectorDevice[i].vectorTag[pos].value = (node->vectorDevice[i].vectorTag[pos].value * node->vectorDevice[i].vectorTag[pos].coef_A) + node->vectorDevice[i].vectorTag[pos].coef_B;

													//printf("%d \n", (int)node->vectorDevice[i].vectorTag[pos].value);
												}
											}
										}
									}
								}
							}

						} // Закрываем CRC
						else if (config.rx_count == 0xff)
						{
							str.clear();
							str = " Port: " + std::to_string(node->port) + "!!! No response\n";
							write_text_to_log_file(str.c_str());
														
							printf(str.c_str());
						}
						else 
						{
							str.clear();
							str = " Port: " + std::to_string(node->port) + "!!! CRC Error\n";
							write_text_to_log_file(str.c_str());

							printf(str.c_str());
						}



						read_buffer_vector.clear();
						

						#if GTEST_DEBUG == 1	
							return 0;
						#endif


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




						//Проходим по тэгам устройства (OPC)
						for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
						{
							if (node->vectorDevice[i].vectorTag[j].on == 1)
							{								

								if (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::int16)
								{
									
									UA_Variant_init(&value);
									opc_value_int16 = (UA_Int16)node->vectorDevice[i].vectorTag[j].value;									
									UA_Variant_setScalar(&value, &opc_value_int16, &UA_TYPES[UA_TYPES_INT16]); 
									UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);						


								}
								else if (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::uint16)
								{
									UA_Variant_init(&value);
									opc_value_uint16 = (UA_UInt16)node->vectorDevice[i].vectorTag[j].value;
									UA_Variant_setScalar(&value, &opc_value_uint16, &UA_TYPES[UA_TYPES_UINT16]);
									UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
								}
								else if (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::int32)
								{
									UA_Variant_init(&value);
									opc_value_int32 = (UA_Int32)node->vectorDevice[i].vectorTag[j].value;
									UA_Variant_setScalar(&value, &opc_value_int32, &UA_TYPES[UA_TYPES_INT32]);
									UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
								}
								else if (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::uint32)
								{
									UA_Variant_init(&value);
									opc_value_uint32 = (UA_UInt32)node->vectorDevice[i].vectorTag[j].value;
									UA_Variant_setScalar(&value, &opc_value_uint32, &UA_TYPES[UA_TYPES_UINT32]);
									UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
								}
								else if ((node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::float_BE) ||
									(node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::float_BE_swap) ||
									(node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::float_LE) ||
									(node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::float_LE_swap))
								{
									UA_Variant_init(&value);
									opc_value_float = (UA_Float)node->vectorDevice[i].vectorTag[j].value;
									UA_Variant_setScalar(&value, &opc_value_float, &UA_TYPES[UA_TYPES_FLOAT]);
									UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, value);
								
								}



							}

							//printf("%d %s %d\n", node->vectorDevice[i].device_address, node->vectorDevice[i].vectorTag[j].name.c_str(), node->vectorDevice[i].vectorTag[j].value);
						
						} //Закрываем for.. "Проходим по тэгам устройства (OPC)"


					} //Закрываем if ..."Проверяем количество байт, если 0 то обрыв или ошибка"					
					else if (config.rx_count < expected_size)
					{

						std::string s = " Port: " + std::to_string(node->port) + " !!! No response (rx_count (" + std::to_string(config.rx_count) + ") < expected (" + std::to_string(config.rx_expected) + "))";
						write_text_to_log_file(s.c_str());		
						
						printf("Port: %d. !!!No response (rx_count (%d) < expected (%d))\n", node->port, config.rx_count, config.rx_expected);
						
						for (int w = 0; w < config.rx_count; w++)
						{
							printf("%X ", read_buffer[w]);
						}
						printf("\n");

					}
					//else if (config.rx_count == 0)
					//{
					//	std::string s = " Warning Port: " + std::to_string(node->port) + "!!! rx_count == 0";
					//	write_text_to_log_file(s.c_str());

					//	printf("Warning! Port: %d !!! rx_count == 0 \n", node->port);
					//}


				} // Закрываем for... "Отправляем запросы и принимаем ответы по порядку"

				//Проходим по тэгам, ищем выборки
				for (int j = 0; j < node->vectorDevice[i].vectorTag.size(); j++)
				{
					if (node->vectorDevice[i].vectorTag[j].enum_data_type == Data_type::sample)
					{						

						//std::cout << node->vectorDevice[i].vectorTag[j].name << std::endl;

						timespec_get(&time, TIME_UTC);

						ch_read.adr = 2;
						ch_read.channel = 1;
						ch_read.data_size = sizeof(sample_buffer);
						ch_read.data = sample_buffer;						
						
						ch_read.start_time = time;
						ch_read.start_time.tv_sec -= 1; //read data from 1 seconds
						//ch_read.start_time.tv_sec = 0;
						//ch_read.start_time.tv_nsec = 0;
						ch_read.end_time = time;
						

						int ioret = ioctl(node->f_id, RS485_SAMPLE_READ, &ch_read);

 						if ((ch_read.count_block > 0) && (ch_read.block_size > 0))
						{
							rs485_buffer_pack_t* block = (rs485_buffer_pack_t*) ch_read.data;
							
							
							for (int v = 0; v < ch_read.count_block; v++)
							{
								//block = ch_read.data + ch_read.block_size * i;
								if ((block->points > 0) && (block->resolution > 0))
								{
									real_points = sp_master_read(block, &out_float[0]);
									

									for (int e = 0; e < real_points; e++)
									{
										//UA_Variant_init(&sample_value);
										//opc_value_sample = (UA_Float) out_float[e];
										//UA_Variant_setScalar(&sample_value, &opc_value_sample, &UA_TYPES[UA_TYPES_FLOAT]);
										//UA_Server_writeValue(server, node->vectorDevice[i].vectorTag[j].tagNodeId, sample_value);												
									}
									
								}


							}
						}

						
						//std::cout << "out_float.size = " << out_float.size() << " :: " << ch_read.count_block << " :: " << ch_read.block_size << " :: "<< out_float.size() << std::endl;
						
						

					}
				}

						
				//Debug to log file
				//std::string s = " Port: " + std::to_string(node->port);
				//write_text_to_log_file(s.c_str());
				


			} //Закрываем vectorDevice[i].on == 1




			vector_optimize[i].response.clear();
			//std::vector<Optimize>().swap(vector_optimize); //free memory

		} // Закрываем for ... "Проходим по устройствам"

		





		clock_gettime(CLOCK_REALTIME, &stop);
		duration = time_diff(start, stop);
		dur_ms = duration.tv_sec * 100 + (duration.tv_nsec / 1000000);
		//printf("Time %d", duration.tv_sec*100 + (duration.tv_nsec / 1000000) );

		if (dur_ms < node->poll_period)
		{
			usleep((node->poll_period - dur_ms) * 1000);
			printf("Poll time %d ms", (node->poll_period - dur_ms));
		}

		clock_gettime(CLOCK_REALTIME, &stop2);
		common_duration = time_diff(start, stop2);
		printf("\nCommon Duration Time %d ms\n\n", common_duration.tv_sec*1000 + (common_duration.tv_nsec / 1000000) );
	

		
		//std::string s = " Memory: " + std::to_string(getRam());
		str = " RES Memory: " + std::to_string(getCurrentProccessMemory());
		write_text_to_log_file(str.c_str());
		
		
		//printf("%d \n", getTotalSystemMemory());


	} // Закрываем while



	return 0;
}


