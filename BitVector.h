#pragma once
#include "part.h"

const unsigned long BitVectorSize = 8 * ClusterSize;

class BitVector
{
	char *bitVector;
	Partition *myPart;
public:
	BitVector(Partition*);
	ClusterNo getEmpty();
	void freeCluster(ClusterNo);
	void format();
};

