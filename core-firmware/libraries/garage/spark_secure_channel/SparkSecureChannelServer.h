/*
 * This class runs the main event loop of listening for encrypted transmissions from clients,
 * decrypting the payload, processing or delegating the payload, and finally sending encrypted responses.
 *
 * This server requires a handshake used to defend against replay attacks. The handshake is
 * implemented as follows:
 *	Client 1) encryptAndSend("NEED_CHALLENGE")
 *	Spark 1) Use PRNG to generate random Challenge[16]
 *	Spark 2) Calculate conversationToken == HMAC(Master_Key, Challenge[16])
 *	Spark 3) Start expiration timer
 *	Spark 4) encryptAndSend(Challenge[16])
 *
 * All following messages in the current conversation (limited by expiration timer) are handled as
 * follows:
 * 	Client 1) encryptAndSend( [conversationToken, COMMAND] )
 * 	Spark 1) Make sure that local conversationToken matched the received one, and that conversation has not expired.
 * 	Spark 1a) If conversation is valid, delegate COMMAND to SecureMessageConsumer
 * 	Spark 2a) If not, encryptAndSend("SESSION_EXPIRED")
 *
 *
 * The specifics of sending and receiving data are abstracted into CommunicationChannel.
 *
 * The specifics of processing commands are abstracted into SecureMessageConsumer
 *
 *
 *  Created on: Nov 16, 2014
 *      Author: Val Blant
 */

#ifndef LIBRARIES_SPARKSECURECHANNELSERVER_H_
#define LIBRARIES_SPARKSECURECHANNELSERVER_H_

#include "tropicssl/sha1.h"
#include "tropicssl/aes.h"
#include <master_key.h> // Contains the super secret shared key
#include <spark_secure_channel/SparkRandomNumberGenerator.h>
#include <string.h>
#include <utils.h>
#include <Timer.h>

/**
 * Interface to be implemented by the message consumer.
 */
class SecureMessageConsumer {
public:
	virtual ~SecureMessageConsumer() {}

	/**
	 * Decrypted messages will be provided to tis method
	 */
	virtual String processMessage(String message) = 0;
};

/**
 * Interface to be implemented for a specific communication approach, such as WiFi for example.
 * SecureChannelServer will use this interface to whenever it needs to read or write encrypted data.
 */
class CommunicationChannel {
public:
	virtual ~CommunicationChannel() {}

	/**
	 * Opens/starts the communication channel
	 */
	virtual void open() = 0;

	/**
	 * Read 'size' bytes into provided 'buffer'
	 */
	virtual int read(uint8_t *buffer, size_t size) = 0;

	/**
	 * Write 'size' bytes from the provided 'buffer'
	 */
	virtual size_t write(const uint8_t *buffer, size_t size) = 0;
};

#define MAX_TRANSMISSION_SIZE 256	// 256 - Length[2] - IV[16] - HMAC[20] - CONV_TOKEN[20] = max 198 byte messages and responses

class SecureChannelServer {
public:
	SecureChannelServer(CommunicationChannel* cc, SecureMessageConsumer* mc, int conversationDuration) :
			transmissionLength(0), receive_buffer {0}, send_buffer {0},
			conversationToken {0}, conversationTokenValid(false), conversationTimer(conversationDuration),
			msgState(NEED_TRANSMISSION_LENGTH)
	{
		commChannel = cc;
		msgConsumer = mc;

		reset_transmission_state();
	}

	/**
	 * Call this from the main Spark loop.
	 *
	 * It will process any data available from the CommunicationChannel,
	 * and delegate decrypted messages to SecureMessageConsumer
	 */
	void loop();

private:
	CommunicationChannel* commChannel;
	SecureMessageConsumer* msgConsumer;

	/**
	 * Used to store the first 2 bytes of an incoming transmission, which indicate the length of the
	 * entire transmission
	 */
	int transmissionLength;

	/**
	 * Reserved memory space for holding incoming transmissions
	 */
	uint8_t receive_buffer[MAX_TRANSMISSION_SIZE];

	/**
	 * Reserved memory space for constructing outgoing transmissions
	 */
	uint8_t send_buffer[MAX_TRANSMISSION_SIZE];

	/**
	 * Locally computed Conversation Token ( HMAC(Master_Key, Challenge[16]) )
	 */
	unsigned char conversationToken[20];
	bool conversationTokenValid;

	/**
	 * This timer expires the Conversation token after a specified amount of time
	 */
	Timer conversationTimer;


