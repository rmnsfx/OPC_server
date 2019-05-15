#pragma once


#ifndef RS485_H
#define RS485_H

#include "main.h"

void* connectDeviceRS485(void *args);
void* pollingDeviceRS485(void *args);

#endif