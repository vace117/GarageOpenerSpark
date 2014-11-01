/**
 * The main WiFi loop for the Garage Opener project
 *
 * @author Val Blant
 */

#include "application.h"
#include "wifi_support.h"
#include "Timer.h"
#include "utils.h"
#include "Garage.h"
#include "RandomSeeds.h"


// Do not connect to the Cloud
SYSTEM_MODE(MANUAL);

// Set up listening server
#define LISTEN_PORT 6666
TCPServer server(LISTEN_PORT);
TCPClient client;


/**
 * Used for pinging the gatekeeper every 20 seconds in order to detect disconnects
 */
Timer pingTimer(20000);
IPAddress gatekeeper(192, 168, 0, 10);

/**
 * Controls the garage
 */
Garage garage;

bool clientConnected = false;
String commandBuffer; // Current command buffer



#define EXTERNAL_FLASH_START_ADDRESS 0x80000

void setup() {
	init_serial_over_usb();

	initNewRandomSeed();

	uint16_t seedvec[3];
	readRandomSeedFromFlash(seedvec);
	readRandomSeedFromFlash(seedvec);
}



/**
 * The main WiFi loop
 */
void loop() {



//	if ( WiFi.ready() ) {
//
//		// Ping gatekeeper to make sure our connection is live
//		//
//		if ( pingTimer.isRunning() && pingTimer.isElapsed() ) {
//			int numberOfReceivedPackets = WiFi.ping(gatekeeper, 3);
//			if ( numberOfReceivedPackets > 0 ) {
//				pingTimer.start();
//			}
//			else {
//				debug("Oh-oh. We can't ping gatekeeper. Re-initializing WiFi...");
//				WiFi.disconnect(); WiFi.off();
//			}
//		}
//
//		if (client.connected()) { // There is a connected client
//			if ( !clientConnected ) {
//				debug("Client connected!");
//				clientConnected = true;
//			}
//
//			// Read commands from the client. COmmands are terminated by '\n'.
//			while (client.available()) {
//				char c = client.read();
//				if ( c != '\n' ) {
//					commandBuffer += c; // Build up the command
//				}
//				else {
//					// Process the received command
//					//
//					String response = garage.processCommand(commandBuffer);
//					if ( response.length() > 0 ) {
//						server.write((uint8_t*)response.c_str(), response.length());
//					}
//					commandBuffer = ""; // Reset command buffer
//				}
//			}
//		} else {
//			// If no client is connected, check for a new connection
//			//
//			if ( clientConnected ) {
//				debug("Client disconnected. Waiting for another connection...");
//				clientConnected = false;
//			}
//			client = server.available();
//		}
//	}
//	else { // If there is no WiFi connection
//		debug("Reconnecting to WiFi...");
//
//		client.stop();
//
//		clientConnected = false;
//
//		connect_to_wifi(); // Blocks trying to get a WiFi connection. Times out if unsuccessful.
//
//		// Start listening for clients
//		//
//		if ( WiFi.ready() ) {
//			server.begin();
//			debug("Listening on ", 0); debug(WiFi.localIP(), 0); debug(":", 0); debug(LISTEN_PORT);
//
//			pingTimer.start();
//		}
//	}
}
