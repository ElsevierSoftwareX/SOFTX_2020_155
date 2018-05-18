/* imported the crc32 checksum code from gds/nds2 server */
#ifndef CHECKSUM_CRC32_HH
#define CHECKSUM_CRC32_HH
#include <string>
#include <stdint.h>

/**  The %checksum_crc32 class accumulates a CRC32 checksum from one or more
  *  input strings. The non-zero bytes of the length is added to the resulting
  *  check-sum is added to produce the final checksum code.
  *  \brief CRC32 checksum calculation class.
  *  \author john.zweizig@ligo.org
  *  \version $Id$
  */
class checksum_crc32 {
public:
    /**  Construct an initialized checksum instance.
      */
    checksum_crc32(void);
    /**  Destroy the %checksum_crc32 instance
      */
    virtual ~checksum_crc32(void);

    /**  Add a string of data to the accumulated checksum value.
      *  \brief Add a string.
      *  \param data   Data string pointer.
      *  \param length Length of data string in bytes.
      */
    void add(const void* data, uint32_t length);

    /**  Add a string of data to the accumulated checksum value.
      *  \brief Add a string.
      *  \param str Data string.
      */
    void add(const std::string& str);

    /**  Clear the accumulated value and length.
      *  \brief Reset accumulated value.
      */
    void reset(void);

    /**  Calculate and return the final checksum result.
      *  \brief Final checksum result.
      *  \return Final checksum.
      */
    uint32_t result(void) const;
private:
    uint32_t _value;
    uint32_t _length;
};

//======================================  Inline methods.
inline void
checksum_crc32::add(const std::string& str) {
    add(str.c_str(), str.size());
}

#endif // !defined(CHECKSUM_CRC32_HH)
