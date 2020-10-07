#include "file.h"
#include "KernelFile.h"

File::File(FileEntry* f, Root* r, char m) {
	myImpl = new KernelFile(f, r, m);
}

File::~File() { delete File::myImpl; }

char File::write(BytesCnt size, char * buffer) { return File::myImpl->write(size, buffer); }

BytesCnt File::read(BytesCnt size, char * buffer) { return File::myImpl->read(size, buffer); }

char File::seek(BytesCnt size) { return File::myImpl->seek(size); }

BytesCnt File::filePos() { return File::myImpl->filePos(); }

char File::eof() { return File::myImpl->eof(); }

BytesCnt File::getFileSize() { return File::myImpl->getFileSize(); }

char File::truncate() { return File::myImpl->truncate(); }
