#ifndef LOGGER_H
#define LOGGER_H

#include <semaphore.h>
#include <iostream>

enum LOG_LEVEL { LOG_NO_INFO=0, LOG_ERROR=1, LOG_NOT_IMPLEMENTED=2,LOG_CALLS=3,LOG_TRACE=4};

#ifndef NDEBUG
#define LOG(level,esp)					\
{							\
	if(level<=Log::getLevel())			\
	{						\
		Log l(level);				\
		l() << esp << std::endl;		\
	}						\
}
#else
#define LOG(level,esp) {}
#endif

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
