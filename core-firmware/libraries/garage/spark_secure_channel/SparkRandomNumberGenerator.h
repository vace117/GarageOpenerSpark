/**
 * This is a pseudo-random number generator.
 *
 * The entropy used for seeding the PRNG function (seed48) is mixed together from 3 different sources:
 * 	1) rand48 is seeded with one of 65536 pre-computed 48-bit seeds, stored in External Flash.
 * 		Every time the Spark reboots, the next seed is used.
 *
 * 	2) A specified network server is pinged 10 times. Each ping time is used as a round of HMAC(Master_Key, ping_time).
 * 		The first 128 bits of the resulting HMAC is additional entropy XORed with every call to rand48.
 *
 * 	3) The generated 128-bit random number is XORed with first 128 bits of HMAC(Master_key, Current_Timestamp).
 * 		So the time at which the random number was requested is used for additional entropy.
 *
 * @author Val Blant
 */

#ifndef LIBRARIES_RANDOM_SEEDS_H_
#define LIBRARIES_RANDOM_SEEDS_H_

#include <stdlib.h>
#include <master_key.h> // Contains the super secret shared key
#include <tropicssl/sha1.h>



#define EXTERNAL_FLASH_START_ADDRESS 0x80000
#define NUMBER_OF_SEEDS 0xFFFF
#define SEEDS_SIZE (sizeof(uint16_t) * 3) // 48-bit seeds
#define CURRENT_SEED_INDEX_ADDRESS 	(EXTERNAL_FLASH_START_ADDRESS + (NUMBER_OF_SEEDS * SEEDS_SIZE) + SEEDS_SIZE) // 0xE0000




//#define PING_TEST_SERVER	// Comment this out to disable gathering of entropy from test server pings
//#define ROTATE_SEED	 // Comment this out to disable seed rotation. This saves on External Flash writes

//#define DEBUG_PRINT_SEED
//#define DEBUG_PRINT_PING_ENTROPY
//#define DEBUG_PRINT_TIMER_ENTROPY
//#define DEBUG_PRINT_NONCE

/**
 * Singleton class
 */
class SparkRandomNumberGenerator {
public:
	/**
	 * This class is a singleton.
	 */
	static SparkRandomNumberGenerator& getInstance() {
		// Guaranteed to be destroyed. Instantiated on first use.
		//
		static SparkRandomNumberGenerator instance;
		return instance;
	}

	/**
	 * Fills challengeNonce with 128 bits of randomness
	 */
	void generateRandomChallengeNonce(uint32_t challengeNonce[]);


private:
	SparkRandomNumberGenerator() :
			seed_vector { 0, 0, 0 },
			current_seed_index(0),
			testServerIP(8, 8, 8, 8), // A DNS server
			networkEntropy {0, 0, 0, 0} {

		//
		// We don't init entropy from timer here, b/c doing so on the first request makes the value much more
		// unpredictable
		//

	};

	// Make sure these are unaccessible. Otherwise we may accidently get copies of
	// the singleton appearing.
	//
	SparkRandomNumberGenerator(SparkRandomNumberGenerator const&);
	void operator=(SparkRandomNumberGenerator const&);

	uint16_t seed_vector[3];
	uint16_t current_seed_index;
	IPAddress testServerIP;
	uint32_t networkEntropy[4];

	void initializeRandomness();
	void rotateRandomSeed();
	void readRandomSeedIndexFromFlash();
	void readRandomSeedFromFlash();
	void getEntropyFromTimer(uint32_t timerEntropy[]);
	void initEntropyFromNetwork();
	netapp_pingreport_args_t pingTestServer();
};

/**
 * Reads the current seed index from External Flash
 */
void SparkRandomNumberGenerator::readRandomSeedIndexFromFlash() {
	sFLASH_ReadBuffer((uint8_t*) &current_seed_index,
			CURRENT_SEED_INDEX_ADDRESS, sizeof(current_seed_index));

#ifdef DEBUG_PRINT_SEED
	debug("Reading seed index from flash: ", false);
	debug(current_seed_index);
#endif
}

/**
 * Reads persisted seed index, increments it and stores it back into Flash
 */
void SparkRandomNumberGenerator::rotateRandomSeed() {
	readRandomSeedIndexFromFlash();

#ifdef ROTATE_SEED
	current_seed_index++;

	debug("Persisting new seed index: ", false); debug(current_seed_index);

	sFLASH_EraseSector(CURRENT_SEED_INDEX_ADDRESS);
	sFLASH_WriteBuffer((uint8_t*)&current_seed_index, CURRENT_SEED_INDEX_ADDRESS, sizeof(current_seed_index));
#endif
}

/**
 * Reads a pre-computed PRGA seed from External Flash
 */
