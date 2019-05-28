#pragma once

#include "main.h"


#include <time.h>
#include <sys/time.h>


std::vector<Optimize> reorganizeNodeIntoPolls(Node* node);
void distributeResponse(Node* node, std::vector<Optimize> vector_response, int16_t sequence_number);



