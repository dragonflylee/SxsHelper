#ifndef _LOG_FILE_H_
#define _LOG_FILE_H_

#ifndef LOG_DEFAULT
#ifdef _DEBUG
#define LOG_DEFAULT LL_DEBUG
#else 
#define LOG_DEFAULT LL_INFO
#endif
#endif

class CLog
{
public:
    enum LOG_LEVEL
    {
        LL_ERROR = 0,
        LL_WARNING,
        LL_INFO,
        LL_DEBUG,
        LL_ALL
    };
    static void Log(LOG_LEVEL level, LPCTSTR szFile, int nLine, LPCTSTR szFunc, LPCTSTR szFormat, ...)
    { 
        static CLog l;
        if (level > l.mLevel || !l.OpenFile()) return;

        static TCHAR *szLevel[] = {TEXT("error"), TEXT("warning"), TEXT("info"), TEXT("debug"), TEXT("none")};
        va_list args;
        va_start(args, szFormat);
        int nLen = _vsctprintf(szFormat, args);
        if (nLen > 0)
        {
            TCHAR szHead[MAX_PATH];
            SYSTEMTIME tm;
            ::GetLocalTime(&tm);
            int nHead = _stprintf_s(szHead, MAX_PATH, TEXT("%.2d:%.2d:%.2d:%.3d [%d] %s:%s:%d(%s) "), 
                tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds, ::GetCurrentThreadId(), szLevel[level], szFile, nLine, szFunc);
            size_t cbSize = (nLen + 1) * sizeof(TCHAR);

            LPTSTR pBuffer = (LPTSTR)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, cbSize);
            nLen = _vsntprintf_s(pBuffer, cbSize, nLen, szFormat, args);
            ::EnterCriticalSection(&l.mLock);
            l.Write(szHead, nHead * sizeof(TCHAR));
            l.Write(pBuffer, nLen * sizeof(TCHAR));
            l.Write(TEXT("\r\n"), 2 * sizeof(TCHAR));
            ::LeaveCriticalSection(&l.mLock);

            ::HeapFree(::GetProcessHeap(), 0, pBuffer);

        }
        va_end(args);
    }
protected:
    CLog(): mLevel(LOG_DEFAULT), mFile(INVALID_HANDLE_VALUE)
    {
        SYSTEMTIME tm;
        ::GetLocalTime(&tm);
        ::GetModuleFileName(NULL, mPath, MAX_PATH);
        ::PathRemoveExtension(mPath);
        int cbSize = _tcslen(mPath);
        _stprintf_s(mPath + cbSize, MAX_PATH - cbSize, TEXT("_%.2d%.2d%.2d[%d].log"), 
            tm.wYear, tm.wMonth, tm.wDay, ::GetCurrentProcessId());
        ::InitializeCriticalSection(&mLock);  
    }
    virtual ~CLog()
    {
        if (INVALID_HANDLE_VALUE != mFile) 
        {
            ::FlushFileBuffers(mFile);
            ::CloseHandle(mFile);
        }

        ::DeleteCriticalSection(&mLock);
    }
    bool OpenFile()//打开文件，指针到文件尾
    {
        if (INVALID_HANDLE_VALUE != mFile) return true; 

        mFile = ::CreateFile(mPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 

        if (INVALID_HANDLE_VALUE == mFile) return false;

        ::SetFilePointer(mFile, 0, NULL, FILE_END);

#ifdef _UNICODE
        Write(TEXT("\xFEFF"), 2);
#endif
        return true;
    }
    DWORD Write(LPCTSTR lpBuffer, DWORD cbSize)
    {
        ::WriteFile(mFile, lpBuffer, cbSize, &cbSize, NULL);
#ifdef _DEBUG
        ::OutputDebugString(lpBuffer);
#endif
        return cbSize;
    }
private:
    CLog(const CLog&);
    CLog&operator=(const CLog&);
protected:
    LOG_LEVEL mLevel;
    HANDLE mFile;
    TCHAR mPath[MAX_PATH];  
    CRITICAL_SECTION mLock;
};

#define __WIDEN(str)    L##str
#define _WIDEN(str)     __WIDEN(str)

#ifdef _UNICODE
#define __TFILE__ _WIDEN(__FILE__)
#define __TFUNCTION__ _WIDEN(__FUNCTION__)
#else
#define __TFILE__ __FILE__
#define __TFUNCTION__ __FUNCTION__
#endif

#define LOG_ERROR(format, ...) CLog::Log(CLog::LL_ERROR, __TFILE__, __LINE__, __TFUNCTION__, TEXT(format), __VA_ARGS__)
#define LOG_WARNING(format, ...) CLog::Log(CLog::LL_WARNING, __TFILE__, __LINE__, __TFUNCTION__, TEXT(format), __VA_ARGS__)
#define LOG_INFO(format, ...) CLog::Log(CLog::LL_INFO, __TFILE__, __LINE__, __TFUNCTION__, TEXT(format), __VA_ARGS__)
#define LOG_DEBUG(format, ...) CLog::Log(CLog::LL_DEBUG, __TFILE__, __LINE__, __TFUNCTION__, TEXT(format), __VA_ARGS__)


#define HR_CHECK(_hr_) hr = _hr_; if (FAILED(hr)) { LOG_ERROR("0x%.8x", hr); goto exit; }
#define BOOL_CHECK(_hr_) if (_hr_) { hr = HRESULT_FROM_WIN32(::GetLastError()); LOG_ERROR("0x%.8x", hr); goto exit; }
#define HANDLE_CHECK(_handle_) BOOL_CHECK(INVALID_HANDLE_VALUE == (_handle_))

#endif // _LOG_FILE_H_
