import paho.mqtt.client as mqtt
import math
import robot_state_pb2
from time import sleep

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

client = mqtt.Client()
client.on_connect = on_connect

client = mqtt.Client("PlotJuggler-test") #create new instance

client.connect("127.0.0.1", 1883, 60)

time = 0.0


page_number = 0;



while True:
    robot_state = robot_state_pb2.RobotState()
    robot_state.query = "query";
    sleep(0.20)
    time += 0.20

    page_number = page_number +1
    robot_state.page_number = page_number
    robot_state.result_per_page = page_number*2

    robot_state.pos.x = 1.1
    robot_state.pos.y = 2.2
    robot_state.pos.z = 3.3

#    robot_state.corpus = 3

    robot_state.sensor_reading["sensor1"] = 15;
    robot_state.sensor_reading["sensor2"] = page_number;

    msg = robot_state.SerializeToString()

    ret = client.publish("plotjuggler/stuff", msg, qos=0 )
    print( ret.is_published() )
