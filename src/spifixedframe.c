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

#include <assert.h>
#include <hardware/spi.h>
#include <hardware/timer.h>
#include <pico/divider.h>
#include <pico/malloc.h>
#include <pico/time.h>
#include <pico/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include "../include/spifixedframe.h"
#include "../include/util.h"

const spifixedframe_t SPIFIXEDFRAME_NULL_FRAME = {
    .is_first = false,
    .data = 0
};

void spifixedframe_get_null_frame(
    spifixedframe_t* const frame) {
        assert(frame != NULL);
        *frame = SPIFIXEDFRAME_NULL_FRAME;
}

size_t spifixedframe_calc_frame_count(
    const size_t byte_len) {

        static_assert(SPIFIXEDFRAME_DATA_SIZE_BITS > 0);
        assert(util_size_t_in_range(byte_len, 0, SPIFIXEDFRAME_MAX_BYTES));

        uint32_t remainder;
        const uint32_t bitLen = byte_len * UINT8_WIDTH;

        // divide number of bits by the frame length
        uint32_t quotient = divmod_u32u32_rem(
            bitLen,
            SPIFIXEDFRAME_DATA_SIZE_BITS,
            &remainder);

        // if there is a remainder, an extra frame is needed
        if(remainder > 0) {
            quotient++;
        }

        return quotient;

}

spifixedframe_t* spifixedframe_create_frames(
    const size_t byte_len,
    size_t* frame_count) {

        assert(byte_len > 0);

        const size_t frameCount = spifixedframe_calc_frame_count(byte_len);
        const size_t allocBytes = sizeof(spifixedframe_t) * frameCount;

        if(frame_count != NULL) {
            *frame_count = frameCount;
        }

        return malloc(allocBytes);

}

spifixedframe_buffer_t* spifixedframe_create_buffers(
    const size_t frames_len) {
        assert(frames_len > 0);
        const size_t allocBytes = sizeof(spifixedframe_buffer_t) * frames_len;
        return malloc(allocBytes);
}

void spifixedframe_serialise_frames(
    const spifixedframe_t* const frames,
    spifixedframe_buffer_t* const buffers,
    const size_t frames_len) {

        assert(frames != NULL);
        assert(buffers != NULL);

        for(size_t i = 0; i < frames_len; ++i) {

            buffers[i] = util_set_bits16(
                0,
                SPIFIXEDFRAME_IS_FIRST_OFFSET,
                SPIFIXEDFRAME_IS_FIRST_SIZE_BITS,
                (uint8_t)frames[i].is_first);

            buffers[i] = util_set_bits16(
                buffers[i],
                SPIFIXEDFRAME_DATA_OFFSET,
                SPIFIXEDFRAME_DATA_SIZE_BITS,
                frames[i].data);

        }

}

void spifixedframe_deserialise_frames(
    const spifixedframe_buffer_t* const buffers,
    spifixedframe_t* const frames,
    const size_t frames_len) {

        assert(buffers != NULL);
        assert(frames != NULL);

        for(size_t i = 0; i < frames_len; ++i) {

            frames[i].is_first = (bool)util_get_bits16(
                buffers[i],
                SPIFIXEDFRAME_IS_FIRST_OFFSET,
                SPIFIXEDFRAME_IS_FIRST_SIZE_BITS);

            frames[i].data = util_get_bits16(
                buffers[i],
                SPIFIXEDFRAME_DATA_OFFSET,
                SPIFIXEDFRAME_DATA_SIZE_BITS);

        }

}

spifixedframe_error_t spifixedframe_send_bytes(
    spi_inst_t* const spi,
    const uint8_t* const bytes,
    const size_t byte_len,
    const uint timeout_us) {

        assert(spi != NULL);
        assert(bytes != NULL);

        SPIFIXEDFRAME_CHECK_BYTE_COUNT(byte_len);

        size_t frameCount;
        spifixedframe_t* const frames = spifixedframe_create_frames(
            byte_len,
            &frameCount);

        if(frames == NULL) {
            return SPIFIXEDFRAME_ERROR_DYNAMIC_MEMORY_FAIL;
        }

        spifixedframe_fragment_bytes(
            bytes,
            byte_len,
            frames);

        const spifixedframe_error_t code = spifixedframe_chain_write(
            spi,
            frames,
            frameCount,
            timeout_us);

        free(frames);

        return code;

}

spifixedframe_error_t spifixedframe_recv_bytes(
    spi_inst_t* const spi,
    uint8_t* const bytes,
    const size_t byte_len,
    const uint timeout_us) {

        assert(spi != NULL);
        assert(bytes != NULL);

        SPIFIXEDFRAME_CHECK_BYTE_COUNT(byte_len);

        spifixedframe_error_t code;
        size_t frameCount;

        spifixedframe_t* const frames = spifixedframe_create_frames(
            byte_len,
            &frameCount);

        if(frames == NULL) {
            return SPIFIXEDFRAME_ERROR_DYNAMIC_MEMORY_FAIL;
        }

        code = spifixedframe_chain_read(
            spi,
            frameCount,
            frames,
            timeout_us);

        if(code == SPIFIXEDFRAME_ERROR_OK) {
            code = spifixedframe_defragment_frames(
                frames,
                frameCount,
                bytes,
                byte_len);
        }

        free(frames);

        return code;

}

