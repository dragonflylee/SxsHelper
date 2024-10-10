#include "StdAfx.h"

int CALLBACK CheckPath(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        if (NULL != lpData)
        {
            ::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
        }
        break;
    case BFFM_SELCHANGED:
        LPITEMIDLIST pidlSelected = (LPITEMIDLIST)lParam;
        TCHAR szPath[MAX_PATH] = { 0 };
        if (::SHGetPathFromIDList(pidlSelected, szPath))
        {
            ::PathCombine(szPath, szPath, TEXT("Servicing\\Packages"));
            ::SendMessage(hWnd, BFFM_ENABLEOK, NULL, ::PathIsDirectory(szPath));
        }
        break;
    }
    return 0;
}

BOOL SelectFolder(LPTSTR szFolder)
{
    BOOL bRet = FALSE;
    CAtlString szTitle;
    szTitle.LoadString(IDS_SOURCE);

    BROWSEINFO bi = { 0 };
    bi.hwndOwner = HWND_DESKTOP;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN | BIF_NONEWFOLDERBUTTON;
    bi.lParam = (LPARAM)szFolder;
    bi.lpfn = CheckPath;
    bi.lpszTitle = szTitle;

    LPITEMIDLIST pidlSelected = ::SHBrowseForFolder(&bi);
    if (NULL != pidlSelected)
    {
        bRet = ::SHGetPathFromIDList(pidlSelected, szFolder);
        ::CoTaskMemFree(pidlSelected);
    }
    return bRet;
}

int Run(HINSTANCE hInst, int nCmdShow)
{
    CAtlString szText;
    ::GetWindowsDirectory(szText.GetBufferSetLength(MAX_PATH), MAX_PATH);
#ifndef _DEBUG
    if (!SelectFolder(szText.GetBuffer(MAX_PATH))) return FALSE;
#endif
    szText.ReleaseBuffer();

    CMainDlg wndMain(szText);
    RECT rcWnd = {0, 0, 600, 500};
    HWND hWnd = wndMain.Create(HWND_DESKTOP, rcWnd);
    if (NULL == hWnd)
    {
        szText.Format(IDS_ERROR, ::GetLastError());
        return wndMain.MessageBox(szText, CMainDlg::GetWndCaption(), MB_ICONERROR);
    }
    wndMain.ShowWindow(nCmdShow);

    HACCEL hAccMain = ::LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_MAIN));
    ATLASSERT(hAccMain);

    // ����Ϣѭ��:
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0))
    {
        if (!wndMain.FindMsg(&msg) && !::TranslateAccelerator(hWnd, hAccMain, &msg))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HANDLE hMutex = ::CreateMutex(NULL, TRUE, TEXT("CSample"));
    if (::GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CAtlString szText;
        szText.LoadString(IDS_RUNNING);
        return ::MessageBox(HWND_DESKTOP, szText, CMainDlg::GetWndCaption(), MB_ICONWARNING);
    }

    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    ATLASSERT(SUCCEEDED(hr));

    hr = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hr));

    int nRet = Run(hInstance, nCmdShow);

    _Module.Term();
    ::CoUninitialize();
    ::CloseHandle(hMutex);
    return nRet;
}