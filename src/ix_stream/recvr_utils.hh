#ifndef IX_STREAM_MISC_RECV_UTILS_HH
#define IX_STREAM_MISC_RECV_UTILS_HH

#include <stdint.h>
#include <pthread.h>

#include <algorithm>
#include <stdexcept>

class spin_lock {
public:
    spin_lock(): l_()
    {
        pthread_spin_init(&l_, PTHREAD_PROCESS_PRIVATE);
    }
    ~spin_lock()
    {
        pthread_spin_destroy(&l_);
    }
    void lock()
    {
        pthread_spin_lock(&l_);
    }
    void unlock()
    {
        pthread_spin_unlock(&l_);
    }
private:
    pthread_spinlock_t l_;
};

/***
 * A simple lock guard that locks/unlocks anything with
 * lock/unlock member functions.
 *
 * @note Creation of the object is lock acquisition
 * Destruction of the object is lock release
 */
template <typename T>
class lock_guard
{
public:
    explicit lock_guard(T& locker): locker_(locker)
    {
        locker_.lock();
    }
    ~lock_guard()
    {
        locker_.unlock();
    }
private:
    lock_guard(const lock_guard& other);
    lock_guard& operator=(const lock_guard& other);

    T& locker_;
};

/***
 * A simple lock guard specialized for pthread spinlocks.
 * Use this to ensure locks are always released.
 *
 * @note Creation of the object is lock acquisition
 * Destruction of the object is lock release
 */
template<>
class lock_guard<pthread_spinlock_t>
{
public:
    explicit lock_guard(pthread_spinlock_t& sp): sp_(sp)
    {
        pthread_spin_lock(&sp_);
    }
    ~lock_guard()
    {
        pthread_spin_unlock(&sp_);
    }
private:
    lock_guard(const lock_guard& other);
    lock_guard& operator=(const lock_guard& other);

    pthread_spinlock_t& sp_;
};


/**
 * A strong typedef, provided to give stronger/compiler differentiation
 * between multiple distinct types that share the same underlying basic type
 * \tparam T The basic type of the object
 * \tparam TAG A tag value to distinguish different strong types, not otherwise used
 */
template <typename T, size_t TAG=0>
class strong_type {
public:
    typedef T value_type;

    explicit strong_type(T val): data_(val) {}

    T get() const
    {
        return data_;
    }
private:
    T data_;
};

template <typename T>
typename T::value_type get_value(T& has_get)
{
    return has_get.get();
}

size_t get_value(size_t val)
{
    return val;
}

/**
 * A simple wrapper around a 2d array that can do bounds checking.
 * @tparam T Base type
 * @tparam N size of the 1st dimension
 * @tparam M size of the 2nd dimension
 */
template <typename T, size_t N, size_t M, typename Nindex_t = size_t, typename Mindex_t = size_t>
class debug_array_2d {
public:
    class span {
    public:
        friend class debug_array_2d;
        T& at(Mindex_t index)
        {
            size_t index_ = static_cast<size_t>(get_value(index));
            if (index_ < 0 || index_ >= M)
            {
                throw std::out_of_range("Out of range access");
            }
            return data_[index_];
        }
        T& at(Mindex_t index) const
        {
            size_t index_ = static_cast<size_t>(get_value(index));
            if (index_ < 0 || index_ >= M)
            {
                throw std::out_of_range("Out of range access");
            }
            return data_[index_];
        }
        T& operator[](Mindex_t index)
        {
            size_t index_ = static_cast<size_t>(get_value(index));
            //if (index_ < 0 || index_ >= M)
            //{
            //    throw std::out_of_range("Out of range access");
            //}
            return data_[index_];
        }
        const T& operator[](Mindex_t index) const
        {
            size_t index_ = static_cast<size_t>(get_value(index));
            //if (index_ < 0 || index_ >= M)
            //{
            //    throw std::out_of_range("Out of range access");
            //}
            return data_[index_];
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

    span at(Nindex_t index)
    {
        size_t index_ = static_cast<size_t>(get_value(index));
        if (index_ < 0 || index_ >= N){
            throw std::out_of_range("Bad index");
        }
        return span(data_[index_]);
    }
    const span at(Nindex_t index) const
    {
        size_t index_ = static_cast<size_t>(get_value(index));
        if (index_ < 0 || index_ >= N){
            throw std::out_of_range("Bad index");
        }
        return span(data_[index_]);
    }
    span operator[](Nindex_t index)
    {
        size_t index_ = static_cast<size_t>(get_value(index));
        //if (index_ < 0 || index_ >= N){
        //    throw std::out_of_range("Bad index");
        //}
        return span(data_[index_]);
    }
    const span operator[](Nindex_t index) const
    {
        size_t index_ = static_cast<size_t>(get_value(index));
        //if (index_ < 0 || index_ >= N){
        //    throw std::out_of_range("Bad index");
        //}
        return span(data_[index_]);
    }
    T* operator*() const
    {
        return data_;
    }
private:
    T data_[N][M];
};

/***
 * A simple atomic flag that should be implemented lock free.
 * This uses compiler builtins that clang & gcc recognize.
 * This is suitable for signaling between threads.
 */
class atomic_flag {
public:
    atomic_flag(): val_(0) {}

