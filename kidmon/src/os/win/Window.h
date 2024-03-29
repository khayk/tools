#include <kidmon/os/Window.h>
#include <kidmon/geometry/Rect.h>

#include <Windows.h>

class WindowImpl : public Window
{
    HWND hwnd_ {nullptr};
    Rect rect_;
    std::string id_;

public:
    WindowImpl(HWND hwnd) noexcept;

    const std::string& id() const override;
    std::string title() const override;
    std::string className() const override;
    fs::path ownerProcessPath() const override;
    uint64_t ownerProcessId() const noexcept override;
    Rect boundingRect() const noexcept override;

    bool capture(const ImageFormat format, std::vector<char>& content) override;
};
