/*
 * RsdMsg.h
 *
 *  Created on: 17.03.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_RSDMSG_H_
#define INCLUDE_RSDMSG_H_

#include <string>

using namespace std;


class RsdMsg{

	public:

		RsdMsg(int sender, string* content);


		RsdMsg(RsdMsg* msg);


		~RsdMsg();



		int getSender(){return this->sender;}
		string* getContent(){return this->content;}
		static void printCounters();

	private:
		int sender;
		string* content;
		static int countMalloc;
		static int countDealloc;

};


#endif /* INCLUDE_RSDMSG_H_ */
