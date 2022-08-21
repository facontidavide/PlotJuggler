#!/usr/bin/env python3

import zmq
import math
import json
import argparse

from time import sleep
import numpy as np

PORT = 9872

parser = argparse.ArgumentParser("start_test_publisher")

parser.add_argument("--topic|-t",
                    dest="topic",
                    help="Topic on which messages will be published",
                    type=str,
                    required=False)

args = parser.parse_args()
topic = args.topic


def main():
    context = zmq.Context()
    server_socket = context.socket(zmq.PUB)
    server_socket.bind("tcp://*:" + str(PORT))
    ticks = 0

    while True:
        data = {
            "ticks": ticks,
            "data": {
                "cos": math.cos(ticks),
                "sin": math.sin(ticks),
                "floor": np.floor(np.cos(ticks)),
                "ceil": np.ceil(np.cos(ticks))
            }
        }

        if topic:
            print(f"[{topic}] - " + json.dumps(data))
            server_socket.send_multipart(
                [topic.encode(), json.dumps(data).encode()])
        else:
            print(json.dumps(data))
            server_socket.send(json.dumps(data).encode())

        ticks += 1

        sleep(0.1)


if __name__ == '__main__':
    main()