	/**
	 * Transmission stages
	 */
	enum MessageState { NEED_TRANSMISSION_LENGTH, RECEIVING_TRANSMISSION };
	MessageState msgState;

	/**
	 * Lose all state and start waiting on a new transmission
	 */
	void reset_transmission_state();

	/**
	 * Returns true if conversationTimer has not expired and 'received_conv_token' equals
	 * our local 'conversationToken'
	 */
	bool isConversationValid(uint8_t received_conv_token[]);

	/**
	 * This will be executed in the main loop to make sure that the conversation
	 * is invalidated after conversationDuration milliseconds
	 */
	void invalidateConversationTokenIfExpired();

	/**
	 * Responsible for handling a transmission after it was received in entirety.
	 *
	 * There are a couple of scenarios:
	 *  1) If received payload is "NEED_CHALLENGE", generates Challenge[16], computes conversationToken = HMAC(Master_Key, Challenge[16])
	 *  	and sends Challenge[16] back to the client
	 *
	 *  2) Else verifies that Payload is of the form [conversationToken, MESSAGE], verifies that conversationToken is valid
	 *  	and delegates MESSAGE to consumer. If conversationToken was no longer valid, responds with "SESSION_EXPIRED"
	 *
	 */
	int processReceivedTransmission(uint8_t received_data[], uint8_t response_data[]);

	/**
	 * Extracts and decrypts the payload from a transmission in the following form:
	 *
	 * 	[Message_Length[2], IV_Send[16], AES_CBC(Key, IV_Send, PAYLOAD), <==== HMAC(Master_Key)
	 *
	 */
	int decryptTransmission(uint8_t received_data[], uint8_t decrypted_payload[]);

	/**
	 * Encrypts and encodes the response payload into the following transmission form:
	 *
	 * 	[Message_Length[2], IV_Response[16], AES_CBC(Key, IV_Response, PAYLOAD), <==== HMAC(Master_Key)
	 *
	 */
	int encryptResponsePayload(unsigned char* responseMessage, int payload_length, uint8_t encrypted_response_transmission[]);
};



int SecureChannelServer::decryptTransmission(uint8_t received_data[], uint8_t decrypted_payload[]) {

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
		return -1;
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

	memcpy(decrypted_payload, plaintext, message_size);

	return message_size;
}


int SecureChannelServer::encryptResponsePayload(unsigned char* response_payload, int payload_length, uint8_t encrypted_response_transmission[]) {
	uint32_t iv_response[4]; SparkRandomNumberGenerator::getInstance().generateRandomChallengeNonce(iv_response);

	uint8_t* iv_response_start = encrypted_response_transmission + 2;
	memcpy(iv_response_start, iv_response, sizeof(iv_response)); // Add IV_Response[16]

	// Figure out the length of the payload we are sending, and the appropriate padding
	//
	int aes_buffer_length = (payload_length & ~15) + 16; // Round up to next 16 byte length
	char pad = aes_buffer_length - payload_length;

	// Setup plaintext buffer
	//
	uint8_t aes_buffer[aes_buffer_length];
	memcpy(aes_buffer, response_payload, payload_length); // Add the payload
	memset(aes_buffer + payload_length, pad, pad); // Followed by PKCS #7 padding

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
	uint16_t dataLength = hmac_start + sizeof(hmac) - encrypted_response_transmission;
	memcpy(encrypted_response_transmission, &dataLength, 2);

	// Calculate HMAC(Key) of all data in send_data so far
	//
	sha1_hmac(	(uint8_t*) MASTER_KEY, sizeof(MASTER_KEY),
				encrypted_response_transmission, hmac_start - encrypted_response_transmission,
				hmac);

	// Append the HMAC to send_data
	//
	memcpy(hmac_start, hmac, sizeof(hmac));

	// Return the length of prepared send_data
	//
	uint8_t* end_of_data = hmac_start + sizeof(hmac);
	return end_of_data - encrypted_response_transmission;
}



void SecureChannelServer::reset_transmission_state() {
	memset(receive_buffer, 0, MAX_TRANSMISSION_SIZE);
	memset(send_buffer, 0, MAX_TRANSMISSION_SIZE);
	transmissionLength = 0;
	msgState = NEED_TRANSMISSION_LENGTH;
}

void SecureChannelServer::invalidateConversationTokenIfExpired() {
	if ( conversationTimer.isRunning() && conversationTimer.isElapsed() ) {
		debug("Invalidating Conversation.\n");
		memset(conversationToken, 0, 20);
		conversationTokenValid = false;
	}
}

