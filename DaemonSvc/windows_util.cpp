#include <Windows.h>
#include "scoped_handle.h"
#include "logger.h"
#include "windows_util.h"


#pragma comment(lib, "Advapi32.lib")



void* WindowsUtil::load_function(const std::string& module_name,
                                   const std::string& func_name,
                                   const bool log /*= true*/)
{
    HMODULE hModule = GetModuleHandleA(module_name.c_str());
    if (NULL == hModule)
    {
        if (log)
        {
            ErrorLogLastErr("GetModuleHandle[%s] fail", module_name.c_str());
        }
        return NULL;
    }
    else
    {
        FARPROC func_addr = GetProcAddress(hModule, func_name.c_str());
        if (NULL == func_addr)
        {
            if (log)
            {
                ErrorLogLastErr("GetProcAddress[%s:%s] fail", module_name.c_str(), func_name.c_str());
            }
        }
        return func_addr;
    }
}

bool WindowsUtil::set_privilege(const tstring& privilege_name, const bool enable)
{
    bool bReturn = false;

    do 
    {
        scoped_handle<> hToken;
        if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, hToken.get_ptr()))
        {
            ErrorLogLastErr("OpenProcessToken fail");
            break;
        }

        LUID luid = {0};
        if (!LookupPrivilegeValue(NULL,         // lookup privilege on local system
            privilege_name.c_str(),             // privilege to lookup
            &luid))                             // receives LUID of privilege
        {
            ErrorLogLastErr(TSTR("LookupPrivilegeValue[%s] fail"),
                privilege_name.c_str());
            break;
        }

        TOKEN_PRIVILEGES tp = {0};
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        if (enable)
        {
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        }
        else
        {
            tp.Privileges[0].Attributes = 0;
        }

        // Enable the privilege or disable all privileges.
        BOOL adjust_success = AdjustTokenPrivileges(hToken.get_ref(), FALSE, &tp,
            sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL);
        CLastErrorFormat e;
        if (!adjust_success)
        {
            ErrorLogLastErrEx(e, TSTR("AdjustTokenPrivileges fail when %s [%s]"),
                enable ? TSTR("enable") : TSTR("disable"), privilege_name.c_str());
            break;
        }

        if (e.code() == ERROR_NOT_ALL_ASSIGNED)
        {
            ErrorLog(TSTR("not all privileges were assigned when %s [%s]"),
                enable ? TSTR("enable") : TSTR("disable"), privilege_name.c_str());
            break;
        }

        bReturn = true;

    } while (false);

    return bReturn;
}


