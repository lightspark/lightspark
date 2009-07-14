#ifndef LOGGER_H
#define LOGGER_H

#include <semaphore.h>
#include <iostream>

enum LOG_LEVEL { NO_INFO=0, ERROR=1, NOT_IMPLEMENTED=2,CALLS=3,TRACE=4};

#define LOG(level,esp)		\
{				\
	Log l(level);	\
	if(l) l() << esp << std::endl; \
}

class Log
{
private:
	static sem_t mutex;
	LOG_LEVEL cur_level;
	bool valid;
	static const char* level_names[];
	static LOG_LEVEL log_level;

public:
	Log(LOG_LEVEL l);
	~Log();
	std::ostream& operator()();
	operator bool() { return valid; }
	static void initLogging(LOG_LEVEL l);
	static LOG_LEVEL getLevel(){return log_level;}

};

#endif
