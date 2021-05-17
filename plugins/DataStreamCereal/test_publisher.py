#!/usr/bin/python

# todo: ensure PYTHONPATH is your cloned openpilot dir5
from cereal.messaging import PubMaster, new_message
import math
import random
from time import sleep

pm = PubMaster(['deviceState'])

rate = 20.
time = 0.0
last_temp = 30.

while True:
  sleep(1 / rate)
  time += 1 / rate

  msg = new_message('deviceState')
  last_temp += random.uniform(-1 / rate, 1 / rate * 2)
  last_temp = max(min(last_temp, 60), 20)
  msg.deviceState.batteryTempC = last_temp
  print(f'Publishing: {msg}')
  pm.send('deviceState', msg)
