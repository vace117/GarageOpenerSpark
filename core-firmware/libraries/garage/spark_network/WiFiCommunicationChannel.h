/*
 * WiFi based CommunicationChannel implementation. Takes care of all the details involved with keeping the
 * WiFi connection available, and starts a TCP/IP server on the specified port, so data can be read over the
 * network.
 *
 *  Created on: Nov 16, 2014
 *      Author: val
 */

#ifndef LIBRARIES_GARAGE_WIFICOMMUNICATIONCHANNEL_H_
#define LIBRARIES_GARAGE_WIFICOMMUNICATIONCHANNEL_H_


#include <spark_secure_channel/SparkSecureChannelServer.h>
#include "Timer.h"
#include "utils.h"

class WiFiCommunicationChannel : public CommunicationChannel {
public:
	WiFiCommunicationChannel(int listenPort, int pingInterval, IPAddress pingTarget) :
		listenPort(listenPort),
		server(listenPort),
		pingTimer(pingInterval),
		pingTarget(pingTarget),
		clientConnected(false) {

	}

	/**
	 * Reads from client
	 */
	int read(uint8_t *buffer, size_t size);

	/**
	 * Writes to server
	 */
	size_t write(const uint8_t *buffer, size_t size);

	/**
	 * Blocks trying to get a WiFi connection. Times out if unsuccessful.
	 */
	void open();

private:
	/**
	 * Port to listen on
	 */
	int listenPort;

	/**
	 * Instance of TCP/IP server
	 */
	TCPServer server;

	/**
	 * Currently connected client
	 */
	TCPClient client;

	/**
	 * Used for pinging the pingTarget every pingInterval seconds in order to detect disconnects
	 */
	Timer pingTimer;
	IPAddress pingTarget;

	/**
	 * true when there is a client connected
	 */
	bool clientConnected;

	/**
	 * Manages the WiFi connection and client state.
	 *
	 * This method is called every time before we try to read or write anything to/from the network.
	 * It ensures that WiFi connectivity is present and functioning. This is accomplished by
	 * pinging pingTarget every pingInterval seconds.
	 *
	 * It also manages clientConnected state.
	 */
	bool isClientConnected();

};

void WiFiCommunicationChannel::open() {
	// WiFi setup
	//
	debug("WiFi OFF...");
	WiFi.off();
	delay(1000);

	debug("Connecting to WiFi... ", 0);
	WiFi.on();
	WiFi.connect();
	debug("Connected.");

	debug("Acquiring DHCP info... ", 0);

	Timer wifiConnectTimer(20000); // Restart connection attempts every 20 seconds
	wifiConnectTimer.start();
	while (!WiFi.ready()) {
		SPARK_WLAN_Loop();

		if ( wifiConnectTimer.isRunning() && wifiConnectTimer.isElapsed() ) {
			// Couldn't connect for 20 seconds. Retry.
			return;
		}
	}

	delay(1000);

	debug("Done");
	debug("SSID: ", false);	debug(WiFi.SSID());
	debug("IP: ", false);	debug(WiFi.localIP());
	debug("Gateway: ", false);	debug(WiFi.gatewayIP());
}

bool WiFiCommunicationChannel::isClientConnected() {
	if ( WiFi.ready() ) {

		// Ping pingTarget to make sure our connection is live
		//
		if ( pingTimer.isRunning() && pingTimer.isElapsed() ) {
			int numberOfReceivedPackets = WiFi.ping(pingTarget, 3);
			if ( numberOfReceivedPackets > 0 ) {
				pingTimer.start();
			}
			else {
				debug("Oh-oh. We can't ping pingTarget. Re-initializing WiFi...");
				WiFi.disconnect(); WiFi.off();
			}
		}

		if (client.connected()) { // There is a connected client
			if ( !clientConnected ) {
				debug("Client connected!");
				clientConnected = true;
			}

		} else {
			// If no client is connected, check for a new connection
			//
			if ( clientConnected ) {
				debug("Client disconnected. Waiting for another connection...\n\n");
				clientConnected = false;
			}
			client = server.available();
		}
	}
	else { // If there is no WiFi connection
		debug("Reconnecting to WiFi...");

		client.stop();

		clientConnected = false;

		open(); // Blocks trying to get a WiFi connection. Times out if unsuccessful.

		// Start listening for clients
		//
		if ( WiFi.ready() ) {
			server.begin();
			debug("Listening on ", 0); debug(WiFi.localIP(), 0); debug(":", 0); debug(listenPort);

			pingTimer.start();
		}
	}

	return clientConnected;
}

int WiFiCommunicationChannel::read(uint8_t *buffer, size_t size) {
	int bytesRead = 0;

	if ( isClientConnected() ) {
		if ( client.available() ) {
			bytesRead = client.read(buffer, size);
		}
	}

	return bytesRead;
}

size_t WiFiCommunicationChannel::write(const uint8_t *buffer, size_t size) {
	int bytesSent = 0;

	if ( isClientConnected() ) {
		bytesSent = server.write(buffer, size);
	}

	return bytesSent;
}

#endif /* LIBRARIES_GARAGE_WIFICOMMUNICATIONCHANNEL_H_ */
