#pragma once

#include <cstdint>

class Robot {
public:
	double _x;
	double _y;

	double _dx;
	double _dy;

	uint32_t _t;

	Robot();
	Robot(uint32_t t_);

	void advance(uint32_t t_);
	void set_data(double x_, double y_, double dx_, double dy_, uint32_t t_);
};

