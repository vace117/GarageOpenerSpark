/**
 * This class represents the garage. Specifics of processing Garage commands are handled here.
 *
 * @author Val Blant
 */

#ifndef LIBRARIES_GARAGE_GARAGE_H_
#define LIBRARIES_GARAGE_GARAGE_H_

#include <spark_secure_channel/SparkSecureChannelServer.h>
#include "Timer.h"
#include "application.h"


#define DOOR_SENSOR_PIN 	D1
#define DOOR_CONTROL_PIN 	D2

// Response string mappings for each State
static const char * GarageStateStrings[] { "DOOR_OPEN", "DOOR_CLOSED", "DOOR_MOVING" };

class Garage : public SecureMessageConsumer {

public:
	enum State { DOOR_OPEN, DOOR_CLOSED, DOOR_MOVING };

	Garage() : doorTravelTimer(10000) {
		pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP); // Using internal 40k pull-up resistor
		pinMode(DOOR_CONTROL_PIN, OUTPUT);
		digitalWrite(DOOR_CONTROL_PIN, LOW); // Open transistor switch
	};

	void openDoor();
	void closeDoor();
	State getDoorStatus();
	void pressDoorSwitch();

	bool isDoorOpen() { return getDoorStatus() == DOOR_OPEN; };
	bool isDoorClosed() { return getDoorStatus() == DOOR_CLOSED; };
	bool isDoorMoving() { return getDoorStatus() == DOOR_MOVING; };

	String processMessage(String command);


private:

	Timer doorTravelTimer;
	State readDoorSensor();

	uint32_t iv_response[4]; // This is the IV to be used for next response

};


/**
 * Accepts a command received over the network. The command is decoded with our shared key
 * and executed. Only known commands result in any kind of work or response.
 */
String Garage::processMessage(String command) {
	bool respond = true;

	debug("Garage received command: ", 0); debug(command);

	if ( command == "OPEN" ) {
		debug("Opening bay doors...");
		openDoor();
	}
	else if ( command == "CLOSE" ) {
		debug("Closing bay doors...");
		closeDoor();
	}
	else if ( command == "PRESS_BUTTON" ) {
		debug("Simulating manual button click...");
		pressDoorSwitch();
	}
	else if ( command == "GET_STATUS" ) {
		// Nothing to do
		debug("Door Status Requested...");
	}
	else {
		respond = false; // Only respond to valid commands
	}

	return respond ? GarageStateStrings[ getDoorStatus() ] : "";
}

/**
 * Uses doorTravelTimer to see if the door is still in motion, or if we can read the Door reed sensor now.
 */
Garage::State Garage::getDoorStatus() {
	if ( doorTravelTimer.isRunning() ) {
		if ( doorTravelTimer.isElapsed() ) {
			return readDoorSensor();
		}
		else return DOOR_MOVING;
	}
	else {
		return readDoorSensor();
	}

}

/**
 * Open the door, if it is closed
 */
void Garage::openDoor() {
	if ( !isDoorMoving() && isDoorClosed() ) {
		pressDoorSwitch();
	}
}

/**
 * Open the door, if it is open
 */
void Garage::closeDoor() {
	if ( !isDoorMoving() && isDoorOpen() ) {
		pressDoorSwitch();
	}
}




/**
 * Figures out if the door is open or closed based on the reed sensor state
 */
Garage::State Garage::readDoorSensor() {
	return digitalRead(DOOR_SENSOR_PIN) == HIGH ? DOOR_OPEN : DOOR_CLOSED;
}

/**
 * Simulates a manual click of the button in the garage
 */
void Garage::pressDoorSwitch() {
	digitalWrite(DOOR_CONTROL_PIN, HIGH);
	delay(1000);
	digitalWrite(DOOR_CONTROL_PIN, LOW);

	doorTravelTimer.start(); // Give the door time to travel

}


#endif /* LIBRARIES_GARAGE_GARAGE_H_ */
