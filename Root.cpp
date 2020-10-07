#define _CRT_SECURE_NO_WARNINGS
#include "Root.h"
#include "BitVector.h"
#include "file.h"
#include <map>

bool Root::isFreeEntry(Entry * entry) {
	for (int i = 0; i < 8; i++)
		if (entry->name[i] != 0) return false;
	return true;
}

bool Root::fileNameCompare(char *fname, Entry *entry) {
	int k = 0;
	bool help = false;

	for (int i = 0; i < FNAMELEN; i++) {
		if (i < strlen(fname) && fname[i] == '.') {
			k = i + 1;
			help = true;
			if (entry->name[i] != ' ') return false;
		}
		if (!help && entry->name[i] != fname[i])
			return false;
	}

	for (int i = 0; i < FEXTLEN; i++)
		if (k >= strlen(fname) || entry->ext[i] != fname[k++])
			return false;
	
	return true;
}

FileEntry * Root::newFile(char * fname, char mode) {
	char lvl1Buffer[ClusterSize];
	char lvl2Buffer[ClusterSize];

	readCluster(1, root);
	ClusterNo *rootCache = (ClusterNo*)root;

	ClusterNo *clusterArray = (ClusterNo*)lvl1Buffer;
	Entry *entryArray = (Entry*)lvl2Buffer;

	for (int i = 0; i < IndexEntriesNo; i++) {
		if (rootCache[i] == 0) { 
			ClusterNo lvl2Entry = allocCluster(rootCache, lvl1Buffer, 1, i); //broj klastera gde je lvl2
			ClusterNo entryCluster = allocCluster(clusterArray, lvl2Buffer, lvl2Entry, 0); //broj klastera u kom su ulazi
			entryArray = (Entry*)lvl2Buffer;
			addEntry(fname, entryArray);
			writeCluster(entryCluster, (const char*)entryArray);
			return new FileEntry(fname, mode, entryArray, entryCluster, 0);
		}
		else {
			readCluster(rootCache[i], lvl1Buffer);
			clusterArray = (ClusterNo*)lvl1Buffer;
			for (ClusterNo j = 0; j < IndexEntriesNo; j++) {
				if (clusterArray[j] == 0) {
					ClusterNo entryCluster = allocCluster(clusterArray, lvl2Buffer, rootCache[i], j);
					entryArray = (Entry*)lvl2Buffer;
					addEntry(fname, entryArray);
					writeCluster(entryCluster, (const char*)entryArray);
					return new FileEntry(fname, mode, entryArray, entryCluster, j);
				}
				else {
					readCluster(clusterArray[j], lvl2Buffer); //ili [i]
					entryArray = (Entry*)lvl2Buffer;
					for (ClusterNo k = 0; k < DirectoryEntriesNo; k++) {
						if (isFreeEntry(entryArray + k)) {
							addEntry(fname, entryArray + k);
							writeCluster(clusterArray[j], (const char*)entryArray);
							return new FileEntry(fname, mode, entryArray + k, clusterArray[j], k);
						}
					}
				}
			}
		}
	}
	return nullptr;
}

ClusterNo Root::allocCluster(ClusterNo * rootCache, char * buffer, ClusterNo pos, ClusterNo off) {
	ClusterNo k = myBitVector->getEmpty();
	((ClusterNo*)rootCache)[off] = k;
	writeCluster(pos, (const char*)rootCache);
	for (ClusterNo i = 0; i < ClusterSize; i++)
		buffer[i] = 0;
	return k;
}

void Root::addEntry(char * fname, Entry * entry) {
	int k = 0;
	bool flag = false;

	for (int i = 0; i < 8; i++) {
		if (fname[i] == '.' && i < strlen(fname)) {
			k = i + 1;
			flag = true;
		}
		if (flag) entry->name[i] = ' ';
		else entry->name[i] = fname[i];
	}

	for (int i = 0; i < FEXTLEN; i++) {
		if (k < strlen(fname)) entry->ext[i] = fname[k++];
		else entry->ext[i] = ' ';
	}

	entry->fileSize = 0;
	entry->firstIndexCluster = myBitVector->getEmpty();

}



Root::Root(Partition *myPartition_, BitVector *myBitVector_) {
	myPartition = myPartition_;
	readCluster(1, root);
	myBitVector = myBitVector_;
	filesClosed = CreateSemaphore(0, 1, 32, 0);
	mutex = CreateSemaphore(0, 1, 1, 0);
}



