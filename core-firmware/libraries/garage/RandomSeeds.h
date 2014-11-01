/**
 * This class represents the garage. Specifics of processing Garage commands are handled here.
 *
 * @author Val Blant
 */

#ifndef LIBRARIES_RANDOM_SEEDS_H_
#define LIBRARIES_RANDOM_SEEDS_H_

#define EXTERNAL_FLASH_START_ADDRESS 0x80000
#define NUMBER_OF_SEEDS 0xFFFF
#define SEEDS_SIZE (sizeof(uint16_t) * 3) // 48-bit seeds
#define CURRENT_SEED_INDEX_ADDRESS 	(EXTERNAL_FLASH_START_ADDRESS + (NUMBER_OF_SEEDS * SEEDS_SIZE) + SEEDS_SIZE) // 0xE0000

// Globals used to cache the index. We read from Flash only once.
//
bool readSeedIndexFromFlashInd = true;
uint16_t currentSeedIndex;

/**
 * Reads the current seed index from External Flash
 */
uint16_t readRandomSeedIndexFromFlash() {
	if ( readSeedIndexFromFlashInd ) {
		sFLASH_ReadBuffer((uint8_t*)&currentSeedIndex, CURRENT_SEED_INDEX_ADDRESS, sizeof(currentSeedIndex));
		readSeedIndexFromFlashInd = false; // Read from flash only the first time after reboot

		debug("Reading seed index from flash: ", false); debug(currentSeedIndex);
	}

	return currentSeedIndex;
}

/**
 * Reads persisted seed index, increments it and stores it back into Flash
 */
void initNewRandomSeed() {
	readRandomSeedIndexFromFlash();

	currentSeedIndex++;

	debug("Persisting new seed index: ", false); debug(currentSeedIndex);

	sFLASH_EraseSector(CURRENT_SEED_INDEX_ADDRESS);
	sFLASH_WriteBuffer((uint8_t*)&currentSeedIndex, CURRENT_SEED_INDEX_ADDRESS, sizeof(currentSeedIndex));
}

/**
 * Reads a pre-computed PRGA seed from External Flash
 */
void readRandomSeedFromFlash(uint16_t seedvec[]) {
	readRandomSeedIndexFromFlash();

	sFLASH_ReadBuffer((uint8_t*)seedvec, EXTERNAL_FLASH_START_ADDRESS + (SEEDS_SIZE * currentSeedIndex), SEEDS_SIZE);

	debug("------------");
	debug(seedvec[0]);
	debug(seedvec[1]);
	debug(seedvec[2]);
	debug("------------");
}

#endif /* LIBRARIES_RANDOM_SEEDS_H */
