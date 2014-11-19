/**
 * The main WiFi loop for the Garage Opener project
 *
 * @author Val Blant
 */

#include "application.h"

#include "utils.h"
#include "Garage.h"

#include <spark_secure_channel/SparkSecureChannelServer.h>
#include <spark_network/WiFiCommunicationChannel.h>


// Do not connect to Spark Cloud
SYSTEM_MODE(MANUAL);


/**
 * The channel will listen on port 6666, and ping gatekeeper every 20 seconds in order to detect disconnects
 */
IPAddress gatekeeper(192, 168, 0, 10);
WiFiCommunicationChannel wifiCommChannel(6666, 20000, gatekeeper);

/**
 * Garage hardware controller. This is the Message Consumer for the secure channel
 */
Garage garage;

/**
 * Manages the encryption of all data going in and out
 */
SecureChannelServer secureChannel(&wifiCommChannel, &garage);




void setup() {
	init_serial_over_usb();

	wifiCommChannel.open(); // Blocks trying to get a WiFi connection. Times out if unsuccessful.
}


/**
 * The main loop
 */
void loop() {
	secureChannel.loop();
}
