#pragma once

class Point
{
    int x_ {0};
    int y_ {0};

public:
    Point() noexcept = default;
    Point(int x, int y) noexcept
        : x_(x)
        , y_(y)
    {
    }

    // clang-format off
    [[nodiscard]] int x() const noexcept { return x_; }
    [[nodiscard]] int y() const noexcept { return y_; }
    void x(int value) noexcept { x_ = value; }
    void y(int value) noexcept { y_ = value; }
    // clang-format on

    bool operator==(const Point&) const = default;
};

inline Point operator+(const Point& lhs, const Point& rhs)
{
    return {lhs.x() + rhs.x(), lhs.y() + rhs.y()};
}

inline Point operator-(const Point& lhs, const Point& rhs)
{
    return {lhs.x() - rhs.x(), lhs.y() - rhs.y()};
}
