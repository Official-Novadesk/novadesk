#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>

static const wchar_t *kManageWindowClassName = L"NovadeskManagerWindow";
static const wchar_t *kManageCloseMessageName = L"Novadesk.Manage.RequestClose";

static std::wstring GetExeDir()
{
    wchar_t path[MAX_PATH + 1] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path().wstring();
}

static std::wstring JoinPath(const std::wstring &a, const std::wstring &b)
{
    std::filesystem::path p(a);
    p /= b;
    return p.wstring();
}

static void RequestManageClose()
{
    HWND manageWindow = FindWindowW(kManageWindowClassName, nullptr);
    if (!manageWindow)
    {
        return;
    }

    const UINT closeMessage = RegisterWindowMessageW(kManageCloseMessageName);
    if (closeMessage == 0)
    {
        return;
    }

    DWORD_PTR result = 0;
    SendMessageTimeoutW(
        manageWindow,
        closeMessage,
        0,
        0,
        SMTO_ABORTIFHUNG,
        1500,
        &result);

    for (int i = 0; i < 80; ++i)
    {
        if (!FindWindowW(kManageWindowClassName, nullptr))
        {
            break;
        }
        Sleep(100);
    }
}

static bool StartManageWindow()
{
    const std::wstring exeDir = GetExeDir();
    const std::wstring manageExePath = JoinPath(exeDir, L"manage_novadesk.exe");

    DWORD attrs = GetFileAttributesW(manageExePath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        return false;
    }

    std::wstring commandLine = L"\"" + manageExePath + L"\"";
    std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    const BOOL started = CreateProcessW(
        nullptr,
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        exeDir.c_str(),
        &si,
        &pi);

    if (!started)
    {
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    RequestManageClose();
    Sleep(1000);
    return StartManageWindow() ? 0 : 1;
}
