// Wizard.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "Wizard.h"

#pragma data_seg("SHARE")
static HHOOK g_hHook = NULL;
#pragma data_seg()

#pragma comment(linker, "/SECTION:SHARE,RWS")

HINSTANCE g_hInstance = NULL;
HMODULE   g_hLib = NULL;
HANDLE    g_hFileHandle = NULL;
HGLOBAL   g_hHardSrc = NULL;
HGLOBAL   g_hSoftSrc = NULL;

#define sz_Or_Ord WORD
typedef struct { 
    WORD dlgVer; 
    WORD signature; 
    DWORD helpID; 
    DWORD exStyle; 
    DWORD style; 
    WORD cDlgItems; 
    short x; 
    short y; 
    short cx; 
    short cy; 
    sz_Or_Ord menu; 
    sz_Or_Ord windowClass; 
}DLGPARTEX;

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)  
{
    WCHAR ButtonTitle[255] = {0};
    WCHAR strWin7[] = L"Install this driver software anyway";
    
    if(hwnd)
    {
        CString txt;
        int nLen = GetWindowText(hwnd, ButtonTitle, 250);
        txt.Format( L"(%s) = %d", (LPCWSTR)ButtonTitle, nLen );
        /*MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); */
        if(nLen > 0)
        {
            if (ButtonTitle[wcslen(ButtonTitle) - 2] == TEXT('Y')
                || ButtonTitle[wcslen(ButtonTitle) - 2] == TEXT('C')
                || ButtonTitle[1] == TEXT('Y')
                || ButtonTitle[1] == TEXT('C'))
            {
                SendMessage(hwnd, WM_LBUTTONDOWN, 0, 0);
                SendMessage(hwnd, WM_LBUTTONUP, 0, 0);
                return FALSE;
            }
#if 0            
            else if((nLen >= 5) &&(wcsncmp(ButtonTitle, strWin7, 7) == 0))
            {
                SendMessage(hwnd, WM_LBUTTONDOWN, 0, 0);
                SendMessage(hwnd, WM_LBUTTONUP, 0, 0);
                return FALSE;
            }
#endif            
        }
    }
    return TRUE;
}

LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
/*#if DBG_ERROR_MESSAGE_BOX   
    MessageBox(NULL, L"Hooking",NULL,MB_YESNO); 
#endif    */
    if (nCode >= 0)
    {
        WCHAR strTitle[100] = {0};
        WCHAR strWin7[] = L"Windows Security";
        CWPSTRUCT *pMsg = (CWPSTRUCT*)lParam;
        if (WM_SHOWWINDOW == pMsg->message)
        {
            int nLen = GetWindowText(pMsg->hwnd, strTitle, 100);
            if (0 != nLen)
            {
                HANDLE hFileHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, TEXT("TONY"));
                if (NULL == hFileHandle)
                {
                    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
                }
                PVOID pInfo = MapViewOfFile (hFileHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
                if (NULL == pInfo)
                {
                    CloseHandle(hFileHandle);
                    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
                }
                WCHAR *pStrInfo = (WCHAR*)pInfo;
                int nOffset;
                for (int i = 0; i < 2; i++)
                {
                    nOffset = pStrInfo[0];
                    if ((nOffset != 0) && (wcsncmp(strTitle, pStrInfo + 1, nLen) == 0))
                    {
                        RECT r;
                        GetWindowRect(pMsg->hwnd, &r);
                
                        MoveWindow(pMsg->hwnd, -100, -100, r.right -r.left, r.top - r.bottom, TRUE);
                        EnumChildWindows(pMsg->hwnd, EnumChildProc, NULL);                
                    }
#if 0            
                    
                    else if((nOffset != 0) && (nLen >= 7) && (wcsncmp(strTitle, strWin7, nLen-1) == 0)) 
                    {
                        RECT r;
                        CString txt;
                        GetWindowRect(pMsg->hwnd, &r);
                        txt.Format( L"find windows ( %s ) = %d", (LPCWSTR)strTitle, nLen );
                        MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
                
                        MoveWindow(pMsg->hwnd, -100, -100, r.right -r.left, r.top - r.bottom, TRUE);
                        EnumChildWindows(pMsg->hwnd, EnumChildProc, NULL); 
                        break;
                    }
#endif                    
                    pStrInfo += nOffset + 1;
                }
                UnmapViewOfFile(pInfo);
                CloseHandle(hFileHandle);
            }
        }
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

BOOL Allocate()
{
    /*Hardware Installation*/
    HRSRC hSrc = FindResource(g_hLib, TEXT("#5314"), RT_DIALOG);
    if (NULL == hSrc)
    {
#if DBG_ERROR_MESSAGE_BOX   
        MessageBox(NULL, L"hSrc1",NULL,MB_YESNO); 
#endif
        return FALSE;
    }
    
    g_hHardSrc = LoadResource((HINSTANCE)g_hLib, hSrc);
    if (NULL == g_hHardSrc)
    {
#if DBG_ERROR_MESSAGE_BOX   
        MessageBox(NULL, L"g_hHardSrc2",NULL,MB_YESNO); 
#endif
        return FALSE;
    }
    PVOID lp = LockResource(g_hHardSrc);

    WCHAR wcWindowTitle[200] = {0};
    WCHAR *wcInfo = (WCHAR*)lp;
    int nHardLen = 0;
    int nSoftLen = 0;
    
    if(wcInfo[0] == 0x01 && wcInfo[1] == 0xffff)
    {
        lp = (char *)lp + sizeof(DLGPARTEX) - 2;
        wcInfo = (WCHAR*)(lp);
    }
    else
    {
        wcInfo = wcInfo + 11;
    }
    nHardLen = (int)wcslen(wcInfo);
    wcWindowTitle[0] = nHardLen;
    wcsncpy(wcWindowTitle + 1, wcInfo, nHardLen);

    /*Software Installation*/
    hSrc = FindResource(g_hLib, TEXT("#5316"), RT_DIALOG);
    if (NULL != hSrc)
    {
        g_hSoftSrc = LoadResource((HINSTANCE)g_hLib, hSrc);
        if (NULL == g_hSoftSrc)
        {
#if DBG_ERROR_MESSAGE_BOX   
            MessageBox(NULL, L"g_hSoftSrc3",NULL,MB_YESNO);
#endif
            return FALSE;
        }
        lp = LockResource(g_hSoftSrc);
        wcInfo = (WCHAR*)lp;

        if(wcInfo[0] == 0x01 && wcInfo[1] == 0xffff)
        {
            lp = (char *)lp + sizeof(DLGPARTEX) - 2;
            wcInfo = (WCHAR*)(lp);
        }
        else
        {
            wcInfo = wcInfo + 11;
        }
        nSoftLen = (int)wcslen(wcInfo);
        wcWindowTitle[nHardLen + 1] = nSoftLen;
        wcsncpy(wcWindowTitle + nHardLen + 2, wcInfo, nSoftLen);
    }
    g_hFileHandle = CreateFileMapping(INVALID_HANDLE_VALUE,
                                        NULL,
                                        PAGE_READWRITE, 
                                        0, 
                                        255,
                                        TEXT("TONY"));
    if (NULL == g_hFileHandle)
    {
#if DBG_ERROR_MESSAGE_BOX   
        MessageBox(NULL, L"g_hFileHandle4",NULL,MB_YESNO); 
#endif
        return FALSE;
    }

    PVOID pInfo = MapViewOfFile(g_hFileHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (NULL == pInfo)
    {
#if DBG_ERROR_MESSAGE_BOX   
        MessageBox(NULL, L"MapViewOfFile5",NULL,MB_YESNO); 
#endif
        return FALSE;
    }
    memcpy(pInfo, wcWindowTitle, 200);
    UnmapViewOfFile(pInfo);

    g_hHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, g_hInstance, 0);
    /*Cannot set nonlocal hook without a module handle. */
#if DBG_ERROR_MESSAGE_BOX   
    if(NULL == g_hHook)
    {
        CString txt = L"";     
        txt.Format( L"Hook error: 0x%x", GetLastError());
        MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
    }
    else
    {
        MessageBox(NULL, L"Hooked OK",NULL,MB_YESNO); 
    }
#endif
    return TRUE;
}

BOOL Release()
{
    if (NULL != g_hHook)
    {
        UnhookWindowsHookEx(g_hHook);
    }
    if (NULL != g_hHardSrc)
    {
        UnlockResource(g_hHardSrc);
    }
    if (NULL != g_hSoftSrc)
    {
        UnlockResource(g_hSoftSrc);
    }
    if (NULL != g_hLib)
    {
        FreeLibrary(g_hLib);
    }
    if (NULL != g_hFileHandle)
    {
        CloseHandle(g_hFileHandle);
    }
	return TRUE;
}

