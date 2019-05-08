#ifndef _FIND_DLG_H_
#define _FIND_DLG_H_

class CFindDlg : public CWindow
{
public:
    DWORD Create(DWORD dwFlags = FR_DOWN, HWND hParent = NULL)
    {
        fr.Flags |= dwFlags;
        fr.hwndOwner = (hParent == NULL) ? ::GetActiveWindow() : hParent;

        m_hWnd = ::FindText(&fr);
        return (NULL == m_hWnd) ? ::CommDlgExtendedError() : 0;
    }

    LPFINDREPLACE GetNotifier() { return &fr; }

private:
    FINDREPLACE fr;
    TCHAR szFind[MAX_PATH];

public:
    CFindDlg()
    {
        memset(&szFind, 0, sizeof(szFind));
        memset(&fr, 0, sizeof(fr));
        fr.lStructSize = sizeof(FINDREPLACE);
        fr.Flags = FR_NOWHOLEWORD | FR_NOUPDOWN;
        fr.lpstrFindWhat = szFind;
        fr.wFindWhatLen = _countof(szFind);
    }
    virtual ~CFindDlg() {}

    static UINT WM_FIND;
};

#endif // _FIND_DLG_H_