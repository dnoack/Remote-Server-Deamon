/*
 * RsdMsg.h
 *
 *  Created on: 17.03.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_RSDMSG_H_
#define INCLUDE_RSDMSG_H_

class RsdMsg{

	public:

		RsdMsg(int sender, string* content)
		{
			this->sender = sender;
			this->content = content;
		};

		RsdMsg(RsdMsg* msg)
		{
			this->sender = msg->getSender();
			this->content = new string(msg->getContent()->c_str(), msg->getContent()->size());
		}

		~RsdMsg()
		{
			delete content;
		};


		int getSender(){return this->sender;}
		string* getContent(){return this->content;}

	private:
		int sender;
		string* content;

};

#endif /* INCLUDE_RSDMSG_H_ */
