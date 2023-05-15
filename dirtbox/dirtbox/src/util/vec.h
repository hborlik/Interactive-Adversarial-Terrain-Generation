/**
 * @file vec.h
 * @author Hunter Borlik
 * @brief 
 * @version 0.3
 * @date 2021-05-04
 * 
 * 
 */
#pragma once
#ifndef HBORLIK_VEC_H
#define HBORLIK_VEC_H

#include <cmath>
#include <iostream>
#include <type_traits>
#include <limits>
#include <algorithm>
#include <cmath>

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
class vec2 {
    T val[2];
public:
    vec2(T in1, T in2) : val{in1, in2} {}
    vec2() : val{0} {} //default constructor
    explicit vec2(T in) : val{in,in} {}
    vec2(const vec2<T>& in) {*this = in;}

    T x() const noexcept { return val[0]; }
    T y() const noexcept { return val[1]; }
    T& x() noexcept { return val[0]; }
    T& y() noexcept { return val[1]; }
    void setX(T inX) noexcept { val[0] = inX; }
    void setY(T inY) noexcept { val[1] = inY; }

    //overload operators
    vec2<T>& operator+=(const vec2 &v) noexcept {
        val[0] += v.val[0];
        val[1] += v.val[1];
        return *this;
    }

    vec2<T>& operator-=(const vec2 &v) noexcept {
        val[0] -= v.val[0];
        val[1] -= v.val[1];
        return *this;
    }

    //mult by scalar
    template<typename S>
    vec2<T>& operator*=(const S Sc) noexcept {
        *this = *this * Sc;
        return *this;
    }

    template<typename S>
    vec2<T>& operator/=(const S Sc) noexcept {
        *this *= 1/Sc;
        return *this;
    }

    vec2<T>& operator=(const vec2<T>& o) noexcept {
        val[0] = o.val[0];
        val[1] = o.val[1];
        return *this;
    }

    T const& operator[](int i) const {
        return val[i];
    }

    T& operator[](int i) {
        return val[i];
    }

    template<typename C>
    operator vec2<C>() const {
        return {
            static_cast<C>(val[0]),
            static_cast<C>(val[1])
        };
    }


    float leng() const noexcept {
        return std::sqrt(val[0]*val[0] + val[1]*val[1]);
    }

    T dot(const vec2<T>& o) const noexcept {
        return val[0]*o.val[0] + val[1]*o.val[1];
    }

    vec2<T> normalized() const {
        return *this / leng();
    }

private:
    friend std::ostream &operator<<(std::ostream &output, const vec2<T> &D) {
        output << to_string(D);
        return output;
    }

    friend std::string to_string(const vec2<T> &D) {
        return std::to_string(D.val[0]) + " " + std::to_string(D.val[1]);
    }

    friend vec2<T> elementMul(const vec2<T>& a, const vec2<T>& b) noexcept {
        return {
            a.val[0] * b.val[0],
            a.val[1] * b.val[1]
        };
    }

    friend vec2<T> pow(const vec2<T>& a, const T& b) noexcept {
        return {
            std::pow(a.val[0], b),
            std::pow(a.val[1], b)
        };
    }

    friend vec2<T> clamp(const vec2<T>& a, const T& min, const T& max) noexcept {
        return {
            std::clamp(a.x(), min, max),
            std::clamp(a.y(), min, max),
            std::clamp(a.z(), min, max)
        };
    }

    friend vec2<T> clamp(const vec2<T>& a, const vec2<T>& min, const vec2<T>& max) noexcept {
        return {
            std::clamp(a.x(), min.x(), max.x()),
            std::clamp(a.y(), min.y(), max.y())
        };
    }

    // binary operators

    //mult by scalar
    template<typename S>
    friend vec2<T> operator*(const S Sc, const vec2<T>& v) noexcept {
        return {(T)(v.val[0] * Sc), (T)(v.val[1] * Sc)};
    }

    template<typename S>
    friend vec2<T> operator*(const vec2<T>& v, const S Sc) noexcept {
        return {(T)(v.val[0] * Sc), (T)(v.val[1] * Sc)};
    }

    friend vec2<T> operator*(const vec2<T>& v, const vec2<T>& b) noexcept {
        return {v.val[0] * b.val[0], v.val[1] * b.val[1]};
    }

    template<typename S>
    friend vec2<T> operator/(const S Sc, const vec2<T>& v) noexcept {
        return {(T)(Sc / v.val[0]), (T)(Sc / v.val[1])};
    }

    template<typename S>
    friend vec2<T> operator/(const vec2<T>& v, const S Sc) noexcept {
        return {(T)(v.val[0] / Sc), (T)(v.val[1] / Sc)};
    }

