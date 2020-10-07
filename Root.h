#pragma once
#include "KernelFS.h"
#include <map>
#include "part.h"
#include "fs.h"
#include "KernelFile.h"
#include "Semaphore.h"
#include "BitVector.h"

const unsigned long DirectoryEntriesNo = 2048/32; // broj ulaza u direktorijumu: 2048/20+12
const unsigned long IndexEntriesNo = 512; // broj ulaza u indeksu: 2048/4 (koliko intova ima)

class KernelFile;

class FileEntry {
	friend class Root;
	friend class KernelFile;
	unsigned long writers, readers, writersWaiting, readersWaiting;
	char *fname;
	char mode;
	Entry *entry;
	ClusterNo entryIndex, lvl2Cluster;
	Semaphore mutex, writeSem, readSem;
public:
	FileEntry(char *fname, char mode, Entry *entry, ClusterNo, BytesCnt);
	Entry* getEntry() { return entry; }
};

class Root {
	friend class KernelFile;

	bool wantToUnmount = false;
	std::map<std::string, FileEntry*> openFiles;
	char root[ClusterSize];
	bool dirtyCache = false;
	Partition *myPartition;
	BitVector *myBitVector;
	Semaphore mutex;

	bool isFreeEntry(Entry* entry);
	bool fileNameCompare(char* fname, Entry* entry);
	FileEntry* newFile(char* fname, char mode);
	ClusterNo allocCluster(ClusterNo* rootCache, char* buffer, ClusterNo pos, ClusterNo off);
	void addEntry(char* fname, Entry* entry);
	FileEntry* doesExistFE(char* fname, char mode);
public:
	Semaphore filesClosed;

	Root(Partition*, BitVector*);
	~Root();

	void readCluster(ClusterNo c, char *buffer);
	void writeCluster(ClusterNo c, const char *buffer);
	void signalUnmount();
	void format();
	FileCnt fileCount();
	char doesExist(char* fname);
	File* open(char* fname, char mode);
	void close(KernelFile*);
	char deleteFile(char* fname);
	void free(ClusterNo clNo);
};

