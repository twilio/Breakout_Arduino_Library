#ifndef __I_OWL_SERIAL_H__
#define __I_OWL_SERIAL_H__

#include <cstdint>

class IOwlSerial {
 public:
  virtual ~IOwlSerial() {
  }

  /**
   * Number of bytes available on the interface
   * @return - number of bytes available or a negative value for an error
   */
  virtual int32_t available() = 0;

  /**
   * Non-blocking read from the device
   * @param buf - buffer to store the read data
   * @param count - maximum number of bytes to read
   * @return - number of bytes read. Can be zero if no data is available or negative in case of an error
   */
  virtual int32_t read(uint8_t *buf, uint32_t count) = 0;

  /**
   * Non-blocking write to the device
   * @param buf - buffer containing the written data
   * @param count - number of data to write
   * @return - number of bytes actually written. Can be negative in case of an error
   */
  virtual int32_t write(const uint8_t *buf, uint32_t count) = 0;
};
#endif  // __I_OWL_SERIAL_H__
