
#include "WorkerInterfaceMock.hpp"


WorkerInterfaceMock::WorkerInterfaceMock()
{
	clear();
}


WorkerInterfaceMock::~WorkerInterfaceMock()
{
	clear();
}


int WorkerInterfaceMock::transmit(char* data, int size)
{
	if(size < MOCK_BUFFER_SIZE)
	{
		memcpy(buffer, data, size);
	}
	return size;
}


int WorkerInterfaceMock::transmit(const char* data, int size)
{
	if(size < MOCK_BUFFER_SIZE)
	{
		 memcpy(buffer, data, size);
	}
	return size;
}

int WorkerInterfaceMock::transmit(RPCMsg* msg)
{
	string* data = msg->getContent();
	if(data->size() < MOCK_BUFFER_SIZE)
	{
		memcpy(buffer, data->c_str(), data->size());
	}
	return data->size();
}
