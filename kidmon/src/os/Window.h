#pragma once

#include "../geometry/Rect.h"

#include <cstdint>
#include <string>
#include <memory>

class Window
{
public:
   virtual ~Window() = default;

   virtual const std::string& id() const = 0;
   virtual std::string title() const = 0;
   virtual std::string className() const = 0;
   virtual std::string ownerProcessPath() const = 0;
   virtual uint64_t ownerProcessId() const noexcept = 0;
   virtual Rect boundingRect() const noexcept = 0;
};

using WindowPtr = std::unique_ptr<Window>;
