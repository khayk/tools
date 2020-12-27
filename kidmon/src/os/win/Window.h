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
    std::string ownerProcessPath() const override;
    uint64_t ownerProcessId() const noexcept override;
    Rect boundingRect() const noexcept override;
};