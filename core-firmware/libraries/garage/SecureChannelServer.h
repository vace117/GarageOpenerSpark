/*
 * This class runs the main event loop of listening for encrypted messages from clients
 * and sending back encrypted responses.
 *
 *  Created on: Nov 16, 2014
 *      Author: val
 */

#ifndef LIBRARIES_GARAGE_SECURECHANNELSERVER_H_
#define LIBRARIES_GARAGE_SECURECHANNELSERVER_H_

#include "tropicssl/sha1.h"
#include "tropicssl/aes.h"
#include <master_key.h> // Contains the super secret shared key
#include <string.h>
#include "RandomSeeds.h"

/**
 * Interface to be implemented by the message consumer
 */
class SecureMessageConsumer {
public:
	virtual ~SecureMessageConsumer() {}
	virtual String processMessage(String message) = 0;
};

class CommunicationChannel {
public:
	virtual ~CommunicationChannel() {}
	virtual int read(uint8_t *buffer, size_t size) = 0;
	virtual size_t write(const uint8_t *buffer, size_t size) = 0;
};

#define MAX_MESSAGE_SIZE 256

class SecureChannelServer {
public:
	SecureChannelServer(CommunicationChannel* cc, SecureMessageConsumer* mc) :
			msgLength(0), receive_buffer {0}, send_buffer {0}, msgState(NEED_MSG_LENGTH)
	{
		commChannel = cc;
		msgConsumer = mc;
	}

	void loop();

private:
	CommunicationChannel* commChannel;
	SecureMessageConsumer* msgConsumer;

	int msgLength;
	uint8_t receive_buffer[MAX_MESSAGE_SIZE];
	uint8_t send_buffer[MAX_MESSAGE_SIZE];
	enum MessageState { NEED_MSG_LENGTH, RECEIVING_MSG };
	MessageState msgState;

	void reset_buffers();
	int processData(uint8_t received_data[], uint8_t response_data[]);
	String decryptMessage(uint8_t received_data[]);
	int encryptResponse(char* response, uint8_t response_data[]);
};



String SecureChannelServer::decryptMessage(uint8_t received_data[]) {

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
		debug("BAD HMAC received!\n");
		return "";
	}

	// Grab the IV that was used to encrypt this data
	//
	uint32_t iv_send[4];
	uint8_t* iv_send_start = received_data + 2;
	memcpy(iv_send, iv_send_start, sizeof(iv_send));

	uint8_t* ciphertext_start = iv_send_start + 16;

	// Decrypt the message
	//
	aes_context aes;
	int aes_buffer_length = hmac_data_length - (ciphertext_start - received_data);

	uint8_t ciphertext[aes_buffer_length];
	uint8_t plaintext[aes_buffer_length];
	memcpy(ciphertext, ciphertext_start, aes_buffer_length);

	aes_setkey_dec(&aes, (uint8_t*) MASTER_KEY, 128);
	aes_crypt_cbc(&aes, AES_DECRYPT, aes_buffer_length, (uint8_t*)iv_send, ciphertext, plaintext);

	// Remove PKCS #7 padding from our message by padding with zeroes
	//
	int message_size = aes_buffer_length - plaintext[aes_buffer_length - 1];
	memset(plaintext + message_size, 0, aes_buffer_length - message_size);

	return (char*) plaintext; // A String object is created here
}


/**
 * Puts together the following data for transmission:
 *
 * 		<Message_Length[2], IV_Response[16], AES_CBC(Key, IV_Response, COMMAND), <==== HMAC(Key)[20]>
 */
