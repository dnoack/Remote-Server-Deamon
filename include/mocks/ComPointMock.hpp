
#ifndef WORKERINTERFACEMOCK_HPP_
#define WORKERINTERFACEMOCK_HPP_

#define MOCK_BUFFER_SIZE 2048


#include "RPCMsg.hpp"
#include "ComPoint.hpp"


class ComPointMock : public ComPoint{

	public:
		ComPointMock(int socket, ProcessInterface* pInterface, int uniqueID, bool viceVersaRegister = true, bool startInstant = true):
			ComPoint(socket, pInterface, uniqueID, viceVersaRegister, startInstant)
		{
			clear();
		}

		virtual ~ComPointMock();


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

		void socketToFDSET()
		{
			FD_ZERO(&rfds);
			FD_SET(currentSocket, &rfds);
		}

	private:
		char buffer[MOCK_BUFFER_SIZE];
};



#endif

