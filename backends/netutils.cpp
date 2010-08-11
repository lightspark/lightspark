/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "swf.h"
#include "netutils.h"
#include "compat.h"
#include <string>
#include <iostream>
#ifdef ENABLE_CURL
#include <curl/curl.h>
#endif

using namespace lightspark;
extern TLSDATA SystemState* sys;

Downloader* CurlDownloadManager::download(const tiny_string& u)
{
	CurlDownloader* curlDownloader=new CurlDownloader(u);
	sys->addJob(curlDownloader);
	return curlDownloader;
}

void CurlDownloadManager::destroy(Downloader* d)
{
	d->wait();
	delete d;
}

Downloader::~Downloader()
{
	sem_destroy(&available);
	sem_destroy(&mutex);
	sem_destroy(&terminated);
}

Downloader::Downloader():buffer(NULL),len(0),tail(0),waiting(false),failed(false)
{
	sem_init(&available,0,0);
	sem_init(&mutex,0,1);
	sem_init(&terminated,0,0);
}

void Downloader::setLen(uint32_t l)
{
	len=l;
	assert_and_throw(buffer==NULL);
	buffer=new uint8_t[len];
	setg((char*)buffer,(char*)buffer,(char*)buffer);
}

void Downloader::setFailed()
{
	sem_wait(&mutex);
	failed=true;
	if(waiting) //If we are waiting for some bytes, gives up and return EOF
		sem_post(&available);
	else
		sem_post(&mutex);
}

void Downloader::append(uint8_t* buf, uint32_t added)
{
	if(added==0)
		return;
	if((tail+added)>len)
		throw RunTimeException("Downloaded file is too big");
	sem_wait(&mutex);
	memcpy(buffer + tail,buf,added);
	tail+=added;
	if(waiting)
	{
		waiting=false;
		sem_post(&available);
	}
	else
		sem_post(&mutex);
}

void Downloader::stop()
{
	failed=true;
	sem_post(&available);
}

void Downloader::wait()
{
	sem_wait(&terminated);
}

Downloader::int_type Downloader::underflow()
{
	sem_wait(&mutex);
	assert_and_throw(gptr()==egptr());
	unsigned int firstIndex=tail;

	if((buffer+tail)==(uint8_t*)egptr()) //We have no more bytes
	{
		if(failed || (tail==len && len!=0))
		{
			sem_post(&mutex);
			return -1;
		}
		else //More bytes should follow
		{
			waiting=true;
			sem_post(&mutex);
			sem_wait(&available);
			if(failed)
			{
				sem_post(&mutex);
				return -1;
			}
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
	//Cast to unsigned, otherwise 0xff would become eof
	return (unsigned char)buffer[firstIndex];
}

Downloader::pos_type Downloader::seekpos(pos_type pos, std::ios_base::openmode mode)
{
	assert_and_throw(mode==std::ios_base::in);
	sem_wait(&mutex);
	setg((char*)buffer,(char*)buffer+pos,(char*)buffer+tail);
	sem_post(&mutex);
	return pos;
}

Downloader::pos_type Downloader::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode mode)
{
	assert_and_throw(mode==std::ios_base::in);
	assert_and_throw(off==0);
	pos_type ret=gptr()-eback();
	return ret;
}

CurlDownloader::CurlDownloader(const tiny_string& u)
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
	url=tmp2;
}

void CurlDownloader::threadAbort()
{
	Downloader::stop();
}

void CurlDownloader::execute()
{
	if(url.len()==0)
	{
		setFailed();
		return;
	}
#ifdef ENABLE_CURL
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	if(curl)
	{
		std::cout << url << std::endl;
		curl_easy_setopt(curl, CURLOPT_URL, url.raw_buf());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		res = curl_easy_perform(curl);
		if(res!=0)
			setFailed();
		curl_easy_cleanup(curl);
	}
	else
		setFailed();
#else
	//ENABLE_CURL not defined
	setFailed();
	LOG(LOG_ERROR,"CURL not enabled in this build. Downloader will always fail.");
#endif
	sem_post(&(Downloader::terminated));
}

int CurlDownloader::progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	CurlDownloader* th=static_cast<CurlDownloader*>(clientp);
	return th->aborting || th->failed;
}

size_t CurlDownloader::write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	CurlDownloader* th=static_cast<CurlDownloader*>(userp);
	size_t added=size*nmemb;
	th->append((uint8_t*)buffer,added);
	return added;
}

size_t CurlDownloader::write_header(void *buffer, size_t size, size_t nmemb, void *userp)
{
	CurlDownloader* th=static_cast<CurlDownloader*>(userp);
	char* headerLine=(char*)buffer;

	std::cerr << "CURL header: " << headerLine;

	if(strncmp(headerLine,"HTTP/1.1 4",10)==0) //HTTP error, let's fail
		th->setFailed();
//	else if(strncmp(headerLine,"HTTP/1.1 3",10)==0); //HTTP redirect
	else if(strncmp(headerLine,"Content-Length: ",16)==0)
	{
		//Now read the length and allocate the byteArray
		th->setLen(atoi(headerLine+16));
	}
	return size*nmemb;
}

