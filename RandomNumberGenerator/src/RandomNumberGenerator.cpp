//============================================================================
// Name        : RandomNumberGenerator.cpp
// Author      : Val Blant
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
using namespace std;

int main() {
	srand( time(NULL) );

	FILE *file = fopen("seeds.bin", "wb");

	unsigned short int seedvec[3];

	for ( int i = 0; i <= 0xFFFF; i++ ) {
		int r = rand();
		memcpy(&seedvec, &r, 4);

		r = rand() & 0xffff;
		memcpy(&seedvec[2], &r, 2);

//		printf("%04x %04x %04x\n", seedvec[0], seedvec[1], seedvec[2]);
		fwrite(seedvec, sizeof(seedvec), 1, file);
	}

	// Initialize the seed index to 0
	unsigned short int index = 1;
	fwrite(&index, sizeof(index), 1, file);

	fclose(file);

	cout << "!!!Done!!!" << endl;
	return 0;
}
