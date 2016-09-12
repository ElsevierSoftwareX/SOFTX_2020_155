#ifndef PARAMETER_SET_HH
#define PARAMETER_SET_HH

#include <map>
#include <sstream>

/**
 * @brief The parameter_set class defines a mapping of key to value.
 *
 * @note When queried the set always returns a value as the user must
 * provide a default value for when the key does not exist.
 */
class parameter_set {
    typedef std::map<std::string, std::string> map_type;

    map_type _map;
public:
    parameter_set() {}
    parameter_set(const parameter_set& other): _map(other._map) {}

    parameter_set &operator=(const parameter_set &other) {
        if (this != &other) {
            _map = other._map;
        }
        return *this;
    }

    void set(const std::string &key, const std::string &value) {
        _map[key] = value;
    }

    template <typename T>
    T get(const std::string& key, const T& default_val) const {
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
