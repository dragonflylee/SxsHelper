#include "SxsHelper.h"
/**
* 处理单个元素
*/
HRESULT CreateNode(IXMLDOMNode *pNode, CAssemblyNode **ppNode)
{
    CComPtr<IXMLDOMNamedNodeMap> pMap;
    HRESULT hr = pNode->get_attributes(&pMap);
    if (FAILED(hr) || NULL == pMap) return hr;

    CComPtr<IXMLDOMNode> pItem;
    CComVariant vbValue;
    CAssemblyNode * pAssembly = new CAssemblyNode();

    hr = pMap->getNamedItem(L"name", &pItem);
    if (SUCCEEDED(hr) && SUCCEEDED(pItem->get_nodeValue(&vbValue)))
    {
        pAssembly->szName = vbValue.bstrVal;
        pItem.Release();
    }

    hr = pMap->getNamedItem(L"processorArchitecture", &pItem);
    if (SUCCEEDED(hr) && SUCCEEDED(pItem->get_nodeValue(&vbValue)))
    {
        pAssembly->szArch = vbValue.bstrVal;
        pItem.Release();
    }

    hr = pMap->getNamedItem(L"language", &pItem);
    if (SUCCEEDED(hr) && SUCCEEDED(pItem->get_nodeValue(&vbValue)))
    {
        pAssembly->szLanguage = vbValue.bstrVal;
        pItem.Release();
    }

    hr = pMap->getNamedItem(L"version", &pItem);
    if (SUCCEEDED(hr) && SUCCEEDED(pItem->get_nodeValue(&vbValue)))
    {
        pAssembly->szVersion = vbValue.bstrVal;
        pItem.Release();
    }

    hr = pMap->getNamedItem(L"publicKeyToken", &pItem);
    if (SUCCEEDED(hr) && SUCCEEDED(pItem->get_nodeValue(&vbValue)))
    {
        pAssembly->szToken = vbValue.bstrVal;
        pItem.Release();
    }
    *ppNode = pAssembly;
    return hr;
}

/**
* 查找多个元素
*/
HRESULT CreateList(IXMLDOMElement* pRoot, BSTR queryString, CAssemblyMap& nodeList)
{
    CComPtr<IXMLDOMNodeList> pList;
    HRESULT hr = pRoot->selectNodes(queryString, &pList);
    if (FAILED(hr) || NULL == pList) return hr;

    long length = 0;
    hr = pList->get_length(&length);
    if (FAILED(hr) || 0 == length) return hr;

    CComVariant vbValue;
    for (long index = 0; index < length; index++)
    {
        CComPtr<IXMLDOMNode> pNode;
        hr = pList->get_item(index, &pNode);
        if (SUCCEEDED(hr) && NULL != pNode)
        {
            CComPtr<IXMLDOMNamedNodeMap> pMap;
            hr = pNode->get_attributes(&pMap);
            if (SUCCEEDED(hr) && NULL != pMap)
            {
                CComPtr<IXMLDOMNode> pItem;
                hr = pMap->getNamedItem(L"name", &pItem);
                if (SUCCEEDED(hr) && SUCCEEDED(pItem->get_nodeValue(&vbValue)))
                {
                    nodeList.Add(vbValue.bstrVal, NULL);
                }
            }
        }
    }
    return hr;
}

/**
* 递归插入TreeView
*/
void CMainFrm::DoInsert(HTREEITEM hParent, CAssemblyNode *pParent)
{
    TV_INSERTSTRUCT tvi;
    tvi.hParent = hParent;
    tvi.hInsertAfter = TVI_SORT;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;

    for (int i = 0; i < pParent->Package.GetSize(); i++)
    {
        CAssemblyNode *pChild = pParent->Package.GetValueAt(i);
        tvi.item.pszText = pChild->szName.GetBuffer();
        tvi.item.lParam = reinterpret_cast<LPARAM>(pChild);
        HTREEITEM hItem = TreeView_InsertItem(wndTree, &tvi);
        pChild->Parent.SetAt(pParent, hItem);
        // 添加到搜索容器
        if (pChild->Package.GetSize() > 0) DoInsert(hItem, pChild);
    }
}

