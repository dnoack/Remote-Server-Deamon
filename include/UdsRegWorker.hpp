
#ifndef INCLUDE_UDSREGWORKER_HPP_
#define INCLUDE_UDSREGWORKER_HPP_

#include "errno.h"
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>


#include "JsonRPC.hpp"
#include "RsdMsg.hpp"
#include "WorkerInterface.hpp"
#include "WorkerThreads.hpp"
#include "Plugin.hpp"
#include "Registration.hpp"
#include "Plugin_Error.h"
#include "Utils.h"

class Registration;

/**
 * \class UdsRegWorker
 * UdsRegWorker is a communication endpoint between a plugin and UdsRegServer.
 * It can be used to receive and send data during the registration process of a plugin.
 */
class UdsRegWorker : public WorkerInterface<RsdMsg>, WorkerThreads{

	public:

		/**
		 * Constructor.
		 * \param socket Valid socket fd to a accepted conenction via the unix domain socket file for registration.
		 */
		UdsRegWorker(int socket);

		/**
		 * Destructor.
		 */
		virtual ~UdsRegWorker();


		/**
		 * \return The name of the plugin that is registered to this UdsRegWorker. This maybe NULL if no plugin is registered or
		 * the registration process is not complete.
		 */
		string* getPluginName();



		/**
		 * Sends a message through the uds-connections to the plugin.
		 * \param msg Pointer to a constant character array, which has to be send.
		 * \param size Length of data.
		 * \return On success it return the number of bytes which where send, on fail it return -1 (errno is set).
		*/
		int transmit(const char* data, int size);

		/**
		 * Sends a message through the uds-connections to the plugin.
		 * \param msg Pointer to RsdMsg, which has to be send.
		 * \return On success it return the number of bytes which where send, on fail it return -1 (errno is set).
		*/
		int transmit(RsdMsg* msg);


	private:

		/*! Contains the registration process.*/
		Registration* registration;


		virtual void thread_listen();

		virtual void thread_work();


};


#endif /* INCLUDE_UDSREGWORKER_HPP_ */
