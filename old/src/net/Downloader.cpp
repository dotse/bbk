#include "Downloader.h"
#include <iostream>
#include <cassert>

using namespace std;

const int kInitialBufferSize = 1024*1024;

Downloader::Downloader():
curlHandle(curl_easy_init()), data(NULL), length(0), allocatedLength(0)
{
}

Downloader::~Downloader() {
	curl_easy_cleanup(curlHandle);
	if (data) {
		free(data);
		data = NULL;
		allocatedLength = 0;
		length = 0;
	}
}

bool Downloader::downloadFile(const std::string url) {
	length = 0;

	//  set up transfer
	CURLcode curlErr = curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
	if (curlErr != CURLE_OK) {
		cerr << "got curl error on setopt url " << curlErr << endl;
		return false;
	}
	curlErr = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, Fetch);
	if (curlErr != CURLE_OK) {
		cerr << "got curl error on setopt func " << curlErr << endl;
		return false;
	}
	curlErr = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, this);
	if (curlErr != CURLE_OK) {
		cerr << "got curl error on setopt url " << curlErr << endl;
		return false;
	}

	//  perform transfer
	curlErr = curl_easy_perform(curlHandle);
	if (curlErr != CURLE_OK) {
		cerr << "got curl error on perform " << curlErr << endl;
		length = 0;
		return false;
	}

	char* contentType = NULL;
	curlErr = curl_easy_getinfo(curlHandle, CURLINFO_CONTENT_TYPE, &contentType);
	if (curlErr != CURLE_OK) {
		cerr << "got curl error on getopt for CONTENT_TYPE " << curlErr << endl;
		length = 0;
		return false;
	}
	else
	{
		type = string(contentType);
	}

	long rcode;
	curlErr = curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &rcode);
	if (curlErr != CURLE_OK) {
		cerr << "got curl error on getopt for CURLINFO_RESPONSE_CODE " << curlErr << endl;
		length = 0;
		return false;
	}
	else
	{
		response_code = rcode;
	}

	return true;
}

long Downloader::GetResponseCode()
{
	return response_code;
}

std::string Downloader::contentType() const {
	return type;
}

unsigned int Downloader::contentLength() const {
	return length;
}

char* Downloader::content() const {
	return data;
}

size_t Downloader::Fetch(void* ptr, size_t size, size_t nmemb, void* stream) {
	Downloader* handle = (Downloader*)stream;
	size_t newDataSize = size*nmemb;

	if (handle->data == NULL) {
		assert(handle->allocatedLength == 0);
		handle->data = (char*)calloc(1, kInitialBufferSize);
		handle->allocatedLength = kInitialBufferSize;
		handle->length = 0;
	}

	while (handle->length+newDataSize+1 > handle->allocatedLength) {
		assert(handle->allocatedLength > 0);
		handle->data = (char*)realloc(handle->data, handle->allocatedLength*2);
		handle->allocatedLength *= 2;
	}

	memcpy(handle->data + handle->length, ptr, newDataSize);
	handle->length += ((unsigned int)newDataSize);
	*(handle->data + handle->length) = '\0';
	return newDataSize;
}