spifixedframe_error_t spifixedframe_req_resp(
    spi_inst_t* const spi,
    const uint8_t* const req_bytes,
    const size_t req_len,
    uint8_t* const resp_bytes,
    const size_t resp_len,
    const uint timeout_us) {

        assert(spi != NULL);
        assert(req_bytes != NULL);
        assert(resp_bytes != NULL);

        SPIFIXEDFRAME_CHECK_BYTE_COUNT(req_len);
        SPIFIXEDFRAME_CHECK_BYTE_COUNT(resp_len);

        spifixedframe_error_t code;

        code = spifixedframe_send_bytes(
            spi,
            req_bytes,
            req_len,
            timeout_us);

        if(code != SPIFIXEDFRAME_ERROR_OK) {
            return code;
        }

        return spifixedframe_recv_bytes(
            spi,
            resp_bytes,
            resp_len,
            timeout_us);

}

spifixedframe_error_t spifixedframe_fragment_bytes(
    const uint8_t* const bytes,
    const size_t byte_len,
    spifixedframe_t* const frames) {

        assert(bytes != NULL);
        assert(frames != NULL);

        SPIFIXEDFRAME_CHECK_BYTE_COUNT(byte_len);

        spifixedframe_buffer_t currentChunk = 0;
        size_t chunkIndex = 0;
        size_t bitsRemaining = 0;

        // iterate over each byte
        for(size_t byteNum = 0; byteNum < byte_len; ++byteNum) {

            // iterate over each bit in the byte
            for(ssize_t bitPos = UINT8_WIDTH - 1; bitPos >= 0; --bitPos) {

                // if the current chunk isn't completely filled, add another bit
                if(bitsRemaining < SPIFIXEDFRAME_DATA_SIZE_BITS) {
                    currentChunk |= ((bytes[byteNum] >> bitPos) & 1) << bitsRemaining;
                    bitsRemaining++;
                }

                // when current chunk is filled, set the data to the frame
                // and reset the other vars
                if(bitsRemaining == SPIFIXEDFRAME_DATA_SIZE_BITS) {
                    frames[chunkIndex].is_first = (chunkIndex == 0);
                    frames[chunkIndex].data = currentChunk;
                    chunkIndex++;
                    bitsRemaining = 0;
                    currentChunk = 0;
                }

            }
        }

        // if there are any bits remaining, add them to the last frame
        if(bitsRemaining > 0) {
            frames[chunkIndex].is_first = (chunkIndex == 0);
            frames[chunkIndex].data = currentChunk;
        }

        return SPIFIXEDFRAME_ERROR_OK;

}

spifixedframe_error_t spifixedframe_defragment_frames(
    const spifixedframe_t* const frames,
    const size_t frame_count,
    uint8_t* const bytes,
    const size_t expected_bytes_len) {

        assert(frames != NULL);
        assert(bytes != NULL);

        SPIFIXEDFRAME_CHECK_FRAME_COUNT(frame_count);
        SPIFIXEDFRAME_CHECK_BYTE_COUNT(expected_bytes_len);
        SPIFIXEDFRAME_CHECK_CHAIN(frames, frame_count);

        size_t bitsFilled = 0;
        uint8_t currentByte = 0;
        size_t byteIndex = 0;

        // iterate over each frame
        for(size_t frameIndex = 0; frameIndex < frame_count; ++frameIndex) {

            // iterate over each bit in the frame
            for(ssize_t bitPos = SPIFIXEDFRAME_DATA_SIZE_BITS - 1; bitPos >= 0; --bitPos) {

                // if all bytes have been processed, skip processing any more bits
                // and fall through to the end
                if(byteIndex >= expected_bytes_len) {
                    bitsFilled++;
                    continue;
                }

                // if the current byte is unfilled, add a bit to it
                if(bitsFilled < UINT8_WIDTH) {
                    currentByte |= ((frames[frameIndex].data >> bitPos) & 1) << ((UINT8_WIDTH - 1) - bitsFilled);
                    bitsFilled++;
                }

                // if the number of bits has equalled a full byte, add it to the buffer
                if(bitsFilled == UINT8_WIDTH) {

                    if(byteIndex < expected_bytes_len) {
                        bytes[byteIndex] = currentByte;
                        byteIndex++;
                    }
                    else {
                        return SPIFIXEDFRAME_ERROR_OK;
                    }

                    currentByte = 0;
                    bitsFilled = 0;

                }

            }

        }

        // for any leftovers, add them if more bytes expected
        if(bitsFilled > 0 && byteIndex < expected_bytes_len) {
            bytes[byteIndex] = currentByte;
        }

        return SPIFIXEDFRAME_ERROR_OK;

}

