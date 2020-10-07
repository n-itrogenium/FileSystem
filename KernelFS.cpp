#include "KernelFS.h"
#include "Semaphore.h"
#include "part.h"
#include "Root.h"

KernelFS::KernelFS() : myPartition(nullptr), myBitVector(nullptr), myRoot(nullptr) {
	readyToMount = CreateSemaphore(0, 0, 1, 0);
}

char KernelFS::mount(Partition * partition) {
	if (myPartition != nullptr) wait(readyToMount);
	myPartition = partition;
	myBitVector = new BitVector(myPartition);
	myRoot = new Root(myPartition, myBitVector);
	return 1;
}

char KernelFS::unmount() {
	myRoot->signalUnmount();
	wait(myRoot->filesClosed);
	myPartition = nullptr;
	delete myBitVector;
	delete myRoot;
	signal(readyToMount);
	return 0;
}

char KernelFS::format() {
	if (myPartition != nullptr) {
		myBitVector->format();
		myRoot->format();
		return 1;
	}
	return 0;
}

FileCnt KernelFS::readRootDir() {
	if (myRoot == nullptr) return -1;
	return myRoot->fileCount();
}

char KernelFS::doesExist(char * fname) {
	if (myRoot == nullptr) return 0;
	return myRoot->doesExist(fname);
}

File * KernelFS::open(char * fname, char mode) {
	if (myRoot == nullptr) return nullptr;
	return myRoot->open(fname, mode);
}

char KernelFS::deleteFile(char * fname) {
	if (myRoot == nullptr) return 0;
	return myRoot->deleteFile(fname);
}


KernelFS::~KernelFS() { unmount(); }
