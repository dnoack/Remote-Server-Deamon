#ifndef INCLUDE_UDSREGSERVER_HPP_
#define INCLUDE_UDSREGSERVER_HPP_


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

#include "AcceptThread.hpp"
#include "JsonRPC.hpp"
#include <ComPoint.hpp>
#include "Error.hpp"
#include "Utils.h"
#include "Registration.hpp"



/**
 * \class UdsRegServer
 * UdsRegServer will listen on a specific unix domain socket file and accept incomming
 * connections within a separated accept-thread. It also manages all accepted connections
 * in a list of UdsRegWorkers.
 */
class UdsRegServer : public AcceptThread{

	public:

		/**
		 * Constructor.
		 * \param udsFile Path to unix domain socket file, which will be used for all registration processes.
		 */
		UdsRegServer(const char* udsFile);

		/**
		 * Destructor.
		 */
		virtual ~UdsRegServer();

		/**
		 * Removes all RegWorker from the intern list and also deallocates them.
		 */
		void deleteWorkerList();


		/**
		 * Creates a new thread for uds_accept. If creating this thread fails, a
		 * PluginError exception will be thrown.
		 */
		void start();


		/**
		 * Checks the intern list for deletable RegWorker. If there is one, it will be
		 * removed and deallocated from the intern list + it the corresponding plugin
		 * will be removed from the RSD list for registered plugins.
		 */
		void checkForDeletableWorker();


	private:

		/*! Unix domain socket for registering plugins.*/
		static int connection_socket;

		/*!list of pthread ids with all the active RegWorker.*/
		static list<ComPoint*> workerList;
		/*! Mutex for protecting the intern list of UdsRegWorkers.*/
		static pthread_mutex_t wLmutex;

		/*! Address for conection_socket.*/
		static struct sockaddr_un address;
		/*! Address length for connection_socket.*/
		static socklen_t addrlen;

		/*! Thread id for the accept-thread.*/
		pthread_t accepter;
		/*! optionflag for connection_socket.*/
		int optionflag;


		/* Accepts new incommingconnection over the unix domain registry file.
		 * A new accepted connection results in a new UdsRegWorker, which will be added to the intern list.
		 * \note This function has to be started in a separated thread.
		 */
		virtual void thread_accept();

		/*
		 * Creates a new instance of UdsRegWorker and adds it to the intern list of UdsRegWorkers.
		 * \param new_socket The socket fd which was returned by accept.
		 * \note This function uses uLmutex to protect the intern list.
		 */
		 void pushWorkerList(int new_socket);

};


#endif /* INCLUDE_UDSSERVER_HPP_ */
