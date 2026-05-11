#include <kidmon/os/Window.h>
#include <kidmon/geometry/Rect.h>


class WindowImpl : public km::Window
{
    km::Rect bounds_;
    std::string id_;

public:
    const std::string& id() const override;
    std::string title() const override;
    fs::path ownerProcessPath() const override;
    uint64_t ownerProcessId() const override;
    km::Rect boundingRect() const noexcept override;

    bool capture(km::ImageFormat format, std::vector<char>& content) override;
};
