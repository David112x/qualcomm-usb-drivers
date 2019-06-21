// qdclr.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "infdev.h"
#include <iostream>

int main()
{
	gExecutionMode = EXEC_MODE_REMOVE_OEM;
	MainRemovalTask();
	printf("   Copyright (c) 2015 Qualcomm Technologies, Inc.\n");
}

