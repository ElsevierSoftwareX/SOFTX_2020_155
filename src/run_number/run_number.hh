//
// Created by jonathan.hanks on 6/12/17.
//

#ifndef DAQD_RUN_NUMBER_HH
#define DAQD_RUN_NUMBER_HH

#include <algorithm>
#include <string>

namespace daqdrn {

    typedef int number;

    struct run_number_state {
        run_number_state(): value(0) {}
        run_number_state(number val, std::string hash): value(val), hash(hash) {}

        number value;
        std::string hash;
    };

    class file_backing_store {
    public:
        file_backing_store(const std::string& path): _path(path) {}

        run_number_state get_state();

        void save_state(const run_number_state& state);
    private:
        std::string _path;
    };

    template<class BackingStore>
    class run_number {
    public:
        run_number(BackingStore& data_store): _data_store(data_store), _state(data_store.get_state()) {}

        number get_number(const std::string& hash) {
            if (hash != _state.hash) {
                run_number_state new_state(_state.value + 1, hash);
                _data_store.save_state(new_state);
                std::swap(new_state, _state);
            }
            return _state.value;
        }

    private:
        run_number_state _state;
        BackingStore& _data_store;
    };

}


#endif //DAQD_RUN_NUMBER_HH