void SparkRandomNumberGenerator::readRandomSeedFromFlash() {
	readRandomSeedIndexFromFlash();

	sFLASH_ReadBuffer((uint8_t*) seed_vector,
			EXTERNAL_FLASH_START_ADDRESS + (SEEDS_SIZE * current_seed_index),
			SEEDS_SIZE);

#ifdef DEBUG_PRINT_SEED
	debug("--- SEED ---");
	debug(seed_vector[0]);
	debug(seed_vector[1]);
	debug(seed_vector[2]);
	debug("------------");
#endif
}

void SparkRandomNumberGenerator::initializeRandomness() {
	if (seed_vector[0] == 0) {
		rotateRandomSeed();

		readRandomSeedFromFlash();

		seed48(seed_vector); // Seed the PRGA

		// Collect entropy from pinging testServerIP at startup
		initEntropyFromNetwork();
	}
}

void SparkRandomNumberGenerator::generateRandomChallengeNonce(uint32_t challengeNonce[4]) {
	if (seed_vector[0] == 0) {
		initializeRandomness();
	}

	uint32_t entropyFromTimer[4];
	this->getEntropyFromTimer(entropyFromTimer);

	challengeNonce[0] = mrand48() ^ entropyFromTimer[0] ^ networkEntropy[0];
	challengeNonce[1] = mrand48() ^ entropyFromTimer[1] ^ networkEntropy[1];
	challengeNonce[2] = mrand48() ^ entropyFromTimer[2] ^ networkEntropy[2];
	challengeNonce[3] = mrand48() ^ entropyFromTimer[3] ^ networkEntropy[3];

#ifdef DEBUG_PRINT_NONCE
	debug("--- NONCE ---");
	Serial.print(challengeNonce[0], HEX);
	Serial.print(challengeNonce[1], HEX);
	Serial.print(challengeNonce[2], HEX);
	Serial.println(challengeNonce[3], HEX);
	debug("------------");
#endif
}

/**
 * Reads the current time in millis and returns a 128-bit HMAC of that value with our Master Key
 *
 * HMAC is used for key expansion here, since our random value is 32 bits long, whereas we require
 * 128-bit of randomness
 */
void SparkRandomNumberGenerator::getEntropyFromTimer(uint32_t timerEntropy[4]) {
	uint32_t mils = millis();
	unsigned char hmac[20];

	sha1_hmac(	(uint8_t*) MASTER_KEY, sizeof(MASTER_KEY),
				(uint8_t*) &mils, sizeof(mils),
				hmac);

	memcpy(timerEntropy, hmac, 16);

#ifdef DEBUG_PRINT_TIMER_ENTROPY
	debug("--- TIMER ---");
	debug(mils);
	Serial.print(timerEntropy[0], HEX);
	Serial.print(timerEntropy[1], HEX);
	Serial.print(timerEntropy[2], HEX);
	Serial.println(timerEntropy[3], HEX);
	debug("------------");
#endif
}

/**
 * Pings the test server 10 times and uses each number as HMAC round input. The first 16 bytes of resulting
 * HMAC is our network entropy.
 */
void SparkRandomNumberGenerator::initEntropyFromNetwork() {
	uint32_t pingSum = 0;
	unsigned char hmac[20];

	sha1_context ctx;
	sha1_hmac_starts(&ctx, (uint8_t*) MASTER_KEY, sizeof(MASTER_KEY));

	for ( int i = 0; i < 10; i++ ) {
#ifdef PING_TEST_SERVER
		debug("Gathering entropy from network...");
		pingSum = this->pingTestServer().avg_round_time;
#else
		pingSum = 43;
#endif
#ifdef DEBUG_PRINT_PING_ENTROPY
		debug(pingSum);
#endif
		sha1_hmac_update(&ctx, (uint8_t*) &pingSum, sizeof(pingSum));
	}

	sha1_hmac_finish(&ctx, hmac);

	memcpy(networkEntropy, hmac, 16);

#ifdef DEBUG_PRINT_PING_ENTROPY
	debug("--- PING ---");
	Serial.print(networkEntropy[0], HEX);
	Serial.print(networkEntropy[1], HEX);
	Serial.print(networkEntropy[2], HEX);
	Serial.println(networkEntropy[3], HEX);
	debug("------------");
#endif
}

/**
 * Ping the test server 3 times and return the report
 */
netapp_pingreport_args_t SparkRandomNumberGenerator::pingTestServer() {
	uint8_t nTries = 3;
	uint32_t pingIPAddr = testServerIP[3] << 24 | testServerIP[2] << 16
			| testServerIP[1] << 8 | testServerIP[0];
	unsigned long pingSize = 32UL;
	unsigned long pingTimeout = 500UL; // in milliseconds

	memset(&ping_report, 0, sizeof(netapp_pingreport_args_t));
	ping_report_num = 0;

	long psend = netapp_ping_send((UINT32*) &pingIPAddr, (unsigned long) nTries,
			pingSize, pingTimeout);
	unsigned long lastTime = millis();
	while (ping_report_num == 0
			&& (millis() < lastTime + 2 * nTries * pingTimeout)) {
	}
	return ping_report;
}

#endif /* LIBRARIES_RANDOM_SEEDS_H */