spifixedframe_error_t spifixedframe_write_frames(
    spi_inst_t* const spi,
    const spifixedframe_t* const frames,
    const size_t frames_len,
    const uint timeout_us) {

        assert(spi != NULL);
        assert(frames != NULL);

        SPIFIXEDFRAME_CHECK_FRAME_COUNT(frames_len);

        const absolute_time_t timeout = make_timeout_time_us(timeout_us);
        //spifixedframe_buffer_t* const buffer = spifixedframe_create_buffers(frames_len);
        spifixedframe_buffer_t buffer[frames_len];

        // note: all 16 bits in each frame are set to 0 if not used,
        // so malloc is OK instead of calloc

        spifixedframe_serialise_frames(
            frames,
            buffer,
            frames_len);

        if(!util_spi_is_writable_timeout(spi, &timeout)) {
            return SPIFIXEDFRAME_ERROR_TIMEOUT;
        }

        const int spiCode = spi_write16_blocking(
            spi,
            buffer,
            frames_len);

        if(spiCode > 0 && (size_t)spiCode == frames_len) {
            return SPIFIXEDFRAME_ERROR_OK;
        }

        switch(spiCode) {
        case PICO_ERROR_IO:
        case PICO_ERROR_GENERIC:
            return SPIFIXEDFRAME_ERROR_SPI_WRITE_FAIL;
        default:
            return SPIFIXEDFRAME_ERROR_SPI_GENERIC;
        }

}

spifixedframe_error_t spifixedframe_read_frames(
    spi_inst_t* const spi,
    spifixedframe_t* const frames,
    const size_t frames_len,
    const uint timeout_us) {

        assert(spi != NULL);
        assert(frames != NULL);

        SPIFIXEDFRAME_CHECK_FRAME_COUNT(frames_len);

        const absolute_time_t timeout = make_timeout_time_us(timeout_us);
        spifixedframe_buffer_t outbuffer;
        spifixedframe_buffer_t inbuffer[frames_len];

        spifixedframe_serialise_frames(
            &SPIFIXEDFRAME_NULL_FRAME,
            &outbuffer,
            1);

        if(!util_spi_is_readable_timeout(spi, &timeout)) {
            return SPIFIXEDFRAME_ERROR_TIMEOUT;
        }

        const int spiCode = spi_read16_blocking(
            spi,
            outbuffer,
            inbuffer,
            frames_len);

        // if read succeeded, create the frames from the buffer
        if(spiCode > 0 && (size_t)spiCode == frames_len) {
            spifixedframe_deserialise_frames(inbuffer, frames, frames_len);
            return SPIFIXEDFRAME_ERROR_OK;
        }

        switch(spiCode) {
        case PICO_ERROR_IO:
        case PICO_ERROR_GENERIC:
            return SPIFIXEDFRAME_ERROR_SPI_READ_FAIL;
        default:
            return SPIFIXEDFRAME_ERROR_SPI_GENERIC;
        }

}

spifixedframe_error_t spifixedframe_chain_write(
    spi_inst_t* const spi,
    const spifixedframe_t* const frames,
    const size_t frames_to_write,
    const uint timeout_us) {

        assert(spi != NULL);
        assert(frames != NULL);

        SPIFIXEDFRAME_CHECK_FRAME_COUNT(frames_to_write);
        SPIFIXEDFRAME_CHECK_CHAIN(frames, frames_to_write);

        return spifixedframe_write_frames(
            spi,
            frames,
            frames_to_write,
            timeout_us);

}

spifixedframe_error_t spifixedframe_chain_read(
    spi_inst_t* const spi,
    const size_t frames_to_read,
    spifixedframe_t* const frames,
    const uint timeout_us) {

        assert(spi != NULL);
        assert(frames != NULL);

        SPIFIXEDFRAME_CHECK_FRAME_COUNT(frames_to_read);

        const spifixedframe_error_t code = 
            spifixedframe_chain_wait_first_frame(
                spi,
                &frames[0],
                timeout_us);

        if(code != SPIFIXEDFRAME_ERROR_OK) {
            return code;
        }

        if(frames_to_read == 1) {
            return SPIFIXEDFRAME_ERROR_OK;
        }

        return spifixedframe_read_frames(
            spi,
            &frames[1],
            frames_to_read - 1,
            timeout_us);

}

spifixedframe_error_t spifixedframe_chain_wait_first_frame(
    spi_inst_t* const spi,
    spifixedframe_t* const first_frame,
    const uint timeout_us) {

        assert(spi != NULL);
        assert(first_frame != NULL);

        spifixedframe_error_t code;
        spifixedframe_t temp;
        spifixedframe_get_null_frame(&temp);

        do {

            code = spifixedframe_read_frames(
                spi,
                &temp,
                1,
                timeout_us);

            if(code != SPIFIXEDFRAME_ERROR_OK) {
                return code;
            }

        }
        while(!temp.is_first);

        *first_frame = temp;

        return SPIFIXEDFRAME_ERROR_OK;

}
