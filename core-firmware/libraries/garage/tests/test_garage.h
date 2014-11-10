/*
 * test_garage.h
 *
 *  Created on: Nov 9, 2014
 *      Author: val
 */

#ifndef LIBRARIES_GARAGE_TESTS_TEST_GARAGE_H_
#define LIBRARIES_GARAGE_TESTS_TEST_GARAGE_H_


#include "utils.h"
#include "master_key.h"
#include "Garage.h"
#include "RandomSeeds.h"
#include "tropicssl/sha1.h"
#include "tropicssl/aes.h"

/**
 * Puts together the following data for transmission:
 *
 * 		<Message_Length[2], IV_Send[16], IV_Response[16], AES_CBC(Key, IV_Send, COMMAND), <==== HMAC(Key)[20]>
 */
int android_request(char* command, uint8_t send_data[]) {
	uint32_t iv_send[4]; RandomNumberGenerator::getInstance().generateRandomChallengeNonce(iv_send);
	uint32_t iv_response[4]; RandomNumberGenerator::getInstance().generateRandomChallengeNonce(iv_response);

	debug("Sending: ", false); debug(command);

	uint8_t* iv_send_start = send_data + 2;
	memcpy(iv_send_start, iv_send, sizeof(iv_send)); // Add IV_Send[16]

	uint8_t* iv_response_start = iv_send_start + sizeof(iv_send);
	memcpy(iv_response_start, iv_response, sizeof(iv_response)); // Followed by IV_Response[16]

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
	uint8_t* aes_ciphertext_start = iv_response_start + sizeof(iv_response);
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

void test_android_to_spark(String commandFromAndroid) {
	uint8_t send_data[180];

	int data_length = android_request((char*)commandFromAndroid.c_str(), send_data);
	debug("Sent bytes: ", false); debug(data_length);

	String command = Garage::getInstance().decryptCommand(send_data);
	debug("Received: ", false); debug(command);
}


#endif /* LIBRARIES_GARAGE_TESTS_TEST_GARAGE_H_ */
