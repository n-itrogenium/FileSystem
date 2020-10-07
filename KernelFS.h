#pragma once
#include "fs.h"
#include "Semaphore.h"
#include "BitVector.h"

class Partition;
class BitVector;
class Root;
class File;

struct Entry {
	char name[FNAMELEN]; // 8B
	char ext[FEXTLEN]; // 3B
	char reserved = 0; // 1B
	unsigned long firstIndexCluster; // 4B
	BytesCnt fileSize; // 4B
	char extra[EXTRALEN] = { 0 }; // 12B
};

class KernelFS
{
	Partition *myPartition;
	BitVector *myBitVector;
	Root *myRoot;
	Semaphore readyToMount;
public:
	KernelFS();
	char mount(Partition* partition); //montira particiju
	// vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
	char unmount(); //demontira particiju
	// vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
	char format(); //formatira particiju;
	// vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
	FileCnt readRootDir();
	// vraca -1 u slucaju neuspeha ili broj fajlova u slucaju uspeha
	char doesExist(char* fname); //argument je naziv fajla sa
	//apsolutnom putanjom
	File* open(char* fname, char mode);
	char deleteFile(char* fname);
	~KernelFS();
};

