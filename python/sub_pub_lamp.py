import paho.mqtt.client as mqtt
import requests

topic = "deviceid/chyuk/evt/#"
server = "54.84.225.1"

def on_connect(client, userdata, flags, rc):
    print("Connected with RC : " + str(rc))
    client.subscribe(topic)

def on_message(client, userdata, msg):
    if msg.topic == topic.replace("#", "light"):
        light = int(msg.payload)

        if light < 30:
            lamp = 1
        else:
            lamp = 0
        client.publish("deviceid/chyuk2/cmd/lamp", str(lamp))
        print("light : ", light, "  lamp :  ", lamp)


client = mqtt.Client()
client.connect(server, 1883, 60)
client.on_connect = on_connect
client.on_message = on_message

client.loop_forever()