bool SecureChannelServer::isConversationValid(uint8_t received_conv_token[]) {
	bool valid = false;

	if ( conversationTokenValid && conversationTimer.isRunning() && !conversationTimer.isElapsed() ) {
		if ( memcmp(conversationToken, received_conv_token, 20) == 0 ) {
			valid = true;
		}
	}

	return valid;
}

int SecureChannelServer::processReceivedTransmission(uint8_t received_data[], uint8_t response_data[]) {
	uint8_t decrypted_payload[MAX_TRANSMISSION_SIZE] = {0};
	int decryptedPayloadLength = decryptTransmission(received_data, decrypted_payload);

	unsigned char* responsePayloadBytes;
	int responsePayloadLength = 0; // The length of the response payload
	String messageConsumerResponse; // Used to hold the response payload memory from SecureMessageConsumer

	int responseTransmissionLength = 0; // Total encoded response transmission length


	debug("Received ", 0); debug(decryptedPayloadLength, 0); debug("-byte payload: ", 0); debug((const char *)decrypted_payload);

	if ( decryptedPayloadLength > 0 ) {
		if ( memcmp(decrypted_payload, "NEED_CHALLENGE", decryptedPayloadLength) == 0 ) {
			// Generate a challenge nonce
			//
			debug("Generating Conversation Token...");
			uint32_t challenge[4];
			SparkRandomNumberGenerator::getInstance().generateRandomChallengeNonce(challenge);
			responsePayloadBytes = (unsigned char*) challenge;
			responsePayloadLength = 16;

			// Calculate Conversation Token based on generated challenge
			//
			sha1_hmac(	(uint8_t*) MASTER_KEY, sizeof(MASTER_KEY),
						responsePayloadBytes, responsePayloadLength,
						conversationToken);

			// Start Conversation Timer
			//
			conversationTimer.start();
			conversationTokenValid = true;

	//		debug(conversationToken, 20);
		}
		else {
			// Any other message must contain a Conversation Token prepended to the message in the payload
			//
			debug("Verifying Conversation Token...", 0);

	//		debug("Ours  : ", 0); debug(conversationToken, 20);
	//		debug("Theirs: ", 0); debug(decrypted_message, 20);

			if ( isConversationValid(decrypted_payload) ) {
				debug(" OK");

				// Pass the message to the consumer
				//
				messageConsumerResponse = msgConsumer->processMessage((const char*)(decrypted_payload+20));
				debug("Consumer answered: ", 0); debug(messageConsumerResponse);

				responsePayloadBytes = (unsigned char*)messageConsumerResponse.c_str();
				responsePayloadLength = messageConsumerResponse.length();
			}
			else {
				debug(" FAILED");

				// Tell the client that this conversation has been closed
				//
				responsePayloadBytes = (unsigned char*) "SESSION_EXPIRED";
				responsePayloadLength = strlen((const char*)responsePayloadBytes);
				debug("Answering: ", 0); debug((const char*)responsePayloadBytes);
			}
		}


		if ( responsePayloadLength > 2 ) {
			responseTransmissionLength = encryptResponsePayload(responsePayloadBytes, responsePayloadLength, response_data);
		}
	}

	return responseTransmissionLength;

}

void SecureChannelServer::loop() {
	invalidateConversationTokenIfExpired();

	if ( msgState == NEED_TRANSMISSION_LENGTH ) {
		uint8_t transmissionLengthBuffer[2] = {0};
		int bytesRead = commChannel->read(transmissionLengthBuffer, 2);

		if ( bytesRead > 0 ) {
			memcpy(&transmissionLength, transmissionLengthBuffer, 2);

			if ( transmissionLength > 0 && transmissionLength < MAX_TRANSMISSION_SIZE ) {
				memcpy(receive_buffer, transmissionLengthBuffer, 2);
				msgState = RECEIVING_TRANSMISSION;
			}
			else {
				reset_transmission_state();
			}

			debug("Incoming transmission length: ", 0); debug(transmissionLength, 0); debug(" bytes");
		}

	}
	else if (msgState == RECEIVING_TRANSMISSION) {
		int bytesRead = commChannel->read(receive_buffer + 2, transmissionLength - 2);
		if ( bytesRead == (transmissionLength - 2) ) {
			int response_length = processReceivedTransmission(receive_buffer, send_buffer);

			if ( response_length > 0 ) {
				debug("Sending ", 0); debug(response_length, 0); debug(" bytes to client...\n");
				commChannel->write(send_buffer, response_length);
			}
		}

		reset_transmission_state();
	}
}

#endif /* LIBRARIES_SPARKSECURECHANNELSERVER_H_ */
