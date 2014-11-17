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

#include "SecureChannelServer.h"
#include "WiFiCommunicationChannel.h"


// Do not connect to the Cloud
SYSTEM_MODE(MANUAL);


/**
 * Used for pinging the gatekeeper every 20 seconds in order to detect disconnects
 */
Timer pingTimer(20000);
IPAddress gatekeeper(192, 168, 0, 10);


void setup() {
	init_serial_over_usb();

//	connect_to_wifi(); // Blocks trying to get a WiFi connection. Times out if unsuccessful.

//	uint32_t challengeNonce[4];
//
//	for ( int i = 0; i < 3; i++ ) {
//		RandomNumberGenerator::getInstance().generateRandomChallengeNonce(challengeNonce);
//	}

	test_android_to_spark("NEED_CHALLENGE");
	debug("---------------");
	test_android_to_spark("OPEN");

//	test_spark_to_android("OPEN\n");


//	if ( WiFi.ready() ) {
//		rng.entropyFromNetwork(challengeNonce);
//	}

}


/**
 * The main loop
 */
void loop() {

}
