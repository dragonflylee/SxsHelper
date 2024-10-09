#include "SxsHelper.h"

/**
* 获取元素名称
*/
HRESULT GetNodeName(IXMLDOMNode *pNode, CAtlString& szName)
{
    CComPtr<IXMLDOMNamedNodeMap> pMap;
    HRESULT hr = pNode->get_attributes(&pMap);
    if (FAILED(hr) || NULL == pMap) return hr;

    CComPtr<IXMLDOMNode> pItem;
    hr = pMap->getNamedItem(L"name", &pItem);
    if (FAILED(hr) || NULL == pItem) return hr;

    CComVariant vbValue;
    hr = pItem->get_nodeValue(&vbValue);
    if (SUCCEEDED(hr)) szName = vbValue.bstrVal;
    return hr;
}

/**
* 查找多个元素
*/
HRESULT QueryList(IXMLDOMElement* pRoot, BSTR queryString, CAssemblyMap& mapNode)
{
    CComPtr<IXMLDOMNodeList> pList;
    HRESULT hr = pRoot->selectNodes(queryString, &pList);
    if (FAILED(hr) || NULL == pList) return hr;

    long length = 0;
    hr = pList->get_length(&length);
    if (FAILED(hr) || 0 == length) return hr;

    CAtlString szName;
    for (long index = 0; index < length; index++)
    {
        CComPtr<IXMLDOMNode> pNode;
        hr = pList->get_item(index, &pNode);
        if (SUCCEEDED(hr) && SUCCEEDED(GetNodeName(pNode, szName)))
        {
            mapNode.Add(szName, NULL);
        }
    }
    return hr;
}

/**
* 递归插入TreeView
*/
void TreeInsert(HTREEITEM hParent, CAssemblyNode *pParent, HWND hTree, const ATL::CString& szParent)
{
    TV_INSERTSTRUCT tvi;
    ZeroMemory(&tvi, sizeof(tvi));
    tvi.hParent = hParent;
    tvi.hInsertAfter = TVI_SORT;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;

    for (int i = 0; i < pParent->Package.GetSize(); i++)
    {
        CAssemblyNode *pChild = pParent->Package.GetValueAt(i);
		ATL::CString& szName = pParent->Package.GetKeyAt(i);
		if (!szName.Compare(szParent)) continue;
		tvi.item.pszText = (LPTSTR)szName.GetString();
        tvi.item.lParam = reinterpret_cast<LPARAM>(pChild);
        HTREEITEM hItem = TreeView_InsertItem(hTree, &tvi);
        pChild->Parent.SetAt(pParent, hItem);
        // 添加到搜索容器
		if (pChild->Package.GetSize() > 0) TreeInsert(hItem, pChild, hTree, szName);
    }
}

/**
* 扫描线程工作函数
*/
DWORD CALLBACK CMainFrm::ThreadScan(LPVOID lpParam)
{
    CMainFrm *pT = reinterpret_cast<CMainFrm *>(lpParam);
    CComPtr<ITaskbarList3> pTaskbar;
    CComPtr<IXMLDOMDocument> pXml;
    CFindFile hFind;
    HRESULT hr = S_OK;
    
    HR_CHECK(::CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE));
    // 初始化任务栏
    HR_CHECK(pTaskbar.CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER));
    HR_CHECK(pTaskbar->HrInit());
    HR_CHECK(pTaskbar->SetProgressState(pT->m_hWnd, TBPF_INDETERMINATE));

    pT->mapPackage.RemoveAll();
    // 初始化 XML 对象
    HR_CHECK(pXml.CoCreateInstance(OLESTR("Msxml2.DOMDocument"), NULL, CLSCTX_INPROC_SERVER));

    // 开始遍历文件
    TCHAR szPath[MAX_PATH];
    BOOL_CHECK(::PathCombine(szPath, pT->m_dlgFolder.GetFolderPath(), TEXT("Servicing\\Packages")));

    TCHAR szSearch[MAX_PATH];
    ::PathCombine(szSearch, szPath, TEXT("*Package*~*~~*.mum"));
    BOOL_CHECK(hFind.FindFile(szSearch));     
    do
    {
        // 加载 mum 文件
        CComPtr<IXMLDOMElement> pRoot;
        VARIANT_BOOL vbLoaded = VARIANT_FALSE;
        HR_CHECK(pXml->load(CComVariant(hFind.GetFilePath()), &vbLoaded));
        if (vbLoaded == VARIANT_TRUE && SUCCEEDED(pXml->get_documentElement(&pRoot)))
        {
            CAtlString szName;
            CComPtr<IXMLDOMNode> pNode;
            hr = pRoot->selectSingleNode(L"//assembly/assemblyIdentity", &pNode);
            if (SUCCEEDED(hr) && SUCCEEDED(GetNodeName(pNode, szName)))
            {
                CAssemblyNode *pAssembly = new CAssemblyNode();
                hr = QueryList(pRoot, L"//assembly/package/update/package/assemblyIdentity", pAssembly->Package);
                pT->mapPackage.Add(szName, pAssembly);
            }
        }
    } while (hFind.FindNextFile());
    hFind.Close();

    // 设置封包父子关系
    for (int i = 0; i < pT->mapPackage.GetSize(); i++)
    {
        CAssemblyNode *pParent = pT->mapPackage.GetValueAt(i);
        for (int j = 0; j < pParent->Package.GetSize(); j++)
        {
            CAssemblyNode *pChild = NULL;
            do {
                pChild = pT->mapPackage.Lookup(pParent->Package.GetKeyAt(j));
                if (pChild) break;
                pParent->Package.RemoveAt(j);
            } while (j < pParent->Package.GetSize());

            if (NULL != pChild)
            {
                pChild->Parent.Add(pParent, NULL);
                pParent->Package.SetAtIndex(j, pParent->Package.GetKeyAt(j), pChild);
            }
        }
    }

    // 将顶级封包添加到Root
    TV_INSERTSTRUCT tvi;
    ZeroMemory(&tvi, sizeof(tvi));
    tvi.hInsertAfter = TVI_SORT;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;

    pT->wndTree.DeleteAllItems();
    for (int i = 0; i < pT->mapPackage.GetSize(); i++)
    {
        CAssemblyNode *pNode = pT->mapPackage.GetValueAt(i);
        if (pNode->Parent.GetSize() == 0)
        {
			ATL::CString& szName = pT->mapPackage.GetKeyAt(i);
			tvi.item.pszText = (LPTSTR)szName.GetString();
            tvi.item.lParam = reinterpret_cast<LPARAM>(pNode);
            HTREEITEM hRoot = pT->wndTree.InsertItem(&tvi);
            if (pNode->Package.GetSize() > 0)
				TreeInsert(hRoot, pNode, pT->wndTree, szName);
        }
    }
    CStatusBarCtrl(pT->m_hWndStatusBar).SetText(0, pT->m_dlgFolder.GetFolderPath());

exit:
    CloseHandle(pT->m_hThread);
    pT->m_hThread = NULL;
    return pTaskbar->SetProgressState(pT->m_hWnd, SUCCEEDED(hr) ? TBPF_NOPROGRESS : TBPF_ERROR);
}