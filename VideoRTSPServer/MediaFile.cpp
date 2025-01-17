#include "MediaFile.h"

MediaFile::MediaFile() :
	m_pFile(NULL),
	m_type(FileType::UNKNOWN)
{

}

MediaFile::~MediaFile()
{
	Close();
}

int MediaFile::Open(const Buffer& path, FileType type)
{
	fopen_s(&m_pFile, path, "rb");
	if (!m_pFile)
	{
		return -1;
	}
	m_type = type;
	fseek(m_pFile, 0, SEEK_END);
	m_size = ftell(m_pFile);
	Reset();
	return 0;
}

Buffer MediaFile::ReadOneFrame()
{
	switch (m_type)
	{
		case FileType::H264: return ReadH264Frame(); break;
	}
	return Buffer();
}

void MediaFile::Close()
{
	m_type = FileType::UNKNOWN;
	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
}

void MediaFile::Reset()
{
	if (m_pFile)
	{
		fseek(m_pFile, 0, SEEK_SET);
	}
}

Buffer MediaFile::ReadH264Frame()
{
	if (m_pFile)
	{
		long off = FindH264Head();
		if (off == -1) return Buffer();
		fseek(m_pFile, off + 3, SEEK_SET);
		long tail = FindH264Head();
		if (tail == -1) tail = m_size;
		long size = tail - off;
		fseek(m_pFile, off, SEEK_SET);
		Buffer frame(size);
		fread((void*)frame, 1, size, m_pFile);
		return frame;
	}
	return Buffer();
}

// 0x 00 00 01 or 0x 00 00 00 01
long MediaFile::FindH264Head()
{
	if (m_pFile)
	{
		char c = 0x7F;
		while (!feof(m_pFile))
		{
			c = fgetc(m_pFile);
			if (c == 0) break;
		}
		if (!feof(m_pFile))
		{
			c = fgetc(m_pFile);
			if (c == 0)
			{
				c = fgetc(m_pFile);
				if (c == 1)
				{
					return ftell(m_pFile) - 3; // 0x000001
				}
				else if (c == 0)
				{
					c = fgetc(m_pFile);
					if (c == 1) return ftell(m_pFile) - 4; // 0x00000001
				}
			}
		}
	}
	return -1;
}

#if 0
Buffer MediaFile::ReadH264Frame()
{
    if (m_pFile)
    {
        long off = FindH264Head();
        if (off >= 0)
        {
            fseek(m_pFile, off, SEEK_SET);
            Buffer frame;
            char buf[1024];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), m_pFile)) > 0)
            {
                frame.append(buf, n);
                if (FindH264Head() >= 0)
                {
                    fseek(m_pFile, -n, SEEK_CUR);
                    break;
                }
            }
            return frame;
        }
    }
    return Buffer();
}

long MediaFile::FindH264Head()
{
    if (!m_pFile)
    {
        return -1;
    }

    long start_pos = ftell(m_pFile);
    char buf[4];
    while (fread(buf, 1, 4, m_pFile) == 4)
    {
        if ((buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x00 && buf[3] == 0x01) ||
            (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01))
        {
            fseek(m_pFile, -4, SEEK_CUR);
            return ftell(m_pFile);
        }
        fseek(m_pFile, -3, SEEK_CUR);
    }

    return -1;
}
#endif