/**
* 扫描线程工作函数
*/
DWORD CALLBACK CMainFrm::ThreadScan(LPVOID lpParam)
{
    CMainFrm *pT = (CMainFrm *)lpParam;
    TCHAR szPackage[MAX_PATH], szSearch[MAX_PATH], szXml[MAX_PATH];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    CComPtr<IXMLDOMDocument> pXml;
    TV_INSERTSTRUCT tvi = { 0 };
    WIN32_FIND_DATA wfd = { 0 };
    HRESULT hr = S_OK;
    CComPtr<ITaskbarList3> pTaskbar;
    
    HR_CHECK(::CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE));
    // 初始化任务栏
    HR_CHECK(pTaskbar.CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER));
    HR_CHECK(pTaskbar->HrInit());
    HR_CHECK(pTaskbar->SetProgressState(pT->m_hWnd, TBPF_INDETERMINATE));
    // 初始化 XML 对象
    HR_CHECK(pXml.CoCreateInstance(_uuidof(DOMDocument), NULL, CLSCTX_INPROC_SERVER));

    // 开始遍历文件
    ::PathCombine(szPackage, pT->nodeRoot.szName, TEXT("Servicing\\Packages"));
    ::PathCombine(szSearch, szPackage, TEXT("*Package*~*~~*.mum"));
    hFind = ::FindFirstFile(szSearch, &wfd);
    HR_CHECK(INVALID_HANDLE_VALUE  != hFind);        
    do
    {
        VARIANT_BOOL vbLoaded = VARIANT_FALSE;
        ::PathCombine(szXml, szPackage, wfd.cFileName);
        // 加载 mum 文件
        hr = pXml->load(CComVariant(szXml), &vbLoaded);
        if (SUCCEEDED(hr) && vbLoaded == VARIANT_TRUE)
        {
            CComPtr<IXMLDOMElement> pRoot;
            hr = pXml->get_documentElement(&pRoot);
            if (SUCCEEDED(hr) && pRoot != NULL)
            {
                CAssemblyNode *pAssembly = NULL;
                CComPtr<IXMLDOMNode> pNode;
                hr = pRoot->selectSingleNode(L"//assembly/assemblyIdentity", &pNode);
                if (SUCCEEDED(hr) && SUCCEEDED(CreateNode(pNode, &pAssembly)))
                {
                    CreateList(pRoot, L"//assembly/package/update/package/assemblyIdentity", pAssembly->Package);
                    pT->mapPackage.Add(pAssembly->szName, pAssembly);
                }
            }
        }
    } while (::FindNextFile(hFind, &wfd));
    HR_CHECK(::FindClose(hFind));

    // 设置封包父子关系
    for (int i = 0; i < pT->mapPackage.GetSize(); i++)
    {
        CAssemblyNode *pChild, *pParent = pT->mapPackage.GetValueAt(i);
        for (int j = 0; j < pParent->Package.GetSize(); j++)
        {
            while (NULL == (pChild = pT->mapPackage.Lookup(pParent->Package.GetKeyAt(j))))
            {
                pParent->Package.RemoveAt(j);
                if (j >= pParent->Package.GetSize()) break;
            }
            if (NULL != pChild) 
            {
                pChild->Parent.Add(pParent, NULL);
                pParent->Package.SetAt(pChild->szName, pChild);
            }
        }
        /*for (size_t j = 0; j < pParent->Depend.GetCount(); j++)
        {
            while (NULL == (pChild = pDlg->mapPackage.Lookup(pParent->Depend.GetAt(j)->szName)))
            {
                pParent->Depend.RemoveAt(j);
                if (j >= pParent->Depend.GetCount()) break;
            }
            if (NULL != pChild)
            {
                pParent->Depend.SetAt(j, pChild);
            }
        }*/
    }

    // 将顶级封包添加到Root
    for (int i = 0; i < pT->mapPackage.GetSize(); i++)
    {
        CAssemblyNode *pNode = pT->mapPackage.GetValueAt(i);
        if (pNode->Parent.GetSize() == 0) 
            pT->nodeRoot.Package.Add(pNode->szName, pNode);
    }

    // 插入根节点
    tvi.hInsertAfter = TVI_ROOT;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.pszText = szPackage;
    tvi.item.lParam = reinterpret_cast<LPARAM>(&pT->nodeRoot);

    HTREEITEM hRoot = TreeView_InsertItem(pT->wndTree, &tvi);
    pT->DoInsert(hRoot, &pT->nodeRoot);

    TreeView_SetItemState(pT->wndTree, hRoot, 0, TVIS_STATEIMAGEMASK);
    TreeView_Expand(pT->wndTree, hRoot, TVE_EXPAND);

exit:
    pTaskbar->SetProgressState(pT->m_hWnd, TBPF_NOPROGRESS);
    ::CoUninitialize();
    CloseHandle(pT->m_hThread);
    pT->m_hThread = NULL;
    return hr;
}