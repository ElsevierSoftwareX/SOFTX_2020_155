#include "run_number.hh"

#include <fstream>
#include <iostream>
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
        while (fs) {
            run_number_state tmp_state;
            std::string tmp = "";
            fs >> tmp_state.value >> tmp;
            if (fs) {
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

        std::ofstream fs;
        fs.open(_path, std::fstream::out | std::fstream::app);
        fs << state.value << " " << hex_val << "\n";
        fs.close();
    }

}