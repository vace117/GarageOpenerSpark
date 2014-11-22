/**
 * This class represents the garage. Specifics of processing Garage commands and working
 * with Garage hardware are handled here.
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

	Garage() : doorTravelTimer(15000) {
		pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP); // Using internal 40k pull-up resistor
		pinMode(DOOR_CONTROL_PIN, OUTPUT);
		digitalWrite(DOOR_CONTROL_PIN, LOW); // Open transistor switch
	};

	/**
	 * Open the door, if it is closed
	 */
	void openDoor();

	/**
	 * Close the door, if it is open
	 */
	void closeDoor();

	/**
	 * Uses doorTravelTimer to see if the door is still in motion, or if we can read the Door sensor now.
	 */
	State getDoorStatus();

	/**
	 * Simulates a manual click of the button in the garage
	 */
	void pressDoorSwitch();

	bool isDoorOpen() { return getDoorStatus() == DOOR_OPEN; };
	bool isDoorClosed() { return getDoorStatus() == DOOR_CLOSED; };
	bool isDoorMoving() { return getDoorStatus() == DOOR_MOVING; };

	/**
	 * Accepts a command received over the network. Only known commands result in any kind of work or response.
	 */
	String processMessage(String command);


private:

	/**
	 * Estimate of how long it takes for the door to open and close.
	 */
	Timer doorTravelTimer;

	/**
	 * Reads the magnetic reed switch sensor attached to the garage door.
	 *
	 * Velleman HAA28 sensor is being used.
	 *
	 * When the door is closed, the switch is closed and DOOR_SENSOR_PIN is pulled to ground
	 */
	State readDoorSensor();

};


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


Garage::State Garage::getDoorStatus() {

	if ( doorTravelTimer.isRunning() ) {
		if ( doorTravelTimer.isElapsed() ) {
			debug("Door Timer Elapsed.");
			return readDoorSensor();
		}
		else return DOOR_MOVING;
	}
	else {
		return readDoorSensor();
	}

}

void Garage::openDoor() {
	if ( !isDoorMoving() && isDoorClosed() ) {
		pressDoorSwitch();
	}
}

void Garage::closeDoor() {
	if ( !isDoorMoving() && isDoorOpen() ) {
		pressDoorSwitch();
	}
}


Garage::State Garage::readDoorSensor() {
	return digitalRead(DOOR_SENSOR_PIN) == HIGH ? DOOR_OPEN : DOOR_CLOSED;
}

void Garage::pressDoorSwitch() {
	digitalWrite(DOOR_CONTROL_PIN, HIGH);
	delay(1000);
	digitalWrite(DOOR_CONTROL_PIN, LOW);

	debug("Door timer started.");
	doorTravelTimer.start(); // Give the door time to travel

}


#endif /* LIBRARIES_GARAGE_GARAGE_H_ */
