/*
 * JsonRPC.h
 *
 *  Created on: 16.01.2015
 *      Author: dnoack
 */

#ifndef SRC_JSONRPC_H_
#define SRC_JSONRPC_H_

#define JSON_PROTOCOL_VERSION "2.0"


#include <map>
#include <vector>


//rapdijson includes
#include "document.h"
#include "writer.h"


using namespace std;
using namespace rapidjson;






class JsonRPC {

	public:

		JsonRPC()
		{
			this->currentValue = NULL;
			this->result = NULL;


			jsonWriter = new Writer<StringBuffer>(sBuffer);
			requestDOM = new Document();
			responseDOM = new Document();
			errorDOM = new Document();

			generateResponseDOM(*responseDOM);
			generateErrorDOM(*errorDOM);

		};



		~JsonRPC()
		{
			delete jsonWriter;
			delete requestDOM;
			delete responseDOM;
			delete errorDOM;

		};


		//receive json-rpc msg, check if it is a request, process
		char* handle(string* request, string* identity);

		/**
		 * Checks if the json rpc msg has the mandatory members (jsonrpc and method).
		 * It also calls checkJsonRpcVersion.
		 */
		bool checkJsonRpc_RequestFormat(Document &dom);


		/**
		 * Checks if the json rpc msg member "jsonrpc" has the correct protocol version.
		 */
		bool checkJsonRpcVersion(Document &dom);



	private:

		/**
		 * Checks if there is a member named "id". If not the msg is assumed to be
		 * a notification. If "id" is existing, the msg is a request. For checking the other
		 * mandatory request fields, you need to call checkJsonRpc_RequestFormat().
		 */
		bool isRequest(Document &dom);

		//compare functor handels char* compare for the map
		struct cmp_keys
		{
			bool operator()(char const* input, char const* key)
			{
				return strcmp(input, key) < 0;
			}
		};

		StringBuffer sBuffer;
		Writer<StringBuffer>* jsonWriter;


		//represents the current jsonrpc msg as dom (document object model)
		Document* requestDOM;
		Document* responseDOM;
		Document* errorDOM;

		//Value from dom which is currently examined
		Value currentValue;

		//result from the processed function
		Value result;
		char* responseMsg;
		char* error;


		//lookup for function
		char* processRequest(Value &method, Value &params, Value &id, string* identity);

		char* generateResponse(Value &id);

		char* generateResponseError(Value &id, int code, char* msg);

		void generateResponseDOM(Document &dom);

		void generateErrorDOM(Document &dom);

};


#endif /* SRC_JSONRPC_H_ */
