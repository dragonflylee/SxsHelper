#ifndef _FIND_DLG_H_
#define _FIND_DLG_H_

class CFindDlg : public CWindow
{
public:
    DWORD Create(DWORD dwFlags = FR_DOWN, HWND hParent = NULL)
    {
        m_fr.Flags |= dwFlags;
        m_fr.hwndOwner = (hParent == NULL) ? ::GetActiveWindow() : hParent;

        m_hWnd = ::FindText(&m_fr);
        return (NULL == m_hWnd) ? ::CommDlgExtendedError() : 0;
    }

    LPFINDREPLACE GetNotifier() { return &m_fr; }

private:
    FINDREPLACE m_fr;
    TCHAR m_szFind[MAX_PATH];

public:
    CFindDlg()
    {
        memset(&m_szFind, 0, sizeof(m_szFind));
        memset(&m_fr, 0, sizeof(m_fr));
        m_fr.lStructSize = sizeof(FINDREPLACE);
        m_fr.Flags = FR_NOWHOLEWORD | FR_NOUPDOWN;
        m_fr.lpstrFindWhat = m_szFind;
        m_fr.wFindWhatLen = _countof(m_szFind);
    }
    virtual ~CFindDlg() {}

    static UINT WM_FINDMESSAGE;
};

#endif // _FIND_DLG_H_