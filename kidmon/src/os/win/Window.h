#include "../Window.h"
#include "../../geometry/Rect.h"

#include <Windows.h>

class WindowImpl : public Window
{
    HWND hwnd_{ nullptr };
    Rect rect_;
    std::string id_;

public:
    WindowImpl(HWND hwnd) noexcept;

    const std::string& id() const override;
    std::string title() const override;
    std::string className() const override;
    std::wstring ownerProcessPath() const override;
    uint64_t ownerProcessId() const noexcept override;
    Rect boundingRect() const noexcept override;

    bool capture(const ImageFormat format, std::vector<char>& content) override;
};
