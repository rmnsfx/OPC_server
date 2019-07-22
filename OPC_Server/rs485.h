#pragma once


#include "main.h"

char* pathToPort(int node_port);

void* connectDeviceRS485(void *args);
void* pollingDeviceRS485(void *args);

