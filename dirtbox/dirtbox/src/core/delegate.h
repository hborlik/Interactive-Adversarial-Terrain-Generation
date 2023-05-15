/**
 
 * @author Hunter Borlik
 * @brief 
 * @version 0.4
 * @date 2018-10-3
 * 
 * 
 * 2020-8-16: Comments and function docs
 * 2021-3-15: fix for create for const functions
 * 2021-5-14: Added Event class
 * 
 * Example:
 * 
 * class Sample {
 * public:
 *     double InstanceFunction(int, char, const char*) { return 0.1; }
 *     double ConstInstanceFunction(int, char, const char*) const { return 0.2; }
 *     static double StaticFunction(int, char, const char*) { return 0.3; }
 * }; //class Sample
 * 
 * delegate<double(int, char, const char*)> d;
 * 
 * auto dInstance = decltype(d)::create<Sample, &Sample::InstanceFunction>(&sample);
 * auto dConst = decltype(d)::create<Sample, &Sample::ConstInstanceFunction>(&sample);
 * auto dFunc = decltype(d)::create<&Sample::StaticFunction>();
 */

/**
 * @brief from https://www.codeproject.com/Articles/1170503/The-Impossibly-Fast-Cplusplus-Delegates-Fixed 
 * 
 * With added lambda function support
 */
#pragma once
#ifndef delegate_H_
#define delegate_H_

#include <list>
#include <memory>
#include <vector>

namespace util {

template<typename T>
class delegate;

/**
 * @brief This creates a pointer to a templated static function that calls a function 
 *  or member function, based on input arguments.
 * 
 * @tparam Ret 
 * @tparam Params 
 */
template<typename Ret, typename ...Params>
class delegate<Ret(Params...)> {
public:
    using stub_type = Ret(*)(void* this_ptr, Params&&...);

    delegate() = default;

    /**
     * @brief Construct a new delegate object
     * 
     * @param other 
     */
    delegate(const delegate& other) {
        this->object = other.object;
        this->method_ptr = other.method_ptr;
    }

    /**
     * @brief Check if the function pointer is valid.
     * 
     * @return true 
     * @return false 
     */
    inline bool isNull() const noexcept {
        return method_ptr == nullptr;
    }

    inline bool operator==(const delegate& other) const noexcept {
        return other.method_ptr == method_ptr && other.object == object;
    }
    inline bool operator!=(const delegate& other) const noexcept {
        return other.method_ptr != method_ptr || other.object != object;
    }

    /**
     * @brief Allow checking for validity by casting to a bool
     * 
     * @return true 
     * @return false 
     */
    inline explicit operator bool() const noexcept {
        return !isNull();
    }

    /**
     * @brief Allow assignment to nullptr
     * 
     * @param fn 
     * @return delegate<Ret(Params...)>& 
     */
    inline delegate<Ret(Params...)>& operator=(decltype(nullptr) fn) noexcept {
        method_ptr = nullptr;
        return *this;
    }

    /**
     * @brief Calls delegate
     * 
     * @param params 
     * @return Ret 
     */
    Ret operator()(Params... params) const {
        return (*method_ptr)(object, static_cast<Params&&>(params)...);
    }

    /**
     * @brief Create an delegate from a member function with type Ret(T::*TMethod)(Params...)
     * 
     * @tparam T 
     * @tparam Ret(T::*TMethod)(Params...) 
     * @param object 
     * @return delegate 
     */
    template<class T, Ret(T::*TMethod)(Params...)>
    static delegate create(T* object) {
        return delegate(object, method_stub<T, TMethod>);
    }

    /**
     * @brief Create an delegate from a const qualified member function with type Ret(T::*TMethod)(Params...) const
     * 
     * @tparam T 
     * @tparam const 
     * @param object 
     * @return delegate 
     */
    template<class T, Ret(T::*TMethod)(Params...) const>
    static delegate create(T* object) {
         return delegate(object, method_stub_const<T, TMethod>);
    }

    /**
     * @brief Create an delegate from a non-member function with type Ret (*TMethod)(Params...)
     * 
     * @tparam (*TMethod)(Params...) 
     * @return delegate 
     */
    template<Ret (*TMethod)(Params...)>
    static delegate create() {
        return delegate(nullptr, function_stub<TMethod>);
    }

    /**
     * @brief Allow creation from unnamed class types
     * 
     * @tparam Lambda 		lambda type
     * @param object 
     * @return delegate 
     */
    template<typename Lambda>
    static delegate create(const std::shared_ptr<Lambda> object) {
        return delegate((void*)&object, lambda_stub<Lambda>);
    }

private:

    delegate(std::shared_ptr<void> this_ptr, stub_type stub) noexcept :
            object{this_ptr}, method_ptr{stub} {
    }

    /* In the templated static funtions below, the variadic r value references automatically
     * expand to & or && depending on the types of arguments passed to the function */

    /* function call to member function */
    template<class T, Ret (T::*TMethod)(Params...)>
    static Ret method_stub(void* this_ptr, Params&&... params) {
        T* obj = static_cast<T*>(this_ptr);
        return (obj->*TMethod)(static_cast<Params&&>(params)...);
    }

    /* function call to const member function */
    template<class T, Ret (T::*TMethod)(Params...) const>
    static Ret method_stub_const(void* this_ptr, Params&&... params) {
        T* const obj = static_cast<T*>(this_ptr);
        return (obj->*TMethod)(static_cast<Params&&>(params)...);
    }

    /* function call to static function */
    template<Ret (*TMethod)(Params...)>
    static Ret function_stub(void* this_ptr, Params&&... params) {
        return (TMethod)(static_cast<Params&&>(params)...);
    }

    /* lambda wrapper */
    template<typename Lambda>
    static Ret lambda_stub(void* this_ptr, Params&&... params) {
        //Lambda* object = static_cast<Lambda*>(this_ptr);
        return (((Lambda*)this_ptr)->operator())(static_cast<Params&&>(params)...);
    }

    std::shared_ptr<void> object = nullptr;
    stub_type method_ptr = nullptr;
};

template<typename T>
class Event;

template<typename Ret, typename ...Params>
class Event<Ret(Params...)> {
public:
    using delegate_type = delegate<Ret(Params...)>;

    void operator+=(const delegate_type& o) {
        subscribers.push_back(o);
    }
    void operator-=(const delegate_type& o) {
        subscribers.remove(o); 
    }

    // non-void return
    template<typename T = Ret> // allow SFINE
    typename std::enable_if<!std::is_same<T, void>::value, std::vector<Ret>>::type operator()(Params&&... params) {
        std::vector<Ret> returns;
        for (auto& d : subscribers) {
            returns.emplace_back(d(params...)); // do not forward args
        }
        return returns;
    }

    // void return
    template<typename T = Ret> // allow SFINE
    typename std::enable_if<std::is_same<T, void>::value, void>::type operator()(Params&&... params) {
        for (auto& d : subscribers) {
            d(params...);
        }
    }

private:
    std::list<delegate_type> subscribers;
};

}
#endif