    friend vec2<T> operator+(const vec2<T>& a, const vec2<T> &b) noexcept {
        return {a.val[0] + b.val[0], a.val[1] + b.val[1]};
    }

    friend vec2<T> operator-(const vec2<T>& a, const vec2<T> &b) noexcept {
        return {a.val[0] - b.val[0], a.val[1] - b.val[1]};
    }

    friend bool operator==(const vec2<T>& a, const vec2<T>& b) noexcept {
        return a.val[0] == b.val[0] && a.val[1] == b.val[1];
    }

    friend bool operator!=(const vec2<T>& a, const vec2<T>& b) noexcept {
        return !(a == b);
    }
};

using point2d   = vec2<double>;
using point2f   = vec2<float>;
using vec2d     = point2d;
using vec2f     = vec2<float>;
using vec2i     = vec2<int>;
using vec2u     = vec2<unsigned int>;
using vec2ul    = vec2<unsigned long>;

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
class vec3 {
    T val[3];
public:
    vec3(T in1, T in2, T in3) noexcept : val{in1, in2, in3} {}
    vec3() noexcept : val{0} {} //default constructor
    explicit vec3(T in) noexcept : val{in,in,in} {}
    vec3(const vec2<T>& xy, T z) noexcept : val{xy.x(), xy.y(), z} {}
    
    vec3(const vec3<T>& in) {*this = in;}

    T const& x() const noexcept { return val[0]; }
    T const& y() const noexcept { return val[1]; }
    T const& z() const noexcept { return val[2]; }
    T& x() noexcept { return val[0]; }
    T& y() noexcept { return val[1]; }
    T& z() noexcept { return val[2]; }
    vec2<T> xy() const noexcept { return {x(), y()}; }
    void setX(T inX) noexcept { val[0] = inX; }
    void setY(T inY) noexcept { val[1] = inY; }
    void setZ(T inZ) noexcept { val[2] = inZ; }

    //unary operators

    vec3<T>& operator+=(const vec3<T>& v) noexcept {
        val[0] += v.val[0];
        val[1] += v.val[1];
        val[2] += v.val[2];
        return *this;
    }

    vec3<T>& operator*=(const vec3<T>& v) noexcept {
        val[0] *= v.val[0];
        val[1] *= v.val[1];
        val[2] *= v.val[2];
        return *this;
    }

    vec3<T>& operator/=(const vec3<T>& c) {
        val[0] /= c[0];
        val[1] /= c[1];
        val[2] /= c[2];
        return *this;
    }

    //mult by scalar
    template<typename S, std::enable_if_t<std::is_arithmetic<S>::value, int> = 0>
    vec3<T>& operator*=(const S Sc) noexcept {
        val[0] *= Sc;
        val[1] *= Sc;
        val[2] *= Sc;
        return *this;
    }

    template<typename S, std::enable_if_t<std::is_arithmetic<S>::value, int> = 0>
    vec3<T>& operator/=(const S Sc) {
        val[0] /= Sc;
        val[1] /= Sc;
        val[2] /= Sc;
        return *this;
    }

    vec3<T> operator-() const noexcept {
        return *this * -1;
    }

    vec3<T>& operator=(const vec3<T>& o) noexcept {
        val[0] = o.val[0];
        val[1] = o.val[1];
        val[2] = o.val[2];
        return *this;
    }

    template<typename C>
    operator vec3<C>() const {
        return {
            static_cast<C>(val[0]),
            static_cast<C>(val[1]),
            static_cast<C>(val[2])
        };
    }

    T const& operator[](int i) const noexcept {
        return val[i];
    }

    T& operator[](int i) noexcept {
        return val[i];
    }

    float leng() const noexcept {
        return std::sqrt(val[0]*val[0] + val[1]*val[1] + val[2]*val[2]);
    }

    float leng2() const noexcept {
        return val[0]*val[0] + val[1]*val[1] + val[2]*val[2];
    }

    T dot(const vec3<T>& o) const noexcept {
        return val[0]*o.val[0] + val[1]*o.val[1] + val[2]*o.val[2];
    }

    vec3<T> cross(const vec3<T>& b) const noexcept {
        return {
            y()*b.z() - b.y()*z(),
            z()*b.x() - b.z()*x(),
            x()*b.y() - b.x()*y()
        };
    }

    vec3<T> normalized() const {
        T invL = 1 / leng();
        return *this * invL;
    }

private:
    friend std::ostream &operator<<(std::ostream &output, const vec3<T> &D) {
        output << to_string(D);
        return output;
    }

