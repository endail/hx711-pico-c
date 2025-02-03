// MIT License
// 
// Copyright (c) 2025 Daniel Robertson
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef SPIFIXEDFRAME_H_E10C5828_E3DE_493A_9DD3_C0D58B0166CD
#define SPIFIXEDFRAME_H_E10C5828_E3DE_493A_9DD3_C0D58B0166CD

#include <hardware/spi.h>
#include <pico/types.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An individual frame is a spifixedframe_t
 * A series of frames where the first frame is flagged is_first is a chain
 * A frame where all bits are 0 is a NULL frame
 * 
 * WARNING: this is very memory inefficient due to allocations and copying
 */

#define SPIFIXEDFRAME_TOTAL_BITS                    16u

#define SPIFIXEDFRAME_IS_FIRST_OFFSET               0u
#define SPIFIXEDFRAME_DATA_OFFSET                   1u

#define SPIFIXEDFRAME_IS_FIRST_SIZE_BITS            1u
#define SPIFIXEDFRAME_DATA_SIZE_BITS                15u

/**
 * These limits are completely arbitrary
 */
#define SPIFIXEDFRAME_MAX_BYTES                     16000u
#define SPIFIXEDFRAME_MAX_FRAMES                    1000u

typedef enum {
    SPIFIXEDFRAME_ERROR_OK =                        0,
    SPIFIXEDFRAME_ERROR_GENERIC =                   -1,
    SPIFIXEDFRAME_ERROR_UNKNOWN =                   -2,
    SPIFIXEDFRAME_ERROR_BYTE_LIMIT_EXCEEDED =       -3,
    SPIFIXEDFRAME_ERROR_FRAME_LIMIT_EXCEEDED =      -4,
    SPIFIXEDFRAME_ERROR_CHAIN_NO_FIRST =            -5,
    SPIFIXEDFRAME_ERROR_CHAIN_MULTIPLE_FIRST =      -6,
    SPIFIXEDFRAME_ERROR_TIMEOUT =                   -7,
    SPIFIXEDFRAME_ERROR_TRY_LIMIT_EXCEEDED =        -8,
    SPIFIXEDFRAME_ERROR_SPI_GENERIC =               -9,
    SPIFIXEDFRAME_ERROR_SPI_WRITE_FAIL =            -10,
    SPIFIXEDFRAME_ERROR_SPI_READ_FAIL =             -11,
    SPIFIXEDFRAME_ERROR_DYNAMIC_MEMORY_FAIL =       -12,
    SPIFIXEDFRAME_ERROR_TOO_FEW_FRAMES =            -13,
    SPIFIXEDFRAME_ERROR_TOO_FEW_BYTES =             -14
} spifixedframe_error_t;

#define SPIFIXEDFRAME_CHECK_FRAME_COUNT(COUNT) \
    do { \
        UTIL_RETURNIF(COUNT == 0, SPIFIXEDFRAME_ERROR_TOO_FEW_FRAMES); \
        UTIL_RETURNIF(COUNT > SPIFIXEDFRAME_MAX_FRAMES, SPIFIXEDFRAME_ERROR_FRAME_LIMIT_EXCEEDED); \
    } \
    while(0)

#define SPIFIXEDFRAME_CHECK_BYTE_COUNT(COUNT) \
    do { \
        UTIL_RETURNIF(COUNT == 0, SPIFIXEDFRAME_ERROR_TOO_FEW_BYTES); \
        UTIL_RETURNIF(COUNT > SPIFIXEDFRAME_MAX_BYTES, SPIFIXEDFRAME_ERROR_BYTE_LIMIT_EXCEEDED); \
    } \
    while(0)

#define SPIFIXEDFRAME_CHECK_CHAIN(FRAMES, FRAME_LEN) \
    do { \
        UTIL_RETURNIF(!FRAMES[0].is_first, SPIFIXEDFRAME_ERROR_CHAIN_NO_FIRST); \
        for(size_t _spifixedframe_check_chain_i = 1; _spifixedframe_check_chain_i < FRAME_LEN; ++_spifixedframe_check_chain_i) { \
            UTIL_RETURNIF(FRAMES[_spifixedframe_check_chain_i].is_first, SPIFIXEDFRAME_ERROR_CHAIN_MULTIPLE_FIRST); \
        } \
    } \
    while(0)

typedef uint16_t spifixedframe_buffer_t;

typedef struct {
    bool is_first: SPIFIXEDFRAME_IS_FIRST_SIZE_BITS;
    spifixedframe_buffer_t data: SPIFIXEDFRAME_DATA_SIZE_BITS;
} spifixedframe_t;

extern const spifixedframe_t SPIFIXEDFRAME_NULL_FRAME;

/**
 * @brief Set the given frame to a null frame.
 * 
 * @param frame 
 */
void spifixedframe_get_null_frame(
    spifixedframe_t* const frame);

/**
 * @brief Calculate the number of frames needed for
 * the given number of bytes.
 * 
 * @param byte_len 
 * @return size_t 
 */
size_t spifixedframe_calc_frame_count(
    const size_t byte_len);

/**
 * @brief Serialise a frame to bytes.
 * 
 * @param frame 
 * @return spifixedframe_buffer_t 
 */
spifixedframe_buffer_t spifixedframe_serialise(
    const spifixedframe_t* const frame);

/**
 * @brief Serialise a series of frames.
 * 
 * @param frames 
 * @param buffers 
 * @param frames_len 
 */
void spifixedframe_bulk_serialise(
    const spifixedframe_t* const frames,
    spifixedframe_buffer_t* const buffers,
    const size_t frames_len);

