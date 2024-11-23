#pragma once

#include <kidmon/geometry/Rect.h>
#include <kidmon/common/Enums.h>

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class Window
{
public:
    virtual ~Window() = default;

    [[nodiscard]] virtual const std::string& id() const = 0;
    [[nodiscard]] virtual std::string title() const = 0;
    [[nodiscard]] virtual std::string className() const = 0;
    [[nodiscard]] virtual fs::path ownerProcessPath() const = 0;
    [[nodiscard]] virtual uint64_t ownerProcessId() const noexcept = 0;
    [[nodiscard]] virtual Rect boundingRect() const noexcept = 0;

    /**
     * @brief Retrieve the content of the window with a requested format.
     *
     * @param format  The format of the image
     * @param content The content of the given window

     * @return true, if window is captured, otherwise false
     */
    virtual bool capture(ImageFormat format, std::vector<char>& content) = 0;
};

using WindowPtr = std::unique_ptr<Window>;
