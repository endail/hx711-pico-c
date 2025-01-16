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
#include "../include/spifixedframe.h"
#include "../include/util.h"

const spifixedframe_t SPIFIXEDFRAME_NULL_FRAME = {
    .is_first = false,
    .data = 0
};

size_t spifixedframe_calc_frame_count(
    const size_t byte_len) {

        assert(SPIFIXEDFRAME_DATA_BITS_PER_FRAME > 0);

        uint32_t remainder;

        // divide number of bits by the frame length
        size_t quotient = divmod_u32u32_rem(
            byte_len * 8,
            SPIFIXEDFRAME_DATA_BITS_PER_FRAME,
            &remainder);

        // if there is a remainder, an extra frame is needed
        if(remainder > 0) {
            quotient++;
        }

        return quotient;

}

spifixedframe_buffer_t spifixedframe_to_buffer(
    const spifixedframe_t* const frame) {

        assert(frame != NULL);
        assert(bytes != NULL);

        spifixedframe_buffer_t buff = 0;

        buff = util_set_bits16(
            buff,
            SPIFIXEDFRAME_IS_FIRST_OFFSET,
            SPIFIXEDFRAME_IS_FIRST_SIZE_BITS,
            (uint)frame->is_first);

        buff = util_set_bits16(
            buff,
            SPIFIXEDFRAME_DATA_OFFSET,
            SPIFIXEDFRAME_DATA_SIZE_BITS,
            frame->data);

        return buff;

}

void spifixedframe_from_buffer(
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
        assert(byte_len > 0);

        bool success = false;

        const size_t frameCount = spifixedframe_calc_frame_count(byte_len);
        spifixedframe_t* const frames = 
            malloc(sizeof(spifixedframe_t) * frameCount);

        if(frames == NULL) {
            return false;
        }

        success = spifixedframe_fragment_bytes(
            bytes,
            byte_len,
            frameCount,
            frames);

        if(success) {
            success = spifixedframe_write_chain_blocking(
                spi,
                frames,
                frameCount);
        }

        free(frames);

        return success;

}

bool spifixedframe_recv_bytes(
    spi_inst_t* const spi,
    uint8_t* const bytes,
    const size_t byte_len) {

        assert(spi != NULL);
        assert(bytes != NULL);
        assert(byte_len > 0);

        size_t __unused out_bytes_len;
        bool success = false;

        const size_t frameCount = spifixedframe_calc_frame_count(byte_len);
        spifixedframe_t* frames = malloc(sizeof(spifixedframe_t) * frameCount);

        if(frames == NULL) {
            return false;
        }

        success = spifixedframe_read_chain_blocking(
            spi,
            frameCount,
            frames);

        if(success) {
            success = spifixedframe_defragment_frames(
                frames,
                frameCount,
                bytes,
                &out_bytes_len);
        }

        free(frames);

        return success;

}

bool spifixedframe_fragment_bytes(
    const uint8_t* const bytes,
    const size_t byte_len,
    const size_t frame_count,
    spifixedframe_t* const frames) {

        assert(bytes != NULL);
        assert(byte_len > 0);
        assert(frame_count > 0);
        assert(frames != NULL);

        size_t chunk_index = 0;
        spifixedframe_buffer_t current_chunk = 0;
        uint8_t bits_remaining = 0;

        for(size_t i = 0; i < byte_len; ++i) {
            for(int j = 7; j >= 0; --j) {

                // accumulate bits into chunk
                if(bits_remaining < SPIFIXEDFRAME_DATA_BITS_PER_FRAME) {
                    current_chunk |= ((bytes[i] >> j) & 1) << bits_remaining;
                    bits_remaining++;
                }

                // when chunk reaches frame bit length, put in frame
                if(bits_remaining == SPIFIXEDFRAME_DATA_BITS_PER_FRAME) {

                    frames[chunk_index].is_first = (current_chunk == 0);
                    frames[chunk_index].data = current_chunk;
                    chunk_index++;
                    current_chunk = 0;
                    bits_remaining = 0;

                    // exceeded calculated number of frames
                    if(chunk_index >= frame_count) {
                        return false;
                    }

                }

            }
        }

        // handle any remaining bits in the last chunk
        if(bits_remaining > 0) {
            frames[chunk_index].is_first = (chunk_index == 0);
            frames[chunk_index].data = current_chunk;
            chunk_index++;
        }

        return true;

}

