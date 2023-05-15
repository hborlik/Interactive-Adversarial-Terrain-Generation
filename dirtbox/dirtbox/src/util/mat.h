/**
 
 * @brief 
 * @version 0.1
 * @date 2020-10-08
 * 
 * updated 3-8-2021:
 *      changes for use with bgfx
 * 
 */
#pragma once
#ifndef UTIL_MAT_H
#define UTIL_MAT_H

#include <cmath>
#include <iostream>
#include <type_traits>
#include <limits>

#include <util/vec.h>

/**
 * @brief matrix, column vectors
 * 
 * @tparam T 
 */
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
struct mat3 {
    vec3<T> val[3]; // row then col

    mat3(T in0_0, T in0_1, T in0_2,
         T in1_0, T in1_1, T in1_2,
         T in2_0, T in2_1, T in2_2) : 
         val{
            {in0_0, in0_1, in0_2},
            {in1_0, in1_1, in1_2},
            {in2_0, in2_1, in2_2}} {}
    mat3(const vec3<T> &row0, const vec3<T> &row1, const vec3<T> &row2) : 
        val{row0,
            row1,
            row2} {}
    mat3() : val{0} {} //default constructor
    explicit mat3(T in) : 
        mat3{
            {in, 0, 0},
            {0, in, 0},
            {0, 0, in}} {}

    mat3(const mat3<T> &in) { *this = in; }

    T* data() {
        return static_cast<T*>(val);
    }

    const T* data() const {
        return static_cast<T*>(val);
    }

    vec3<T> row(uint8_t row) const {
        return val[row];
    }

    vec3<T> col(uint8_t col) const {
        return {
            val[0][col],
            val[1][col],
            val[2][col]
        };
    }

    /**
     * @brief get row i
     * 
     * @param i 
     * @return vec3<T> const& 
     */
    vec3<T> const& operator[](int i) const {
        return val[i];
    }

    /**
     * @brief get row i
     * 
     * @param i 
     * @return vec3<T>& 
     */
    vec3<T>& operator[](int i) {
        return val[i];
    }

    mat3<T> operator*(const double Sc) const {
        return {col(0) * Sc, col(1) * Sc, col(2) * Sc};
    }

    mat3<T> operator*(const mat3<T>& o) const {
        return {
            row(0).dot(o.col(0)), row(0).dot(o.col(1)), row(0).dot(o.col(2)),
            row(1).dot(o.col(0)), row(1).dot(o.col(1)), row(1).dot(o.col(2)),
            row(2).dot(o.col(0)), row(2).dot(o.col(1)), row(2).dot(o.col(2))
        };
    }

    vec3<T> operator*(const vec3<T>& o) const {
        return {
            row(0).dot(o),
            row(1).dot(o),
            row(2).dot(o)
        };
    }

    mat3<T> transpose() const {
        return {
            col(0),
            col(1),
            col(2)
        };
    }

    T det() const {
        return  val[0][0]   * (val[1][1] * val[2][2] - val[2][1] * val[1][2])
                -val[0][1]  * (val[1][0] * val[2][2] - val[1][2] * val[2][0])
                +val[0][2]  * (val[1][0] * val[2][1] - val[1][1] * val[2][0]);
    }


private:
    friend std::ostream &operator<<(std::ostream &output, const mat3<T> &D) {
        for(uint8_t i = 0; i < 3; i++) {
            output << D.row(i) << "\n";
        }
        return output;
    }

    friend mat3<T> operator-(const mat3<T>& A, const mat3<T>& B) {
        return {
            A[0] - B[0],
            A[1] - B[1],
            A[2] - A[2]
        };
    }
};

using mat3d = mat3<double>;
using mat3f = mat3<float>;

template<typename T>
mat3<T> Inverse(const mat3<T>& m) {
    auto om = m;
    mat3<T> output{1};
    for(int i = 0; i < 3; ++i) {

        auto row = om[i];
        auto inv = 1.0 / row[i];
        row *= inv;

        // update augmented mat
        output[i] *= inv;
        om[i] = row;

        // eliminate other values
        for(int j = 0; j < 2; ++j) {
            int p = (i + j + 1) % 3;
            // value we want to set to 0
            auto scalar = -om[p][i];

            // row multiplied by scalar for addition operation
            auto lrow = om[i] * scalar;
            auto orow = output[i] * scalar;

            output[p] += orow;
            om[p] += lrow;
        }
    }
    return output;
}


/**
 * @brief matrix, column major
 * 
 * @tparam T 
 */
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
struct mat4 {
    vec4<T> val[4]; // row, col

    mat4(T in0_0, T in0_1, T in0_2, T in0_3,
         T in1_0, T in1_1, T in1_2, T in1_3,
         T in2_0, T in2_1, T in2_2, T in2_3,
         T in3_0, T in3_1, T in3_2, T in3_3) : 
         val{
            {in0_0, in0_1, in0_2, in0_3},
            {in1_0, in1_1, in1_2, in1_3},
            {in2_0, in2_1, in2_2, in2_3},
            {in3_0, in3_1, in3_2, in3_3}} {}
    mat4(const vec4<T> &row1, const vec4<T> &row2, const vec4<T> &row3, const vec4<T> &row4) : 
        val{
            row1,
            row2,
            row3,
            row4} {}
    mat4() : val{{},{},{},{}} {} //default constructor
    explicit mat4(T in) :
        mat4{
            {in, 0, 0, 0},
            {0, in, 0, 0},
            {0, 0, in, 0},
            {0, 0, 0, in}} {}
    explicit mat4(const mat3<T>& ul) :
        mat4 {
            {ul[0], 0},
            {ul[1], 0},
            {ul[2], 0},
            {0, 0, 0, 1}
        } {}

