#include "Window.h"
#include "GuiUtils.h"

#include <psapi.h>
#include <dwmapi.h>

#pragma warning( push, 4 )
#pragma warning( disable : 4458 )

#include <gdiplus.h>
#include <shellscalingapi.h>

#pragma warning( pop )

#include <spdlog/spdlog.h>

#include <array>

#pragma comment (lib, "Gdiplus.lib")
#pragma comment (lib, "Shcore.lib")
#pragma comment (lib, "Psapi.lib")
#pragma comment (lib, "Dwmapi.lib")

std::string hwndToString(HWND hwnd)
{
    uint64_t value = *reinterpret_cast<const uint64_t*>(&hwnd);

    return fmt::format("{:#010x}", value);
}

WindowImpl::WindowImpl(HWND hwnd) noexcept
    : hwnd_(hwnd)
    , id_(hwndToString(hwnd_))
{
}

class GdiPlusInitializer
{
    ULONG_PTR gdiplusToken_{ 0 };
public:
    GdiPlusInitializer()
    {
        // Initialize GDI+.
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput, NULL);
    }

    ~GdiPlusInitializer()
    {
        Gdiplus::GdiplusShutdown(gdiplusToken_);
    }
};

const std::string& WindowImpl::id() const
{
    return id_;
}

std::string WindowImpl::title() const
{
    std::array<char, 256> buffer {};
    const int size = GetWindowTextA(hwnd_, buffer.data(), static_cast<int>(buffer.size()));

    if (size)
    {
        return std::string(buffer.data(), size);
    }

    return {};
}

std::string WindowImpl::className() const
{
    std::array<char, 256> buffer {};
    const int size = RealGetWindowClassA(hwnd_, buffer.data(), static_cast<int>(buffer.size()));

    if (size)
    {
        return std::string(buffer.data(), size);
    }

    return {};
}

fs::path WindowImpl::ownerProcessPath() const
{
    DWORD procId = static_cast<DWORD>(ownerProcessId());
    HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procId);

    if (nullptr == proc)
    {
        return {};
    }

    HMODULE modules{ nullptr };
    DWORD modulesCount{ 0 };
    DWORD sz{ 0 };
    std::array<wchar_t, 256> modulePath {};

    if (EnumProcessModules(proc, &modules, sizeof(modules), &modulesCount))
    {
        sz = GetModuleFileNameExW(proc, modules, modulePath.data(),
                                  static_cast<DWORD>(modulePath.size()));
    }

    CloseHandle(proc);

    return fs::path(std::wstring_view(modulePath.data(), sz));
}

uint64_t WindowImpl::ownerProcessId() const noexcept
{
    DWORD procId{ 0 };
    GetWindowThreadProcessId(hwnd_, &procId);

    return procId;
}

Rect WindowImpl::boundingRect() const noexcept
{
    WINDOWPLACEMENT wndpl = {};
    wndpl.length = sizeof(WINDOWPLACEMENT);

    if (GetWindowPlacement(hwnd_, &wndpl))
    {
        const RECT& rc = wndpl.rcNormalPosition;

        Point pt(rc.left, rc.top);
        Dimensions dims(rc.right - rc.left, rc.bottom - rc.top);

        return Rect(pt, dims);
    }

    return Rect();
}

bool saveToBuffer(HBITMAP bitmap, const ImageFormat format, std::vector<char>& content)
{
    static GdiPlusInitializer initializer_;
    Gdiplus::Bitmap bmp(bitmap, nullptr);

    // Write to IStream
    IStream* istream{ nullptr };
    auto hr = CreateStreamOnHGlobal(NULL, TRUE, &istream);

    if (FAILED(hr))
    {
        return false;
    }

    // Autmatic cleanup on scope exit
    std::unique_ptr<IStream, ComPtrDeleter> streamPtr(istream);
    istream = nullptr;

    // Define encoding
    std::wstring_view guid = L"";
    switch (format)
    {
    case ImageFormat::bmp: guid = L"{557cf400-1a04-11d3-9a73-0000f81ef32e}"; break;
    case ImageFormat::jpg: guid = L"{557cf401-1a04-11d3-9a73-0000f81ef32e}"; break;
    case ImageFormat::gif: guid = L"{557cf402-1a04-11d3-9a73-0000f81ef32e}"; break;
    case ImageFormat::tif: guid = L"{557cf405-1a04-11d3-9a73-0000f81ef32e}"; break;
    case ImageFormat::png: guid = L"{557cf406-1a04-11d3-9a73-0000f81ef32e}"; break;
    default:
        return false;
    }

    CLSID clsid{};
    if (FAILED(CLSIDFromString(guid.data(), &clsid)))
    {
        return false;
    }

    Gdiplus::Status status = bmp.Save(streamPtr.get(), &clsid, nullptr);
    if (status != Gdiplus::Status::Ok)
    {
        return false;
    }

    // Get memory handle associated with istream
    HGLOBAL hg{ nullptr };
    if (FAILED(GetHGlobalFromStream(streamPtr.get(), &hg)))
    {
        return false;
    }

    auto bufSize = GlobalSize(hg);
    content.resize(bufSize);

    // Lock & unlock memory
    LPVOID pimage = GlobalLock(hg);

    if (pimage)
    {
        memcpy(content.data(), pimage, bufSize);
    }

    GlobalUnlock(hg);

    return true;
}


bool WindowImpl::capture(const ImageFormat format, std::vector<char>& content)
{
    content.clear();

    ScopedReleaseDC windowDC(hwnd_, GetWindowDC(hwnd_));
    ScopedDeleteDC memDC(CreateCompatibleDC(windowDC.hdc()));

    if (!memDC.hdc())
    {
        spdlog::error("CreateCompatibleDC has failed");
        return false;
    }

    RECT rect {}, frame {};
    GetWindowRect(hwnd_, &rect);
    HRESULT hr = DwmGetWindowAttribute(hwnd_, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(frame));

    if (SUCCEEDED(hr))
    {
        RECT border {};

        border.left = frame.left - rect.left;
        border.top = frame.top - rect.top;
        border.right = rect.right - frame.right;
        border.bottom = rect.bottom - frame.bottom;

        // Adjust rect to visible part
        rect.left += border.left;
        rect.top += border.top;
        rect.right -= border.right;
        rect.bottom -= border.bottom;
    }

    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    GdiObject<HBITMAP> bmWnd(CreateCompatibleBitmap(windowDC.hdc(), windowWidth, windowHeight));

    if (!bmWnd.handle())
    {
        spdlog::error("CreateCompatibleBitmap Failed");
        return false;
    }

    ScopedSelectObject sso(memDC.hdc(), bmWnd.handle());
    ScopedReleaseDC screenDC(nullptr, GetDC(nullptr));

    //int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    //int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    SetStretchBltMode(memDC.hdc(), HALFTONE);

    if (!BitBlt(memDC.hdc(),
        0, 0,
        windowWidth, windowHeight,
        screenDC.hdc(),
        rect.left, rect.top,
        SRCCOPY))
    {
        spdlog::error("BitBlt has failed");
        return false;
    }

    return saveToBuffer(bmWnd.handle(), format, content);
}
