#!/usr/bin/env python3

import zmq
import math
import json
import argparse

from time import sleep, time_ns
import numpy as np

PORT = 9872

parser = argparse.ArgumentParser("start_test_publisher")

parser.add_argument(
    "--topic|-t",
    dest="topic",
    help="Topic on which messages will be published",
    type=str,
    required=False,
)
parser.add_argument(
    "--timestamp",
    dest="timestamp",
    help="Send timestamp as message part, requires topic to work properly",
    required=False,
    action="store_true",
)

args = parser.parse_args()
topic = args.topic
timestamp = args.timestamp


def main():
    context = zmq.Context()
    server_socket = context.socket(zmq.PUB)
    server_socket.bind("tcp://*:" + str(PORT))
    ticks = 0

    while True:
        out_str = []
        packet = []
        data = {
            "ticks": ticks,
            "data": {
                "cos": math.cos(ticks),
                "sin": math.sin(ticks),
                "floor": np.floor(np.cos(ticks)),
                "ceil": np.ceil(np.cos(ticks)),
            },
        }

        if topic:
            out_str.append(f"[{topic}] - ")
            packet.append(topic.encode())

        out_str.append(json.dumps(data))
        packet.append(out_str[-1].encode())

        if timestamp:
            timestamp_s = str(time_ns() * 1e-9)
            out_str.append(" - timestamp: " + timestamp_s)
            packet.append(timestamp_s.encode())

        print("".join(out_str))
        server_socket.send_multipart(packet)

        ticks += 1

        sleep(0.1)


if __name__ == "__main__":
    main()
