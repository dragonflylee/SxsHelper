#pragma once

class CFindDlg : public CFindReplaceDialogImpl<CFindDlg>, public CMessageFilter
{
public:
    BOOL PreTranslateMessage(MSG* pMsg)
    {
        HWND hWndFocus = ::GetFocus();
        if ((m_hWnd == hWndFocus) || IsChild(hWndFocus))
            return IsDialogMessage(pMsg);
        return FALSE;
    }

    BEGIN_MSG_MAP(CFindDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        CMessageLoop* pLoop = _Module.GetMessageLoop();
        ATLASSERT(pLoop != NULL);
        bHandled = FALSE;
        return pLoop->AddMessageFilter(this);
    }

    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        CMessageLoop* pLoop = _Module.GetMessageLoop();
        ATLASSERT(pLoop != NULL);
        bHandled = FALSE;
        return pLoop->RemoveMessageFilter(this);
    }
};
