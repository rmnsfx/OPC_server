#pragma once

#define _BSD_SOURCE //for <sys/time.h>

#include <time.h>
#include <sys/time.h>


timespec time_diff(timespec start, timespec end);
