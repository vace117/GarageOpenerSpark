/**
 * This class represents the garage. Specifics of processing Garage commands are handled here.
 *
 * @author Val Blant
 */

#ifndef LIBRARIES_GARAGE_GARAGE_H_
#define LIBRARIES_GARAGE_GARAGE_H_

#include "Timer.h"
#include "application.h"
#include "tropicssl/sha1.h"
#include "tropicssl/aes.h"
#include <master_key.h> // Contains the super secret shared key


#define DOOR_SENSOR_PIN 	D1
#define DOOR_CONTROL_PIN 	D2

// Response string mappings for each State
static const char * GarageStateStrings[] { "DOOR_OPEN\n", "DOOR_CLOSED\n", "DOOR_MOVING\n" };

class Garage {

public:
	enum State { DOOR_OPEN, DOOR_CLOSED, DOOR_MOVING };
	static Garage& getInstance() {
		// Guaranteed to be destroyed. Instantiated on first use.
		//
		static Garage instance;
		return instance;
	}

	void openDoor();
	void closeDoor();
	State getDoorStatus();
	void pressDoorSwitch();

	bool isDoorOpen() { return getDoorStatus() == DOOR_OPEN; };
	bool isDoorClosed() { return getDoorStatus() == DOOR_CLOSED; };
	bool isDoorMoving() { return getDoorStatus() == DOOR_MOVING; };

	String processCommand(uint8_t received_data[]);
	String decryptCommand(uint8_t received_data[]);
	String handleDecryptedCommand(String command);

private:
	Garage() : doorTravelTimer(10000) {
		pinMode(DOOR_SENSOR_PIN, INPUT_PULLDOWN); // Using internal 40k pull-down resistor
		pinMode(DOOR_CONTROL_PIN, OUTPUT);
		digitalWrite(DOOR_CONTROL_PIN, LOW); // Open transistor switch
	};

	// Make sure these are unaccessible. Otherwise we may accidently get copies of
	// the singleton appearing.
	//
	Garage(Garage const&);
	void operator=(Garage const&);


	Timer doorTravelTimer;
	State readDoorSensor();

	uint32_t iv_response[4]; // This is the IV to be used for next response

};

String Garage::processCommand(uint8_t received_data[]) {
	String decryptedCommand = decryptCommand(received_data);
	String response = handleDecryptedCommand(decryptedCommand);

	return response;
}

String Garage::decryptCommand(uint8_t received_data[]) {

	// Get the length of this data
	//
	int data_length = 0;
	memcpy(&data_length, received_data, 2);

	// Calculate our own HMAC of received data
	//
	int hmaced_data_length = data_length - 20;
	unsigned char local_hmac[20];
	sha1_hmac(	(uint8_t*) MASTER_KEY, sizeof(MASTER_KEY),
				received_data, hmaced_data_length,
				local_hmac);

	// Compare our HMAC to received HMAC
	//
	if ( memcmp(local_hmac, received_data + hmaced_data_length, 20) != 0 ) {
		debug("BAD HMAC detected!\n");
		return "";
	}

	// Grab the IV that was used to encrypt this data
	//
	uint32_t iv_send[4];
	uint8_t* iv_send_start = received_data + 2;
	memcpy(iv_send, iv_send_start, sizeof(iv_send));

	// Grab the IV that should be used to encrypt the response
	//
	uint8_t* iv_response_start = iv_send_start + sizeof(iv_send);
	memcpy(iv_response, iv_response_start, 16);

	uint8_t* ciphertext_start = iv_response_start + 16;

	// Decrypt the message
	//
	aes_context aes;
	int aes_buffer_length = hmaced_data_length - (ciphertext_start - received_data);

	uint8_t ciphertext[aes_buffer_length];
	uint8_t plaintext[aes_buffer_length];
	memcpy(ciphertext, ciphertext_start, aes_buffer_length);

	aes_setkey_dec(&aes, (uint8_t*) MASTER_KEY, 128);
	aes_crypt_cbc(&aes, AES_DECRYPT, aes_buffer_length, (uint8_t*)iv_send, ciphertext, plaintext);

	// Remove PKCS #7 padding from our message by padding with zeroes
	//
	int message_size = aes_buffer_length - plaintext[aes_buffer_length - 1]; // The last byte contains the number of padded bytes
	memset(plaintext + message_size, 0, aes_buffer_length - message_size);

	return (char*) plaintext; // A String object is created here
}

/**
 * Accepts a command received over the network. The command is decoded with our shared key
 * and executed. Only known commands result in any kind of work or response.
 */
String Garage::handleDecryptedCommand(String command) {
	bool respond = true;

	if ( command == "OPEN" ) {
		debug("Opening bay doors...");
		//openDoor();
	}
	else if ( command == "CLOSE" ) {
		debug("Closing bay doors...");
//		closeDoor();
	}
	else if ( command == "PRESS_BUTTON" ) {
		debug("Simulating manual button click...");
//		pressDoorSwitch();
	}
	else if ( command == "GET_STATUS" ) {
		// Nothing to do
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
	delay(500);
	digitalWrite(DOOR_CONTROL_PIN, LOW);

	doorTravelTimer.start(); // Give the door time to travel

}


#endif /* LIBRARIES_GARAGE_GARAGE_H_ */
