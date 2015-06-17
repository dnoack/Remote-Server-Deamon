
#include "RegServer.hpp"
#include "RSD.hpp"




RegServer::RegServer( const char* udsFile)
{
	accept_thread_active = false;
	optionflag = 1;
	address.sun_family = AF_UNIX;
	infoIn.logLevel = _LOG_INPUT;
	infoIn.logName = "IPC IN:";
	infoOut.logLevel = _LOG_OUTPUT;
	infoOut.logName = "IPC OUT:";
	info.logLevel = _LOG_INFO;
	info.logName = "ComPoint for Registry:";
	this->udsFile = udsFile;

	memset(address.sun_path, '\0', 108);
	strncpy(address.sun_path, udsFile, strlen(udsFile));
	addrlen = sizeof(address);


	connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if(connection_socket < 0)
		throw Error (-206 , "Could not create connection_socket", strerror(errno));

	if( pthread_mutex_init(&wLmutex, NULL) != 0)
			throw Error (-207 , "Could not init wLmutex", strerror(errno));

	if( unlink(udsFile) != 0 )
		printf("%s", strerror(errno));
		//throw Error(-200, "Error while unlinking udsFile", strerror(errno));

	if( setsockopt(connection_socket, SOL_SOCKET, SO_REUSEADDR, &optionflag, sizeof(optionflag)) != 0 )
		throw Error(-201, "Error while setting socket option", strerror(errno));

	if(bind(connection_socket, (struct sockaddr*)&address, addrlen) != 0)
		throw Error (-202, "Error while binding socket", strerror(errno));
}


RegServer::~RegServer()
{
	accept_thread_active = false;
	close(connection_socket);
	pthread_t accepter = getAccepter();
	if(accepter != 0)
		pthread_cancel(accepter);

	WaitForAcceptThreadToExit();

	deleteWorkerList();

	pthread_mutex_destroy(&wLmutex);
}



void RegServer::thread_accept()
{
	int new_socket = 0;
	ComPoint* comPoint = NULL;

	if( listen(connection_socket, 5) != 0)
		throw Error(-203, "Could not listen to socket", strerror(errno));

	accept_thread_active = true;

	while(accept_thread_active)
	{
		new_socket = accept(connection_socket, (struct sockaddr*)&address, &addrlen);
		if(new_socket > 0)
		{
			comPoint = new ComPoint(new_socket, new Registration(), -1, false);
			comPoint->configureLogInfo(&infoIn, &infoOut, &info);
			comPoint->setLogMethod(SYSLOG_LOG);
			comPoint->setSyslogFacility(LOG_LOCAL0);
			pushWorkerList(comPoint);
			comPoint->startWorking();
		}
	}
}


void RegServer::pushWorkerList(ComPoint* comPoint)
{
	pthread_mutex_lock(&wLmutex);
		workerList.push_back(comPoint);
	pthread_mutex_unlock(&wLmutex);
}


void RegServer::checkForDeletableWorker()
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
		}
		else
			++i;
	}
	pthread_mutex_unlock(&wLmutex);
}


void RegServer::deleteWorkerList()
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




void RegServer::start()
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
