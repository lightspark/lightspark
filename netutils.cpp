/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "netutils.h"
#include <curl/curl.h>
#include <string>

using namespace lightspark;

CurlDownloader::CurlDownloader(const tiny_string& u):buffer(NULL),len(0),tail(0),failed(false),waiting(false)
{
	//TODO: Url encode the string
	std::string tmp2;
	tmp2.reserve(u.len()*2);
	for(int i=0;i<u.len();i++)
	{
		if(u[i]==' ')
		{
			char buf[4];
			sprintf(buf,"%%%x",(unsigned char)u[i]);
			tmp2+=buf;
		}
		else
			tmp2.push_back(u[i]);
	}
	url=tmp2.c_str();

	sem_init(&available,0,0);
	sem_init(&mutex,0,1);
}

void CurlDownloader::execute()
{
	if(url.len()==0)
	{
		setFailed();
		return;
	}
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url.raw_buf());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
		res = curl_easy_perform(curl);
		if(res!=0)
			setFailed();
		curl_easy_cleanup(curl);
	}
	else
		setFailed();

	return;
}

bool CurlDownloader::download()
{
	execute();
	return !failed;
}

size_t CurlDownloader::write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	CurlDownloader* th=static_cast<CurlDownloader*>(userp);
	sem_wait(&th->mutex);
	memcpy(th->buffer + th->tail,buffer,size*nmemb);
	th->tail+=(size*nmemb);
	if(th->waiting)
	{
		th->waiting=false;
		assert(size*nmemb);
		sem_post(&th->available);
	}
	else
		sem_post(&th->mutex);
	return size*nmemb;
}

size_t CurlDownloader::write_header(void *buffer, size_t size, size_t nmemb, void *userp)
{
	CurlDownloader* th=static_cast<CurlDownloader*>(userp);
	char* headerLine=(char*)buffer;
	if(strncmp(headerLine,"HTTP/1.1 4",10)==0) //HTTP error, let's fail
		th->setFailed();
	else if(strncmp(headerLine,"Content-Length: ",16)==0)
	{
		//Now read the length and allocate the byteArray
		assert(th->buffer==NULL);
		th->len=atoi(headerLine+16);
		th->buffer=new uint8_t[th->len];
		th->setg((char*)th->buffer,(char*)th->buffer,(char*)th->buffer);
	}
	return size*nmemb;
}

void CurlDownloader::setFailed()
{
	sem_wait(&mutex);
	failed=true;
	if(waiting) //If we are waiting for some bytes, gives up and return EOF
		sem_post(&available);
	else
		sem_post(&mutex);
}

CurlDownloader::int_type CurlDownloader::underflow()
{
	sem_wait(&mutex);
	assert(gptr()==egptr());
	unsigned int firstIndex=tail;

	if((buffer+tail)==(uint8_t*)egptr()) //We have no more bytes
	{
		if(failed)
		{
			sem_post(&mutex);
			return -1;
		}
		else
		{
			waiting=true;
			sem_post(&mutex);
			sem_wait(&available);
		}
	}

	//The current pointer has to be saved
	char* cur=gptr();

	if(failed)
	{
		sem_post(&mutex);
		return -1;
	}

	//We have some bytes now, let's use them
	setg((char*)buffer,cur,(char*)buffer+tail);
	sem_post(&mutex);
	//Cast to signed, otherwise 0xff would become eof
	return (unsigned char)buffer[firstIndex];
}

CurlDownloader::pos_type CurlDownloader::seekpos(pos_type pos, std::ios_base::openmode mode)
{
	assert(mode==std::ios_base::in);
	sem_wait(&mutex);
	setg((char*)buffer,(char*)buffer+pos,(char*)buffer+tail);
	sem_post(&mutex);
	return pos;
}

CurlDownloader::pos_type CurlDownloader::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode mode)
{
	assert(mode==std::ios_base::in);
	assert(off==0);
	pos_type ret=gptr()-eback();
	return ret;
}
