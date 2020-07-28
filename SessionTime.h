
#pragma once

#include <assert.h>

#include <chrono>

class SessionTime {
public:
	unsigned long long _startTime = 0;
	
	SessionTime() {
		_startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	uint32_t getTimeMS() {
		unsigned long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		unsigned long long dT = now - _startTime;

		assert(dT <= std::numeric_limits<uint32_t>::max());

		return static_cast<uint32_t> (dT);

	}

};
