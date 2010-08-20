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
#include <fstream>
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

Downloader* LocalDownloadManager::download(const tiny_string& u)
{
	LocalDownloader* localDownloader=new LocalDownloader(u);
	sys->addJob(localDownloader);
	return localDownloader;
}

void LocalDownloadManager::destroy(Downloader* d)
{
	d->wait();
	delete d;
}

Downloader::~Downloader()
{
	if(buffer != NULL)
	{
		free(buffer);
	}
	sem_destroy(&available);
	sem_destroy(&mutex);
	sem_destroy(&terminated);
}

Downloader::Downloader():buffer(NULL),allowBufferRealloc(false),len(0),tail(0),waiting(false),failed(false)
{
	sem_init(&available,0,0);
	sem_init(&mutex,0,1);
	sem_init(&terminated,0,0);
}

void Downloader::setLen(uint32_t l)
{
	len=l;
	//Create buffer
	if(buffer == NULL)
	{
		buffer= (uint8_t*) calloc(len, sizeof(uint8_t));
		setg((char*)buffer,(char*)buffer,(char*)buffer);
	}
	//Realloc, only if allowed
	else if(getAllowBufferRealloc())
	{
		//Remember the relative positions of the input pointers
		intptr_t curPos = (intptr_t) (gptr()-eback());
		intptr_t curLen = (intptr_t) (egptr()-eback());
		buffer = (uint8_t*) realloc(buffer, len);
		//Do some pointer arithmetic to point the input pointers to the right places in the new buffer
		setg((char*)buffer,(char*)(buffer+curPos),(char*)(buffer+curLen));
	}
	else
	{
		assert_and_throw(buffer==NULL);
	}
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
	{
		//Only grow the buffer when allowed
		if(getAllowBufferRealloc())
		{
			LOG(LOG_NO_INFO, "Downloaded file bigger than buffer: " << 
					tail+added << ">" << len << ", reallocating buffer.");
			//TODO: Confirm whats best: allocate more memory at a time, at the risk of allocating too much
			//			or allocate exactly the amount needed, at the risk of having to reallocate a lot
			setLen(len+4096);
		}
		//The real data is longer than the provided Content-Length header
		else
		{
			throw RunTimeException("Downloaded file is too big");
		}
	}
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
	requestStatus = 0;
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
		//Its probably a good idea to limit redirections, 100 should be more than enough
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
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
	if(th->getLen() == 0)
	{
		//If the HTTP request doesn't contain a Content-Length header, allow growing the buffer
		th->setAllowBufferRealloc(true);
	}
	if(th->getRequestStatus() != 3)
	{
		th->append((uint8_t*)buffer,added);
	}
	return added;
}

size_t CurlDownloader::write_header(void *buffer, size_t size, size_t nmemb, void *userp)
{
	CurlDownloader* th=static_cast<CurlDownloader*>(userp);
	char* headerLine=(char*)buffer;

	std::cerr << "CURL header: " << headerLine;

	if(strncmp(headerLine,"HTTP/1.1 ",9)==0) 
	{
		char* status = new char[3];
		strncpy(status, headerLine+9, 3);
		th->setRequestStatus(atoi(status));
		delete[] status;
		if(th->getRequestStatus()/100 == 4 || th->getRequestStatus()/100 == 5 || 
				th->getRequestStatus()/100 == 6)
		//HTTP error or server error or proxy error, let's fail
		//TODO: don't we need to return the data anyway?
		{
			th->setFailed();
		}
		else if(th->getRequestStatus()/100 == 3); //HTTP redirect
		else if(th->getRequestStatus()/100 == 2); //HTTP OK
	}
	else if(strncmp(headerLine,"Content-Length: ",16)==0)
	{
		//Now read the length and allocate the byteArray
		//Only read the length when we're not redirecting
		if(th->getRequestStatus()/100 != 3)
		{
			th->setLen(atoi(headerLine+16));
		}
	}
	return size*nmemb;
}

LocalDownloader::LocalDownloader(const tiny_string& u) : url(u)
{
}

void LocalDownloader::threadAbort()
{
	Downloader::stop();
}

void LocalDownloader::execute()
{
	if(url.len()==0)
	{
		setFailed();
		return;
	}
	else
	{
		std::ifstream file;
		LOG(LOG_NO_INFO, "LocalDownloader::execute: reading local file: " << url.raw_buf());
		file.open(url.raw_buf(), std::ifstream::in);

		if(file.is_open())
		{
			file.seekg(0, std::ios::end);
			setLen(file.tellg());
			file.seekg(0, std::ios::beg);

			//TODO: Maybe this size should be a macro or something.
			size_t buffSize = 8192;
			char * buffer = new char[buffSize];

			bool failed = 0;
			while(!file.eof())
			{
				if(file.fail() || hasFailed())
				{
					failed = 1;
					break;
				}
				file.read(buffer, buffSize);
				append((uint8_t *) buffer, file.gcount());
			}
			if(failed)
			{
				setFailed();
				LOG(LOG_ERROR, "LocalDownloader::execute: reading from local file failed: " << url.raw_buf());
			}
			file.close();
			delete buffer;
		}
		else
		{
				setFailed();
				LOG(LOG_ERROR, "LocalDownloader::execute: could not open local file: " << url.raw_buf());
		}
	}
	sem_post(&(Downloader::terminated));
}

