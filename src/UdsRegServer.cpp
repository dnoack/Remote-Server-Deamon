
#include "RSD.hpp"
#include "UdsRegServer.hpp"


int UdsRegServer::connection_socket;

list<ComPoint*> UdsRegServer::workerList;
pthread_mutex_t UdsRegServer::wLmutex;

struct sockaddr_un UdsRegServer::address;
socklen_t UdsRegServer::addrlen;



UdsRegServer::UdsRegServer( const char* udsFile)
{
	accept_thread_active = false;
	optionflag = 1;
	accepter = 0;
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, udsFile, strlen(udsFile));
	addrlen = sizeof(address);

	connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if(connection_socket < 0)
		throw Error (-206 , "Could not create connection_socket", strerror(errno));

	if( pthread_mutex_init(&wLmutex, NULL) != 0)
		throw Error (-207 , "Could not init wLmutex", strerror(errno));

	unlink(udsFile);
	//if( unlink(udsFile) != 0 )
		//throw Error(-200, "Error while unlinking udsFile", strerror(errno));

	if( setsockopt(connection_socket, SOL_SOCKET, SO_REUSEADDR, &optionflag, sizeof(optionflag)) != 0 )
		throw Error(-201, "Error while setting socket option", strerror(errno));

	if(bind(connection_socket, (struct sockaddr*)&address, addrlen) != 0)
		throw Error (-202, "Error while binding socket", strerror(errno));
}


UdsRegServer::~UdsRegServer()
{
	if(accepter != 0)
		pthread_cancel(accepter);

	deleteWorkerList();
	close(connection_socket);

	pthread_mutex_destroy(&wLmutex);
}



void UdsRegServer::thread_accept()
{
	int new_socket = 0;

	if( listen(connection_socket, 5) != 0)
		throw Error(-203, "Could not listen to socket", strerror(errno));

	accept_thread_active = true;

	while(accept_thread_active)
	{
		new_socket = accept(connection_socket, (struct sockaddr*)&address, &addrlen);
		if(new_socket > 0)
		{
			pushWorkerList(new_socket);
		}
	}
}



void UdsRegServer::pushWorkerList(int new_socket)
{
	Registration* registry = new Registration();
	pthread_mutex_lock(&wLmutex);
		workerList.push_back(new ComPoint(new_socket, registry));
	pthread_mutex_unlock(&wLmutex);
}


void UdsRegServer::checkForDeletableWorker()
{
	pthread_mutex_lock(&wLmutex);
	Registration* registry = NULL;
	list<ComPoint*>::iterator i = workerList.begin();
	while(i != workerList.end())
	{
		if((*i)->isDeletable())
		{
			registry = (Registration*)(*i)->getProcessInterface();
			RSD::deletePlugin(registry->getPluginName());
			delete  *i;
			i = workerList.erase(i);
			dyn_print("UdsRegServer: UdsRegWorker was deleted.\n");
		}
		else
			++i;
	}
	pthread_mutex_unlock(&wLmutex);
}


void UdsRegServer::deleteWorkerList()
{
	pthread_mutex_lock(&wLmutex);
	list<ComPoint*>::iterator i = workerList.begin();

	while(i != workerList.end())
	{
		delete *i;
		i = workerList.erase(i);
	}
	pthread_mutex_unlock(&wLmutex);
}




void UdsRegServer::start()
{
	try
	{
		StartAcceptThread();
		if(wait_for_accepter_up() < 0)
			throw Error(-205, "Could not start Accept thread in time.");
	}
	catch(Error &e)
	{
		throw;
	}
}
