#pragma once

// File: file.h
#include "fs.h"
#include "Root.h"
#include "KernelFile.h"

class KernelFile;
class Root;
class FileEntry;

class File {
	friend class Root;
public:
	~File(); //zatvaranje fajla
	char write(BytesCnt, char* buffer);
	BytesCnt read(BytesCnt, char* buffer);
	char seek(BytesCnt);
	BytesCnt filePos();
	char eof();
	BytesCnt getFileSize();
	char truncate();
private:
	friend class FS;
	friend class KernelFS;
	File(FileEntry*, Root*, char); //objekat fajla se može kreirati samo otvaranjem
	KernelFile *myImpl;
};