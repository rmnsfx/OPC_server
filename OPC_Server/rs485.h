#pragma once


#include "main.h"
#include "rs485_device_ioctl.h"

char* pathToPort(int node_port);

void* connectDeviceRS485(void *args);
void* pollingDeviceRS485(void *args);
int sp_master_read(rs485_buffer_pack_t* channel, float* data, size_t data_size);

