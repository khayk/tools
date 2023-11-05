#include "../Window.h"
#include "../../geometry/Rect.h"

class WindowImpl : public Window
{
    Rect rect_;
    std::string id_;

public:
    const std::string& id() const override;
    std::string title() const override;
    std::string className() const override;
    fs::path ownerProcessPath() const override;
    uint64_t ownerProcessId() const noexcept override;
    Rect boundingRect() const noexcept override;

    bool capture(const ImageFormat format, std::vector<char>& content) override;
};
