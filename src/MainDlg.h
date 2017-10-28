#ifndef _MAIN_DLG_H_
#define _MAIN_DLG_H_

class CAssemblyNode;

class CMainDlg : public CWindowImpl<CMainDlg, CWindow, CFrameWinTraits>
{
public:
    DECLARE_WND_CLASS(TEXT("CSxsWnd"))

    BEGIN_MSG_MAP(CMainDlg)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(CFindDlg::WM_FINDMESSAGE, OnFind)
        COMMAND_ID_HANDLER(IDM_EXPORT, OnExport)
        COMMAND_ID_HANDLER(IDM_SEARCH, OnSearch)
        COMMAND_ID_HANDLER(IDM_FRESH, OnFresh)
        COMMAND_ID_HANDLER(IDM_FINDNEXT, OnFindNext)
        COMMAND_ID_HANDLER(IDM_FINDPREV, OnFindPrev)
        NOTIFY_CODE_HANDLER(TVN_ITEMCHANGED, OnItemChange)
        NOTIFY_CODE_HANDLER(NM_RCLICK, OnContext)
    END_MSG_MAP()

public:
    /**
    * 对话框相关
    */
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnSysCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnFind(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    /**
    * TreeView
    */
    LRESULT OnItemChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
    LRESULT OnContext(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
    /**
    * 菜单事件
    */
    LRESULT OnSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFindNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFindPrev(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    // 查找对话框处理
    inline BOOL FindMsg(LPMSG lpMsg) { return mFind.IsWindow() && mFind.IsDialogMessage(lpMsg); }

protected:
    /**
    * 扫描封包线程
    */
    static DWORD CALLBACK ThreadScan(LPVOID lpParam);

    /**
    * 递归插入节点
    */
    void RecurveInsert(HTREEITEM hParent, CAssemblyNode *pParent);

    /**
    * 检查工作线程
    */
    BOOL IsWorking();

private:
    HANDLE m_hThread;
    HMENU m_hPopup;
    TCHAR m_szExport[MAX_PATH];
  
    CWindow mTree;
    CFindDlg mFind;

    CAssemblyMap mMap;
    CComPtr<CAssemblyNode> mRoot;

public:
    CMainDlg(LPTSTR szPath);
    virtual ~CMainDlg();
};

#endif // _MAIN_DLG_H_