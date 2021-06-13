#!/usr/bin/python

import zmq
import math
import json
from time import sleep

context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind("tcp://*:9872")

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
    socket.send_string( json.dumps(data) )
