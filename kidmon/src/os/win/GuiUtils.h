#pragma once

#include <Windows.h>

class ScopedDeleteDC
{
    HDC hdc_{ nullptr };

    ScopedDeleteDC(const ScopedDeleteDC&) = delete;
    ScopedDeleteDC(ScopedDeleteDC&&) = delete;
    ScopedDeleteDC& operator=(const ScopedDeleteDC&) = delete;
    ScopedDeleteDC& operator=(ScopedDeleteDC&&) = delete;

public:
    ScopedDeleteDC(HDC hdc) noexcept
        : hdc_(hdc)
    {}

    ~ScopedDeleteDC()
    {
        if (hdc_)
        {
            DeleteDC(hdc_);
            hdc_ = nullptr;
        }
    }

    HDC hdc() const noexcept
    {
        return hdc_;
    }
};


class ScopedReleaseDC
{
    HWND hwnd_{ nullptr };
    HDC hdc_{ nullptr };

    ScopedReleaseDC(const ScopedReleaseDC&) = delete;
    ScopedReleaseDC(ScopedReleaseDC&&) = delete;
    ScopedReleaseDC& operator=(const ScopedReleaseDC&) = delete;
    ScopedReleaseDC& operator=(ScopedReleaseDC&&) = delete;

public:
    ScopedReleaseDC(HWND hwnd, HDC hdc) noexcept
        : hwnd_(hwnd)
        , hdc_(hdc)
    {}

    ~ScopedReleaseDC()
    {
        if (hdc_)
        {
            ReleaseDC(hwnd_, hdc_);
            hdc_ = nullptr;
            hwnd_ = nullptr;
        }
    }

    HDC hdc() const noexcept { return hdc_; }
};


class ScopedSelectObject
{
    HDC hdc_{ nullptr };
    HGDIOBJ obj_{ nullptr };
    HGDIOBJ prevObj_{ nullptr };

    ScopedSelectObject(const ScopedSelectObject&) = delete;
    ScopedSelectObject(ScopedSelectObject&&) = delete;
    ScopedSelectObject& operator=(const ScopedSelectObject&) = delete;
    ScopedSelectObject& operator=(ScopedSelectObject&&) = delete;

public:
    ScopedSelectObject(HDC hdc, HGDIOBJ obj) noexcept
        : hdc_(hdc)
        , obj_(obj)
    {
        prevObj_ = SelectObject(hdc_, obj_);
    }

    ~ScopedSelectObject()
    {
        SelectObject(hdc_, prevObj_);
    }
};


template <class T>
class GdiObject
{
    T hobj_{ nullptr };

    GdiObject(const GdiObject&) = delete;
    GdiObject& operator=(const GdiObject&) = delete;

public:
    GdiObject(T hobj) noexcept
        : hobj_(hobj)
    {
    }

    ~GdiObject()
    {
        if (hobj_)
        {
            DeleteObject(hobj_);
            hobj_ = nullptr;
        }
    }

    GdiObject(GdiObject&& rhs) noexcept
    {
        hobj_ = rhs.hobj_;
        rhs.hobj_ = nullptr;
    }

    GdiObject& operator=(GdiObject&& rhs) noexcept
    {
        if (this != &rhs)
        {
            hobj_ = rhs.hobj_;
            rhs.hobj_ = nullptr;
        }

        return *this;
    }

    T handle() const noexcept
    {
        return hobj_;
    }
};


struct ComPtrDeleter
{
    void operator()(IUnknown* ptr)
    {
        if (ptr)
        {
            ptr->Release();
            ptr = nullptr;
        }
    }
};
