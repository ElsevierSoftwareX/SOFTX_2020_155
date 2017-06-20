#include "catch.hpp"

#include <sstream>
#include <string>
#include <stdexcept>

#include <stdlib.h>
#include <unistd.h>

#include "run_number.hh"

class BasicBackingStore {
public:
    daqdrn::run_number_state get_state() {
        daqdrn::run_number_state state;
        return state;
    }

    void save_state(const daqdrn::run_number_state& state) { }
};

class FailtoSaveBackingStore {
public:
    FailtoSaveBackingStore(daqdrn::run_number_state initial_state): _state(initial_state) {}

    daqdrn::run_number_state get_state() {
        daqdrn::run_number_state state(_state);
        return state;
    }

    void save_state(const daqdrn::run_number_state& state) { throw std::runtime_error("Error"); }
private:
    daqdrn::run_number_state _state;
};

class RMDirOnExit {
private:
    std::string _dir;
public:
    RMDirOnExit(const char *dir): _dir(dir) {}
    ~RMDirOnExit() {
        if (_dir.size() > 0) rmdir(_dir.c_str());
    }
};

class RMFileOnExit {
private:
    std::string _path;
public:
    RMFileOnExit(const std::string& path): _path(path) {}
    ~RMFileOnExit() {
        if (_path.size() > 0) unlink(_path.c_str());
    }
};

TEST_CASE("Test run number generation") {
    BasicBackingStore data_store;
    daqdrn::run_number<BasicBackingStore> rn(data_store);

    REQUIRE(rn.get_number(std::string("")) == 0);
    REQUIRE(rn.get_number(std::string("\0", 1)) == 1);
    REQUIRE(rn.get_number(std::string("abc")) == 2);
    REQUIRE(rn.get_number(std::string("abc")) == 2);
    REQUIRE(rn.get_number(std::string("def")) == 3);
    REQUIRE(rn.get_number(std::string("def")) == 3);
    REQUIRE(rn.get_number(std::string("abc")) == 4);
    REQUIRE(rn.get_number(std::string("abc\0ABC", 7)) == 5);
    REQUIRE(rn.get_number(std::string("abc\0ABC", 7)) == 5);
    REQUIRE(rn.get_number(std::string("abc")) == 6);
}

TEST_CASE("Test run number when you cannot persist new changes") {
    FailtoSaveBackingStore data_store(daqdrn::run_number_state(5, "abc"));
    daqdrn::run_number<FailtoSaveBackingStore> rn(data_store);

    REQUIRE(rn.get_number(std::string("abc")) == 5);
    REQUIRE_THROWS(rn.get_number(std::string("def")));
    REQUIRE(rn.get_number(std::string("abc")) == 5);
}

TEST_CASE("Test run number backing store") {
    char tmpdir[100] = "/tmp/test_rn.tmpXXXXXX";
    REQUIRE(mkdtemp(tmpdir) != NULL);
    RMDirOnExit _rmdir((char *)tmpdir);

    std::ostringstream os;
    os << (char *)tmpdir << "/" << "db.txt";

    std::string db_path = os.str();

    RMFileOnExit _rmfile(db_path);

    {
        daqdrn::file_backing_store data_store(db_path);
        daqdrn::run_number_state state = data_store.get_state();
        REQUIRE(state.value == 0);
        REQUIRE(state.hash == "");

        state.value = 42;
        state.hash = std::string("\0\0\0abc\0DEF", 10);

        REQUIRE_NOTHROW(data_store.save_state(state));
    }

    {
        daqdrn::file_backing_store data_store(db_path);
        daqdrn::run_number_state state = data_store.get_state();
        REQUIRE(state.value == 42);
        REQUIRE(state.hash == std::string("\0\0\0abc\0DEF", 10));

        state.value = 45;
        state.hash = std::string("abc\0DEF", 7);

        REQUIRE_NOTHROW(data_store.save_state(state));
    }
}