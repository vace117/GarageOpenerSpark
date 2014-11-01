/*
 * wifi_support.h
 *
 *  Created on: Oct 13, 2014
 *      Author: val
 */

#ifndef LIBRARIES_GARAGE_WIFI_SUPPORT_H_
#define LIBRARIES_GARAGE_WIFI_SUPPORT_H_

#include "application.h"
#include "Timer.h"
#include "utils.h"


void connect_to_wifi() {
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




#endif /* LIBRARIES_GARAGE_WIFI_SUPPORT_H_ */