    mat4(const mat4<T> &in) { *this = in; }

    T* data() {
        return static_cast<T*>(val);
    }

    const T* data() const {
        return static_cast<T*>(val);
    }

    vec4<T> row(uint8_t row) const {
        return val[row];
    }

    vec4<T> col(uint8_t col) const {
        return {
            val[0][col],
            val[1][col],
            val[2][col],
            val[3][col]
        };
    }

    void setCol(uint8_t col, const vec4<T>& v) {
        val[0][col] = v[0];
        val[1][col] = v[1];
        val[2][col] = v[2];
        val[3][col] = v[3];
    }

    /**
     * @brief get row i
     * 
     * @param i 
     * @return vec4<T> const& 
     */
    vec4<T> const& operator[](int i) const {
        return val[i];
    }

    /**
     * @brief get row i
     * 
     * @param i 
     * @return vec4<T>& 
     */
    vec4<T>& operator[](int i) {
        return val[i];
    }

    void operator*=(const mat4<T>& o) {
        val[0] = {row(0).dot(o.col(0)), row(0).dot(o.col(1)), row(0).dot(o.col(2)), row(0).dot(o.col(3))};
        val[1] = {row(1).dot(o.col(0)), row(1).dot(o.col(1)), row(1).dot(o.col(2)), row(1).dot(o.col(3))};
        val[2] = {row(2).dot(o.col(0)), row(2).dot(o.col(1)), row(2).dot(o.col(2)), row(2).dot(o.col(3))};
        val[3] = {row(3).dot(o.col(0)), row(3).dot(o.col(1)), row(3).dot(o.col(2)), row(3).dot(o.col(3))};
    }

    mat4<T> operator*(const double Sc) const {
        return {row(0) * Sc, row(1) * Sc, row(2) * Sc, row(3) * Sc};
    }

    mat4<T> operator*(const mat4<T>& o) const {
        return {
            row(0).dot(o.col(0)), row(0).dot(o.col(1)), row(0).dot(o.col(2)), row(0).dot(o.col(3)),
            row(1).dot(o.col(0)), row(1).dot(o.col(1)), row(1).dot(o.col(2)), row(1).dot(o.col(3)),
            row(2).dot(o.col(0)), row(2).dot(o.col(1)), row(2).dot(o.col(2)), row(2).dot(o.col(3)),
            row(3).dot(o.col(0)), row(3).dot(o.col(1)), row(3).dot(o.col(2)), row(3).dot(o.col(3))
        }; 
    }

    vec4<T> operator*(const vec4<T>& o) const {
        return {
            row(0).dot(o), 
            row(1).dot(o), 
            row(2).dot(o), 
            row(3).dot(o)
        };
    }

    mat4<T> transpose() const {
        return {
            col(0),
            col(1),
            col(2),
            col(3)
        };
    }

private:
    friend std::ostream &operator<<(std::ostream &output, const mat4<T> &D) {
        for(uint8_t i = 0; i < 4; i++) {
            output << D.row(i) << "\n";
        }
        return output;
    }
};

using mat4d = mat4<double>;
using mat4f = mat4<float>;

/**
 * @brief transform matrix inverse
 * 
 * @tparam T 
 * @param m 
 * @return mat4<T> 
 */
template<typename T>
mat4<T> Inverse(const mat4<T>& m) {
    mat3<T> ul{
        m[0].xyz(),
        m[1].xyz(),
        m[2].xyz()
    };
    ul = Inverse(ul);
    vec4<T> pos{-(ul * m.col(3).xyz()), 1};
    mat4<T> ret{ul};
    ret.setCol(3, pos);
    return ret;
}

/**
 * @brief scale matrix along each axis by s
 * 
 * @tparam T 
 * @param s 
 * @return mat4<T> 
 */
template<typename T>
mat4<T> Scale(const vec3<T>& s) {
    return mat4<T>{
            {s.x(), 0, 0, 0},
            {0, s.y(), 0, 0},
            {0, 0, s.z(), 0},
            {0, 0, 0, 1}
        };
}

template<typename T>
mat4<T> Translate(const vec3<T>& s) {
    return mat4<T>{
            {1, 0, 0, s.x()},
            {0, 1, 0, s.y()},
            {0, 0, 1, s.z()},
            {0, 0, 0, 1}
        };
}


template<typename T>
mat4<T> Rotate(const vec3<T>& axis, T angle) {
    auto cosTheta = std::cos(angle);
    auto sinTheta = std::sin(angle);
    auto d = axis.normalized();
    auto d2 = d*d;
    auto ust = d.x() * sinTheta;
    auto vst = d.y() * sinTheta;
    auto wst = d.z() * sinTheta;
    auto uvomc = d.x()*d.y()*(1-cosTheta);
    auto uwomc = d.x()*d.z()*(1-cosTheta);
    auto vwomc = d.y()*d.z()*(1-cosTheta);
    return mat4<T>{
            {d2.x() + (d2.y() + d2.z()) * cosTheta, uvomc - wst, uwomc + vst, 0},
            {uvomc + wst, d2.y() + (d2.x() + d2.z()) * cosTheta, vwomc - ust, 0},
            {uwomc - vst, vwomc + ust, d2.z() + (d2.x() + d2.y()) * cosTheta, 0},
            {0, 0, 0, 1}
        };
}

#endif // UTIL_MAT_H