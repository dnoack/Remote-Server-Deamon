 

import socket, time


for i in range (1):

  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  s.connect(("127.0.0.1",1234))

  print "unkown function"
  s.send("{\"jsonrpc\": \"2.0\", \"params\":  {\"port\": 0}, \"method\": \"Aardvark.aa_foo\", \"id\": 1}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "unkown plugin"
  s.send("{\"jsonrpc\": \"2.0\", \"params\":  {\"port\": 0}, \"method\": \"dfasdf.aa_open\", \"id\": 2}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "no namespace but DOT"
  s.send("{\"jsonrpc\": \"2.0\", \"params\":  {\"port\": 0}, \"method\": \".aa_open\", \"id\": 3}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "no namespace"
  s.send("{\"jsonrpc\": \"2.0\", \"params\":  {\"port\": 0}, \"method\": \"aa_open\", \"id\": 4}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "parsing error"
  s.send("{\"jsonrpc\": \"2.0\", \"params\":  \"port\": 0}, \"method\": \"aa_open\", \"id\": 5}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "wrong jsonrpc version"
  s.send("{\"jsonrpc\": \"2.1\", \"params\":  {\"port\": 0}, \"method\": \"aa_open\", \"id\": 6}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "no method"
  s.send("{\"jsonrpc\": \"2.0\", \"params\":  {\"port\": 0}, \"id\": 7}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "no params"
  s.send("{\"jsonrpc\": \"2.0\", \"method\":  \"Aardvark.aa_unique_id\", \"id\": 8}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "no id"
  s.send("{\"jsonrpc\": \"2.0\", \"method\":  \"Aardvark.aa_unique_id\", \"params\" : { \"port\": 9 }")
  data = s.recv(1024)
  print 'Received', repr(data)
  
  # Msg for RSd 
  
  print "Requesting plugin list"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"RSD.showAllRegisteredPlugins\", \"params\" : { \"nothing\": 0 }, \"id\" : 95}")
  data = s.recv(1024)
  print 'Received', repr(data)


  print "Requesting function list"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"RSD.showAllKownFunctions\", \"params\" : { \"nothing\": 0 }, \"id\" : 96}")
  data = s.recv(1024)
  print 'Received', repr(data)
  
  # Requesting device list
  print "Find devices"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_find_devices\", \"params\" : { \"num_devices\": 2  }, \"id\" : 97}")
  data = s.recv(1024)
  print 'Received', repr(data)
  
  print "Find devices ext"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_find_devices_ext\", \"params\" : { \"num_devices\": 2  }, \"id\" : 98}")
  data = s.recv(1024)
  print 'Received', repr(data)
  
  
  print "Open device with extended information"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_open_ext\", \"params\" : { \"port\": 0  }, \"id\" : 99}")
  data = s.recv(1024)
  print 'Received', repr(data)
  
  print "Get port from handle"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_port\", \"params\" : { \"Aardvark\": 1  }, \"id\" : 100}")
  data = s.recv(1024)
  print 'Received', repr(data)
  
  print "Get features from handle"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_features\", \"params\" : { \"Aardvark\": 1  }, \"id\" : 101}")
  data = s.recv(1024)
  print 'Received', repr(data)
  
  print "Get status string for code -10"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_status_string\", \"params\" : { \"status\": -10  }, \"id\" : 102}")
  data = s.recv(1024)
  print 'Received', repr(data)
  
  
  print "Get version of from handle"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_version\", \"params\" : { \"Aardvark\": 1  }, \"id\" : 103}")
  data = s.recv(1024)
  print 'Received', repr(data)
  
  
  print "close device"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_close\", \"params\" : { \"Aardvark\": 1  }, \"id\" : 110}")
  data = s.recv(1024)
  print 'Received', repr(data)
  
  print "Request aardvark devices over i2c"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"i2c.getI2cDevices\", \"params\" : { \"device\": 0 }, \"id\" : 888}")
  data = s.recv(1024)
  print 'Received', repr(data)
  

  
  # Lichtorgel 
  print "open device handle"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_open\", \"params\" : { \"port\": 0 }, \"id\" : 111}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "get unique id"
  s.send("{\"jsonrpc\": \"2.0\", \"method\":  \"Aardvark.aa_unique_id\", \"params\": {\"Aardvark\": 1 }, \"id\": 1}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "enable pullups"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_target_power\", \"params\" : { \"Aardvark\" : 1 , \"powerMask\" : 3}, \"id\" : 2}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "configure pins of ioExpander as outputs"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [3, 0] }, \"id\" : 3}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "turn all lights on"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 0] }, \"id\" : 4}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "turn all lights off"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 255] }, \"id\" : 4}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "turn d0 on"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 254] }, \"id\" : 5}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "add d1 on"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 252] }, \"id\" : 6}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "add d2 on"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 248] }, \"id\" : 7}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "add d3 on"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 240] }, \"id\" : 8}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "add d4 on"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 224] }, \"id\" : 9}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "add d5 on"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 192] }, \"id\" : 10}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "add d6 on"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 128] }, \"id\" : 11}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "add d7 on"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 0] }, \"id\" : 12}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "turn all off"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [1, 255] }, \"id\" : 13}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "configure i/o expander lines as inputs"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_i2c_write\", \"params\" : { \"Aardvark\": 1, \"slave_addr\": 56, \"AardvarkI2cFlags\": 0, \"num_bytes\": 2, \"data_out\" : [3, 255] }, \"id\" : 14}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "disable pullups"
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_target_power\", \"params\" : { \"Aardvark\" : 1 , \"powerMask\" : 0}, \"id\" : 15}")
  data = s.recv(1024)
  print 'Received', repr(data)

  print "close handle"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"Aardvark.aa_close\", \"params\" : { \"Aardvark\": 1 }, \"id\" : 16}")
  data = s.recv(1024)
  print 'Received', repr(data)


  print "request for i2c Di-Plugin"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"i2c.write\", \"params\" : { \"device\": 225456, \"slave_addr\": 56, \"data_out\" : [3, 0] }, \"id\" : 17}")
  data = s.recv(1024)
  print 'Received', repr(data)
  
  print "request for i2c Di-Plugin"
  time.sleep(1)
  s.send("{\"jsonrpc\": \"2.0\", \"method\": \"i2c.write\", \"params\" : { \"device\": 2237936547, \"slave_addr\": 56, \"data_out\" : [1, 191] }, \"id\" : 18}")
  data = s.recv(1024)
  print 'Received', repr(data)

  s.close()