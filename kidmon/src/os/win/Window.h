#include <kidmon/os/Window.h>
#include <kidmon/geometry/Rect.h>

#include <Windows.h>

class WindowImpl : public km::Window
{
    HWND hwnd_ {nullptr};
    km::Rect bounds_;
    std::string id_;

public:
    WindowImpl(HWND hwnd) noexcept;

    const std::string& id() const override;
    std::string title() const override;
    fs::path ownerProcessPath() const override;
    uint64_t ownerProcessId() const override;
    km::Rect boundingRect() const noexcept override;

    bool capture(km::ImageFormat format, std::vector<char>& content) override;
};
