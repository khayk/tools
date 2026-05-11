#include <kidmon/os/Window.h>
#include <kidmon/geometry/Rect.h>

#include <cstdint>
#include <sys/types.h>

class WindowImpl : public km::Window
{
    uint32_t windowId_; // same underlying type as CGWindowID
    pid_t pid_;
    std::string id_;
    std::string title_;
    std::string ownerName_;
    km::Rect bounds_;

public:
    WindowImpl(uint32_t windowId,
               pid_t pid,
               std::string title,
               std::string ownerName,
               km::Rect bounds) noexcept;

    const std::string& id() const override;
    std::string title() const override;
    fs::path ownerProcessPath() const override;
    uint64_t ownerProcessId() const override;
    km::Rect boundingRect() const noexcept override;

    bool capture(km::ImageFormat format, std::vector<char>& content) override;
};
