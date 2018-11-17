#ifndef IX_STREAM_MISC_RECV_UTILS_HH
#define IX_STREAM_MISC_RECV_UTILS_HH

#include <stdint.h>
#include <pthread.h>

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
 * A simple wrapper around a 2d array that can do bounds checking.
 * @tparam T Base type
 * @tparam N size of the 1st dimension
 * @tparam M size of the 2nd dimension
 */
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
