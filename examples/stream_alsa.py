import time
import roboflex as rf
import roboflex.audio_alsa as raa
import roboflex.transport.zmq as rtz

zmq_context = rtz.ZMQContext()

sensor = raa.AudioSensor()

#sensor > rf.MessagePrinter()
sensor > rtz.ZMQPublisher(zmq_context, "tcp://*:5555")

sensor.start()

time.sleep(1000)

sensor.stop()