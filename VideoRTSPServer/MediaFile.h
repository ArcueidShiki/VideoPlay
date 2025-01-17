#pragma once
#include "Utils.h"

enum class FileType
{
	UNKNOWN = -1,
	VIDEO,
	AUDIO,
	IMAGE,
	H264 = 96,
};

class MediaFile
{
public:
	MediaFile();
	~MediaFile();
	int Open(const Buffer& path, FileType type = FileType::H264);
	Buffer ReadOneFrame();	// buffer size == 0, no frames
	void Close();
	void Reset();	// rewind to the beginning
private:
	Buffer ReadH264Frame();
	long FindH264Head(int &headsz);
private:
	FILE* m_pFile;
	Buffer m_filepath;
	FileType m_type;
	long m_size;
};

