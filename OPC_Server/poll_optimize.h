#pragma once

#include "main.h"


#include <time.h>
#include <sys/time.h>



std::vector<std::vector<int>> splitRegs(std::vector<int>& regs);
bool checkFloatType(std::vector<Tag> vector_tag, uint16_t reg_number);
std::vector<Optimize> reorganizeNodeIntoPolls(Node* node);