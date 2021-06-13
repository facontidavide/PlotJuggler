#!/usr/bin/python

# https://pypi.org/project/websocket_client/

import websocket
import math
import json
from time import sleep

ws = websocket.WebSocket()
ws.connect("ws://localhost:9871")

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
    ws.send( json.dumps(data) )
