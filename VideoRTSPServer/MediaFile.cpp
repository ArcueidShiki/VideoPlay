#include "MediaFile.h"

MediaFile::MediaFile() :
	m_pFile(NULL),
	m_type(FileType::H264)
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
		int headsz = 0;
		long off = FindH264Head(headsz);
		if (off == -1) return Buffer();
		fseek(m_pFile, off + headsz, SEEK_SET);
		long tail = FindH264Head(headsz);
		if (tail == -1) tail = m_size;
		long size = tail - off;
		fseek(m_pFile, off, SEEK_SET);
		Buffer frame(size);
		fread((void*)frame, 1, size, m_pFile);
		// include head
		return frame;
	}
	return Buffer();
}

#if 1
long MediaFile::FindH264Head(int& headsz)
{
	if (!m_pFile)
	{
		return -1;
	}

	long start_pos = ftell(m_pFile);
	char buf[4];
	while (fread(buf, 1, 4, m_pFile) == 4)
	{
		if ((buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x00 && buf[3] == 0x01))
		{
			headsz = 4;
			fseek(m_pFile, -headsz, SEEK_CUR);
			return ftell(m_pFile);
		}
		else if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01)
		{
			headsz = 3;
			fseek(m_pFile, -headsz, SEEK_CUR);
			return ftell(m_pFile);
		}
		fseek(m_pFile, -3, SEEK_CUR);
	}

	return -1;
}
#endif

#if 0
// 0x 00 00 01 or 0x 00 00 00 01
long MediaFile::FindH264Head(int& headsz)
{
	if (m_pFile)
	{
		char c = 0x7F;
		while (!feof(m_pFile))
		{
			c = fgetc(m_pFile);
			if (c == 0) break;
		}
		while (!feof(m_pFile))
		{
			// There have severe issues!! fataals
			c = fgetc(m_pFile);
			if (c == 0)
			{
				c = fgetc(m_pFile);
				if (c == 1)
				{
					headsz = 3;
					return ftell(m_pFile) - headsz; // 0x000001
				}
				else if (c == 0)
				{
					c = fgetc(m_pFile);
					if (c == 1)
					{
						headsz = 4;
						return ftell(m_pFile) - headsz; // 0x00000001
					}
				}
			}
		}
	}
	return -1;
}
#endif

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