#pragma once
#include "file.h"
#include "Root.h"

class Root;
class FileEntry;

const ClusterNo clusterInd = 512ul;

class KernelFile {
	friend class Root;

	Root *myRoot;
	char mode;
	BytesCnt size;
	FileEntry *fileEntry;

	BytesCnt cursor, cluster, offset;
	ClusterNo clusterLvl1, clusterLvl2, clusterData;

public:
	KernelFile(FileEntry*, Root*, char);
	char write(BytesCnt, char* buffer);
	bool writeByte(char* buffer);
	BytesCnt read(BytesCnt, char* buffer);
	bool readByte(char* buffer);
	char seek(BytesCnt);
	BytesCnt filePos();
	char eof();
	BytesCnt getFileSize();
	char truncate();
	~KernelFile(); 
};

