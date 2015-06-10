/*
 * ProcessInterfaceMock.hpp
 *
 *  Created on: 03.06.2015
 *      Author: dave2
 */

#ifndef INCLUDE_MOCKS_PROCESSINTERFACEMOCK_HPP_
#define INCLUDE_MOCKS_PROCESSINTERFACEMOCK_HPP_

#include "ProcessInterface.hpp"


class ProcessInterfaceMock : public ProcessInterface
{
	public:
		ProcessInterfaceMock()
		{
			msgReceived = false;
			receivedMsg = NULL;
		};

		virtual ~ProcessInterfaceMock(){};


		bool msgReceived;
		RPCMsg* receivedMsg;

		void reset()
		{
			msgReceived = false;
			delete receivedMsg;
		}

	private:


		void process(RPCMsg* msg)
		{
			receivedMsg = msg;
			msgReceived = true;
		}
};



#endif /* INCLUDE_MOCKS_PROCESSINTERFACEMOCK_HPP_ */
