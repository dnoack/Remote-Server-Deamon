
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


  print "unkown plugin"
  msg = ("{\"jsonrpc\": \"2.0\", \"params\":  {\"port\": 0}, \"method\": \"dfasdf.aa_open\", \"id\": 2}")
  sendToRSD(s, msg)
  
  print "no namespace but DOT"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"params\":  {\"port\": 0}, \"method\": \".aa_open\", \"id\": 3}")
  sendToRSD(s, msg)
  
  print "parsing error"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"params\":  \"port\": 0}, \"method\": \"aa_open\", \"id\": 5}")
  sendToRSD(s, msg)
  

  # Msg for RSd 
  
  print "Requesting plugin list"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"RSD.showAllRegisteredPlugins\", \"params\" : { \"nothing\": 0 }, \"id\" : 95}")
  sendToRSD(s, msg)


  print "Requesting function list"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"RSD.showAllKownFunctions\", \"params\" : { \"nothing\": 0 }, \"id\" : 96}")
  sendToRSD(s, msg)
  
  
  # Lichtorgel 
  print "open device handle"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_open\", \"params\" : { \"port\": 0 }, \"id\" : 111}")
  sendToRSD(s, msg)

  print "get unique id"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\":  \"Aardvark.aa_unique_id\", \"params\": {\"Aardvark\": 1 }, \"id\": 1}")
  sendToRSD(s, msg)

  print "enable pullups"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_target_power\", \"params\" : { \"Aardvark\" : 1 , \"powerMask\" : 3}, \"id\" : 2}")
  sendToRSD(s, msg)

  print "configure pins of ioExpander as outputs"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [3, 0] }, \"id\" : 3}")
  sendToRSD(s, msg)

  print "turn all lights on"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 0] }, \"id\" : 4}")
  sendToRSD(s, msg)

  print "turn all lights off"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 255] }, \"id\" : 4}")
  sendToRSD(s, msg)

  print "turn d0 on"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 254] }, \"id\" : 5}")
  sendToRSD(s, msg)

  print "add d1 on"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 252] }, \"id\" : 6}")
  sendToRSD(s, msg)

  print "add d2 on"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 248] }, \"id\" : 7}")
  sendToRSD(s, msg)

  print "add d3 on"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 240] }, \"id\" : 8}")
  sendToRSD(s, msg)

  print "add d4 on"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 224] }, \"id\" : 9}")
  sendToRSD(s, msg)

  print "add d5 on"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 192] }, \"id\" : 10}")
  sendToRSD(s, msg)

  print "add d6 on"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 128] }, \"id\" : 11}")
  sendToRSD(s, msg)

  print "add d7 on"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 0] }, \"id\" : 12}")
  sendToRSD(s, msg)

  print "turn all off"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 255] }, \"id\" : 13}")
  sendToRSD(s, msg)

  print "configure i/o expander lines as inputs"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [3, 255] }, \"id\" : 14}")
  sendToRSD(s, msg)

  print "disable pullups"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_target_power\", \"params\" : { \"Aardvark\" : 1 , \"powerMask\" : 0}, \"id\" : 15}")
  sendToRSD(s, msg)

  print "close handle"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_close\", \"params\" : { \"Aardvark\": 1 }, \"id\" : 16}")
  sendToRSD(s, msg)
  data = receiveFromRSD(s)
  print 'Received', repr(data)

  print "Request aardvark devices over i2c"
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"i2c.getI2cDevices\", \"params\" : { \"device\": 0 }, \"id\" : 888}")
  sendToRSD(s, msg)

  print "request for i2c Di-Plugin"
  time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"i2c.write\", \"params\" : { \"device\": 2237936547, \"slave_addr\": 56, \"data_out\" : [3, 0] }, \"id\" : 17}")
  sendToRSD(s, msg)
  
  print "request for i2c Di-Plugin"
  #time.sleep(1)
  msg  =  ("{\"jsonrpc\": \"2.0\", \"method\": \"i2c.write\", \"params\" : { \"device\": 2237936547, \"slave_addr\": 56, \"data_out\" : [1, 191] }, \"id\" : 18}")
  sendToRSD(s, msg)
#  time.sleep(2)
  s.close()

