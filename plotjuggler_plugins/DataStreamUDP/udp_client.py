#!/usr/bin/python

import socket
import math
import json
from time import sleep

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
time = 0.0

while True:
    sleep(0.05)
    time += 0.05

    data = {
        "timestamp": time,
        "test_data": {
            "cos": math.cos(time),
            "sin": math.sin(time)
        }
    }
    sock.sendto( json.dumps(data).encode(), ("127.0.0.1", 9870) )
    
    test_str = "{ \
	  \"1252\": { \
	    \"timestamp\": { \
	      \"microsecond\": 0 \
	    }, \
	    \"value\": { \
	      \"current\": { \
		\"ampere\": null \
	      }, \
	      \"voltage\": { \
		\"volt\": 24.852617263793945 \
	      }\
	    }\
	  } }" 
   
    sock.sendto( test_str.encode('utf-8'), ("127.0.0.1", 9870) )