    friend std::string to_string(const vec3<T> &D) noexcept {
        return std::to_string(D.val[0]) + " " + std::to_string(D.val[1]) + " " + std::to_string(D.val[2]);
    }

    friend bool isnan(const vec3<T>& v) noexcept {
        if (std::isnan(v.val[0]) || std::isnan(v.val[1] || std::isnan(v.val[2])))
            return true;
        return false;
    }

    friend vec3<T> clamp(const vec3<T>& v, const T& min, const T& max) noexcept {
        return {
            std::clamp(v.x(), min, max),
            std::clamp(v.y(), min, max),
            std::clamp(v.z(), min, max)
        };
    }

    // binary operators

    //mult by scalar
    template<typename S>
    friend vec3<T> operator*(const S Sc, const vec3<T>& v) noexcept {
        return {v.val[0] * Sc, v.val[1] * Sc, v.val[2] * Sc};
    }

    template<typename S>
    friend vec3<T> operator*(const vec3<T>& v, const S Sc) noexcept {
        return {v.val[0] * Sc, v.val[1] * Sc, v.val[2] * Sc};
    }

    friend vec3<T> operator*(const vec3<T>& v, const vec3<T>& b) noexcept {
        return {v.val[0] * b.val[0], v.val[1] * b.val[1], v.val[2] * b.val[2]};
    }

    template<typename S>
    friend vec3<T> operator/(const S Sc, const vec3<T>& v) {
        return {Sc / v.val[0], Sc / v.val[1], Sc / v.val[2]};
    }

    template<typename S>
    friend vec3<T> operator/(const vec3<T>& v, const S Sc) {
        return {v.val[0] / Sc, v.val[1] / Sc, v.val[2] / Sc};
    }

    friend vec3<T> operator+(const vec3<T>& a, const vec3<T> &b) noexcept {
        return {a.val[0] + b.val[0], a.val[1] + b.val[1], a.val[2] + b.val[2]};
    }

    friend vec3<T> operator-(const vec3<T>& a, const vec3<T> &b) noexcept {
        return {a.val[0] - b.val[0], a.val[1] - b.val[1], a.val[2] - b.val[2]};
    }

    friend bool operator==(const vec3<T>& a, const vec3<T>& b) noexcept {
        return a.val[0] == b.val[0] && a.val[1] == b.val[1] && a.val[2] == b.val[2];
    }

    friend bool operator!=(const vec3<T>& a, const vec3<T>& b) noexcept {
        return !(a == b);
    }

    friend vec3<T> pow(const vec3<T>& a, const T& b) {
        return {
            std::pow(a.val[0], b),
            std::pow(a.val[1], b),
            std::pow(a.val[2], b)
        };
    }
};


