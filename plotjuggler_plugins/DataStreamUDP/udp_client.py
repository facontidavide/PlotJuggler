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
