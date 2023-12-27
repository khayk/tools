#pragma once

#include "../geometry/Rect.h"
#include "../common/Enums.h"

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

    virtual const std::string& id() const = 0;
    virtual std::string title() const = 0;
    virtual std::string className() const = 0;
    virtual fs::path ownerProcessPath() const = 0;
    virtual uint64_t ownerProcessId() const noexcept = 0;
    virtual Rect boundingRect() const noexcept = 0;

    /**
     * @brief Retrieve the content of the window with a requested format.
     *
     * @param format  The format of the image
     * @param content The content of the given window

     * @return true, if window is captured, otherwise false
     */
    virtual bool capture(const ImageFormat format, std::vector<char>& content) = 0;
};

using WindowPtr = std::unique_ptr<Window>;
