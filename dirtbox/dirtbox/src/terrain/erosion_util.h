/**
 
 * @author Hunter Borlik
 * @brief 
 * @version 0.1
 * @date 2021-03-11
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_EROSION_UTIL

#include <vector>
#include <algorithm>
#include <memory>

#include <util/vec.h>

namespace dirtbox::terrain {

// utility functions

/**
 * @brief slope from p1 to p2
 * 
 * @param e1 elevation at p1
 * @param p1 p1
 * @param e2 elevations at p2
 * @param p2 p2
 * @return float slope
 */
inline float slope(float e1, vec2<float> p1, float e2, vec2<float> p2) {
    float m = (p1 - p1).leng();
    return (e1 - e2) / m;
}

inline float slope(float e1, float e2, float m) {
    return (e1 - e2) / m;
}

/**
 * @brief if x is less than min, 0 is returned. 1 if greater than max. logistic like between
 * 
 * @param x 
 * @param min 
 * @param max 
 * @return float 
 */
inline float logistic_like(float x, float min, float max) {
    float n = 0.f;
    if (x < min)
        ;
    else if(x > max)
        n = 1.f;
    else {
        n = 0.5f * (1 + std::sin(M_PI * ((x - min) / (min - max) - 0.5f)));
    }
    return n;
}

inline float sigmoid(float x) {
    return 1.0f / (1.0f + exp(-x));
}

inline float sigmoid_approx(float x) {
    return 0.5f * (x / (1.f + abs(x)) + 1.f);
}

/**
 * @brief linearly interpolates between [-1, 1] with x [min, max]
 * 
 * @param x 
 * @param min 
 * @param max 
 * @return float 
 */
inline float linear_min_max(float x, float min, float max) {
    return 2.0f * ((x - min) / (max - min)) - 1.0f;
}

/**
 * @brief logistic like curve between [min, max] ranging [0, 1]
 * 
 * @param x 
 * @param min 
 * @param max 
 * @return float [0, 1]
 */
inline float logistic_between(float x, float min, float max, float mul = 1.0f) {
    return sigmoid_approx(mul * linear_min_max(x, min, max));
}

/**
 * @brief random pick index of weights with proportional probability
 * 
 * @param weights 
 * @return int 
 */
inline int random_weighted_pick(const std::vector<float>& weights) {
    float total = .0f;
    for (auto w : weights)
        total += w;
    float r = total * ((float)random() / RAND_MAX);
    for (int i = 0; i < weights.size(); ++i) {
        if (r < weights[i])
            return i;
        r -= weights[i];
    }
    return -1;
}


// requires T to define float Sz for elevation
template<typename T>
struct ModelSubstate {
    std::vector<T> data;
    int width;
    int height;
    float elevationMax;

    struct Neighborhood {
        Neighborhood(vec2i xy) : 
            xy{xy} {}

        vec2i xy;
        // [N, NE, E, SE, S, SW, W, NW]
        std::array<T*, 8> adjacent;
        static std::array<vec2i, 8> directions;

        T*& north()      {return adjacent[0];}
        T*& north_east() {return adjacent[1];}
        T*& east()       {return adjacent[2];}
        T*& south_east() {return adjacent[3];}
        T*& south()      {return adjacent[4];}
        T*& south_west() {return adjacent[5];}
        T*& west()       {return adjacent[6];}
        T*& north_west() {return adjacent[7];}

        uint8_t getNumValid() const {
            uint8_t c = 0;
            for (const auto& n : adjacent)
                if (n)
                    c++;
            return c;
        }
    };


    ModelSubstate(int W, int H, float elevationMax) :
        width{W},
        height{H},
        elevationMax{elevationMax} {
        data.resize(width*height);
    }

    T& at(int x, int y) {
        return data[y * width + x];
    }

    const T& at(int x, int y) const {
        return data[y * width + x];
    }

    T* safeGet(int x, int y) {
        if (x >= 0 && x < width && y >= 0 && y < height)
            return &at(x, y);
        return nullptr;
    }

    T* safeGet(vec2i xy) {
        return safeGet(xy.x(), xy.y());
    }

    const T* safeGet(int x, int y) const {
        if (x >= 0 && x < width && y >= 0 && y < height)
            return &at(x, y);
        return nullptr;
    }

    Neighborhood neighborhoodOf(int x, int y) {
        Neighborhood n{vec2i{x, y}};
        n.north() =         safeGet(x, y - 1);
        n.north_east() =    safeGet(x + 1, y - 1);
        n.east() =          safeGet(x + 1, y);
        n.south_east() =    safeGet(x + 1, y + 1);
        n.south() =         safeGet(x, y + 1);
        n.south_west() =    safeGet(x - 1, y + 1);
        n.west() =          safeGet(x - 1, y);
        n.north_west() =    safeGet(x - 1, y - 1);
        return n;
    }

    void copyElevation(std::shared_ptr<Terrain> terr) {
        for(int j = 0; j < height; ++j)
            for(int i = 0; i < width; ++i) {
                float elevation = elevationMax * terr->at(i, j) / (float)std::numeric_limits<uint16_t>::max();
                at(i, j).Sz = elevation;
            }
    }

    void copyElevationTo(std::shared_ptr<Terrain> terr) {
        for(int j = 0; j < height; ++j)
            for(int i = 0; i < width; ++i) {
                auto& t = at(i, j);
                // t.Sd - t.SE
                terr->at(i, j) = std::clamp(t.Sz / elevationMax * (float)std::numeric_limits<uint16_t>::max(), 0.0f, (float)std::numeric_limits<uint16_t>::max());
            }
    }
};

template<typename T>
std::array<vec2i, 8> ModelSubstate<T>::Neighborhood::directions {
            vec2i{0, -1},
            {1, -1},
            {1, 0},
            {1, 1},
            {0, 1},
            {-1, 1},
            {-1, 0},
            {-1, -1}
        };


}

#endif // DIRTBOX_EROSION_UTIL