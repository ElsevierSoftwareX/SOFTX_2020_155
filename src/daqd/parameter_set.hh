#ifndef PARAMETER_SET_HH
#define PARAMETER_SET_HH

#include <map>
#include <sstream>
#include <pthread.h>
#include "raii.hh"

/**
 * @brief The parameter_set class defines a mapping of key to value.
 *
 * @note When queried the set always returns a value as the user must
 * provide a default value for when the key does not exist.
 */
class parameter_set {
    typedef std::map<std::string, std::string> map_type;

    map_type _map;
    mutable pthread_mutex_t _mut;

public:
    parameter_set() { pthread_mutex_init(&_mut, NULL); }
    parameter_set(const parameter_set& other) {
        raii::lock_guard<pthread_mutex_t> _other_lock(other._mut);
        _map = other._map;
    }
    ~parameter_set() {
        pthread_mutex_destroy(&_mut);
    }

    parameter_set &operator=(const parameter_set &other) {
        raii::lock_guard<pthread_mutex_t> _lock(_mut);
        if (this != &other) {
            raii::lock_guard<pthread_mutex_t> _lock_other(other._mut);
            _map = other._map;
        }
        return *this;
    }

    void set(const std::string &key, const std::string &value) {
        raii::lock_guard<pthread_mutex_t> _lock(_mut);
        _map[key] = value;
    }

    template <typename T>
    T get(const std::string& key, const T& default_val) const {
        raii::lock_guard<pthread_mutex_t> _lock(_mut);
        map_type::const_iterator it = _map.find(key);
        if (it != _map.end()) {
            std::istringstream stream( it->second );
            T value;
            stream >> value;
            return value;
        }
        return default_val;
    }

    std::string get(const char *key, const char *default_val) const {
        return get<std::string>(std::string(key), std::string(default_val));
    }

    /*template <typename T>
    T get(const std::string& key) {
        T default;
        return get<T>(key, default);
    }*/

};

#endif // PARAMETER_SET_HH
