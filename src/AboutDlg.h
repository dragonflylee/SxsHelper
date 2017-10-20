#ifndef _ABOUT_DLG_H_
#define _ABOUT_DLG_H_

class CAboutDlg : public CDialogImpl<CAboutDlg>,
    public CComObjectRootEx<CComSingleThreadModel>
{
public:
    enum { IDD = IDD_ABOUTBOX };

    BEGIN_MSG_MAP(CAboutDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    END_MSG_MAP()

    BEGIN_COM_MAP(CAboutDlg)
    END_COM_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        return CenterWindow(GetParent());
    }
    LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        return EndDialog(HIWORD(wParam));
    }
};

#endif // _ABOUT_DLG_H_