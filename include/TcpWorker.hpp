#ifndef INCLUDE_TCPWORKER_HPP_
#define INCLUDE_TCPWORKER_HPP_


#include "errno.h"
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <list>

#include <pthread.h>
#include "signal.h"

#include "WorkerInterface.hpp"
#include "WorkerThreads.hpp"
#include "Error.hpp"
#include "Utils.h"
#include "RPCMsg.hpp"
#include "LogUnit.hpp"


class ConnectionContext;


/**
 * \class TcpWorker
 * TcpWorker is the comunication unit between client and server over a tcp socket.
 * It is responsible for sending and receiving data over a tcp socket which was accepted by RSD accept thread.
 * The creation of a TcpWorker will be done by a instance of ConnectionContext, which will access this TcpWorker
 * for communication with the client. Also the thread_work() function (which runs in a separated thread) will forward
 * messages to the corresponding ConnectionContext.
 * \note implements WorkerInterface, WorkerThreads and LogUnit
 * \author David Noack
 */
class TcpWorker : public WorkerInterface<RPCMsg>, public WorkerThreads, public LogUnit{

	public:

		/**
		 * Constructor.
		 * \param context Pointer to the corresponding ConnectionContext, TcpWorker will forward incomming data to this context.
		 * \param tcpWorker Pointer to a pointer of this instance, it will be initialized with this, so that ConnectionContext can use this instance of TcpWorker.
		 * \param socket Valid tcp socket fd, which was originally accepted by RSd and forwarded by ConnectionContext.
		 */
		TcpWorker(ConnectionContext* context, TcpWorker** tcpWorker, int socket);

		/**
		 * Destructor.
		 */
		virtual ~TcpWorker();


		/**
		 * Sends a message through the tcp-connections to the client.
		 * \param msg Pointer to a constant character array, which has to be send.
		 * \param size Length of data.
		 * \return On success it return the number of bytes which where send, on fail it return -1 (errno is set).
		*/
		int transmit(const char* data, int size);

		/**
		 * Sends a message through the tcp-connections to the client.
		 * \param msg Pointer to RsdMsg, which has to be send.
		 * \return On success it return the number of bytes which where send, on fail it return -1 (errno is set).
		*/
		int transmit(RPCMsg* data);

	private:

		/*! Corresponding ConnectionContext, this Context will handle our incomming messages.*/
		ConnectionContext* context;


		/*! Receives data over a tcp socket which was accepted trough RSD. A new RsdMsg with the content
		 * of the received data will be created and put into the receiveQueue (WorkerInterface).
		 * \note This function will run in a separate thread, which is created and managed through WorkerThreads.
		 */
		virtual void thread_listen();

		/*!
		 * Gets RsdMsgs from receiveQueue (WorkerInterface) and sends them to ConnectionContext::processMsg().
		 * A RsdMsg will be popped from the receiveQueue if it is in proccess and as long as there are still
		 * new Messages in the receiveQueue, this function will handle them. Once the receiQueue is empty, it
		 * will wait for signal SIGUSR1.
		 * \note This function will run in a separate thread, which is created and managed through WorkerThreads.
		 */
		virtual void thread_work();
};


#endif /* INCLUDE_TCPWORKER_HPP_ */
