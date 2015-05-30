#ifndef INCLUDE_UDSCOMWORKER_HPP_
#define INCLUDE_UDSCOMWORKER_HPP_

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


#include "JsonRPC.hpp"
#include "RPCMsg.hpp"
#include "WorkerInterface.hpp"
#include "WorkerThreads.hpp"
#include "Plugin.hpp"
#include "Error.hpp"
#include "LogUnit.hpp"



using namespace std;


class ConnectionContext;


/**
 * \class UdsComUnit
 * UdComUnit makes it possible to communicate to plugins via unix domain socket. Like TcpComUnit it will be managed
 * by the corresponding instance of ConnectionContext. It's receive/send system is similiar to the one of TcpComUnit.
 * UdsComUnit however saves information about the plugin connected and of course can try to connect to a plugin.
 * With the transmit methods it is possible to send data to the plugin, if it is connected. Also it can receive data
 * from the plugin and will forward it to the corresponding ConnectionContext.
 */
class UdsComWorker : public WorkerInterface<RPCMsg>, public WorkerThreads, public LogUnit{

	public:

		/**
		 * Constructor.
		 * \param context The overlying ConnectionContext.
		 * \param plugin Pointer to plugin instance of a registered plugin (which means, known to RSD).
		 */
		UdsComWorker(ConnectionContext* context, Plugin* plugin);

		/**
		 * Destructor.
		 */
		virtual ~UdsComWorker();

		/**
		 * \return Return the name of the corresponding plugin (maybe connected).
		 */
		string* getPluginName(){return plugin->getName();}

		/**
		 * \return Return the unique id of the corresponding plugin (maybe connected).
		 */
		int getPluginNumber(){return plugin->getPluginNumber();}

		/**
		 * \return Returns true if this UdsComUnit is deletable ,which means that the connected plugin closed the connection.
		 */
		bool isDeletable(){return deletable;}


		/**
		 * Trys to connect to the plugin, whose information was given through the constructor.
		 * If the connect is successful it will also create a thread for listening and working with data.
		 * Should the connect or the creation of the worker/listener thread fail, a instance of PluginError will be thrown.
		 */
		void tryToconnect();



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
		int transmit(RPCMsg* msg);


	private:

		/*! Overlying ConnectionContext instance, which mangages outgoing data and receives incomming data.*/
		ConnectionContext* context;
		/*! Contains information about the plugin which this UdsComUnit can connect.
		 *  This is just a pointer to the same instance of plugin which RSD has.*/
		Plugin* plugin;

		//information about the unix domain socket
		struct sockaddr_un address;
		socklen_t addrlen;
		int optionflag;


		/**
		 * This method is used if this UdsComUnit receives data from a plugin.
		 * First it will forward the message to processMsg() of the ConnectionContext.
		 * If an exception was thrown, it will call handleIncorrectPluginResponse of the ConnectionContext,
		 * which will create a valid json rpc error response for the plugin and send it to that.
		 * \param data A RsdMsg containing a json rpc request or response.
		 */
		void routeBack(RPCMsg* data);


		virtual void thread_listen();

		virtual void thread_work();

};

#endif /* INCLUDE_UDSWORKER_HPP_ */
