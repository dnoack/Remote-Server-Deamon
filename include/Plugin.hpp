/*
 * Plugin.hpp
 *
 *  Created on: 23.03.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_PLUGIN_HPP_
#define INCLUDE_PLUGIN_HPP_

#include <string>
#include <list>

using namespace std;

class Plugin{

	public:
		Plugin(const char* name, int pluginNumber, const char* path)
		{
			this->name = new string(name);
			this->pluginNumber = pluginNumber;
			this->udsFilePath = new string(path);
		}

		~Plugin()
		{
			delete name;
			delete udsFilePath;
			deleteMethodList();
		}



		void addMethod(string* methodName)
		{
			methods.push_back(methodName);
		}

		string* getName(){return this->name;}
		string* getUdsFilePath(){return this->udsFilePath;}
		int getPluginNumber(){return this->pluginNumber;}


	private:

		string* name;
		int pluginNumber;
		string* udsFilePath;
		list<string*> methods;


		void deleteMethodList()
		{
			list<string*>::iterator i = methods.begin();

			while(i != methods.end())
			{
				delete *i;
				i = methods.erase(i);
			}
		}
};



#endif /* INCLUDE_PLUGIN_HPP_ */
