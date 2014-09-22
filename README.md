mqttsn-gw
=========

Repository containing the code of an event-based infrastructure for the Internet of Things.

This infrastructure enables a WSN (Wireless Sensor Network) deployed with Libelium devices (Waspmote and Meshlium) to the publication of sensory information to a TCP/IP network server. For this purpose, sensors execute MQTT-SN protocol through an own library and communicate with a gateway which is executing a transparent gateway software, this software makes a translation between MQTT-SN and MQTT protocols (also through own libraries) in order to communicate the WSN with a TCP/IP network, in which is placed Mosquitto broker.

Later Mosquitto delivers the published information by sensors to any subscriber in a external network.

Repository contains:

  - MQTT-SN library, to be executed over a 802.15.4 WSN.
  - Transparent gateway software, that manages MQTT-SN clients, the translation process and the comunication with Mosquitto. The gateway is the responsible for communicate the WSN with a TCP/IP network. So this software is essential in this sense.
  - MQTT library, to be executed over any device in a TCP/IP network. In this case, in the gateway as part of translation process.
  - NodeJS server, that makes possible that any device in a external network can obtain information published by sensors through a web browser.
