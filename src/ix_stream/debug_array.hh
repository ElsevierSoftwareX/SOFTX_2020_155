//
// Created by jonathan.hanks on 11/8/18.
//

#ifndef DEBUG_ARRAY_HH
#define DEBUG_ARRAY_HH

#include <stdexcept>

template <typename T, size_t N>
class debug_array {
    debug_array(): data_()
    {
    }

    T& operator[](size_t index) const
    {
        if (index < 0 || index >= N){
            throw std::out_of_range("Bad index");
        }
        return data_[N];
    }
    T* operator*() const
    {
        return data_;
    }
private:
    T data_[N];
};


template <typename T, size_t N, size_t M>
class debug_array_2d {
public:
    class span {
    public:
        friend class debug_array_2d;
        T& at(size_t index)
        {
            if (index < 0 || index >= M)
            {
                throw std::out_of_range("Out of range access");
            }
            return data_[index];
        }
        T& at(size_t index) const
        {
            if (index < 0 || index >= M)
            {
                throw std::out_of_range("Out of range access");
            }
            return data_[index];
        }
        T& operator[](size_t index)
        {
            if (index < 0 || index >= M)
            {
                throw std::out_of_range("Out of range access");
            }
            return data_[index];
        }
        const T& operator[](size_t index) const
        {
            if (index < 0 || index >= M)
            {
                throw std::out_of_range("Out of range access");
            }
            return data_[index];
        }
        T* operator*() const
        {
            return data_;
        }
        operator T*()
        {
            return data_;
        }
        operator T*() const
        {
            return data_;
        }
    private:
        span(T* const data): data_(data) {};
        span(T* data, bool): data_(data) {};

        T *data_;
    };
    debug_array_2d(): data_()
    {
    }

    span at(size_t index)
    {
        if (index < 0 || index >= N){
            throw std::out_of_range("Bad index");
        }
        return span(data_[index]);
    }
    const span at(size_t index) const
    {
        if (index < 0 || index >= N){
            throw std::out_of_range("Bad index");
        }
        return span(data_[index]);
    }
    span operator[](size_t index)
    {
        if (index < 0 || index >= N){
            throw std::out_of_range("Bad index");
        }
        return span(data_[index]);
    }
    const span operator[](size_t index) const
    {
        if (index < 0 || index >= N){
            throw std::out_of_range("Bad index");
        }
        return span(data_[index]);
    }
    T* operator*() const
    {
        return data_;
    }
private:
    T data_[N][M];
};



#endif //DAQD_TRUNK_DEBUG_ARRAY_HH
