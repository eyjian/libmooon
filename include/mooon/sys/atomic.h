#ifndef __ATOMIC_H__
#define __ATOMIC_H__
#include <mooon/sys/config.h>
#include <stdint.h>

#if __GNUC__ < 4
#include "mooon/sys/atomic_asm.h"
#else
#include "mooon/sys/atomic_gcc.h"
#if __WORDSIZE==64
#include "mooon/sys/atomic_gcc8.h"
#endif
#endif
#include "mooon/sys/compiler.h"

#if __cplusplus >= 201103L
#include <atomic>
#endif // __cplusplus >= 201103L

SYS_NAMESPACE_BEGIN
template <typename T>
class CAtomic
{
};

template <>
class CAtomic<bool>
{
public:
    CAtomic<bool>()
    {
#if __cplusplus < 201103L
        atomic_set(&_value, 0);
#else
        _value = false;
#endif // __cplusplus < 201103L
    }

    CAtomic<bool>(bool value)
    {
#if __cplusplus < 201103L
        if (value)
            atomic_set(&_value, 1);
        else
            atomic_set(&_value, 0);
#else
        _value = value;
#endif
    }

    CAtomic<bool>(const CAtomic<bool>& other)
    {
#if __cplusplus < 201103L
        const int v = other.get_value();
        atomic_set(&_value, v);
#else
        _value = other.operator bool();
#endif
    }

    CAtomic<bool>& operator =(const CAtomic<bool>& other)
    {
#if __cplusplus < 201103L
        const int v = other.get_value();
        atomic_set(&_value, v);
#else
        if (1 == other.get_value())
            _value = true;
        else
            _value = false;
#endif
        return *this;
    }

    CAtomic<bool>& operator =(bool value)
    {
#if __cplusplus < 201103L
        if (value)
            atomic_set(&_value, 1);
        else
            atomic_set(&_value, 0);
#else
        _value = value;
#endif
        return *this;
    }

    int get_value() const
    {
#if __cplusplus < 201103L
        return atomic_read(&_value);
#else
        if (_value)
            return 1;
        else
            return 0;
#endif
    }

    operator bool() const
    {
#if __cplusplus < 201103L
        const int v = atomic_read(&_value);
        return v != 0;
#else
        return _value.operator bool();
#endif
    }

private:
#if __cplusplus < 201103L
    atomic_t _value;
#else
    std::atomic<bool> _value;
#endif // __cplusplus < 201103L
};

template <>
class CAtomic<int>
{
public:
    CAtomic<int>()
    {
#if __cplusplus < 201103L
        atomic_set(&_value, 0);
#else
        _value = 0;
#endif
    }

    CAtomic<int>(int value)
    {
#if __cplusplus < 201103L
        atomic_set(&_value, value);
#else
        _value = value;
#endif
    }

    CAtomic<int>(const CAtomic<int>& other)
    {
#if __cplusplus < 201103L
        const int v = other.get_value();
        atomic_set(&_value, v);
#else
        _value = other.get_value();
#endif
    }

    CAtomic<int>& operator =(const CAtomic<int>& other)
    {
#if __cplusplus < 201103L
        const int v = other.get_value();
        atomic_set(&_value, v);
#else
        _value = other.get_value();
#endif
        return *this;
    }

    CAtomic<int>& operator =(int value)
    {
#if __cplusplus < 201103L
        atomic_set(&_value, value);
#else
        _value = value;
#endif
        return *this;
    }

    int get_value() const
    {
#if __cplusplus < 201103L
        return atomic_read(&_value);
#else
        return _value.operator int();
#endif
    }

    operator int() const
    {
        return get_value();
    }

    operator bool() const
    {
#if __cplusplus < 201103L
        const int v = atomic_read(&_value);
        return v != 0;
#else
        return _value != 0;
#endif
    }

    CAtomic<int>& operator ++() // 前缀
    {
#if __cplusplus < 201103L
        atomic_add_return(1, &_value);
#else
        ++_value;
#endif
        return *this;
    }