/**
 * @brief Deserialise bytes to a frame.
 * 
 * @param frame 
 * @param buffer 
 */
void spifixedframe_deserialise(
    spifixedframe_t* const frame,
    const spifixedframe_buffer_t buffer);

/**
 * @brief Send a fixed number of bytes.
 * 
 * @param spi 
 * @param bytes 
 * @param byte_len 
 * @param timeout_us pass 0 to block
 * @return spifixedframe_error_t 
 */
spifixedframe_error_t spifixedframe_send_bytes(
    spi_inst_t* const spi,
    const uint8_t* const bytes,
    const size_t byte_len,
    const uint timeout_us);

/**
 * @brief Receive a fixed number of bytes.
 * 
 * @param spi 
 * @param bytes 
 * @param byte_len 
 * @param timeout_us pass 0 to block
 * @return true 
 * @return false 
 */
spifixedframe_error_t spifixedframe_recv_bytes(
    spi_inst_t* const spi,
    uint8_t* const bytes,
    const size_t byte_len,
    const uint timeout_us);

/**
 * @brief Send and receive fixed numbers of bytes. The same
 * timeout length is used for both sending and receiving.
 * 
 * @param spi 
 * @param req 
 * @param req_len 
 * @param resp 
 * @param resp_len 
 * @param timeout_us pass 0 to block
 * @return spifixedframe_error_t 
 */
spifixedframe_error_t spifixedframe_req_resp(
    spi_inst_t* const spi,
    const uint8_t* const req,
    const size_t req_len,
    uint8_t* const resp,
    const size_t resp_len,
    const uint timeout_us);

/**
 * @brief Fragment an array of bytes into an array of frames.
 * Memory for frames must be preallocated and
 * frame_count must be precalculated.
 * 
 * @see spifixedframe_calc_frame_count
 * @param bytes 
 * @param byte_len 
 * @param frames 
 */
spifixedframe_error_t spifixedframe_fragment_bytes(
    const uint8_t* const bytes,
    const size_t byte_len,
    spifixedframe_t* const frames);

/**
 * @brief Defragment an array of frames into an array of bytes.
 * 
 * @param frames 
 * @param frame_count 
 * @param bytes 
 * @return true 
 * @return false 
 */
spifixedframe_error_t spifixedframe_defragment_frames(
    const spifixedframe_t* const frames,
    const size_t frame_count,
    uint8_t* const bytes,
    const size_t expected_bytes_len);

/**
 * @brief Write a single frame to SPI.
 * 
 * @param spi 
 * @param frame 
 * @param timeout_us pass 0 to block
 * @return true 
 * @return false 
 */
spifixedframe_error_t spifixedframe_write_frame(
    spi_inst_t* const spi,
    const spifixedframe_t* const frame,
    const uint timeout_us);

/**
 * @brief Read a single frame from SPI.
 * 
 * @param spi 
 * @param frame 
 * @param timeout_us pass 0 to block
 * @return true 
 * @return false 
 */
spifixedframe_error_t spifixedframe_read_frame(
    spi_inst_t* const spi,
    spifixedframe_t* const frame,
    const uint timeout_us);

/**
 * @brief Write an array of frames to SPI.
 * 
 * @param spi 
 * @param frames 
 * @param frames_len 
 * @param timeout_us pass 0 to block
 * @return true 
 * @return false 
 */
spifixedframe_error_t spifixedframe_bulk_write_frames(
    spi_inst_t* const spi,
    const spifixedframe_t* const frames,
    const size_t frames_len,
    const uint timeout_us);

/**
 * @brief Read an array of frames from SPI.
 * 
 * @param spi 
 * @param frames 
 * @param frames_len 
 * @param timeout_us pass 0 to block
 * @return true 
 * @return false 
 */
spifixedframe_error_t spifixedframe_bulk_read_frames(
    spi_inst_t* const spi,
    spifixedframe_t* const frames,
    const size_t frames_len,
    const uint timeout_us);

/**
 * @brief Write a chain of frames to SPI. A chain is an array
 * of frames where the first and only the first is flagged.
 * 
 * @param spi 
 * @param frames 
 * @param frames_to_write 
 * @param timeout_us
 * @return true 
 * @return false 
 */
spifixedframe_error_t spifixedframe_chain_write(
    spi_inst_t* const spi,
    const spifixedframe_t* const frames,
    const size_t frames_to_write,
    const uint timeout_us);

/**
 * @brief Read a chain of frames from SPI. A chain is an array
 * of frames where the first and only the first is flagged. This
 * function will wait until it receives a chain from beginning to
 * {frames_to_read} number of frames. The same timeout is used
 * for reading the first frame and any remaining frames.
 * 
 * @param spi 
 * @param frames_to_read 
 * @param frames 
 * @param timeout_us
 * @return true 
 * @return false 
 */
spifixedframe_error_t spifixedframe_chain_read(
    spi_inst_t* const spi,
    const size_t frames_to_read,
    spifixedframe_t* const frames,
    const uint timeout_us);

/**
 * @brief Blocks until the first frame in a chain of frames is
 * received. The first frame is set through the pointer.
 * 
 * @param spi 
 * @param first_frame 
 * @param timeout_us
 * @return true 
 * @return false 
 */
spifixedframe_error_t spifixedframe_chain_wait_first_frame(
    spi_inst_t* const spi,
    spifixedframe_t* const first_frame,
    const uint timeout_us);

#ifdef __cplusplus
}
#endif

#endif
