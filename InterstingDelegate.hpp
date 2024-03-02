#pragma once

#include <vector>
#include <mutex>
#include <memory>
#include <functional>

template <typename ReturnType, typename... Args>
class Callable;

template <typename ReturnType, typename... Args>
class Callable<ReturnType(Args...)>
{
public:
    virtual ReturnType Execute(Args... args) = 0;
    virtual ~Callable() {}
};

template <typename FunctionType>
class FunctionWrapper;

// No void specification for FunctionWrapper<void(Args...)>, because there is Delegate<void(Args...)> in which FunctionWrapper<void(Args...)> has been used
template <typename ReturnType, typename... Args>
class FunctionWrapper<ReturnType(Args...)> : public Callable<ReturnType(Args...)>
{
private:
    std::function<ReturnType(Args...)> function;

public:
    FunctionWrapper(const std::function<ReturnType(Args...)>& func) : function(func) {}

    ReturnType Execute(Args... args) override
    {
        return function(args...);
    }
};

template <typename ReturnType, typename... Args>
class Delegate;

template <typename ReturnType, typename... Args>
class Delegate<ReturnType(Args...)>
{
    using callable_ptr = std::shared_ptr<Callable<ReturnType(Args...)>>;

private:
    std::mutex mtx;
    std::vector<callable_ptr> callable_ptrs;

    Delegate<ReturnType(Args...)>& operator+=(const callable_ptr& f) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        callable_ptrs.push_back(f);
        return *this;
    }

    void operator-=(const callable_ptr& f) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto it = callable_ptrs.rbegin(); it != callable_ptrs.rend(); ++it)
        {
            if (*it == f)
            {
                callable_ptrs.erase((++it).base());
                return;
            }
        }
    }

public:
    Delegate() {}

    Delegate(const std::function<ReturnType(Args...)>& func)
    {
        callable_ptrs.push_back(std::make_shared<FunctionWrapper<ReturnType(Args...)>>(func));
    }

    template <typename FunctionType>
    Delegate(const FunctionType& func)
    {
        callable_ptrs.push_back(std::make_shared<FunctionWrapper<ReturnType(Args...)>>(std::function<ReturnType(Args...)>(func)));
    }

    const std::vector<callable_ptr>& GetCallablePtrs() const { return callable_ptrs; }

    Delegate<ReturnType(Args...)>& operator+=(const Delegate<ReturnType(Args...)>& function)
    {
        for (const auto& f : function.GetCallablePtrs())
            *this += f;
        return *this;
    }

    void operator-=(const Delegate<ReturnType(Args...)>& function)
    {
        for (const auto& f : function.GetCallablePtrs())
            *this -= f;
    }

    std::vector<ReturnType> Execute(Args... args) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<ReturnType> results;
        for (const auto& f : callable_ptrs)
            results.push_back(f->Execute(args...));

        return results;
    }

    std::vector<ReturnType> operator()(Args... args) noexcept
    {
        return Execute(args...);
    }
};

// void specialized template
template <typename... Args>
class Delegate<void(Args...)>
{
    using callable_ptr = std::shared_ptr<Callable<void(Args...)>>;

private:
    std::mutex mtx;
    std::vector<callable_ptr> callable_ptrs;

    Delegate<void(Args...)>& operator+=(const callable_ptr& f) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        callable_ptrs.push_back(f);
        return *this;
    }

    void operator-=(const callable_ptr& f) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto it = callable_ptrs.rbegin(); it != callable_ptrs.rend(); ++it)
        {
            if (*it == f)
            {
                callable_ptrs.erase((++it).base());
                return;
            }
        }
    }

public:
    Delegate() {}

    Delegate(const std::function<void(Args...)>& func)
    {
        callable_ptrs.push_back(std::make_shared<FunctionWrapper<void(Args...)>>(func));
    }

    template <typename FunctionType>
    Delegate(const FunctionType& func)
    {
        callable_ptrs.push_back(std::make_shared<FunctionWrapper<void(Args...)>>(std::function<void(Args...)>(func)));
    }

    const std::vector<callable_ptr>& GetCallablePtrs() const { return callable_ptrs; }

    Delegate<void(Args...)>& operator+=(const Delegate<void(Args...)>& function)
    {
        for (const auto& f : function.GetCallablePtrs())
            *this += f;
        return *this;
    }

    void operator-=(const Delegate<void(Args...)>& function)
    {
        for (const auto& f : function.GetCallablePtrs())
            *this -= f;
    }

    void Execute(Args... args) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& f : callable_ptrs)
            f->Execute(args...);
    }

    void operator()(Args... args) noexcept
    {
        Execute(args...);
    }
};

template <typename... Args>
using Action = Delegate<void(Args...)>;