Root::~Root() {
	if (dirtyCache) writeCluster(1, root);
}

void Root::readCluster(ClusterNo c, char * buffer) {
	myPartition->readCluster(c, buffer);
}

void Root::writeCluster(ClusterNo c, const char * buffer) {
	myPartition->writeCluster(c, buffer);
}

void Root::signalUnmount() {
	wantToUnmount = true;
}

void Root::format() {
	for (int i = 0; i < ClusterSize; i++) root[i] = 0;
	writeCluster(1, root);
}

FileCnt Root::fileCount() {
	FileCnt count = 0; 
	ClusterNo *rootEntry = (ClusterNo*)root;

	for (ClusterNo i = 0; i < IndexEntriesNo; i++)

		if (rootEntry[i] != 0) {

			char firstLevel[ClusterSize];
			readCluster(root[i], firstLevel);
			ClusterNo *firstLevelEntry = (ClusterNo*)firstLevel;

			for (ClusterNo j = 0; j < IndexEntriesNo; j++)

				if (firstLevelEntry[j] != 0) {

					char secondLevel[ClusterSize];
					readCluster(firstLevelEntry[j], secondLevel);
					Entry *entries = (Entry*)secondLevel;

					for (ClusterNo k = 0; k < DirectoryEntriesNo; k++)
						if (!isFreeEntry(entries + k)) count++;
				}
		}
	return count;
}


FileEntry* Root::doesExistFE(char * fname, char mode) { 
	readCluster(1, root);
	ClusterNo *rootEntry = (ClusterNo*)root;

	for (ClusterNo i = 0; i < IndexEntriesNo; i++)

		if (rootEntry[i] != 0) {

			char secondLevel[ClusterSize];
			readCluster(root[i], secondLevel);
			ClusterNo *secondLevelEntry = (ClusterNo*)secondLevel;

			for (ClusterNo j = 0; j < IndexEntriesNo; j++)

				if (secondLevelEntry[j] != 0) {

					char entryBuffer[ClusterSize];
					readCluster(secondLevelEntry[j], entryBuffer);
					Entry *entries = (Entry*)entryBuffer;

					for (ClusterNo k = 0; k < DirectoryEntriesNo; k++)
						if (fileNameCompare(fname, entries + k)) {
							return new FileEntry(fname, mode, entries + k, secondLevelEntry[j], k);
						}

				}
		}

	return nullptr;
}

char Root::doesExist(char* fname) {
	FileEntry *fileEntry = doesExistFE(fname, 0);
	char ret;
	if (!fileEntry) ret = 0;
	else ret = 1;
	delete fileEntry;
	return ret;
}



File * Root::open(char * fname, char mode) {

	wait(mutex);
	std::map<std::string, FileEntry*>::iterator it = openFiles.find(fname);

	if (it != openFiles.end() && it->second) { //ako je fajl otvoren

		FileEntry *myFile = it->second;

		if (mode == 'r') {
			if (myFile->writers == 1) {
				myFile->readersWaiting++;
				signalAndWait(mutex, myFile->readSem);
				wait(mutex);
			}
			myFile->readers++;
			signal(mutex);
			return new File(myFile, this, mode);
		}

		if (mode == 'w' || mode == 'a') {
			if (myFile->writers == 1 || myFile->readers > 0) {
				myFile->writersWaiting++;
				signalAndWait(mutex, myFile->writeSem);
				wait(mutex);
			}
			myFile->writers++;
			signal(mutex);
			return new File(myFile, this, mode);
		}
	}
	else { //ako fajl nije otvoren
		FileEntry* myFile = doesExistFE(fname, mode);
		if (myFile) { //neotvoreni fajl postoji
			openFiles.insert(std::pair<char*, FileEntry*>(fname, myFile));
			signal(mutex);
			return new File(myFile, this, mode);
		}

		else { //neotvoreni fajl ne postoji

			if (mode == 'w') {
				FileEntry* myFile = newFile(fname, mode); 
				openFiles.insert(std::pair<char*, FileEntry*>(fname, myFile));
				signal(mutex);
				return new File(myFile, this, mode);
			}

			if (mode == 'r' || mode == 'a') {
				signal(mutex);
				return nullptr;
			}
		}
	}
	signal(mutex);
	return nullptr;
}


