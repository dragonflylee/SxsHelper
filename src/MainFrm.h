#pragma once

struct CAssemblyNode;
typedef ATL::CSimpleMap<CAtlString, CAssemblyNode *> CAssemblyMap;
typedef ATL::CSimpleMap<CAssemblyNode *, HTREEITEM> CAssemblySet;

struct CAssemblyNode
{
    CAssemblySet Parent;
    CAssemblyMap Package;
};

class CFolderDlg : public CFolderDialogImpl<CFolderDlg>
{
public:
    CFolderDlg()
    {
        szTitle.LoadString(IDS_SOURCE);
        m_bi.lpszTitle = szTitle;
        m_bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN | BIF_NONEWFOLDERBUTTON;
    }

    void OnSelChanged(LPITEMIDLIST pidlSelected)
    {
        if (::SHGetPathFromIDList(pidlSelected, m_szFolderPath))
        {
            ::PathCombine(m_szFolderPath, m_szFolderPath, TEXT("Servicing\\Packages"));
            EnableOK(::PathIsDirectory(m_szFolderPath));
        }
    }

private:
    CAtlString szTitle;
};

#include "FindDlg.h"

class CMainFrm : public CFrameWindowImpl<CMainFrm>, public CMessageFilter
{
public:
    DECLARE_FRAME_WND_CLASS(TEXT("CSxsWnd"), IDR_MAIN)

    BEGIN_MSG_MAP(CMainFrm)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_SYSCOMMAND(OnSysCommand)
        MESSAGE_HANDLER_EX(CFindReplaceDialog::GetFindReplaceMsg(), OnFindCmd)
        COMMAND_ID_HANDLER_EX(IDM_OPEN, OnOpen)
        COMMAND_ID_HANDLER_EX(IDM_ABOUT, OnAbout)
        COMMAND_ID_HANDLER_EX(IDM_FIND, OnFind)
        NOTIFY_CODE_HANDLER_EX(TVN_ITEMCHANGING, OnChanging)
        COMMAND_RANGE_HANDLER_EX(ID_FILE_MRU_FIRST, ID_FILE_MRU_LAST, OnRecent)
        CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrm>)
        ALT_MSG_MAP(1)
        ALT_MSG_MAP(2)
    END_MSG_MAP()

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return CFrameWindowImpl<CMainFrm>::PreTranslateMessage(pMsg);
    }

public:
    /**
    * 窗口事件
    */
    int OnCreate(LPCREATESTRUCT /*lpCreate*/);
    void OnSysCommand(UINT /*nID*/, CPoint /*pt*/);
    void OnDestroy();
    LRESULT OnFindCmd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/);
    /**
    * 菜单命令
    */
    void OnOpen(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/);
    void OnAbout(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/);
    void OnFind(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/);
    void OnRecent(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/);
    /**
    * 列表通知
    */
    LRESULT OnChanging(LPNMHDR /*pnmh*/);

protected:
    /**
    * 扫描封包相关
    */
    static DWORD CALLBACK ThreadScan(LPVOID lpParam);

private:
    CFont m_hFont;
    HANDLE m_hThread;
    CFindDlg *m_pFind;
    CFolderDlg m_dlgFolder;
    CRecentDocumentList m_mru;
    CContainedWindowT<CTreeViewCtrl> wndTree;
    CContainedWindowT<CComboBox> wndFilter;
    // 根节点
    CAssemblyMap mapPackage;

public:
    CMainFrm();
};
