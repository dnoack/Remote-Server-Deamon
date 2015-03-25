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

		bool operator==(RsdMsg* msg2)
		{
			string* content1 = this->getContent();
			string* content2 = msg2->getContent();

			if (content1->compare(*content2) && this->getSender() == msg2->getSender())
				return true;
			else
				return false;
		}

		const char* print()
		{
			return content->c_str();
		}

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
