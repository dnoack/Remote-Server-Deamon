/*
 * Plugin_Error.h
 *
 *  Created on: 28.01.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_PLUGIN_ERROR_H_
#define INCLUDE_PLUGIN_ERROR_H_

#include <exception>
#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <cstring>

using namespace std;

class PluginError : exception{


	public:
		PluginError(const char* msg, const char* file, int line)
		{
			this->exMsg = new string(msg);
			memset(lineBuffer, '\0', 33);

			exMsg->append("An error was thrown in file: ");
			exMsg->append(file);
			exMsg->append(" at line: ");
			snprintf(lineBuffer, sizeof(lineBuffer), "%d", line);
			exMsg->append(lineBuffer);
		}



		PluginError(const char* msg)
		{
			this->exMsg = new string(msg);
		}


		~PluginError() throw()
		{
			delete exMsg;
		}

		const char* get() const
		{
			return exMsg->c_str();
		}


	private:
		string* exMsg;
		char lineBuffer[33];



};


#endif /* INCLUDE_PLUGIN_ERROR_H_ */