void Root::close(KernelFile * file) {

	wait(mutex);
	char buffer[ClusterSize];

	unsigned long size = file->getFileSize();
	if (size != file->fileEntry->getEntry()->fileSize) { 
		file->fileEntry->getEntry()->fileSize = size;
		readCluster(file->fileEntry->lvl2Cluster, buffer);
		((Entry*)(buffer))[file->fileEntry->entryIndex].fileSize = size;
		writeCluster(file->fileEntry->lvl2Cluster, buffer);
	}

	std::string fname = file->fileEntry->fname;
	std::map<std::string, FileEntry*>::iterator it = openFiles.find(fname);

	if (it != openFiles.end() && it->second) { //ako je fajl otvoren
		FileEntry *myFile = it->second;
		if (myFile->writers == 0 && myFile->readers == 1) {
			myFile->readers--;
			if (myFile->writersWaiting > 0) {
				myFile->writersWaiting--;
				signal(myFile->writeSem);
			}
			else if (myFile->readersWaiting > 0) {
				while (myFile->readersWaiting) {
					myFile->readersWaiting--;
					signal(myFile->readSem);
				}
			}
			else {
				delete it->second;
				openFiles.erase(it);
				if (openFiles.empty() && wantToUnmount) {
					wantToUnmount = false;
					signal(filesClosed);
				}
			}
		}
		else if (myFile->writers == 1) {
			myFile->writers--;
			if (myFile->writersWaiting > 0) {
				myFile->writersWaiting--;
				signal(myFile->writeSem);
			}
			else if (myFile->readersWaiting > 0) {
				while (myFile->readersWaiting) {
					myFile->readersWaiting--;
					signal(myFile->readSem);
				}
			}
			else {
				delete it->second;
				openFiles.erase(it);
				if (openFiles.empty() && wantToUnmount) {
					wantToUnmount = false;
					signal(filesClosed);
				}
			}
		}
		else if (myFile->readers > 1) myFile->readers--;
	}

	if (openFiles.empty() && wantToUnmount) {
		wantToUnmount = false;
		signal(filesClosed);
	}

	signal(mutex);
}



char Root::deleteFile(char * fname) { 
	//ako je fajl otvoren, ne može da se obriše:
	std::map<std::string, FileEntry*>::iterator it = openFiles.find(fname);
	if (it != openFiles.end() && it->second != nullptr) return 0;

	if (doesExist(fname) == 0) return 0; // ako ne postoji na disku

	FileEntry *fileEntry = it->second;

	//brisanje fajla ako postoji:
	KernelFile *myFile = new KernelFile(fileEntry, this, 'r');
	delete myFile;

	myBitVector->freeCluster(fileEntry->entry->firstIndexCluster);
	char buffer[ClusterSize];
	readCluster(fileEntry->lvl2Cluster, buffer);
	for (ClusterNo i = 0; i < 8; i++)
		buffer[fileEntry->entryIndex * 32 + i] = 0;
	writeCluster(fileEntry->lvl2Cluster, buffer);
	delete fileEntry;

	return 1;
}

void Root::free(ClusterNo clNo) {
	myBitVector->freeCluster(clNo);
}

FileEntry::FileEntry(char * fname_, char mode_, Entry * entry_, ClusterNo position, BytesCnt offset) {
	fname = new char[strlen(fname_) + 1];
	strncpy(fname, fname_, strlen(fname_) + 1);

	mode = mode_;

	entry = new Entry();
	strncpy(entry->name, entry_->name, FNAMELEN);
	strncpy(entry->ext, entry_->ext, FEXTLEN);
	entry->reserved = entry_->reserved;
	entry->firstIndexCluster = entry_->firstIndexCluster;
	entry->fileSize = entry_->fileSize;

	entryIndex = offset;
	lvl2Cluster = position;

	readers = 0;
	writers = 0;
	readersWaiting = 0;
	writersWaiting = 0;

	mutex = CreateSemaphore(0, 1, 1, 0);
	writeSem = CreateSemaphore(0, 0, 32, 0);
	readSem = CreateSemaphore(0, 0, 32, 0);

	if (mode == 'w' || mode == 'a') writers++;
	if (mode == 'r') readers++;
}
