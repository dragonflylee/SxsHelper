#ifndef _MAIN_DLG_H_
#define _MAIN_DLG_H_

class CMainDlg : public CDialogImpl<CMainDlg>,
    public CPkgTree<CMainDlg>,
    public CComObjectRootEx<CComSingleThreadModel>
{
public:
    enum { IDD = IDD_MAINDLG };

    BEGIN_MSG_MAP(CMainDlg)
        CHAIN_MSG_MAP(CPkgTree<CMainDlg>)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()

    BEGIN_COM_MAP(CMainDlg)
    END_COM_MAP()

public:
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnSysCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

protected:
    void CloseDialog(int nVal);
    /**
    * É¨Ãè·â°üÏß³Ì
    */
    static DWORD WINAPI ThreadScan(LPVOID lpParam);

private:
    HANDLE m_hThread;
    LPTSTR m_szPath;
    HTREEITEM m_hRoot;

public:
    CMainDlg();
    virtual ~CMainDlg();
};

#endif // _MAIN_DLG_H_