template<typename T>
inline constexpr vec3<T> orthonormal(const vec3<T>& n) {
    vec3<T> b[3]{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    vec3<T> ret;
    for (int i = 0; i < 3; ++i) {
        auto vec = b[i].cross(n);
        if (vec.leng() > std::numeric_limits<T>::epsilon()) {
            return vec.normalized();
        }
    }
    return ret;
}

/**
 * @brief reflect w about n
 * 
 * @tparam T 
 * @param w vector to reflect (going away from surface)
 * @param n normalized !
 * @return constexpr vec3<T> 
 */
template<typename T>
inline constexpr vec3<T> reflect(const vec3<T> w, const vec3<T>& n) {
    return -w - 2 * w.dot(n) * n;
}

/**
 * @brief make a vector, usually a normal face the same hemisphere as another
 * 
 * @tparam T 
 * @param w vector to face
 * @param n normal
 * @return constexpr vec3<T> 
 */
template<typename T>
inline constexpr vec3<T> faceVec(const vec3<T>& w, const vec3<T>& n) {
    return n.dot(w) > 0 ? n : -n;
}

//Type aliases for vec3
template<typename T>
using point3 = vec3<T>;
using point3d = vec3<double>; //3D point
using point3f = vec3<float>; //3D point
using Color = vec3<unsigned char>; //RGB color
using vec3d = point3d;
using vec3f = point3f;
using vec3u     = vec3<unsigned int>;
using vec3ul    = vec3<unsigned long>;

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
class vec4 {
    T val[4];
public:
    vec4() : val{0} {} //default constructor
    explicit vec4(T in) : vec4{in,in,in,in} {}

    vec4(T in1, T in2, T in3, T in4) : val{in1, in2, in3, in4} {}
    vec4(vec2<T> xy, T z, T w) : val{xy.x(), xy.y(), z, w} {}
    vec4(vec3<T> xyz, T w) : val{xyz.x(), xyz.y(), xyz.z(), w} {}
    
    vec4(const vec4<T>& in) {*this = in;}

    T x() const noexcept { return val[0]; }
    T y() const noexcept { return val[1]; }
    T z() const noexcept { return val[2]; }
    T w() const noexcept { return val[3]; }
    T& x() noexcept { return val[0]; }
    T& y() noexcept { return val[1]; }
    T& z() noexcept { return val[2]; }
    T& w() noexcept { return val[3]; }
    void setX(T inX) noexcept { val[0] = inX; }
    void setY(T inY) noexcept { val[1] = inY; }
    void setZ(T inZ) noexcept { val[2] = inZ; }
    void setW(T inW) noexcept { val[3] = inW; }

    vec3<T> xyz() const noexcept {
        return {val[0], val[1], val[2]};
    }

    //overload operators
    vec4<T>& operator+=(const vec4<T> &v) noexcept {
        val[0] += v.val[0];
        val[1] += v.val[1];
        val[2] += v.val[2];
        val[3] += v.val[3];
        return *this;
    }

    //mult by scalar
    template<typename S, std::enable_if_t<std::is_arithmetic<S>::value, int> = 0>
    vec4<T>& operator*=(const S Sc) noexcept {
        *this = *this * Sc;
        return *this;
    }

    template<typename S, std::enable_if_t<std::is_arithmetic<S>::value, int> = 0>
    vec4<T>& operator/=(const S Sc) {
        *this *= (1/Sc);
        return *this;
    }

    vec4<T> operator+(const vec4<T> &v) const noexcept {
        return {val[0] + v.val[0], val[1] + v.val[1], val[2] + v.val[2], val[3] + v.val[3]};
    }

    vec4<T> operator-(const vec4<T> &v) const noexcept {
        return *this + (-1.0 * v);
    }

    vec4<T> operator-() const noexcept {
        return *this * -1;
    }

    //mult by scalar
    template<typename S, std::enable_if_t<std::is_arithmetic<S>::value, int> = 0>
    vec4<T> operator*(const S Sc) const noexcept {
        return {val[0] * Sc, val[1] * Sc, val[2] * Sc, val[3] * Sc};
    }

    template<typename S, std::enable_if_t<std::is_arithmetic<S>::value, int> = 0>
    vec4<T> operator/(const S Sc) const {
        return *this * (1/Sc);
    }

    vec4<T>& operator=(const vec4<T>& o) noexcept {
        val[0] = o.val[0];
        val[1] = o.val[1];
        val[2] = o.val[2];
        val[3] = o.val[3];
        return *this;
    }

    bool operator==(const vec4<T>& o) noexcept {
        return val[0] == o.val[0] && val[1] == o.val[1] && val[2] == o.val[2] && val[3] == o.val[3];
    }

    T const& operator[](int i) const noexcept {
        return val[i];
    }

    T& operator[](int i) noexcept {
        return val[i];
    }

    template<typename C>
    operator vec4<C>() const {
        return {
            static_cast<C>(val[0]),
            static_cast<C>(val[1]),
            static_cast<C>(val[2]),
            static_cast<C>(val[3])
        };
    }

    T dot(const vec4<T>& o) const noexcept {
        return val[0]*o.val[0] + val[1]*o.val[1] + val[2]*o.val[2] + val[3]*o.val[3];
    }

private:        
    friend std::ostream &operator<<(std::ostream &output, const vec4<T> &D) {
        output << std::to_string(D.val[0]) << " " << std::to_string(D.val[1]) << " " << std::to_string(D.val[2]) << " " << std::to_string(D.val[3]);
        return output;
    }

    //mult by scalar
    template<typename S, std::enable_if_t<std::is_arithmetic<S>::value, int> = 0>
    friend vec4<T> operator*(const S Sc, const vec4<T>& v) noexcept {
        return v * Sc;
    }

    //mult by scalar
    friend vec4<T> elementMul(const vec4<T>& a, const vec4<T>& b) noexcept {
        return {
            a.val[0] * b.val[0],
            a.val[1] * b.val[1],
            a.val[2] * b.val[2],
            a.val[3] * b.val[3],
        };
    }

    friend vec4<T> pow(const vec4<T>& a, const T& b) {
        return {
            std::pow(a.val[0], b),
            std::pow(a.val[1], b),
            std::pow(a.val[2], b),
            std::pow(a.val[3], b)
        };
    }
};

using vec4d = vec4<double>;
using vec4f = vec4<float>;

#endif // HBORLIK_VEC_H