bool spifixedframe_defragment_frames(
    const spifixedframe_t* const frames,
    const size_t frame_count,
    uint8_t* const bytes,
    size_t* const byte_len) {

        assert(frames != NULL);
        assert(frame_count > 0);
        assert(bytes != NULL);
        assert(bytes_len > 0); 

        size_t data_index = 0;
        spifixedframe_buffer_t current_frame = 0;
        uint8_t current_byte = 0;
        uint8_t bits_filled = 0;

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

        for(size_t i = 0; i < frame_count; ++i) {

            current_frame = frames[i].data;

            for(int j = SPIFIXEDFRAME_DATA_BITS_PER_FRAME - 1; j >= 0; --j) { 

                if(bits_filled < 8) {
                    current_byte |= ((current_frame >> j) & 1) << (7 - bits_filled);
                    bits_filled++;
                }

                if(bits_filled == 8) {
                    bytes[data_index++] = current_byte;
                    current_byte = 0;
                    bits_filled = 0;
                }

            }
        }

        // handle any remaining bits in the last byte
        if (bits_filled > 0) {
            bytes[data_index++] = current_byte;
        }

        *byte_len = data_index;

        return true;

}

bool spifixedframe_write_frame_blocking(
    spi_inst_t* const spi,
    const spifixedframe_t* const frame) {

        assert(spi != NULL);
        assert(frame != NULL);

        const spifixedframe_buffer_t buffer = spifixedframe_to_buffer(frame);

        while(!spi_is_writable(spi)) {
            tight_loop_contents();
        }

        return spi_write16_blocking(spi, &buffer, 1) == 1;

}

bool spifixedframe_read_frame_blocking(
    spi_inst_t* const spi,
    spifixedframe_t* const frame) {

        assert(spi != NULL);
        assert(frame != NULL);

        bool success = false;
        spifixedframe_buffer_t inbuffer;
        spifixedframe_buffer_t outbuffer = 
            spifixedframe_to_buffer(&SPIFIXEDFRAME_NULL_FRAME);

        while(!spi_is_readable(spi)) {
            tight_loop_contents();
        }

        success = (spi_read16_blocking(spi, outbuffer, &inbuffer, 1) == 1);

        if(success) {
            spifixedframe_from_buffer(frame, inbuffer);
        }

        return success;

}

bool spifixedframe_bulk_write_frames_blocking(
    spi_inst_t* const spi,
    const spifixedframe_t* const frames,
    const size_t frames_len) {

        assert(spi != NULL);
        assert(frames != NULL);
        assert(frames_len > 0);

        bool success = false;
        spifixedframe_buffer_t* const buffer = malloc(
            SPIFIXEDFRAME_TOTAL_BITS * frames_len);

        // note: all 16 bits in each frame are set to 0 if not used,
        // so malloc is OK instead of calloc

        if(buffer == NULL) {
            return false;
        }

        // convert each frame to 16 bits
        for(size_t i = 0; i < frames_len; ++i) {
            buffer[i] = spifixedframe_to_buffer(&frames[i]);
        }

        while(!spi_is_writable(spi)) {
            tight_loop_contents();
        }

        success = ((size_t)spi_write16_blocking(spi, buffer, frames_len) 
            == frames_len);

        free(buffer);

        return success;

}

bool spifixedframe_bulk_read_frames_blocking(
    spi_inst_t* const spi,
    spifixedframe_t* const frames,
    const size_t frames_len) {

        assert(spi != NULL);
        assert(frames != NULL);
        assert(frames_len > 0);

        bool success = false;
        const spifixedframe_buffer_t outbuffer = 
            spifixedframe_to_buffer(&SPIFIXEDFRAME_NULL_FRAME);

        spifixedframe_buffer_t* const inbuffer = malloc(
            sizeof(spifixedframe_t) * frames_len);

        if(inbuffer == NULL) {
            return false;
        }

        success = ((size_t)spi_read16_blocking(spi, outbuffer, inbuffer, frames_len) 
            == frames_len);

        // if read succeeded, create the frames from the buffer
        if(success) {
            for(size_t i = 0; i < frames_len; ++i) {
                spifixedframe_from_buffer(&frames[i], inbuffer[i]);
            }
        }

        free(inbuffer);

        return success;

}

bool spifixedframe_write_chain_blocking(
    spi_inst_t* const spi,
    const spifixedframe_t* const frames,
    const size_t frames_to_write) {

        assert(spi != NULL);
        assert(frames != NULL);
        assert(frames_to_write > 0);
        assert(frames[0].is_first);

        return spifixedframe_bulk_write_frames_blocking(
            spi,
            frames,
            frames_to_write);

}

bool spifixedframe_read_chain_blocking(
    spi_inst_t* const spi,
    const size_t frames_to_read,
    spifixedframe_t* const frames) {

        assert(spi != NULL);
        assert(frames_to_read > 0);
        assert(frames != NULL);

        bool success = false;

        success = spifixedframe_wait_first_frame(
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

bool spifixedframe_wait_first_frame(
    spi_inst_t* const spi,
    spifixedframe_t* const first_frame) {

        assert(spi != NULL);
        assert(first_frame != NULL);

        spifixedframe_t temp;
        spifixedframe_get_null_frame(&temp);

        do {
            // ignore fails(?)
            spifixedframe_read_frame_blocking(spi, &temp);
        }
        while(!temp.is_first);

        *first_frame = temp;

        return true;

}
