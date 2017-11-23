#include "StdAfx.h"

/**
* 处理单个元素
*/
HRESULT CreateNode(IXMLDOMNode *pNode, CAssemblyNode *pAssembly)
{
    CComPtr<IXMLDOMNamedNodeMap> pMap;
    HRESULT hr = pNode->get_attributes(&pMap);
    if (FAILED(hr) || NULL == pMap) return hr;

    CComPtr<IXMLDOMNode> pItem;
    CComVariant vbValue;

    hr = pMap->getNamedItem(L"name", &pItem);
    if (SUCCEEDED(hr) && SUCCEEDED(pItem->get_nodeValue(&vbValue)))
    {
        pAssembly->szName = vbValue.bstrVal;
        pItem = NULL;
    }

    hr = pMap->getNamedItem(L"processorArchitecture", &pItem);
    if (SUCCEEDED(hr) && SUCCEEDED(pItem->get_nodeValue(&vbValue)))
    {
        pAssembly->szArch = vbValue.bstrVal;
        pItem = NULL;
    }

    pNode = NULL;
    hr = pMap->getNamedItem(L"language", &pItem);
    if (SUCCEEDED(hr) && SUCCEEDED(pItem->get_nodeValue(&vbValue)))
    {
        pAssembly->szLanguage = vbValue.bstrVal;
        pItem = NULL;
    }

    hr = pMap->getNamedItem(L"version", &pItem);
    if (SUCCEEDED(hr) && SUCCEEDED(pItem->get_nodeValue(&vbValue)))
    {
        pAssembly->szVersion = vbValue.bstrVal;
        pItem = NULL;
    }

    hr = pMap->getNamedItem(L"publicKeyToken", &pItem);
    if (SUCCEEDED(hr) && SUCCEEDED(pItem->get_nodeValue(&vbValue)))
    {
        pAssembly->szToken = vbValue.bstrVal;
        pItem = NULL;
    }
    return hr;
}

/**
* 查找多个元素
*/
HRESULT CreateList(IXMLDOMElement* pRoot, BSTR queryString, CAssemblyMap& nodeMap)
{
    CComPtr<IXMLDOMNodeList> pList;
    HRESULT hr = pRoot->selectNodes(queryString, &pList);
    if (FAILED(hr) || NULL == pList) return hr;

    long length = 0;
    hr = pList->get_length(&length);
    if (FAILED(hr) || 0 == length) return hr;
    
    for (long index = 0; index < length; index++)
    {
        CComPtr<IXMLDOMNode> pNode;
        hr = pList->get_item(index, &pNode);
        if (SUCCEEDED(hr) && NULL != pNode)
        {
            CComPtr<CAssemblyNode> pAssembly = new CAssemblyNode();
            if (SUCCEEDED(CreateNode(pNode, pAssembly)))
            {
                nodeMap.Add(pAssembly->szName, pAssembly);
            }
        }
    }
    return hr;
}

/**
* 递归插入TreeView
*/
void CMainDlg::RecurveInsert(HTREEITEM hParent, CAssemblyNode *pParent)
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
        HTREEITEM hItem = TreeView_InsertItem(m_tree, &tvi);
        pChild->Parent.SetAt(pParent, hItem);
        // 添加到搜索容器
        if (pChild->Package.GetSize() > 0) RecurveInsert(hItem, pChild);
    }
}

/**
* 扫描线程工作函数
*/
DWORD CALLBACK CMainDlg::ThreadScan(LPVOID lpParam)
{
    CMainDlg *pDlg = (CMainDlg *)lpParam;
    TCHAR szPackage[MAX_PATH], szSearch[MAX_PATH], szXml[MAX_PATH];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    CComPtr<IXMLDOMDocument> pXml = NULL;
    TV_INSERTSTRUCT tvi = { 0 };
    WIN32_FIND_DATA wfd = { 0 };
    
    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    HR_CHECK(hr);
    // 初始化 XML 对象
    hr = pXml.CoCreateInstance(_uuidof(DOMDocument), NULL, CLSCTX_INPROC_SERVER);
    HR_CHECK(hr);

    // 开始遍历文件
    ::PathCombine(szPackage, pDlg->nodeRoot.szName, TEXT("Servicing\\Packages"));
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
                CComPtr<CAssemblyNode> pAssembly = new CAssemblyNode();
                CComPtr<IXMLDOMNode> pNode;
                hr = pRoot->selectSingleNode(L"//assembly/assemblyIdentity", &pNode);
                if (SUCCEEDED(hr) && SUCCEEDED(CreateNode(pNode, pAssembly)))
                {
                    CreateList(pRoot, L"//assembly/package/parent/assemblyIdentity", pAssembly->Depend);
                    CreateList(pRoot, L"//assembly/package/update/package/assemblyIdentity", pAssembly->Package);
                    CreateList(pRoot, L"//assembly/package/update/component/assemblyIdentity", pAssembly->Component);
                    CreateList(pRoot, L"//assembly/package/update/driver/assemblyIdentity", pAssembly->Driver);
                    pDlg->mapPackage.Add(pAssembly->szName, pAssembly);
                }
            }
        }
    } while (::FindNextFile(hFind, &wfd));
    HR_CHECK(::FindClose(hFind));

    // 设置封包父子关系
    for (int i = 0; i < pDlg->mapPackage.GetSize(); i++)
    {
        CAssemblyNode *pChild, *pParent = pDlg->mapPackage.GetValueAt(i);
        for (int j = 0; j < pParent->Package.GetSize(); j++)
        {
            while (NULL == (pChild = pDlg->mapPackage.Lookup(pParent->Package.GetKeyAt(j))))
            {
                pParent->Package.RemoveAt(j);
                if (j >= pParent->Package.GetSize()) break;
            }
            if (NULL != pChild) 
            {
                pChild->Parent.Add(pParent, NULL);
                pParent->Package.SetAtIndex(j, pChild->szName, pChild);
            }
        }
    }

    // 将顶级封包添加到Root
    for (int i = 0; i < pDlg->mapPackage.GetSize(); i++)
    {
        CAssemblyNode *pNode = pDlg->mapPackage.GetValueAt(i);
        if (pNode->Parent.GetSize() == 0) pDlg->nodeRoot.Package.Add(pNode->szName, pNode);
    }

    // 插入根节点
    tvi.hInsertAfter = TVI_ROOT;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.pszText = szPackage;
    tvi.item.lParam = reinterpret_cast<LPARAM>(&pDlg->nodeRoot);

    HTREEITEM hRoot = TreeView_InsertItem(pDlg->m_tree, &tvi);
    pDlg->RecurveInsert(hRoot, &pDlg->nodeRoot);

    TreeView_SetItemState(pDlg->m_tree, hRoot, 0, TVIS_STATEIMAGEMASK);
    TreeView_Expand(pDlg->m_tree, hRoot, TVE_EXPAND);
exit:
    ::CoUninitialize();
    pDlg->m_hThread = NULL;
    return hr;
}