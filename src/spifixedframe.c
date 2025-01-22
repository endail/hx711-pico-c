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
#include <pico/divider.h>
#include <pico/malloc.h>
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
        assert(byte_len > 0);
        assert(byte_len <= SPIFIXEDFRAME_MAX_BYTES);

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

spifixedframe_buffer_t spifixedframe_serialise(
    const spifixedframe_t* const frame) {

        assert(frame != NULL);

        spifixedframe_buffer_t buff = 0;

        buff = util_set_bits16(
            buff,
            SPIFIXEDFRAME_IS_FIRST_OFFSET,
            SPIFIXEDFRAME_IS_FIRST_SIZE_BITS,
            (uint8_t)frame->is_first);

        buff = util_set_bits16(
            buff,
            SPIFIXEDFRAME_DATA_OFFSET,
            SPIFIXEDFRAME_DATA_SIZE_BITS,
            frame->data);

        return buff;

}

void spifixedframe_bulk_serialise(
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

void spifixedframe_deserialise(
    spifixedframe_t* const frame,
    const spifixedframe_buffer_t buffer) {

        assert(frame != NULL);

        frame->is_first = (bool)util_get_bits16(
            buffer,
            SPIFIXEDFRAME_IS_FIRST_OFFSET,
            SPIFIXEDFRAME_IS_FIRST_SIZE_BITS);

        frame->data = util_get_bits16(
            buffer,
            SPIFIXEDFRAME_DATA_OFFSET,
            SPIFIXEDFRAME_DATA_SIZE_BITS);

}

bool spifixedframe_send_bytes(
    spi_inst_t* const spi,
    const uint8_t* const bytes,
    const size_t byte_len) {

        assert(spi != NULL);
        assert(bytes != NULL);
        assert(byte_len <= SPIFIXEDFRAME_MAX_BYTES);

        if(byte_len > SPIFIXEDFRAME_MAX_BYTES) {
            return false;
        }

        bool success = false;
        const size_t frameCount = spifixedframe_calc_frame_count(byte_len);
        const size_t allocBytes = (sizeof(spifixedframe_t) * frameCount);
        spifixedframe_t* restrict const frames = malloc(allocBytes);

        if(frames == NULL) {
            return false;
        }

        spifixedframe_fragment_bytes(
            bytes,
            byte_len,
            frames);

        success = spifixedframe_chain_write_blocking(
            spi,
            frames,
            frameCount);

        free(frames);

        return success;

}

bool spifixedframe_recv_bytes(
    spi_inst_t* const spi,
    uint8_t* const bytes,
    const size_t byte_len) {

        assert(spi != NULL);
        assert(bytes != NULL);
        assert(byte_len <= SPIFIXEDFRAME_MAX_BYTES);

        bool success = false;
        const size_t frameCount = spifixedframe_calc_frame_count(byte_len);
        const size_t allocBytes = (sizeof(spifixedframe_t) * frameCount);
        spifixedframe_t* const restrict frames = malloc(allocBytes);

        if(frames == NULL) {
            return false;
        }

        success = spifixedframe_chain_read_blocking(
            spi,
            frameCount,
            frames);

        if(success) {
            success = spifixedframe_defragment_frames(
                frames,
                frameCount,
                bytes,
                byte_len);
        }

        free(frames);

        return success;

}

void spifixedframe_fragment_bytes(
    const uint8_t* const bytes,
    const size_t byte_len,
    spifixedframe_t* const frames) {

        assert(bytes != NULL);
        assert(byte_len > 0);
        assert(frames != NULL);

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

}

bool spifixedframe_defragment_frames(
    const spifixedframe_t* const frames,
    const size_t frame_count,
    uint8_t* const bytes,
    const size_t expected_bytes_len) {

        assert(frames != NULL);
        assert(frame_count > 0);
        assert(frame_count <= SPIFIXEDFRAME_MAX_FRAMES);
        assert(bytes != NULL);
        assert(expected_bytes_len > 0);
        assert(expected_bytes_len <= SPIFIXEDFRAME_MAX_BYTES);

        size_t bitsFilled = 0;
        uint8_t currentByte = 0;
        size_t byteIndex = 0;

        // check first frame is flagged as first
        if(!frames[0].is_first) {
            return false;
        }

        // check all remaining frames are not flagged as first
        for(size_t i = 1; i < frame_count; ++i) {
            if(frames[i].is_first) {
                return false;
            }
        }

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
                        return true;
                    }

                    currentByte = 0;
                    bitsFilled = 0;

                }

            }

        }

        // for any leftovers, add them if more bytes expected
        if(bitsFilled > 0 && byteIndex < expected_bytes_len) {
            bytes[byteIndex] = currentByte;
            // byteIndex++;
        }

        return true;

}

