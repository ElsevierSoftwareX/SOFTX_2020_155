#include <iostream>
#include <string>

#include "run_number_client.hh"

void usage(const char *program) {
    std::cerr << "Usage:\n\t" << program << " [options] [connection_string [ hash_value]]" << std::endl;
    std::cerr << "\t-v\t\tVerbose mode" << std::endl;
    std::cerr << "\t-h|--help\t\tThis help" << std::endl;
    std::cerr << "\tconnection_string\ttcp://localhost:5556" << std::endl;
    std::cerr << "\thash_value\t\t11111111111111111111" << std::endl;
}

int main(int argc, char *argv[]) {
    std::string conn_str("tcp://localhost:5556");
    std::string hash("11111111111111111111");

    bool verbose = false;
    bool accept_conn = true;
    bool accept_hash = true;
    bool need_help = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-h" || arg == "--help") {
            need_help = true;
        } else if (arg == "-v" || arg == "--v") {
            verbose = true;
        } else if (accept_conn) {
            conn_str = arg;
            accept_conn = false;
        } else if (accept_hash) {
            hash = arg;
            accept_hash = false;
        } else {
            need_help = true;
        }

        if (need_help) {
            usage(argv[0]);
            return 0;
        }
    }

    if (verbose) {
        std::cout << "Sending " << hash << " to " << conn_str << std::endl;
    }
    std::cout << daqd_run_number::get_run_number(conn_str, hash);
    return 0;
}