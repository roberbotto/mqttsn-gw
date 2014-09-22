Repository containing the code of an event-based infrastructure for the Internet of Things.

This infrastructure enables a WSN (Wireless Sensor Network) deployed with Libelium devices (Waspmote and Meshlium) to the publication of sensory information to a TCP/IP network server. For this purpose, sensors execute MQTT-SN protocol through an own library and communicate with a gateway which is executing a transparent gateway software, this software makes a translation between MQTT-SN and MQTT protocols (also through own libraries) in order to communicate the WSN with a TCP/IP network, in which is placed Mosquitto broker.

Later Mosquitto delivers the published information by sensors to any subscriber in a external network.

Repository contains:

  - MQTT-SN library, to be executed over a 802.15.4 WSN.
  - Transparent gateway software, that manages: (i) MQTT-SN clients, (ii) the translation process and (iii) the comunication with Mosquitto. The gateway is the responsible for communicate the WSN with a TCP/IP network. So this software is essential in this sense.
  - MQTT library, to be executed over any device in a TCP/IP network. In this case, in the gateway as part of translation process.
  - NodeJS server, that makes possible that any device in a external network can obtain information published by sensors through a web browser.

Supported Features
==================

Both MQTT-SN and MQTT libraries support all protocols characteristics except:

  - Last will and testament.
  - Different client states in MQTT-SN.
  - Automatic gateway discovery procedure in MQTT-SN.

MQTT library is for 3.1.0 version and MQTT-SN is for 1.2 version.

How to use libraries?
=====================

In mqttsn-gw/mqtt-sn/test folder and mqttsn-gw/mqtt/src/mqtt-test.c file, there are examples of how to use both libraries for any purpose.

In mqttsn-gw/mqtt/clients there are some clients (some in beta) that can be used for publish and subscribe information using MQTT library.

Building
========

To deploy this infrastructure it's necessary to have configured a development environment formed by a Libelium Waspmote Platform connected to a TCP/IP network in which one of his hosts is executing Mosquitto. Later you can compile running "make" in different folders of the gateway (previous configuration of hosts addresses), and use Waspmote IDE to install and compile MQTT-SN library.

Once the environment is configured, just run all servers and put the right code in sensors.

License
=======

This software is licensed under the GNU/GPL v2 License.
