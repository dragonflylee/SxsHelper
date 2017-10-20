#ifndef _STRING_HELP_H_
#define _STRING_HELP_H_

class CStr
{
protected:
    class CStrData
    {
    public:
        CStrData(LPCTSTR pBuffer, int cbSize) :m_lRef(1), m_pBuffer(NULL), m_cbLength(cbSize)
        {
            m_pBuffer = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (cbSize + 1) * sizeof(TCHAR));
            if (NULL != pBuffer) _tcsncpy(m_pBuffer, pBuffer, cbSize);
        }
        ~CStrData()
        {
            if (NULL != m_pBuffer) HeapFree(GetProcessHeap(), 0, m_pBuffer);
        }
        LONG AddRef()
        {
            return InterlockedIncrement(&m_lRef);
        }
        LONG Release()
        {
            LONG lRes = InterlockedDecrement(&m_lRef);
            if (lRes <= 0) delete this;
            return lRes;
        }
    public:
        LPTSTR m_pBuffer;
        int m_cbLength;
        LONG m_lRef;
        CStrData(const CStrData&);
    };

public:
    CStr(LPCTSTR pBuffer, int cbSize = -1)
    {
        if (cbSize > 0)
            m_pData = new CStrData(pBuffer, cbSize);
        else if (NULL != pBuffer)
            m_pData = new CStrData(pBuffer, (int)_tcslen(pBuffer));
        else
            m_pData = NULL;
    }
#ifndef _UNICODE  
    CStr(LPCWSTR pBuffer, int cbSize = -1)
    {
        int nLength = WideCharToMultiByte(CP_ACP, 0, pBuffer, cbSize, NULL, 0, NULL, NULL);
        m_pData = new CStrData(NULL, nLength);
        WideCharToMultiByte(CP_ACP, 0, pBuffer, cbSize, m_pData->m_pBuffer, nLength, NULL, NULL);
    }
#else
    CStr(LPCSTR pBuffer, int cbSize = -1)
    {
        int nLength = MultiByteToWideChar(CP_ACP, 0, pBuffer, cbSize, NULL, 0);
        m_pData = new CStrData(NULL, nLength);
        MultiByteToWideChar(CP_ACP, 0, pBuffer, cbSize, m_pData->m_pBuffer, nLength);
    }
#endif
    CStr(int cbSize = 0) :m_pData(NULL)
    {
        if (cbSize > 0) m_pData = new CStrData(NULL, cbSize);
    }
    CStr(const CStr& cString) :m_pData(cString.m_pData)
    {
        if (NULL != m_pData) m_pData->AddRef();
    }
    ~CStr()
    {
        if (NULL != m_pData) m_pData->Release();
    }
    int Length() const
    {
        if (NULL != m_pData) return m_pData->m_cbLength; else return 0;
    }
    LPTSTR Buffer() const
    {
        if (NULL != m_pData) return m_pData->m_pBuffer; else return NULL;
    }
    static CStr Format(LPCTSTR szFormat, ...)
    {
        va_list args;
        va_start(args, szFormat);
        int nLen = _vsntprintf(NULL, 0, szFormat, args);
        CStr cStrings(nLen);
        if (nLen > 0) _vsntprintf(cStrings.Buffer(), cStrings.Length(), szFormat, args);
        va_end(args);
        return cStrings;
    }
    static CStr Load(LONG lResId, HINSTANCE hInst = NULL)
    {
        HRSRC hResInfo = FindResource(hInst, MAKEINTRESOURCE(((lResId >> 4) + 1)), RT_STRING);
        if (NULL != hResInfo)
        {
            HGLOBAL hResData = LoadResource(hInst, hResInfo);
            if (NULL != hResData)
            {
                ULONG uResSize = SizeofResource(hInst, hResInfo);
                LPBYTE pImage = (LPBYTE)LockResource(hResData);
                if (pImage != NULL && uResSize > 0)
                {
                    LPBYTE pImageEnd = pImage + uResSize;
                    UINT uIndex = lResId & 0x000F;
                    WORD wLength = *((PWORD)(pImage));
                    while (pImage < pImageEnd && uIndex > 0)
                    {
                        pImage += sizeof(WORD) + wLength * sizeof(WCHAR);
                        wLength = *((PWORD)(pImage));
                        uIndex--;
                    }
                    if (uIndex == 0)
                    {
                        CStr str((LPWSTR)(pImage + sizeof(WORD)), wLength);
                        FreeResource(hResData);
                        return str;
                    }
                }
                FreeResource(hResData);
            }
        }
        return CStr();
    }
    operator LPTSTR() const
    {
        return Buffer();
    }
private:
    CStrData* m_pData;
};

#endif // _STRING_HELP_H_
