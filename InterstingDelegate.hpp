#pragma once

#include <vector>
#include <mutex>
#include <memory>
#include <functional>

template <typename FunctionType>
class FunctionWrapper;

template <typename ReturnType, typename... Args>
class FunctionWrapper<ReturnType(Args...)>
{
    using std::function<ReturnType(Args...)>::function;
private:
    std::function<ReturnType(Args...)> func;
public:
    FunctionWrapper(const std::function<ReturnType(Args...)>& f) : func(f) {}
    FunctionWrapper(const FunctionWrapper<ReturnType(Args...)>& f) : func(f.func) {}
    FunctionWrapper(const FunctionWrapper<ReturnType(Args...)>&& f) : func(std::move(f.func)) {}

    ReturnType operator()(Args... args) const noexcept
    {
        return func(args...);
    }
};

template <typename ReturnType, typename... Args>
class Delegate;

template <typename ReturnType, typename... Args>
class Delegate<ReturnType(Args...)>
{
    using function_ptr = std::shared_ptr<FunctionWrapper<ReturnType(Args...)>>;

private:
    std::mutex mtx;
    std::vector<function_ptr> function_ptrs;

    Delegate<ReturnType(Args...)>& operator+=(const function_ptr& f) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        function_ptrs.push_back(f);
        return *this;
    }

    void operator-=(const function_ptr& f) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto it = function_ptrs.rbegin(); it != function_ptrs.rend(); ++it)
        {
            if (*it == f)
            {
                function_ptrs.erase((++it).base());
                return;
            }
        }
    }

public:
    Delegate() {}

    Delegate(const std::function<ReturnType(Args...)>& func)
    {
        function_ptrs.push_back(std::make_shared<FunctionWrapper<ReturnType(Args...)>>(func));
    }

    template <typename FunctionType>
    Delegate(const FunctionType& func)
    {
        function_ptrs.push_back(std::make_shared<FunctionWrapper<ReturnType(Args...)>>(std::function<ReturnType(Args...)>(func)));
    }

    const std::vector<function_ptr>& GetFunctionPtrs() const { return function_ptrs; }

    Delegate<ReturnType(Args...)>& operator+=(const Delegate<ReturnType(Args...)>& function)
    {
        for (const auto& f : function.GetFunctionPtrs())
            *this += f;
        return *this;
    }

    void operator-=(const Delegate<ReturnType(Args...)>& function)
    {
        for (const auto& f : function.GetFunctionPtrs())
            *this -= f;
    }

    std::vector<ReturnType> Execute(Args... args) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<ReturnType> results;
        for (const auto& f : function_ptrs)
            results.push_back((*f)(args...));

        return results;
    }

    std::vector<ReturnType> operator()(Args... args) noexcept
    {
        return Execute(args...);
    }
};
