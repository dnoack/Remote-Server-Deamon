
#ifndef WORKERINTERFACEMOCK_HPP_
#define WORKERINTERFACEMOCK_HPP_

#define MOCK_BUFFER_SIZE 2048


#include "RPCMsg.hpp"
#include "WorkerInterface.hpp"


class WorkerInterfaceMock : public WorkerInterface<RPCMsg>{

	public:
		WorkerInterfaceMock();


		virtual ~WorkerInterfaceMock();


		int transmit(char* data, int size);
		int transmit(const char* data, int size);
		int transmit(RPCMsg* msg);

		char* getBuffer()
		{
			return &buffer[0];
		}

		void clear()
		{
			memset(buffer, '\0', MOCK_BUFFER_SIZE);
		}

	private:
		char buffer[MOCK_BUFFER_SIZE];
};



#endif

