#pragma once

#include <vector>
#include <mutex>

template <typename ReturnType, typename... Args>
class Callable;

template <typename ReturnType, typename... Args>
class Callable<ReturnType(Args...)>
{
public:
    virtual ReturnType Execute(Args... args) = 0;
    virtual ~Callable() {}
};

template <typename ClassName, typename ReturnType, typename... Args>
class MemberFunction;

template <typename ClassName, typename ReturnType, typename... Args>
class MemberFunction<ClassName, ReturnType(Args...)> : public Callable<ReturnType(Args...)>
{
    using func_ptr = ReturnType (ClassName::*)(Args...);

private:
    ClassName *instance_p;
    func_ptr function_p;

public:
    MemberFunction(ClassName *_insp, func_ptr _fp) : instance_p(_insp), function_p(_fp) {}
    ReturnType Execute(Args... args)
    {
        return (instance_p->*function_p)(args...);
    }
};

template <typename ReturnType, typename... Args>
class StaticFunction;

template <typename ReturnType, typename... Args>
class StaticFunction<ReturnType(Args...)> : public Callable<ReturnType(Args...)>
{
    using func_ptr = ReturnType (*)(Args...);

private:
    func_ptr function_p;

public:
    StaticFunction(func_ptr _fp) : function_p(_fp) {}
    ReturnType Execute(Args... args)
    {
        return function_p(args...);
    }
};

template <typename ReturnType, typename... Args>
class Delegate;

template <typename ReturnType, typename... Args>
class Delegate<ReturnType(Args...)>
{
    using callable_ptr = Callable<ReturnType(Args...)> *;

private:
    std::mutex mtx;

    std::vector<callable_ptr> callable_ptrs;

    Delegate<ReturnType(Args...)> &operator+=(callable_ptr f) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        callable_ptrs.push_back(f);
        return *this;
    }

    void operator-=(callable_ptr f) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (typename ::std::vector<callable_ptr>::reverse_iterator it = callable_ptrs.rbegin(); it < callable_ptrs.rend(); ++it)
        {
            if (*it == f)
                callable_ptrs.erase((++it).base());
            return;
        }
    }

    void ClearCallablePtrs() { callable_ptrs.clear(); }

public:
    Delegate() {}

    Delegate(ReturnType (*_fp)(Args...))
    {
        callable_ptrs.push_back(new StaticFunction<ReturnType(Args...)>(_fp));
    }

    template <typename ClassName>
    Delegate(ClassName *_instance_p, ReturnType (ClassName::*_fp)(Args...))
    {
        callable_ptrs.push_back(new MemberFunction<ClassName, ReturnType(Args...)>(_instance_p, _fp));
    }

    std::vector<callable_ptr> &GetCallablePtrs() { return callable_ptrs; }

    // It might NOT work if i just simply copy the callable_ptrs
    Delegate(const Delegate &_other) = delete;

    Delegate(const Delegate &&_other) : callable_ptrs(_other.GetCallablePtrs())
    {
        _other.ClearCallablePtrs();
    }

    ~Delegate()
    {
        for (callable_ptr f : callable_ptrs)
            delete f;
        callable_ptrs.clear();
    }

    Delegate<ReturnType(Args...)> &operator+=(Delegate<ReturnType(Args...)> &function)
    {
        for (callable_ptr f : function.GetCallablePtrs())
            *this += f;
        return *this;
    }

    void operator-=(Delegate<ReturnType(Args...)> &function)
    {
        for (typename ::std::vector<callable_ptr>::iterator it = function.GetCallablePtrs().begin(); it < function.GetCallablePtrs().end(); ++it)
        {
            *this -= *it;
        }
    }

    ReturnType Execute(Args... args) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        ReturnType res;
        for (callable_ptr f : callable_ptrs)
            res = f->Execute(args...);

        return res;
    }

    ReturnType operator()(Args... args) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        ReturnType res;
        for (callable_ptr f : callable_ptrs)
            res = f->Execute(args...);

        return res;
    }
};

// void specialized template
template <typename... Args>
class Delegate<void(Args...)>
{
    using callable_ptr = Callable<void(Args...)> *;

private:
    std::mutex mtx;

    std::vector<callable_ptr> callable_ptrs;

    Delegate<void(Args...)> &operator+=(callable_ptr f) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        callable_ptrs.push_back(f);
        return *this;
    }

    void operator-=(callable_ptr f) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (typename ::std::vector<callable_ptr>::reverse_iterator it = callable_ptrs.rbegin(); it < callable_ptrs.rend(); ++it)
        {
            if (*it == f)
                callable_ptrs.erase((++it).base());
            return;
        }
    }

public:
    Delegate() {}

    Delegate(void (*_fp)(Args...))
    {
        callable_ptrs.push_back(new StaticFunction<void(Args...)>(_fp));
    }

    template <typename ClassName>
    Delegate(ClassName *_instance_p, void (ClassName::*_fp)(Args...))
    {
        callable_ptrs.push_back(new MemberFunction<ClassName, void(Args...)>(_instance_p, _fp));
    }

    std::vector<callable_ptr> &GetCallablePtrs() { return callable_ptrs; }

    void ClearCallablePtrs() { callable_ptrs.clear(); }

    // It might NOT work if i just simply copy the callable_ptrs
    Delegate(const Delegate &_other) = delete;

    Delegate(const Delegate &&_other) : callable_ptrs(_other.GetCallablePtrs())
    {
        _other.ClearCallablePtrs();
    }

    ~Delegate()
    {
        for (callable_ptr f : callable_ptrs)
            delete f;
        callable_ptrs.clear();
    }

    Delegate<void(Args...)> &operator+=(Delegate<void(Args...)> &function)
    {
        for (callable_ptr f : function.GetCallablePtrs())
            *this += f;
        return *this;
    }

    void operator-=(Delegate<void(Args...)> &function)
    {
        for (typename ::std::vector<callable_ptr>::iterator it = function.GetCallablePtrs().begin(); it < function.GetCallablePtrs().end(); ++it)
        {
            *this -= *it;
        }
    }

    void Execute(Args... args) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (callable_ptr f : callable_ptrs)
            f->Execute(args...);
    }

    void operator()(Args... args) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (callable_ptr f : callable_ptrs)
            f->Execute(args...);
    }
};

template <typename... Args>
using Action = Delegate<void(Args...)>;