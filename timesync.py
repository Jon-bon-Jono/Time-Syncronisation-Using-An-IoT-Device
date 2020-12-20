#!/usr/bin/python

#by Jonathan Williams (z5162987) and Cavinash Chelliah (z3417347)
import socket
import time
import datetime
import struct
#import StringIO
from threading import Thread
import sys

#uses this to send, destination port (remote)
UDP_TIMESYNC_PORT = 3000 # node listens for timesync packets on port 4003
#listens to replies
UDP_REPLY_PORT = 3001 # node listens for reply packets on port 7005

isRunning = True


def udpListenThread():
 # listen on UDP socket port UDP_TIMESYNC_PORT
  recvSocket = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
  recvSocket.bind(("aaaa::1", UDP_REPLY_PORT))
  recvSocket.settimeout(0.5)

  while isRunning:
    try:
      data, addr = recvSocket.recvfrom( 1024 )
      timestamp = (struct.unpack("I", data[0:4]))[0]
      timestamp = int((str(data).split('\'')[1]))
      utc = datetime.datetime.fromtimestamp(timestamp)
      print("Reply from:", addr[0], "UTC[s]:", timestamp, "Localtime:", utc.strftime("%Y-%m-%d %H:%M:%S"))
    except socket.timeout:
      pass
    
def udpSendThread():

  sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM, 0)

  while isRunning:
    timestamp = int(time.time())
    print("Sending timesync packet with UTC[s]:", timestamp, "Localtime:", time.strftime("%Y-%m-%d %H:%M:%S"))

    # send UDP packet to nodes
    # change the IP Address with your sensorTag accordingly.
    # you may start with one sensortag.
    sock.sendto(struct.pack("I", timestamp), ("aaaa::212:4b00:f0d:5284", UDP_TIMESYNC_PORT))
    sock.sendto(struct.pack("I", timestamp), ("aaaa::212:4b00:f0d:4a81", UDP_TIMESYNC_PORT))
    
    # sleep for some seconds
    # the frequency of sending the sych timestamps packet is very important
    # you will see how this affect the sych accuracy in your experiment
    time.sleep(1)


# start UDP listener as a thread
t1 = Thread(target=udpListenThread)
t1.start()
print("Listening for incoming packets on UDP port", UDP_REPLY_PORT)

time.sleep(1)

# start UDP timesync sender as a thread
t2 = Thread(target=udpSendThread)
t2.start()

print("Sending timesync packets on UDP port", UDP_TIMESYNC_PORT)
print("Exit application by pressing (CTRL-C)")

try:
  while True:
    # wait for application to finish (ctrl-c)
    time.sleep(1)
except KeyboardInterrupt:
  print("Keyboard interrupt received. Exiting.")
  isRunning = False




