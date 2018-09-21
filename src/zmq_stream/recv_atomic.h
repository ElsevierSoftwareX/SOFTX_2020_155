#ifndef ZMQ_RECV_ATOMIC_H
#define ZMQ_RECV_ATOMIC_H

// gcc 4.4 does not define std::atomic, so do at least a volatile.
// this is a very minimal interface, only what is needed.
// the goal is to eventually not support gcc 4.4, but until then ...
#ifndef NO_STD_ATOMIC
#include <atomic>

namespace receiver {
    using std::atomic;
}

#else

namespace receiver {

    template <typename T>
    class atomic {
        volatile T data_;
        public:
        atomic() {}
        atomic(const T& other): data_(other) {}

        operator bool() const {
            return (data_ ? true : false);
        }

        bool operator !() const {
            return (data_ ? false: true);
        }

        T load() const {
            return data_;
        }

        atomic<T>& operator |=(const T& other)
        {
            data_ |= other;
            return *this;
        }
    };
}

#endif

#endif // ZMQ_RECV_ATOMIC_H
