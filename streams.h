#include <streambuf>
#include <semaphore.h>

class sync_stream: public std::streambuf
{
public:
	sync_stream();
	std::streamsize xsgetn ( char * s, std::streamsize n );
	std::streamsize xsputn ( const char * s, std::streamsize n );
	std::streampos seekpos ( std::streampos sp, std::ios_base::openmode which/* = std::ios_base::in | std::ios_base::out*/ );
	std::streampos seekoff ( std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which);/* = 
			std::ios_base::in | std::ios_base::out );*/
	std::streamsize showmanyc( );
private:
	char* buffer;
	int head;
	int tail;
	sem_t mutex;
	sem_t empty;
	sem_t full;
	sem_t ready;
	int wait;

	const int buf_size;
};