bool spifixedframe_write_frame_blocking(
    spi_inst_t* const spi,
    const spifixedframe_t* const frame) {

        assert(spi != NULL);
        assert(frame != NULL);

        const spifixedframe_buffer_t buffer = spifixedframe_serialise(frame);

        while(!spi_is_writable(spi)) {
            tight_loop_contents();
        }

        return spi_write16_blocking(
            spi,
            &buffer, 1) == 1;

}

bool spifixedframe_read_frame_blocking(
    spi_inst_t* const spi,
    spifixedframe_t* const frame) {

        assert(spi != NULL);
        assert(frame != NULL);

        spifixedframe_buffer_t inbuffer;
        spifixedframe_buffer_t outbuffer = spifixedframe_serialise(
            &SPIFIXEDFRAME_NULL_FRAME);

        while(!spi_is_readable(spi)) {
            tight_loop_contents();
        }

        // outbuffer contains a null-frame as this function
        // requires a value to transmit
        if(spi_read16_blocking(spi, outbuffer, &inbuffer, 1) != 1) {
            return false;
        }

        spifixedframe_deserialise(frame, inbuffer);

        return true;

}

bool spifixedframe_bulk_write_frames_blocking(
    spi_inst_t* const spi,
    const spifixedframe_t* const frames,
    const size_t frames_len) {

        assert(spi != NULL);
        assert(frames != NULL);
        assert(frames_len <= SPIFIXEDFRAME_MAX_FRAMES);

        bool success = false;
        const size_t allocBytes = (sizeof(spifixedframe_buffer_t) * frames_len);
        spifixedframe_buffer_t* restrict const buffer = malloc(allocBytes);

        // note: all 16 bits in each frame are set to 0 if not used,
        // so malloc is OK instead of calloc

        if(buffer == NULL) {
            return false;
        }

        spifixedframe_bulk_serialise(
            frames,
            buffer,
            frames_len);

        while(!spi_is_writable(spi)) {
            tight_loop_contents();
        }

        success = ((size_t)spi_write16_blocking(
            spi,
            buffer,
            frames_len) == frames_len);

        free(buffer);

        return success;

}

bool spifixedframe_bulk_read_frames_blocking(
    spi_inst_t* const spi,
    spifixedframe_t* const frames,
    const size_t frames_len) {

        assert(spi != NULL);
        assert(frames != NULL);
        assert(frames_len <= SPIFIXEDFRAME_MAX_FRAMES);

        bool success = false;
        const spifixedframe_buffer_t outbuffer = 
            spifixedframe_serialise(&SPIFIXEDFRAME_NULL_FRAME);

        const size_t allocBytes = (sizeof(spifixedframe_buffer_t) * frames_len);
        spifixedframe_buffer_t* restrict const inbuffer = malloc(allocBytes);

        if(inbuffer == NULL) {
            return false;
        }

        success = ((size_t)spi_read16_blocking(
            spi,
            outbuffer,
            inbuffer,
            frames_len) == frames_len);

        // if read succeeded, create the frames from the buffer
        if(success) {
            for(size_t i = 0; i < frames_len; ++i) {
                spifixedframe_deserialise(&frames[i], inbuffer[i]);
            }
        }

        free(inbuffer);

        return success;

}

bool spifixedframe_chain_write_blocking(
    spi_inst_t* const spi,
    const spifixedframe_t* const frames,
    const size_t frames_to_write) {

        assert(spi != NULL);
        assert(frames != NULL);
        assert(frames_to_write <= SPIFIXEDFRAME_MAX_FRAMES);
        assert(frames[0].is_first);

        return spifixedframe_bulk_write_frames_blocking(
            spi,
            frames,
            frames_to_write);

}

bool spifixedframe_chain_read_blocking(
    spi_inst_t* const spi,
    const size_t frames_to_read,
    spifixedframe_t* const frames) {

        assert(spi != NULL);
        assert(frames_to_read <= SPIFIXEDFRAME_MAX_FRAMES);
        assert(frames != NULL);

        bool success = false;

        success = spifixedframe_chain_wait_first_frame_blocking(
            spi,
            &frames[0]);

        if(!success) {
            return false;
        }

        if(frames_to_read == 1) {
            return true;
        }

        return spifixedframe_bulk_read_frames_blocking(
            spi,
            &frames[1],
            frames_to_read - 1);

}

bool spifixedframe_chain_wait_first_frame_blocking(
    spi_inst_t* const spi,
    spifixedframe_t* const first_frame) {

        assert(spi != NULL);
        assert(first_frame != NULL);

        spifixedframe_t temp;
        spifixedframe_get_null_frame(&temp);

        do {
            // if reading fails, there's an IO error
            // so consider this as a fail
            if(!spifixedframe_read_frame_blocking(spi, &temp)) {
                return false;
            }
        }
        while(!temp.is_first);

        *first_frame = temp;

        return true;

}
