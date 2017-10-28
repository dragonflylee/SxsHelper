#ifndef _FIND_DLG_H_
#define _FIND_DLG_H_

class CFindDlg : public CWindowImplBase
{
public:
    DECLARE_EMPTY_MSG_MAP();

    DWORD Create(DWORD dwFlags = FR_DOWN, HWND hParent = NULL)
    {
        m_fr.Flags |= dwFlags;
        m_fr.hwndOwner = (hParent == NULL) ? ::GetActiveWindow() : hParent;

        _Module.AddCreateWndData(&m_thunk.cd, this);

        m_hWnd = ::FindText(&m_fr);
        return (NULL == m_hWnd) ? ::CommDlgExtendedError() : 0;
    }

    LPFINDREPLACE GetNotifier() { return &m_fr; }
private:
    FINDREPLACE m_fr;
    TCHAR m_szFind[MAX_PATH];

    static UINT_PTR CALLBACK FindProc(HWND hWnd, UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/)
    {
        if (WM_INITDIALOG != uMsg) return 0;
        CFindDlg* pT = (CFindDlg*)_Module.ExtractCreateWndData();
        ATLASSERT(pT != NULL);
        ATLASSERT(pT->m_hWnd == NULL);
        ATLASSERT(::IsWindow(hWnd));
        return pT->SubclassWindow(hWnd);
    }

public:
    CFindDlg()
    {
        memset(&m_szFind, 0, sizeof(m_szFind));
        memset(&m_fr, 0, sizeof(m_fr));
        m_fr.lStructSize = sizeof(FINDREPLACE);
        m_fr.Flags = FR_ENABLEHOOK | FR_NOWHOLEWORD;
        m_fr.lpstrFindWhat = m_szFind;
        m_fr.wFindWhatLen = _countof(m_szFind);
        m_fr.lpfnHook = CFindDlg::FindProc;
    }
    virtual ~CFindDlg() {}

    static UINT WM_FINDMESSAGE;
};

#endif // _FIND_DLG_H_