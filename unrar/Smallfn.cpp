
int CRarArchive::twrite(LPVOID buf, DWORD len)
//--------------------------------------------
{
	LONG l;
	if ((!m_dwOutputLen) || (!m_lpOutputFile))
	{
		m_dwOutputLen = m_dwOutputPos + len + 0x10000 + (m_dwStreamLen * 2);
		m_lpOutputFile = (LPBYTE)GlobalAllocPtr(GHND, m_dwOutputLen);
	}
	if ((!m_lpOutputFile) || (len <= 0) || (!buf)) return 0;
	l = m_dwOutputPos + len;
	if (l > (LONG)m_dwOutputLen)
	{
		l += 0x10000;
		LPBYTE p = (LPBYTE)GlobalAllocPtr(GHND, l);
		if (!p) return 0;
		memcpy(p, m_lpOutputFile, m_dwOutputLen);
		GlobalFreePtr(m_lpOutputFile);
		m_dwOutputLen = l;
		m_lpOutputFile = p;
	}
	memcpy(m_lpOutputFile + m_dwOutputPos, buf, len);
	m_dwOutputPos += len;
	return len;
}


int CRarArchive::tread(LPVOID p, DWORD len)
//-----------------------------------------
{
	LONG bytesleft = m_dwStreamLen - m_dwStreamPos;
	if (bytesleft <= 0) return 0;
	if (len > (DWORD)bytesleft) len = bytesleft;
	memcpy(p, m_lpStream + m_dwStreamPos, len);
	m_dwStreamPos += len;
	return len;
}


int CRarArchive::tseek(LONG offset, int fromwhere)
//------------------------------------------------
{
	LONG newofs = offset;
	switch(fromwhere)
	{
	case SEEK_END:
		newofs = m_dwStreamLen - offset;
		break;
	case SEEK_CUR:
		newofs = m_dwStreamPos + offset;
		break;
	}
	if (newofs < 0) newofs = 0;
	if (newofs > (LONG)m_dwStreamLen) newofs = m_dwStreamLen;
	m_dwStreamPos = newofs;
	return 0;
}


