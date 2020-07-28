
#include "stdafx.h"
#include "RobotPark.h"

#include <random>

RobotPark::RobotPark(uint32_t nInstances, uint32_t t) { 
	

    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(-1.f, 1.f);

    double pos_scale = 100.0;
    double vel_scale = 0.001;

    for (uint32_t i = 0; i < nInstances; i++) {
        double x_pos = pos_scale * distribution(generator);
        double y_pos = pos_scale * distribution(generator);
        double x_vel = vel_scale * distribution(generator);
        double y_vel = vel_scale * distribution(generator);

        Robot r;

        r.set_data(x_pos, y_pos, x_vel, y_vel, t);

        _lcRobot.push_back(r);
    }

}

void
RobotPark::advance(uint32_t t) {

    for (Robot& r : _lcRobot) {
        r.advance(t);
    }
}


uint32_t
RobotPark::instances() {
    return uint32_t(_lcRobot.size());
}

void
RobotPark::get_instance_data(std::vector<instance_data>& lcData) {

    for (Robot& r : _lcRobot) {
        lcData.push_back( {float(r._x), float(r._y), float(r._dx), float(r._dy) });
    }
}
