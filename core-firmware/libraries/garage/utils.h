/**
 * Various utilities. Serial2usb debugging calls.
 *
 * @author Val Blant
 */

#ifndef LIBRARIES_GARAGE_UTILS_H_
#define LIBRARIES_GARAGE_UTILS_H_

void debug(const char c[], bool new_line = true) {
	if ( new_line ) {
		Serial.println(c);
	}
	else {
		Serial.print(c);
	}
}

void debug(const Printable& x, bool new_line = true) {
	if ( new_line ) {
		Serial.println(x);
	}
	else {
		Serial.print(x);
	}
}

void debug(const int num, bool new_line = true) {
	if ( new_line ) {
		Serial.println(num);
	}
	else {
		Serial.print(num);
	}
}

void debug(const String &s, bool new_line = true) {
	if ( new_line ) {
		Serial.println(s);
	}
	else {
		Serial.print(s);
	}
}

void init_serial_over_usb() {
	// Activates Serial mode. This line causes /dev/ttyACM0 to become available.
	// Connect with 'screen /dev/ttyACM0 9600'
	//
	Serial.begin(9600);

	// Wait here until the user presses ENTER
	//
//	while (!Serial.available())
//		SPARK_WLAN_Loop();

	debug("Serial over USB ready to go.");
}


#endif /* LIBRARIES_GARAGE_UTILS_H_ */
