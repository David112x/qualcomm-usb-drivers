/*Wizard的头文件*/
#pragma once

#ifdef WIZARD_EXPORTS
extern "C"
{
    __declspec (dllexport) BOOL Allocate();
    __declspec (dllexport) BOOL Release();
}
#else
BOOL Release();
BOOL Allocate();
# define DBG_ERROR_MESSAGE_BOX 0
#endif