 

import socket, time
from threading import Thread

def receive(s):
  for i in range (3):
    data = s.recv(1024)
    print 'Received: ', repr(data)
    
  s.close()

for i in range (1):

  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  s.connect(("127.0.0.1",1234))

  t = Thread(target=receive, args=(s,))
  t.start()
  
  #print "request for i2c with wrong device id"
  #s.send("{\"jsonrpc\": \"2.0\", \"method\": \"i2c.write\", \"params\" : { \"device\": 1265, \"slave_addr\": 56, \"data_out\" : [1, 191] }, \"id\" : 1}")
  #data = s.recv(1024)
  #print 'Received', repr(data)
  
  time.sleep(1)
  print "Dau sends json rpc result to RSD"
  s.send("{\"jsonrpc\": \"2.0\", \"result\": \"Am I doin it right?\", \"id\" : 2}")
  
  time.sleep(1)
  print "Dau is a funny guy and sends json rpc error to RSD"
  s.send("{\"jsonrpc\": \"2.0\", \"error\":{\"code\":-12334,\"message\":\"Server error\",\"data\":\"I dont know what I'm doin.\"},\"id\":3}")
 
  time.sleep(1)
  print "send json rpc with no error/ result/ or method"
  s.send("{\"jsonrpc\": \"2.0\", \"id\":4}")
  
 
  