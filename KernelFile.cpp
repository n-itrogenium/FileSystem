#include "KernelFile.h"

KernelFile::KernelFile(FileEntry *file, Root *root, char m) : cursor(0), cluster(0), offset(0), myRoot(root), mode(m), fileEntry(file) {
	size = fileEntry->entry->fileSize;
	if (mode == 'r') seek(0);
	if (mode == 'w') truncate();
	if (mode == 'a') seek(size);
}

char KernelFile::write(BytesCnt b, char * buffer) {
	BytesCnt cnt;
	for (cnt = 0; cnt < b && writeByte(buffer + cnt); cnt++);
	return (cnt == b);
}

bool KernelFile::writeByte(char * buffer) {
	ClusterNo ind1 = cluster / IndexEntriesNo;
	ClusterNo ind2 = cluster % IndexEntriesNo;
	char cache1[ClusterSize], cache2[ClusterSize], dataCache[ClusterSize];

	// LEVEL 1
	clusterLvl1 = fileEntry->entry->firstIndexCluster;
	myRoot->readCluster(clusterLvl1, cache1);

	if ((size % (ClusterSize*IndexEntriesNo) == 0) && (size == cursor)) {
		clusterLvl2 = ((ClusterNo*)cache1)[ind1] = myRoot->myBitVector->getEmpty();
		myRoot->writeCluster(clusterLvl1, cache1);
	}
	else {
		clusterLvl2 = ((ClusterNo*)cache1)[ind1];
	}

	//LEVEL 2
	myRoot->readCluster(clusterLvl2, cache2);

	if (size % ClusterSize == 0 && cursor == size) {
		clusterData = ((ClusterNo*)cache2)[ind2] = myRoot->myBitVector->getEmpty();
		myRoot->writeCluster(clusterLvl2, cache2);
	}
	else {
		clusterData = ((ClusterNo*)cache2)[ind2];
	}

	//DATA
	myRoot->readCluster(clusterData, dataCache);
	dataCache[cursor%ClusterSize] = *buffer;
	myRoot->writeCluster(clusterData, dataCache);
	size++;
	seek(cursor + 1);
	return true;
}

BytesCnt KernelFile::read(BytesCnt b, char * buffer) {
	BytesCnt cnt;
	for (cnt = 0; cnt < b && readByte(buffer + cnt); cnt++);
	return cnt;
}

bool KernelFile::readByte(char * buffer) {
	if (cursor >= size) return false;

	ClusterNo ind1 = cluster / IndexEntriesNo;
	ClusterNo ind2 = cluster % IndexEntriesNo;
	char cache1[ClusterSize], cache2[ClusterSize], dataCache[ClusterSize];

	//LEVEL 1
	clusterLvl1 = fileEntry->entry->firstIndexCluster;
	myRoot->readCluster(clusterLvl1, cache1);

	//LEVEL2
	clusterLvl2 = ((ClusterNo*)cache1)[ind1];
	myRoot->readCluster(clusterLvl2, cache2);

	//DATA
	clusterData = ((ClusterNo*)cache2)[ind2];
	myRoot->readCluster(clusterData, dataCache);
	*buffer = dataCache[offset];
	seek(cursor + 1);

	return true;
}

char KernelFile::seek(BytesCnt size) {
	cursor = size;
	cluster = cursor / ClusterSize;
	offset = cursor % ClusterSize;
	return 1;
}

BytesCnt KernelFile::filePos() { return cursor; }

char KernelFile::eof() {
	if (cursor < size) return 0;
	if (cursor > size) return 1;
	return 2; // cursor == size
}

BytesCnt KernelFile::getFileSize() { return size; }

char KernelFile::truncate() {
	ClusterNo data = size / ClusterSize + (size%ClusterSize ? 1 : 0);
	char cache1[ClusterSize], cache2[ClusterSize];

	clusterLvl1 = fileEntry->entry->firstIndexCluster;
	myRoot->readCluster(clusterLvl1, cache1);

	for (ClusterNo i = 0; i < data; i++) {
		if (i % IndexEntriesNo == 0) 
			myRoot->readCluster(cache1[i / IndexEntriesNo], cache2);
		myRoot->free(((ClusterNo*)cache2)[i%IndexEntriesNo]);
	}

	ClusterNo lvl2 = data / IndexEntriesNo + (data%IndexEntriesNo ? 1 : 0);

	for (ClusterNo i = 0; i < lvl2; i++)
		myRoot->free(((ClusterNo*)cache1)[i]);

	seek(0);
	size = 0;
	return 1;
}


KernelFile::~KernelFile() { 
	myRoot->close(this);
}