int SecureChannelServer::encryptResponse(char* response, uint8_t response_data[]) {
	uint32_t iv_response[4]; RandomNumberGenerator::getInstance().generateRandomChallengeNonce(iv_response);

	uint8_t* iv_response_start = response_data + 2;
	memcpy(iv_response_start, iv_response, sizeof(iv_response)); // Add IV_Response[16]

	// Figure out the length of the command we are sending, and the appropriate padding
	//
	int msg_length = strlen(response);
	int aes_buffer_length = (msg_length & ~15) + 16; // Round up to next 16 byte length
	char pad = aes_buffer_length - msg_length;

	// Setup plaintext buffer
	//
	uint8_t aes_buffer[aes_buffer_length];
	memcpy(aes_buffer, response, msg_length); // Add the message
	memset(aes_buffer + msg_length, pad, pad); // Followed by PKCS #7 padding

	// Encrypt the plaintext
	//
	aes_context aes;
	uint8_t aes_buffer_encrypted[aes_buffer_length];
	aes_setkey_enc(&aes, (uint8_t*) MASTER_KEY, 128);
	aes_crypt_cbc(&aes, AES_ENCRYPT, aes_buffer_length, (uint8_t*)iv_response, aes_buffer, aes_buffer_encrypted);

	// Add encrypted buffer
	//
	uint8_t* aes_ciphertext_start = iv_response_start + sizeof(iv_response);
	memcpy(aes_ciphertext_start, aes_buffer_encrypted, aes_buffer_length);

	// Calculate data length field
	//
	unsigned char hmac[20];
	uint8_t* hmac_start = aes_ciphertext_start + aes_buffer_length;
	uint16_t dataLength = hmac_start + sizeof(hmac) - response_data;
	memcpy(response_data, &dataLength, 2);

	// Calculate HMAC(Key) of all data in send_data so far
	//
	sha1_hmac(	(uint8_t*) MASTER_KEY, sizeof(MASTER_KEY),
				response_data, hmac_start - response_data,
				hmac);

	// Append the HMAC to send_data
	//
	memcpy(hmac_start, hmac, sizeof(hmac));

	// Return the length of prepared send_data
	//
	uint8_t* end_of_data = hmac_start + sizeof(hmac);
	return end_of_data - response_data;
}



void SecureChannelServer::reset_buffers() {
	memset(receive_buffer, 0, MAX_MESSAGE_SIZE);
	memset(send_buffer, 0, MAX_MESSAGE_SIZE);
	msgLength = 0;
	msgState = NEED_MSG_LENGTH;

}

int SecureChannelServer::processData(uint8_t received_data[], uint8_t response_data[]) {
	String decryptedMessage = decryptMessage(received_data);
	debug("Received message: ", 0); debug(decryptedMessage);


	String response;
	char* responseBytes;

	if ( decryptedMessage == "NEED_CHALLENGE" ) {
		// Generate a challenge nonce
		//
		debug("Generating session nonce...");
		uint32_t challenge[4];
		RandomNumberGenerator::getInstance().generateRandomChallengeNonce(challenge);
		responseBytes = (char*) challenge;
	}
	else {
		// Pass the message to the consumer
		//
		response = msgConsumer->processMessage(decryptedMessage);
		debug("Consumer answered: ", 0); debug(response);
		responseBytes = (char*)response.c_str();
	}


	int responseLength = 0;

	if ( response.length() > 0 ) {
		responseLength = encryptResponse(responseBytes, response_data);
	}

	return responseLength;

}

void SecureChannelServer::loop() {
	if ( msgState == NEED_MSG_LENGTH ) {
		uint8_t msgLengthBuffer[2];
		commChannel->read(msgLengthBuffer, 2);
		memcpy(&msgLength, msgLengthBuffer, 2);

		if ( msgLength > 0 && msgLength < MAX_MESSAGE_SIZE ) {
			memcpy(receive_buffer, msgLengthBuffer, 2);
			msgState = RECEIVING_MSG;
		}
		else {
			reset_buffers();
		}

		debug("Incoming message length: ", 0); debug(msgLength);

	}
	else if (msgState == RECEIVING_MSG) {
		uint8_t* msg_data = receive_buffer + 2;
		int bytesRead = commChannel->read(msg_data, msgLength - 2);
		if ( bytesRead == (msgLength - 2) ) {
			int response_length = processData(receive_buffer, send_buffer);

			if ( response_length > 0 ) {
				commChannel->write(send_buffer, response_length);
			}
		}

		reset_buffers();
	}
}

#endif /* LIBRARIES_GARAGE_SECURECHANNELSERVER_H_ */
