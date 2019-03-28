
#ifndef POLL_OPTIMIZE_H
#define POLL_OPTIMIZE_H

#include "main.h"


#include <time.h>
#include <sys/time.h>


std::vector<Optimize> reorganizeNodeIntoPolls(Node* node);
void distributeResponse(Node* node, std::vector<Optimize> vector_response);


#endif
