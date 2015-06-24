
import socket, time
from struct import *

TAG_FIELD_LENGTH = 1 
LENGTH_FIELD_LENGTH = 4
HEADER_FIELD_LENGTH = 5
RECEIVEBUFFER_LENGTH = 2048

TAG = 93


def makeHeader(value):
 	return pack('>BI', TAG, value)


def readHeader(header):
	temp = unpack('>BI', header)
	return temp[1]


def sendToRSD(s, msg):
	header = makeHeader(len(msg))
	s.send(header)
	s.send(msg)


def receiveFromRSD(s):
	data = s.recv(RECEIVEBUFFER_LENGTH)
	header = readHeader(data[0:HEADER_FIELD_LENGTH])
	while(header > len(data)+HEADER_FIELD_LENGTH):
		temp = s.recv(RECEIVEBUFFER_LENGTH)	
		data += temp
	return data[HEADER_FIELD_LENGTH:header+HEADER_FIELD_LENGTH]


for i in range (1):
  header = []
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  s.connect(("127.0.0.1",1234))

  print "Request aardvark devices over i2c"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"i2c.getI2cDevices\", \"params\" : { \"device\": 0 }, \"id\" : 888}")
  sendToRSD(s, msg)
  data = receiveFromRSD(s)
  print 'Received', repr(data)

  

  print "write request for i2c Di-Plugin"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"i2c.write\", \"params\" : { \"device\": 2237936547, \"slave_addr\": 80, \"data_out\" : [0, 30, 5, 19, 85] }, \"id\" : 18}")
  sendToRSD(s, msg)
  data = receiveFromRSD(s)
  print 'Received', repr(data)


  print "read request for i2c Di-Plugin"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"i2c.read\", \"params\" : { \"device\": 2237936547, \"slave_addr\": 80, \"mem_addr\": 0, \"num_bytes\": 4}, \"id\" : 20}")
  sendToRSD(s, msg)
  data = receiveFromRSD(s)
  print 'Received', repr(data)

  s.close()
