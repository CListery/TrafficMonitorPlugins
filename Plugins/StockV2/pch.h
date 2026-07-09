// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"

#include "resource.h"

#include <wx/wx.h>
#include <wx/sharedptr.h>

#define AFX_MANAGE_STATE_MODULE() \
    AFX_MANAGE_STATE(AfxGetStaticModuleState()) \
    LLOG_DEBUG_F("[HostCall]")

#define SAFE_DELETE(p) do \
{\
    if(p != nullptr) \
    { \
        delete p; \
        p = nullptr; \
    } \
} while (false)

//#define _CRTDBG_MAP_ALLOC
//#include <crtdbg.h>
//#ifdef _DEBUG
//#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
//#endif

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif

#include "Common.h"
#include "DataManager.h"
#include "Logger.h"

#endif //PCH_H
