#pragma once


#include "Robot.h"
#include "ArenaCubes.h"
#include <vector>
#include <cstdint>

class RobotPark {
	std::vector<Robot> _lcRobot;
public:
	RobotPark(uint32_t nInstances, uint32_t t);
	void advance(uint32_t t);
	uint32_t instances();
	void get_instance_data(std::vector<instance_data>& lcData);

};
