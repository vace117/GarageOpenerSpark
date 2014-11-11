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
#include "tests/test_garage.h"


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

bool clientConnected = false;
String commandBuffer; // Current command buffer

void setup() {
	init_serial_over_usb();

//	connect_to_wifi(); // Blocks trying to get a WiFi connection. Times out if unsuccessful.

//	uint32_t challengeNonce[4];
//
//	for ( int i = 0; i < 3; i++ ) {
//		RandomNumberGenerator::getInstance().generateRandomChallengeNonce(challengeNonce);
//	}

	test_android_to_spark("NEED_CHALLENGE");

	test_spark_to_android("OPEN\n");


//	if ( WiFi.ready() ) {
//		rng.entropyFromNetwork(challengeNonce);
//	}

}

enum MessageState { NEED_MSG_LENGTH, RECEIVING_MSG };
MessageState msgState = NEED_MSG_LENGTH;
int msgLength = 0;

#define MAX_MESSAGE_SIZE 256
uint8_t receive_buffer[MAX_MESSAGE_SIZE] = {0};
uint8_t send_buffer[MAX_MESSAGE_SIZE] = {0};

void reset_buffers() {
	memset(receive_buffer, 0, MAX_MESSAGE_SIZE);
	memset(send_buffer, 0, MAX_MESSAGE_SIZE);
	msgLength = 0;
	msgState = NEED_MSG_LENGTH;
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
//			if ( msgState == NEED_MSG_LENGTH ) {
//				uint8_t msgLengthBuffer[2];
//				client.read(msgLengthBuffer, 2);
//				memcpy(&msgLength, msgLengthBuffer, 2);
//
//				if ( msgLength > 0 && msgLength < MAX_MESSAGE_SIZE ) {
//					memcpy(receive_buffer, msgLengthBuffer, 2);
//					msgState = RECEIVING_MSG;
//				}
//				else {
//					reset_buffers();
//				}
//			}
//			else if (msgState == RECEIVING_MSG) {
//				uint8_t* msg_data = receive_buffer + 2;
//				int bytesRead = client.read(msg_data, msgLength - 2);
//				if ( bytesRead == (msgLength - 2) ) {
//					int response_length = Garage::getInstance().processCommand(receive_buffer, send_buffer);
//
//					if ( response_length > 0 ) {
//						server.write(send_buffer, response_length);
//					}
//				}
//
//				reset_buffers();
//			}
//
////			// Read commands from the client. Commands are terminated by '\n'.
////			while (client.available()) {
////				char c = client.read();
////				if ( c != '\n' ) {
////					commandBuffer += c; // Build up the command
////				}
////				else {
////					// Process the received command
////					//
////					String response = Garage::getInstance().processCommand(commandBuffer);
////					if ( response.length() > 0 ) {
////						server.write((uint8_t*)response.c_str(), response.length());
////					}
////					commandBuffer = ""; // Reset command buffer
////				}
////			}
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
