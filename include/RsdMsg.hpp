#ifndef INCLUDE_RSDMSG_HPP_
#define INCLUDE_RSDMSG_HPP_

#include <string>
#include <stdio.h>
#include "Utils.h"

using namespace std;

/**
 * \class RsdMsg
 * Simple message container, which can be used for transporting a json rpc message and it's sender.
 * The sender is identified by a integer value, which is 0 for clients and > 0 for plugins.
 * \note plugins must have unique id's because the message sender will be identified by this id.
 */
class RsdMsg{

	public:

		/**
		 * Constructor
		 * \param sender Sender id (0 or unique plugin number > 0).
		 * \param content (Hopefully) a json rpc message as a pointer to a std::string instance.
		 */
		RsdMsg(int sender, string* content);

		/**
		 * Constructor
		 * \param sender Sender id (0 or unique plugin number > 0).
		 * \param content (Hopefully) a json rpc message as a pointer to a constant character.
		 */
		RsdMsg(int sender, const char* content);

		/**
		 * Copy constructor.
		 */
		RsdMsg(RsdMsg* msg);

		/**
		 * Destructor.
		 */
		virtual ~RsdMsg();


		/**
		 * Overloaded operator == for comparing two messages.
		 * \msg2 The other message which will be compared with the instance of this one.
		 * \return Returns true if both messages are equal, false instead.
		 */
		bool operator==(RsdMsg* msg2)
		{
			string* content1 = this->getContent();
			string* content2 = msg2->getContent();

			if (content1->compare(*content2) && this->getSender() == msg2->getSender())
				return true;
			else
				return false;
		}


		/**
		 * \return Sender id of this message.
		 */
		int getSender(){return this->sender;}

		/**
		 * \return Content of this message as a pointer to std::string.
		 */
		string* getContent(){return this->content;}


	private:
		/*! Sender id which is 0 for tcp clients and greater 0 for plugins.*/
		int sender;
		/*! This is the proper message, hopefully containing a valid json rpc message.*/
		string* content;

};


#endif /* INCLUDE_RSDMSG_HPP_ */
