
#include "RSD.hpp"
#include "UdsRegWorker.hpp"

using namespace rapidjson;


UdsRegWorker::UdsRegWorker(int socket)
{
	this->currentSocket = socket;
	this->logInfo.logName = "UdsRegWorker: ";
	this->logInfoIn.logName = "REG IN: ";
	this->logInfoOut.logName = "REG OUT: ";
	this->registration = new Registration(this);

	StartWorkerThread();

	if(wait_for_listener_up() != 0)
			throw Error(-400, "Timeout for Thread creation");
}


UdsRegWorker::~UdsRegWorker()
{

	pthread_cancel(getListener());
	pthread_cancel(getWorker());

	WaitForListenerThreadToExit();
	WaitForWorkerThreadToExit();

	delete registration;
	deleteReceiveQueue();
}


string* UdsRegWorker::getPluginName()
{
	return registration->getPluginName();
}


void UdsRegWorker::thread_work()
{
	worker_thread_active = true;
	RPCMsg* msg = NULL;

	configSignals();
	StartListenerThread();


	while(worker_thread_active)
	{
		sigwait(&sigmask, &currentSig);
		switch(currentSig)
		{
			case SIGUSR1:
				while(getReceiveQueueSize() > 0)
				{
					try
					{
						msg = receiveQueue.back();
						log(logInfoIn, msg->getContent());
						popReceiveQueueWithoutDelete();
						registration->processMsg(msg);
					}
					catch(...)
					{
						dyn_print("Unkown Exception.\n");
					}

				}
			break;

			default:
				dyn_print("UdsRegWorker: unkown signal \n");
				break;
		}
	}
	close(currentSocket);
}



void UdsRegWorker::thread_listen()
{
	listen_thread_active = true;
	string* content = NULL;
	int retval = 0;
	pthread_t worker_thread = getWorker();


	FD_ZERO(&rfds);
	FD_SET(currentSocket, &rfds);


	while(listen_thread_active)
	{
		memset(receiveBuffer, '\0', BUFFER_SIZE);

		//hier nur hin wenn nichts mehr im buffer ist und auf keine teile einer nachricht gewartet wird.
		recvSize = recv(currentSocket , receiveBuffer, BUFFER_SIZE, 0);
		if(recvSize > 0)
		{
			//copy receiveBuffer to a clean msgBuffer
			msgBufferSize = recvSize;
			msgBuffer = new char[msgBufferSize];
			memset(msgBuffer, '\0', msgBufferSize);
			memcpy(msgBuffer, receiveBuffer, msgBufferSize);

			//As long as there is data in the msgBuffer
			while(msgBufferSize > 0)
			{
				//headersize = 1 byte tagfield + 4 byte lengthfield
				if(msgBufferSize > HEADER_SIZE)
				{
					messageSize = readHeader(msgBuffer);

					//header ok ???
					if(messageSize > -1)
					{
						//Is there at least one complete message in msgBuffer ?
						if(msgBufferSize >= messageSize+HEADER_SIZE)
						{
							//add first complete msg of msgbuffer to the receivequeue and signal the worker
							content = new string(&msgBuffer[HEADER_SIZE], messageSize);
							pushReceiveQueue(new RsdMsg(0, content));
							pthread_kill(worker_thread, SIGUSR1);

							//Is there more data ?
							if(msgBufferSize > messageSize+HEADER_SIZE)
							{
								//copy rest of data to a new clean msgBuffer
								msgBufferSize = msgBufferSize - (messageSize+HEADER_SIZE);
								tempBuffer = new char[msgBufferSize];
								memset(tempBuffer, '\0', msgBufferSize);
								memcpy(tempBuffer, &(msgBuffer[messageSize+HEADER_SIZE]), msgBufferSize);
								delete[] msgBuffer;
								msgBuffer = tempBuffer;
							}
							else
							{
								delete[] msgBuffer;
								msgBufferSize = 0;
							}
						}
						//message is not complete, wait for the rest
						else
						{
							waitForFurtherData();
						}
					}
					else
					{
						delete[] msgBuffer;
						msgBufferSize = 0;
						//TODO: send msg back, that something went wrong and the message was not correctly received
					}
				}
				//not even the header was complete, wait for the rest
				else
				{
					waitForFurtherData();
				}
			}
		}
		else
		{
			deletable = true;
			listen_thread_active = false;
		}
	}
}


int UdsRegWorker::transmit(const char* data, int size)
{
	int sendCount = 0;
	createHeader(headerOut, size);
	log(logInfoOut, data);
	sendCount = send(currentSocket, headerOut, HEADER_SIZE, 0);
	sendCount += send(currentSocket, data, size, 0);
	return sendCount;
};


int UdsRegWorker::transmit(RsdMsg* msg)
{
	int sendCount = 0;
	createHeader(headerOut, msg->getContent()->size());
	log(logInfoOut, msg->getContent());
	sendCount = send(currentSocket, headerOut, HEADER_SIZE, 0);
	sendCount += send(currentSocket, msg->getContent()->c_str(), msg->getContent()->size(), 0);
	return sendCount;
};


