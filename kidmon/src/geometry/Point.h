#pragma once

class Point
{
    int x_{ 0 };
    int y_{ 0 };

public:
    Point() noexcept = default;
    Point(int x, int y) noexcept : x_(x), y_(y) {}

    int x() const noexcept { return x_; }
    int y() const noexcept { return y_; }
    void x(int value) noexcept { x_ = value; }
    void y(int value) noexcept { y_ = value; }
};

inline Point operator+(const Point& lhs, const Point& rhs)
{
    return Point(lhs.x() + rhs.x(), lhs.y() + rhs.y());
}

inline Point operator-(const Point& lhs, const Point& rhs)
{
    return Point(lhs.x() - rhs.x(), lhs.y() - rhs.y());
}
