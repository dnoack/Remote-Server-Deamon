
#include "ComPointMock.hpp"



ComPointMock::~ComPointMock()
{
	clear();
}


int ComPointMock::transmit(char* data, int size)
{
	if(size < MOCK_BUFFER_SIZE)
	{
		memcpy(buffer, data, size);
	}
	return size;
}


int ComPointMock::transmit(const char* data, int size)
{
	if(size < MOCK_BUFFER_SIZE)
	{
		 memcpy(buffer, data, size);
	}
	return size;
}

int ComPointMock::transmit(RPCMsg* msg)
{
	string* data = msg->getContent();
	if(data->size() < MOCK_BUFFER_SIZE)
	{
		memcpy(buffer, data->c_str(), data->size());
	}
	return data->size();
}
