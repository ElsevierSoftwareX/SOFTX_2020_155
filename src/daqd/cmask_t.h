#ifndef DAQD_CMASK_T_H
#define DAQD_CMASK_T_H

#include <limits.h>

#ifdef __cplusplus
#include <iostream>
#include <iomanip>
#endif

#define CMASK_T_INTERNAL_WORD_COUNT 2
/*!
 * @brief a simple bitmask structure.
 * @note this is primarily used to track connections/consumers
 * @note this is in a 'C' header, to be included to show the storage
 * requirements of a plain structure.  The C++ extensions must make
 * no deviation from the layout/size/... of the structure.  So no vtable,
 * no virtual, no ...
 */
typedef struct cmask_t {
    uint64_t mask[CMASK_T_INTERNAL_WORD_COUNT];

#ifdef __cplusplus
    friend std::ostream& operator<<(std::ostream&, const cmask_t&);

    bool get(unsigned int index) const {
        return mask[word_idx(index)] & static_cast<uint64_t>(1) << bit_idx(index);
    }

    void set(unsigned int index) {
        mask[word_idx(index)] |= static_cast<uint64_t>(1) << bit_idx(index);
    }

    void clear(unsigned int index) {
        mask[word_idx(index)] &= ~(static_cast<uint64_t>(1) << bit_idx(index));
    }

    void clear_all() {
        for (unsigned int i = 0; i < num_words(); ++i)
        {
            mask[i] = 0;
        }
    }

    bool empty() const {
        uint64_t val = 0;
        for (unsigned int i = 0; i < num_words(); ++i)
        {
            val |= mask[i];
        }
        return val == 0;
    }

    cmask_t difference_with(const cmask_t &other) const {
        cmask_t result;
        for (unsigned int i = 0; i < num_words(); ++i)
        {
            result.mask[i] = mask[i] & ~other.mask[i];
        }
        return result;
    }

    unsigned int num_bits() const
    {
        return num_words()*sizeof(uint64_t)*CHAR_BIT;
    }

private:
    unsigned int num_words() const
    {
        return CMASK_T_INTERNAL_WORD_COUNT;
    }

    unsigned int word_idx(unsigned int index) const
    {
        return index / (sizeof(uint64_t)*CHAR_BIT);
    }

    unsigned int bit_idx(unsigned int index) const
    {
        return index % (sizeof(uint64_t)*CHAR_BIT);
    }
#endif /* __cplusplus */
} cmask_t;

#define MAX_CONSUMERS (sizeof (cmask_t) * CHAR_BIT - 1)

#ifdef __cplusplus
static_assert(sizeof(cmask_t) == CMASK_T_INTERNAL_WORD_COUNT*sizeof(uint64_t), "cmask_t must be the right size");
static_assert(sizeof(uint64_t)*CHAR_BIT == 64, "64 bit ints should have 64 bits");

inline std::ostream& operator<<(std::ostream& os, const cmask_t& m)
{
    std::ios_base::fmtflags f(os.flags());
    for (int i = m.num_words()-1; i >= 0; --i)
    {
        os << std::setw(16) << std::setfill('0') << std::hex << m.mask[i];
    }
    os.flags(f);
    return os;
}

#endif /* __cplusplus */


#endif /* DAQD_CMASK_T_H */