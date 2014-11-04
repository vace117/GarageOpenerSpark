/**
 * This class represents the garage. Specifics of processing Garage commands are handled here.
 *
 * @author Val Blant
 */

#ifndef LIBRARIES_RANDOM_SEEDS_H_
#define LIBRARIES_RANDOM_SEEDS_H_

#include <stdlib.h>
#include <master_key.h>
#include "tropicssl/sha1.h"


#define EXTERNAL_FLASH_START_ADDRESS 0x80000
#define NUMBER_OF_SEEDS 0xFFFF
#define SEEDS_SIZE (sizeof(uint16_t) * 3) // 48-bit seeds
#define CURRENT_SEED_INDEX_ADDRESS 	(EXTERNAL_FLASH_START_ADDRESS + (NUMBER_OF_SEEDS * SEEDS_SIZE) + SEEDS_SIZE) // 0xE0000

class RandomNumberGenerator {
public:
	RandomNumberGenerator() : seedvec {0,0,0}, currentSeedIndex(0) {

		//
		// We don't init randomness here, b/c doing so on the first request allows us to use
		// the current time as an entropy source
		//

	};

	void initializeRandomness();
	void generateRandomChallengeNonce(uint32_t challengeNonce[]);

private:
	uint16_t seedvec[3];
	uint16_t currentSeedIndex;

	void rotateRandomSeed();
	void readRandomSeedIndexFromFlash();
	void readRandomSeedFromFlash();
	void entropyFromTimer(uint32_t timerEntropy[]);
	void entropyFromNetwork(uint32_t networkEntropy[]);

};

/**
 * Reads the current seed index from External Flash
 */
void RandomNumberGenerator::readRandomSeedIndexFromFlash() {
	sFLASH_ReadBuffer((uint8_t*)&currentSeedIndex, CURRENT_SEED_INDEX_ADDRESS, sizeof(currentSeedIndex));

	debug("Reading seed index from flash: ", false); debug(currentSeedIndex);
}

/**
 * Reads persisted seed index, increments it and stores it back into Flash
 */
void RandomNumberGenerator::rotateRandomSeed() {
	readRandomSeedIndexFromFlash();

//	currentSeedIndex++;
//
//	debug("Persisting new seed index: ", false); debug(currentSeedIndex);
//
//	sFLASH_EraseSector(CURRENT_SEED_INDEX_ADDRESS);
//	sFLASH_WriteBuffer((uint8_t*)&currentSeedIndex, CURRENT_SEED_INDEX_ADDRESS, sizeof(currentSeedIndex));
}

/**
 * Reads a pre-computed PRGA seed from External Flash
 */
void RandomNumberGenerator::readRandomSeedFromFlash() {
	readRandomSeedIndexFromFlash();

	sFLASH_ReadBuffer((uint8_t*)seedvec, EXTERNAL_FLASH_START_ADDRESS + (SEEDS_SIZE * currentSeedIndex), SEEDS_SIZE);

	debug("--- SEED ---");
	debug(seedvec[0]);
	debug(seedvec[1]);
	debug(seedvec[2]);
	debug("------------");
}

void RandomNumberGenerator::initializeRandomness() {
	if ( seedvec[0] == 0 ) {
		rotateRandomSeed();

		readRandomSeedFromFlash();

		seed48(seedvec); // Seed the PRGA
	}
}

void RandomNumberGenerator::generateRandomChallengeNonce(uint32_t challengeNonce[]) {
	if ( seedvec[0] == 0 ) {
		initializeRandomness();
	}

	uint32_t entropyFromTimer[4];
	this->entropyFromTimer(entropyFromTimer);

	challengeNonce[0] = mrand48() ^ entropyFromTimer[0];
	challengeNonce[1] = mrand48() ^ entropyFromTimer[1];
	challengeNonce[2] = mrand48() ^ entropyFromTimer[2];
	challengeNonce[3] = mrand48() ^ entropyFromTimer[3];

	debug("--- NONCE ---");
	Serial.print(	challengeNonce[0], HEX);
	Serial.print(	challengeNonce[1], HEX);
	Serial.print(	challengeNonce[2], HEX);
	Serial.println(	challengeNonce[3], HEX);
	debug("------------");

}

/**
 * Reads the current time in millis and returns a 128-bit HMAC of that value with our Master Key
 */
void RandomNumberGenerator::entropyFromTimer(uint32_t timerEntropy[]) {
	uint32_t mils =  millis();
	unsigned char hmac[20];

	sha1_hmac(
			(uint8_t*)MASTER_KEY, sizeof(MASTER_KEY),
			(uint8_t*)&mils, sizeof(mils),
			hmac);

	memcpy(timerEntropy, hmac, 16);

	debug("--- TIMER ---");
	debug( mils );
	Serial.print(	timerEntropy[0], HEX);
	Serial.print(	timerEntropy[1], HEX);
	Serial.print(	timerEntropy[2], HEX);
	Serial.println(	timerEntropy[3], HEX);
	debug("------------");
}

void RandomNumberGenerator::entropyFromNetwork(uint32_t networkEntropy[]) {
	// TODO
}

#endif /* LIBRARIES_RANDOM_SEEDS_H */
