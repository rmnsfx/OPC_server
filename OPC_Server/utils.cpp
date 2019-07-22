#define _BSD_SOURCE //for <sys/time.h>

#include <time.h>
#include <sys/time.h>
#include "utils.h"
#include "poll_optimize.h"
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <sys/sysctl.h>

#if GTEST_DEBUG == 0

char* print_date_time(void)
{
	//const time_t timer = time(NULL);
	//printf("%s ",  ctime(&timer));

	char buff[20];
	struct tm *sTm;

	time_t now = time(0);
	sTm = gmtime(&now);

	strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", sTm);

	return buff;
	//printf("%s\n", buff);
}



unsigned long long getTotalSystemMemory(void)
{
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	return pages * page_size;
}

#endif

int getRam(void)
{
	FILE *meminfo = fopen("/proc/meminfo", "r");
	
	if (meminfo == NULL) return -1; // handle error
	
	char line[256];
	
	while (fgets(line, sizeof(line), meminfo))
	{
		int ram;
		if (sscanf(line, "MemAvailable: %d kB", &ram) == 1)
		{
			fclose(meminfo);
			return ram;
		}
	}

	// If we got here, then we couldn't find the proper line in the meminfo file:
	// do something appropriate like return an error code, throw an exception, etc.
	fclose(meminfo);
	return -1;
}


void write_text_to_log_file(const char* text)
{
	char buff[20];
	struct tm *sTm;

	time_t now = time(0);
	sTm = gmtime(&now);
	strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", sTm);


	std::string input(buff);


	std::ofstream log_file("log_file.txt", std::ios_base::out | std::ios_base::app);
	log_file << input << text << std::endl;
	log_file.close();

}

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
};