    CAtomic<int>& operator ++(int) // 后缀
    {
#if __cplusplus < 201103L
        atomic_add(1, &_value);
#else
        _value++;
#endif
        return *this;
    }

    CAtomic<int>& operator --() // 前缀
    {
#if __cplusplus < 201103L
        atomic_sub_return(1, &_value);
#else
        --_value;
#endif
        return *this;
    }

    CAtomic<int>& operator --(int) // 后缀
    {
#if __cplusplus < 201103L
        atomic_sub(1, &_value);
#else
        _value--;
#endif
        return *this;
    }

    CAtomic<int>& operator +=(int value)
    {
#if __cplusplus < 201103L
        atomic_add_return(value, &_value);
#else
        _value += value;
#endif
        return *this;
    }

    CAtomic<int>& operator -=(int value)
    {
#if __cplusplus < 201103L
        atomic_sub_return(value, &_value);
#else
        _value -= value;
#endif
        return *this;
    }

private:
#if __cplusplus < 201103L
    atomic_t _value;
#else
    std::atomic<int> _value;
#endif // __cplusplus < 201103L
};

#if __WORDSIZE==64
template <>
class CAtomic<int64_t>
{
public:
    CAtomic<int64_t>()
    {
#if __cplusplus < 201103L
        atomic8_set(&_value, 0);
#else
        _value = 0;
#endif
    }

    CAtomic<int64_t>(int64_t value)
    {
#if __cplusplus < 201103L
        atomic8_set(&_value, value);
#else
        _value = value;
#endif
    }

    CAtomic<int64_t>(const CAtomic<int64_t>& other)
    {
#if __cplusplus < 201103L
        const int64_t v = other.get_value();
        atomic8_set(&_value, v);
#else
        _value = other.get_value();
#endif
    }

    CAtomic<int64_t>& operator =(const CAtomic<int64_t>& other)
    {
#if __cplusplus < 201103L
        const int64_t v = other.get_value();
        atomic8_set(&_value, v);
#else
        _value = other.get_value();
#endif
        return *this;
    }

    CAtomic<int64_t>& operator =(int64_t value)
    {
#if __cplusplus < 201103L
        atomic8_set(&_value, value);
#else
        _value = value;
#endif
        return *this;
    }

    int64_t get_value() const
    {
#if __cplusplus < 201103L
        return atomic8_read(&_value);
#else
        return _value.operator int64_t();
#endif
    }

    operator int64_t() const
    {
        return get_value();
    }

    operator bool() const
    {
#if __cplusplus < 201103L
        const int64_t v = atomic8_read(&_value);
        return v != 0;
#else
        return _value != 0;
#endif
    }

    CAtomic<int64_t>& operator ++() // 前缀
    {
#if __cplusplus < 201103L
        atomic8_add_return(1, &_value);
#else
        ++_value;
#endif
        return *this;
    }

    CAtomic<int64_t>& operator ++(int) // 后缀
    {
#if __cplusplus < 201103L
        atomic8_add(1, &_value);
#else
        _value++;
#endif
        return *this;
    }

    CAtomic<int64_t>& operator --() // 前缀
    {
#if __cplusplus < 201103L
        atomic8_sub_return(1, &_value);
#else
        --_value;
#endif
        return *this;
    }

    CAtomic<int64_t>& operator --(int) // 后缀
    {
#if __cplusplus < 201103L
        atomic8_sub(1, &_value);
#else
        _value--;
#endif
        return *this;
    }

    CAtomic<int64_t>& operator +=(int64_t value)
    {
#if __cplusplus < 201103L
        atomic8_add_return(value, &_value);
#else
        _value += value;
#endif
        return *this;
    }

    CAtomic<int64_t>& operator -=(int64_t value)
    {
#if __cplusplus < 201103L
        atomic8_sub_return(value, &_value);
#else
        _value -= value;
#endif
        return *this;
    }

private:
#if __cplusplus < 201103L
    atomic8_t _value;
#else
    std::atomic<int64_t> _value;
#endif // __cplusplus < 201103L
};
#endif // __WORDSIZE==64

SYS_NAMESPACE_END
#endif /* __ATOMIC_H__ */
