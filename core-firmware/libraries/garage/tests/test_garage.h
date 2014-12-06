/*
 * test_garage.h
 *
 *  Created on: Nov 9, 2014
 *      Author: val
 */

#ifndef LIBRARIES_GARAGE_TESTS_TEST_GARAGE_H_
#define LIBRARIES_GARAGE_TESTS_TEST_GARAGE_H_


#include <SparkRandomNumberGenerator.h>
#include <SparkSecureChannelServer.h>
#include "utils.h"
#include "master_key.h"
#include "Garage.h"
#include "tropicssl/sha1.h"
#include "tropicssl/aes.h"

/**
 * Puts together the following data for transmission:
 *
 * 		<Message_Length[2], IV_Send[16], AES_CBC(Key, IV_Send, COMMAND), <==== HMAC(Key)[20]>
 */
int android_request(char* command, uint8_t send_data[]) {
	uint32_t iv_send[4]; SparkRandomNumberGenerator::getInstance().generateRandomChallengeNonce(iv_send);

	debug("Sending: ", false); debug(command);

	uint8_t* iv_send_start = send_data + 2;
	memcpy(iv_send_start, iv_send, sizeof(iv_send)); // Add IV_Send[16]

	// Figure out the length of the command we are sending, and the appropriate padding
	//
	int msg_length = strlen(command);
	int aes_buffer_length = (msg_length & ~15) + 16; // Round up to next 16 byte length
	char pad = aes_buffer_length - msg_length;

	// Setup plaintext buffer
	//
	uint8_t aes_buffer[aes_buffer_length];
	memcpy(aes_buffer, command, msg_length); // Add the message
	memset(aes_buffer + msg_length, pad, pad); // Followed by PKCS #7 padding

	// Encrypt the plaintext
	//
	aes_context aes;
	uint8_t aes_buffer_encrypted[aes_buffer_length];
	aes_setkey_enc(&aes, (uint8_t*) MASTER_KEY, 128);
	aes_crypt_cbc(&aes, AES_ENCRYPT, aes_buffer_length, (uint8_t*)iv_send, aes_buffer, aes_buffer_encrypted);

	// Add encrypted buffer
	//
	uint8_t* aes_ciphertext_start = iv_send_start + sizeof(iv_send);
	memcpy(aes_ciphertext_start, aes_buffer_encrypted, aes_buffer_length);

	// Calculate data length field
	//
	unsigned char hmac[20];
	uint8_t* hmac_start = aes_ciphertext_start + aes_buffer_length;
	uint16_t dataLength = hmac_start + sizeof(hmac) - send_data;
	memcpy(send_data, &dataLength, 2);

	// Calculate HMAC(Key) of all data in send_data so far
	//
	sha1_hmac(	(uint8_t*) MASTER_KEY, sizeof(MASTER_KEY),
				send_data, hmac_start - send_data,
				hmac);

	// Append the HMAC to send_data
	//
	memcpy(hmac_start, hmac, sizeof(hmac));

	// Return the length of prepared send_data
	//
	uint8_t* end_of_data = hmac_start + sizeof(hmac);
	return end_of_data - send_data;
}

String decrypt_spark_data(uint8_t received_data[]) {

	// Get the length of this data
	//
	int data_length = 0;
	memcpy(&data_length, received_data, 2);

	// Calculate our own HMAC of received data
	//
	int hmac_data_length = data_length - 20;
	unsigned char local_hmac[20];
	sha1_hmac(	(uint8_t*) MASTER_KEY, sizeof(MASTER_KEY),
				received_data, hmac_data_length,
				local_hmac);

	// Compare our HMAC to received HMAC
	//
	if ( memcmp(local_hmac, received_data + hmac_data_length, 20) != 0 ) {
		debug("BAD HMAC from Spark detected!\n");
		return "";
	}

	// Grab the IV that was used to encrypt this data
	//
	uint32_t iv_response[4];
	uint8_t* iv_response_start = received_data + 2;
	memcpy(iv_response, iv_response_start, sizeof(iv_response));

	uint8_t* ciphertext_start = iv_response_start + sizeof(iv_response);

	// Decrypt the message
	//
	aes_context aes;
	int aes_buffer_length = hmac_data_length - (ciphertext_start - received_data);

	uint8_t ciphertext[aes_buffer_length];
	uint8_t plaintext[aes_buffer_length];
	memcpy(ciphertext, ciphertext_start, aes_buffer_length);

	aes_setkey_dec(&aes, (uint8_t*) MASTER_KEY, 128);
	aes_crypt_cbc(&aes, AES_DECRYPT, aes_buffer_length, (uint8_t*)iv_response, ciphertext, plaintext);

	// Remove PKCS #7 padding from our message by padding with zeroes
	//
	int message_size = aes_buffer_length - plaintext[aes_buffer_length - 1];
	memset(plaintext + message_size, 0, aes_buffer_length - message_size);



	return (char*) plaintext;
}


class TestCommunicationChannel : public CommunicationChannel {
public:
	TestCommunicationChannel(uint8_t* data) {
		test_data = data;
	}

	void open() {}

	int read(uint8_t *buffer, size_t size) {
		memcpy(buffer, test_data, size);
		test_data += size;

		return size;
	}

	size_t write(const uint8_t *buffer, size_t size) {
		debug("Sending to Android: ", 0);
		debug(buffer, size);
		return size;
	}

private:
	uint8_t* test_data;
};


class TestMessageConsumer : public SecureMessageConsumer {
public:
	String processMessage(String message) {
		debug("Consumer received command: ", 0); debug(message);

		return "HAPPY DANCE!";
	}
};


void test_android_to_spark(String commandFromAndroid) {
	uint8_t send_data[180];
	TestCommunicationChannel fakeCommChannel(send_data);
	TestMessageConsumer fakeConsumer;
	SecureChannelServer secureChannel(&fakeCommChannel, &fakeConsumer, 5000);

	int data_length = android_request((char*)commandFromAndroid.c_str(), send_data);
	debug("Sent bytes: ", false); debug(data_length);

	secureChannel.loop();
	secureChannel.loop();

}

void test_spark_to_android(String responseFromSpark) {
	uint8_t buffer[180];

//	int data_length = Garage::getInstance().encryptResponse((char*)responseFromSpark.c_str(), buffer);
//	debug("Sending bytes: ", false); debug(data_length);

	String response = decrypt_spark_data(buffer);
	debug("Received: ", false); debug(response);
}


#endif /* LIBRARIES_GARAGE_TESTS_TEST_GARAGE_H_ */
