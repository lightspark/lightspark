#include "logger.h"

sem_t Log::mutex;
LOG_LEVEL Log::log_level=NO_INFO;
const char* Log::level_names[]={"INFO","ERROR","NOT_IMPLEMENTED","TRACE"};

Log::Log(LOG_LEVEL l)
{
	if(l<=log_level)
	{
		cur_level=l;
		valid=true;
		sem_wait(&mutex);
	}
	else
		valid=false;
}

Log::~Log()
{
	if(valid)
		sem_post(&mutex);
}

std::ostream& Log::operator()()
{
	std::cout << level_names[cur_level] << ": ";
	return std::cout;
}

void Log::initLogging(LOG_LEVEL l)
{
	sem_init(&mutex,0,1);
	log_level=l;
}
