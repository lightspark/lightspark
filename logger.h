#ifndef LOGGER_H
#define LOGGER_H

#include <semaphore.h>
#include <iostream>

enum LOG_LEVEL { NO_INFO=0, ERROR=1, NOT_IMPLEMENTED=2,TRACE=3};

#define LOG(level,esp)		\
{				\
	Log l(level);	\
	if(l) l() << esp << std::endl; \
}

class Log
{
private:
	static sem_t mutex;
	static LOG_LEVEL log_level;
	LOG_LEVEL cur_level;
	bool valid;
	static const char* level_names[];

public:
	Log(LOG_LEVEL l);
	~Log();
	std::ostream& operator()();
	operator bool() { return valid; }
	static void initLogging(LOG_LEVEL l);

};

#endif
