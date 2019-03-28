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
        COMMAND_ID_HANDLER(IDM_DUMP, OnExport)
        COMMAND_ID_HANDLER(IDM_SEARCH, OnSearch)
        COMMAND_ID_HANDLER(IDM_FRESH, OnFresh)
        COMMAND_ID_HANDLER(IDM_FINDNEXT, OnFindNext)
        COMMAND_ID_HANDLER(IDM_FINDPREV, OnFindPrev)
        NOTIFY_CODE_HANDLER(TVN_ITEMCHANGED, OnChanged)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnClick)
        NOTIFY_CODE_HANDLER(NM_RCLICK, OnContext)
    ALT_MSG_MAP(1)
    ALT_MSG_MAP(2)
        MESSAGE_HANDLER(WM_CHAR, OnFilterChar)
    END_MSG_MAP()

public:
    static LPCTSTR GetWndCaption();
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
    LRESULT OnChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
    LRESULT OnClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
    LRESULT OnContext(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
    /**
    * 菜单事件
    */
    LRESULT OnSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnDump(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFindNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFindPrev(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    /**
    * 筛选器事件
    */
    LRESULT OnFilterChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

    // 查找对话框处理
    inline BOOL FindMsg(LPMSG lpMsg) { return m_find.IsWindow() && m_find.IsDialogMessage(lpMsg); }

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
    HMENU m_hMenu;
    HFONT m_hFont;
    TCHAR m_szExport[MAX_PATH];

    CFindDlg m_find;
    CContainedWindowT<CWindow, CWinTraitsOR<TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_CHECKBOXES> > m_tree;
    CContainedWindowT<CWindow, CWinTraitsOR<ES_AUTOHSCROLL, WS_EX_CLIENTEDGE> > m_filter;
    // 根节点
    CAssemblyNode nodeRoot;
    CAssemblyMap mapPackage;

public:
    CMainDlg(LPCTSTR szPath);
};

#endif // _MAIN_DLG_H_