#!/usr/bin/python

import argparse
import socket
import math
import json
from time import sleep

parser = argparse.ArgumentParser(description="Send UDP test data.")
parser.add_argument("--address", default="127.0.0.1", help="UDP address")
parser.add_argument("--port", default=9870, type=int, help="UDP port")
args = parser.parse_args()

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
    sock.sendto( json.dumps(data).encode(), (args.address, args.port) )

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

    sock.sendto( test_str.encode("utf-8"), (args.address, args.port) )

