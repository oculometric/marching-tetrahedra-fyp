#pragma once

#include <math.h>
#include <cmath>

namespace MTVT
{

struct Vector3
{
    float x, y, z;

    inline constexpr Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    inline constexpr Vector3() : x(0), y(0), z(0) {}

    inline void operator=(const Vector3& a) { x = a.x; y = a.y; z = a.z; }
    inline void operator+=(const Vector3& a) { x += a.x; y += a.y; z += a.z; }
    inline void operator-=(const Vector3& a) { x -= a.x; y -= a.y; z -= a.z; }
    inline void operator*=(const Vector3& a) { x *= a.x; y *= a.y; z *= a.z; }
    inline void operator/=(const Vector3& a) { x /= a.x; y /= a.y; z /= a.z; }
    inline void operator*=(const float f) { x *= f; y *= f; z *= f; }
    inline void operator/=(const float f) { x /= f; y /= f; z /= f; }
    inline Vector3 operator -() { return Vector3{ -x, -y, -z }; }

    inline static constexpr Vector3 right() { return Vector3{ 1, 0, 0 }; }
    inline static constexpr Vector3 up() { return Vector3{ 0, 1, 0 }; }
    inline static constexpr Vector3 forward() { return Vector3{ 0, 0, 1 }; }
};

inline Vector3 operator+(const Vector3& a, const Vector3& b) { return Vector3{ a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vector3 operator-(const Vector3& a, const Vector3& b) { return Vector3{ a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vector3 operator*(const Vector3& a, const Vector3& b) { return Vector3{ a.x * b.x, a.y * b.y, a.z * b.z }; }
inline Vector3 operator/(const Vector3& a, const Vector3& b) { return Vector3{ a.x / b.x, a.y / b.y, a.z / b.z }; }
inline Vector3 operator*(const Vector3& a, const float f) { return Vector3{ a.x * f, a.y * f, a.z * f }; }
inline Vector3 operator/(const Vector3& a, const float f) { return Vector3{ a.x / f, a.y / f, a.z / f }; }
inline bool operator==(const Vector3& a, const Vector3& b) { return (a.x == b.x) && (a.y == b.y) && (a.z == b.z); }

inline float operator^(const Vector3& a, const Vector3& b) { return (a.x * b.x) + (a.y * b.y) + (a.z * b.z); }
inline Vector3 operator%(const Vector3& a, const Vector3& b) { return Vector3{ (a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x) }; }

inline float sq_mag(const Vector3& a) { return a ^ a; }
inline float mag(const Vector3& a) { return sqrt(sq_mag(a)); }
inline Vector3 norm(const Vector3& a) { return a / mag(a); }
inline Vector3 abs(const Vector3& a) { return Vector3{ fabs(a.x), fabs(a.y), fabs(a.z) }; }
inline Vector3 min(const Vector3& a, const Vector3& b) { return Vector3{ std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) }; }
inline Vector3 max(const Vector3& a, const Vector3& b) { return Vector3{ std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) }; }
inline Vector3 lerp(const Vector3& a, const Vector3& b, const float f) { return a + ((b - a) * f); }
inline Vector3 floor(const Vector3& a) { return Vector3{ ::floor(a.x), ::floor(a.y), ::floor(a.z)}; }
inline Vector3 ceil(const Vector3& a) { return Vector3{ ::ceil(a.x), ::ceil(a.y), ::ceil(a.z) }; }
inline Vector3 fract(const Vector3& a) { return Vector3{ a.x - ::floor(a.x), a.y - ::floor(a.y), a.z - ::floor(a.z) }; }
inline float lerp(const float a, const float b, const float x) { return a + ((b - a) * x); }
inline float angle(const Vector3& a, const Vector3& b) { return acos((a ^ b) / (mag(a) * mag(b))); }

}
