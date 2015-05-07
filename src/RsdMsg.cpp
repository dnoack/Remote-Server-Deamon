
#include "RsdMsg.hpp"


RsdMsg::RsdMsg(int sender, string* content)
{
	this->sender = sender;
	this->content = new string(*content);
};


RsdMsg::RsdMsg(int sender, const char* content)
{
	this->sender = sender;
	this->content = new string(content);
};


RsdMsg::RsdMsg(RsdMsg* msg)
{
	this->sender = msg->getSender();
	this->content = new string(msg->getContent()->c_str(), msg->getContent()->size());
}


RsdMsg::~RsdMsg()
{
	delete content;
};
