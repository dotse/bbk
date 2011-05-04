#ifndef __DOWNLOADER_H
#define __DOWNLOADER_H

#include "curl.h"

#include <string>

class Downloader
{
public:
	Downloader(void);
	~Downloader(void);

	bool downloadFile(const std::string url);
	std::string contentType() const;
	unsigned int contentLength() const;
	char*	content() const;
	long	GetResponseCode();
private:

	static size_t Fetch(void* ptr, size_t size, size_t nmemb, void* stream);

	CURL* curlHandle;
	char*	data;
	long	response_code;
	unsigned int length;
	unsigned int allocatedLength;
	std::string type;
};


#endif