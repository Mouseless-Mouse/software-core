#pragma once

/// @brief A callback interface for any invocable object taking no arguments
class BaseCallback {
public:
    /// @brief A pure virtual method representing invocation of the callback
    virtual void invoke() = 0;
};

/// @brief An implementation of the `BaseCallback` interface
/// @tparam F The underlying type of the invocable object. The current compiler C++ version
/// does not implement CTAD, so this must be specified explicitly or deduced by a factory function or method.
template <typename F>
class Callback : public BaseCallback {
    F callbackFunc;

public:
    Callback(F&& callbackFunc)
        : BaseCallback()
        , callbackFunc{std::forward<F>(callbackFunc)}
    {}
    void invoke() {
        callbackFunc();
    }
};