
# include "stdafx.h"
# include "Robot.h"


Robot::Robot(uint32_t t) : _x(0), _y(0), _dx(0), _dy(0), _t(t) {
	// ...
}

Robot::Robot(): Robot(0) {
	// ...
}


void
Robot::advance(uint32_t t_) {
	unsigned long long dt = t_ - _t;

	_x = _x + _dx * dt;
	_y = _y + _dy * dt;
	_t = t_;
}


void
Robot::set_data(double x_, double y_, double dx_, double dy_, uint32_t t_) {
	_x = x_;
	_y = y_;
	
	_dx = dx_;
	_dy = dy_;
	_t = t_;
}
