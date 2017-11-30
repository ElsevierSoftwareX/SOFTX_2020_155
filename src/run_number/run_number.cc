#include "run_number.hh"

#include <time.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace {
    char hex_char_to_nibble(char ch) {
        if (ch >= '0' && ch <= '9') {
            return ch - '0';
        }
        if (ch >= 'a' && ch <= 'f') {
            return ch - 'a';
        }
        if (ch >= 'A' && ch <= 'F') {
            return ch - 'A';
        }
        return 0;
    }
    std::string hex_to_bin_str(const std::string& input) {
        if (input.size() > 0) {
            std::string::size_type  max = input.size()/2;
            std::vector<char> tmp(max);
            for (std::string::size_type i = 0; i < max; ++i) {
                char val = (hex_char_to_nibble(input[i*2]) << 4) |
                        (hex_char_to_nibble(input[i*2 + 1]) & 0xf);
                tmp[i] = val;
            }
            return std::string(tmp.data(), max);
        }
        return "";
    }
}

namespace daqdrn {

    run_number_state file_backing_store::get_state() {
        run_number_state results;
        std::ifstream fs;
        fs.open(_path, std::fstream::in);
        std::string buffer;
        while (std::getline(fs, buffer)) {
            // The input line is:
            // number hash [optional_timestamp]
            // we only need the number and the hash
            std::istringstream is(buffer);
            run_number_state tmp_state;
            std::string tmp = "";
            is >> tmp_state.value >> tmp;
            if (is) {
                tmp_state.hash = hex_to_bin_str(tmp);
                std::swap(results, tmp_state);
            }
        }
        return results;
    }

    void file_backing_store::save_state(const run_number_state& state)  {
        static char lookup[17]="0123456789abcdef";
        std::vector<char> hex_tmp(state.hash.size()*2);
        std::vector<char>::iterator cur = hex_tmp.begin();
        for (int i = 0; i < state.hash.size(); ++i) {
            *cur++ = lookup[(state.hash[i] >> 4) & 0xf];
            *cur++ = lookup[state.hash[i] & 0xf];
        }
        std::string hex_val(hex_tmp.data(), hex_tmp.size());

        time_t cur_time_t = time(0);
        struct tm cur_time_tm;
        gmtime_r(&cur_time_t, &cur_time_tm);

        // fallback is to write the seconds since UNIX EPOCH
        std::string time_stamp;
        {
            std::ostringstream os;
            os << cur_time_t;
            time_stamp = os.str();
        }

        // But really we want to write the time in a human readable way
        std::vector<char> time_buf(200);
        size_t count = strftime(&time_buf[0], time_buf.size(), "%F %T %Z", &cur_time_tm);
        if (count != 0) {
            time_stamp = &time_buf[0];
        }

        std::ofstream fs;
        fs.open(_path, std::fstream::out | std::fstream::app);
        fs << state.value << " " << hex_val << " " << time_stamp << "\n";
        fs.close();
    }

}