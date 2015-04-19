/*
 * RsdMsg.cpp
 *
 *  Created on: 24.03.2015
 *      Author: dnoack
 */

#include "RsdMsg.h"
#include <stdio.h>
#include "Utils.h"

int RsdMsg::countDealloc;
int RsdMsg::countMalloc;

RsdMsg::RsdMsg(int sender, string* content)
{
	this->sender = sender;
	this->content = content;
	++countMalloc;
};


RsdMsg::RsdMsg(int sender, const char* content)
{
	this->sender = sender;
	this->content = new string(content);
	++countMalloc;
};


RsdMsg::RsdMsg(RsdMsg* msg)
{
	this->sender = msg->getSender();
	this->content = new string(msg->getContent()->c_str(), msg->getContent()->size());
	++countMalloc;
}


RsdMsg::~RsdMsg()
{
	delete content;
	++countDealloc;
};


void RsdMsg::printCounters()
{
	dyn_print("Malloc: %d | Dealloc: %d \n", countMalloc, countDealloc);
}
