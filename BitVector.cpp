#include "BitVector.h"

BitVector::BitVector(Partition *partition) {
	bitVector = new char[ClusterSize];
	myPart = partition;
	myPart->readCluster(0, bitVector);
}

ClusterNo BitVector::getEmpty() {
	ClusterNo ret = 0;
	for (int i = 0; i < ClusterSize; i++) {
		char read = bitVector[i];
		//if (read != 0)
			for (int j = 0; j < 8; j++, ret++, read >>= 1) 
				if (read & 1) {
					bitVector[i] &= ~(1 << j);
					return ret;
				}		
		//else ret += 8;
	}
}

void BitVector::freeCluster(ClusterNo cluster) {
	bitVector[cluster / 8] |= 1 << (cluster % 8);
}

void BitVector::format() {
	bitVector[0] = (~0) ^ (3);
	for (int i = 1; i < ClusterSize; i++) bitVector[i] = ~0;
	myPart->writeCluster(0, bitVector);
}

