#pragma once

#include <cmath>

template<typename T>
struct Vector2 {
    T x, y;

    Vector2(T x = 0, T y = 0) : x(x), y(y) {}

    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2 operator*(T scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    Vector2 operator/(T scalar) const {
        return Vector2(x / scalar, y / scalar);
    }

    bool operator==(const Vector2& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Vector2& other) const {
        return !(*this == other);
    }

    T length() const {
        return std::sqrt(x * x + y * y);
    }

    T lengthSquared() const {
        return x * x + y * y;
    }

    Vector2 normalized() const {
        T len = length();
        if (len > 0) {
            return Vector2(x / len, y / len);
        }
        return Vector2(0, 0);
    }

    T dot(const Vector2& other) const {
        return x * other.x + y * other.y;
    }

    T cross(const Vector2& other) const {
        return x * other.y - y * other.x;
    }
};

using Vec2f = Vector2<float>;
using Vec2i = Vector2<int>;
