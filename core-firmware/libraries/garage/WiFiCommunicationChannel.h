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


#include "wifi_support.h"
#include "Timer.h"
#include "utils.h"
#include "SecureChannelServer.h"

class WiFiCommunicationChannel : public CommunicationChannel {
public:
	WiFiCommunicationChannel(int listenPort, int pingInterval, IPAddress pingTarget) :
		listenPort(listenPort),
		server(listenPort),
		pingTimer(pingInterval),
		pingTarget(pingTarget),
		clientConnected(false) {

	}

	int read(uint8_t *buffer, size_t size);
	size_t write(const uint8_t *buffer, size_t size);

private:
	int listenPort;
	TCPServer server;
	TCPClient client;

	/**
	 * Used for pinging the pingTarget every pingInterval seconds in order to detect disconnects
	 */
	Timer pingTimer;
	IPAddress pingTarget;

	bool clientConnected;

	bool isClientConnected();

};

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
				debug("Client disconnected. Waiting for another connection...");
				clientConnected = false;
			}
			client = server.available();
		}
	}
	else { // If there is no WiFi connection
		debug("Reconnecting to WiFi...");

		client.stop();

		clientConnected = false;

		connect_to_wifi(); // Blocks trying to get a WiFi connection. Times out if unsuccessful.

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