    void set()
    {
        __sync_or_and_fetch(&val_, 1);
    }
    void clear()
    {
        __sync_and_and_fetch(&val_, 0);
    }
    bool is_set()
    {
        return __sync_fetch_and_add(&val_, 0) != 0;
    }
private:
    atomic_flag(const atomic_flag& other);
    atomic_flag& operator=(const atomic_flag& other);

    int val_;
};

/**
 * A simple array of flags, with a fixed length known at compile time.
 * @tparam MAX_LEN The number of flags to use.
 */
template <unsigned int MAX_LEN>
class flag_mask
{
public:
    typedef unsigned int index_type;

    flag_mask(): data_()
    {
        clear();
    }
    flag_mask(const flag_mask& other): data_(other.data_) {}
    flag_mask& operator=(const flag_mask& other)
    {
        std::copy(other.cbegin(), other.cend(), begin());
        return *this;
    }

    /// \brief clear all flags
    void clear()
    {
        std::fill(begin(), end(), false);
    }
    /// \brief set the given flag, bounds are checked
    void set(int index)
    {
        if (static_cast<index_type>(index) < MAX_LEN)
        {
            data_[index] = true;
        }
    }
    /// \brief retreive the requested flag, bounds are checked
    bool get(int index) const
    {
        return (static_cast<index_type>(index) < MAX_LEN ? data_[index] : false);
    }
    /// \brief return true if the flags match between *this and other
    bool operator==(const flag_mask& other) const
    {
        for (index_type i = 0; i < MAX_LEN; ++i)
        {
            if (data_[i] != other.data_[i])
            {
                return false;
            }
        }
        return true;
    }
    /// \brief return false if the flags match between *this and other
    bool operator!=(const flag_mask& other) const
    {
        return !(*this == other);
    }

    /// \brief compare two masks, and call a callback function for each difference
    template<typename F>
    friend void foreach_difference(const flag_mask& mask1, const flag_mask& mask2, F& f)
    {
        for (index_type i = 0; i < MAX_LEN; ++i)
        {
            if (mask1.data_[i] != mask2.data_[i])
            {
                f(mask1.data_[i], mask2.data_[i], i);
            }
        }
    }
private:
    const bool *cbegin() const
    {
        return data_;
    }
    const bool *cend() const
    {
        return data_ + MAX_LEN;
    }
    bool *begin()
    {
        return data_;
    }
    bool *end()
    {
        return data_ + MAX_LEN;
    }
    bool data_[MAX_LEN];
};

/**
 * A structure to record when a FE or DCU has sent data.
 * Designed for use in scoreboards that track the current
 * progress of data arrival/transfer.
 */
typedef struct receive_map_entry_t {
    int64_t gps_sec_and_cycle;
    int64_t s_clock;

    receive_map_entry_t(): gps_sec_and_cycle(0), s_clock(0) {}
} receive_map_entry_t;

/// \brief given a gps second + a cycle count merge them into one value
inline int64_t calc_gps_sec_and_cycle(int64_t gps_sec, int64_t cycle)
{
    return (gps_sec << 4) | (0x0f & cycle);
}

// helper to extract seconds counts out of a combined gps sec + cycle value
inline int64_t extract_gps(int64_t gps_sec_and_cycle)
{
    return gps_sec_and_cycle >> 4;
}

// helper to extract cycle counts out of a combined gps sec + cycle value
inline int64_t extract_cycle(int64_t gps_sec_and_cycle)
{
    return gps_sec_and_cycle & 0x0f;
}

#endif /* IX_STREAM_MISC_RECV_UTILS_HH */
