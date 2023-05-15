/**
 
 * @author Hunter Borlik
 * @brief 
 * @version 0.1
 * @date 2021-03-06
 * 
 * 
 * 
 */
#pragma once
#ifndef DIRTBOX_PARAMETER_H
#define DIRTBOX_PARAMETER_H

#include <string>
#include <map>
#include <vector>
#include <algorithm>

namespace util {

template<typename T>
struct Parameter {
    std::string name;
    T value;
    T min;
    T max;
};

template<typename T>
using ParameterSet = std::map<std::string, Parameter<T>>;

template<typename T>
using ParameterList = std::vector<Parameter<T>>;

template<typename T, typename = std::enable_if<std::is_arithmetic<T>::value>>
class ParameterCollection {
public:
    ParameterList<T> getParams() const {
        ParameterList<T> out;
        std::transform(parameters.begin(),
            parameters.end(),
            back_inserter(out),
            [](const auto& val){return val.second;});
        return out;
    }

    bool setParam(const std::string& key, const T& value) {
        auto p = parameters.find(key);
        if (p != parameters.end()) {
            if ((value < p->second.max && value > p->second.min) || p->second.min == p->second.max) {
                p->second.value = value;
                return true;
            }
        }
        return false;
    }

    T getParam(const std::string& name) const {
        auto p = parameters.find(name);
        if (p != parameters.end())
            return p->second.value;
        return {};
    }

    void addParameter(const std::string& key, const T& min, const T& max, const T& value) {
        parameters.insert(std::make_pair(key, Parameter<T>{key, value, min, max}));
    }

private:
    ParameterSet<T> parameters;
};

}

#endif // 