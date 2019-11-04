//
// Created by jonathan.hanks on 11/1/19.
//
#include "catch.hpp"
#include "cmask_t.h"
#include <algorithm>
#include <iostream>
#include <sstream>

TEST_CASE("You can create a cmask_t type and it has known a known size")
{
    cmask_t val;
    REQUIRE(sizeof(val) == sizeof(std::uint64_t)*CMASK_T_INTERNAL_WORD_COUNT);
    REQUIRE(val.num_bits() == sizeof(std::uint64_t)*CMASK_T_INTERNAL_WORD_COUNT*CHAR_BIT );
    REQUIRE(val.num_bits() == 128 );
}

TEST_CASE("You can use clear_all on a cmask_t")
{
    cmask_t val;
    for (unsigned int i = 0; i < CMASK_T_INTERNAL_WORD_COUNT; ++i)
    {
        val.mask[i] = 0xffffffffffffffff;
    }
    REQUIRE(!val.empty());
    val.clear_all();
    for (unsigned int i = 0; i < val.num_bits(); ++i)
    {
        REQUIRE(!val.get(i) );
    }
    REQUIRE(val.empty() );
}

TEST_CASE("You can read the state of individual bits with get")
{
    cmask_t val;
    val.clear_all();

    REQUIRE(!val.get(0));
    val.mask[0]=0x1;
    REQUIRE(val.get(0));
    val.mask[0]=0x2;
    REQUIRE(val.get(1));
    REQUIRE(!val.get(0));
    REQUIRE(!val.get(63));
    val.mask[0] = 0x8000000000000000;
    REQUIRE(val.get(63));

    val.clear_all();
    REQUIRE(!val.get(64));
    val.mask[1]=0x1;
    REQUIRE(val.get(64));
    val.mask[1]=0x2;
    REQUIRE(val.get(65));
    REQUIRE(!val.get(64));
    REQUIRE(!val.get(127));
    val.mask[1] = 0x8000000000000000;
    REQUIRE(val.get(127));
}

TEST_CASE("get works over all the bits")
{
    cmask_t val;

    for (unsigned int i=0; i < val.num_bits(); ++i)
    {
        val.clear_all();
        unsigned int word = i / 64;
        unsigned int bit = i % 64;
        REQUIRE(!val.get(i));
        val.mask[word] = static_cast<uint64_t>(1) << bit;
        REQUIRE(val.get(i));
        for (unsigned int j = 0; j < val.num_bits(); ++j)
        {
            if (j != i)
            {
                REQUIRE(!val.get(j));
            }
        }
    }
}

TEST_CASE("set sets bits in the mask")
{
    cmask_t val;

    for (unsigned int i = 0; i < val.num_bits(); ++i)
    {
        val.clear_all();
        REQUIRE(val.empty());
        unsigned int word = i / 64;
        unsigned int bit = i % 64;
        REQUIRE(!val.get(i));
        val.set(i);
        REQUIRE(!val.empty());
        REQUIRE(val.get(i));
        REQUIRE(val.mask[word] == static_cast<uint64_t>(1) << bit);
        for (unsigned int j = 0; j < val.num_bits(); ++j)
        {
            if (j != i)
            {
                REQUIRE(!val.get(j));
            }
        }
    }
}

TEST_CASE("you can set multiple bits on the mask")
{
    cmask_t val;

    std::vector<unsigned int> set_points;
    set_points.push_back(2);
    set_points.push_back(12);
    set_points.push_back(70);
    set_points.push_back(5);
    set_points.push_back(127);

    val.clear_all();

    for (int i = 0; i < set_points.size(); ++i)
    {
        val.set(set_points[i]);
    }
    for (unsigned int i = 0; i < val.num_bits(); ++i)
    {
        if (std::find(set_points.begin(), set_points.end(), i) != set_points.end())
        {
            REQUIRE(val.get(i));
        }
        else
        {
            REQUIRE(!val.get(i));
        }
    }
}

TEST_CASE("clear clears individual bits")
{
    cmask_t val;

    for (unsigned int i = 0; i < val.num_bits(); ++i)
    {
        val.clear_all();
        REQUIRE(val.empty());
        unsigned int word = i / 64;
        unsigned int bit = i % 64;
        REQUIRE(!val.get(i));
        val.set(i);
        REQUIRE(!val.empty());
        REQUIRE(val.get(i));
        REQUIRE(val.mask[word] == static_cast<uint64_t>(1) << bit);
        for (unsigned int j = 0; j < val.num_bits(); ++j)
        {
            if (j != i)
            {
                REQUIRE(!val.get(j));
            }
        }
        REQUIRE(!val.empty());
        val.clear(i);
        REQUIRE(val.empty());
    }
}

TEST_CASE("Difference with shows which bits of the current set are set that are not in the given set")
{
    std::vector<unsigned int> bits1 = {1,5,9,18,20,21,50,63,64,80,90,100};
    std::vector<unsigned int> bits2 = {1,5,9,18,20,21,51,66,85,90,100};
    std::vector<unsigned int> expected_diff = {50, 63, 64, 80};
    std::vector<unsigned int> actual_diff = {};

    cmask_t mask1;
    mask1.clear_all();
    for (unsigned int i = 0; i < bits1.size(); ++i)
    {
        mask1.set(bits1[i]);
    }
    cmask_t mask2;
    mask2.clear_all();
    for (unsigned int i = 0; i < bits2.size(); ++i)
    {
        mask2.set(bits2[i]);
    }
    cmask_t diff = mask1.difference_with(mask2);
    for (unsigned int i = 0; i < diff.num_bits(); ++i)
    {
        if (diff.get(i))
        {
            actual_diff.push_back(i);
        }
    }
    REQUIRE(expected_diff.size() == actual_diff.size());
    REQUIRE(std::equal(expected_diff.begin(), expected_diff.end(), actual_diff.begin()));
}

std::string
to_string(const cmask_t& mask)
{
    std::ostringstream os;
    os << mask;
    return os.str();
}

TEST_CASE("You can get a hex dump of a cmask_t")
{
    static const std::string zeros="00000000000000000000000000000000";
    static const std::string entries="1248";
    cmask_t val;
    val.clear_all();
    REQUIRE(to_string(val) == zeros);
    for (unsigned int i = 0; i < val.num_bits(); ++i)
    {
        std::string expected = zeros;
        unsigned int nibble = i / 4;
        expected[expected.size()-1-nibble] = entries[i % 4];

        val.clear_all();
        val.set(i);
        REQUIRE(to_string(val) == expected);
    }
}