#define _BSD_SOURCE //for <sys/time.h>

#include <time.h>
#include <sys/time.h>
#include "utils.h"
#include "poll_optimize.h"
#include <stdio.h>
#include <fstream>
#include <iostream>


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