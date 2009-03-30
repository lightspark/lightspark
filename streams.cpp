#include "streams.h"

using namespace std;

sync_stream::sync_stream():head(0),tail(0),wait(-1),buf_size(1024*1024)
{
	printf("syn stream\n");
	buffer= new char[buf_size];
	sem_init(&mutex,0,1);
	sem_init(&empty,0,0);
	sem_init(&full,0,0);
	sem_init(&ready,0,0);
}

streamsize sync_stream::xsgetn ( char * s, streamsize n )
{
	sem_wait(&mutex);
	if((tail-head+buf_size)%buf_size<n)
	{
		wait=(head+n)%buf_size;
		sem_post(&mutex);
		sem_wait(&ready);
		wait=-1;
	}
	if(head+n>buf_size)
	{
		int i=buf_size-head;
		memcpy(s,buffer+head,i);
		memcpy(s+i,buffer,n-i);
	}
	else
		memcpy(s,buffer+head,n);
	head+=n;
	head%=buf_size;
	sem_post(&mutex);
	return n;
}


streamsize sync_stream::xsputn ( const char * s, streamsize n )
{
	sem_wait(&mutex);
	if((head-tail+buf_size-1)%buf_size<n)
	{
		//throw "no space";
		n=(head-tail+buf_size-1)%buf_size;
	}
	if(tail+n>buf_size)
	{
		int i=buf_size-tail;
		memcpy(buffer+tail,s,i);
		memcpy(buffer,s+i,n-i);
	}
	else
		memcpy(buffer+tail,s,n);
	tail+=n;
	tail%=buf_size;
	if(wait>0 && tail>=wait)
		sem_post(&ready);
	else
		sem_post(&mutex);
	return n;
}

std::streampos sync_stream::seekpos ( std::streampos sp, std::ios_base::openmode which )
{
	printf("puppa1\n");
	abort();
}

std::streampos sync_stream::seekoff ( std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which )
{
	printf("puppa2\n");
	abort();
}

std::streamsize sync_stream::showmanyc( )
{
	printf("puppa3\n");
	abort();
}
