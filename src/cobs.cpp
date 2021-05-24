/**
 * @file    cobs.cpp
 * @brief   Implementation file for the COBS library.
 * @author  Andreas Grommek
 * @version 1.0.0
 * @date    2021-05-21
 * 
 * @section license License
 * 
 * The MIT Licence (MIT)
 * 
 * Copyright (c) 2021 Andreas Grommek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @mainpage Arduino Library for Constistent Overhead Byte Stuffing
 *
 * @section intro_sec Introduction
 * This library implements the COBS algrorithm. 
 * 
 * Using this algorithm, one can transform a buffer of bytes into 
 * another buffer of bytes which does not contain any zero bytes, for 
 * the cost of (slightly) increased buffer length.
 *  
 * For more information, see here:
 * @li https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
 * @li http://www.stuartcheshire.org/papers/COBSforToN.pdf
 *
 * Note that there is nothing special about the "zero byte". The algorithm
 * works equally well for any other byte value. However, for reasons of 
 * simplicity the "special byte" is fixed to 0x00 and not a configurable
 * parameter.
 * 
 * @section author Author
 *
 * Andreas Grommek
 *
 * @section license License
 * 
 * The MIT Licence (MIT)
 * 
 * Copyright (c) 2021 Andreas Grommek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @version  1.0.0
 * @date     2021-05-23
 */
 
#include "cobs.h"

static const uint8_t zero_byte = 0x00;

/**
 * @brief  Calculate the maximum/worst case buffer size needed to hold the result of
 *         a COBS encoding run.
 * @param  input_size 
 *         number of bytes to be encoded with COBS
 * @param  with_trailing_zero 
 *         Takes into account if the encoder appends
 *         a trailing zero for packet delimiting purposes.
 *         This adds one byte to the worst-case-length.
 * @return maximum needed size of output buffer for the given input_size
 * @note   Miniumum overhead is at least one byte.
 * @note   Maximum overhead is one byte for every 254 input bytes.
 *         The overhead is less than worst case if the stretches in the input
 *         stream containing no zeros are shorter than 254 bytes.
 */
size_t getCOBSBufferSize(size_t input_size, bool with_trailing_zero) {
    size_t output_size = input_size + input_size / 254 + 1;
    if (with_trailing_zero) output_size++;
    return output_size;
}

// Macro for reducing code duplication. Only used in function encodeCOBS().
// ToDo: Use lambda expression?
#define FinishBlock(X) (*code_ptr = (X), code_ptr = output_buffer++, code = 0x01 )

/**
 * @brief  Encode a buffer of bytes using the COBS algorithm.
 * @param  input_buffer 
 *         pointer to buffer with bytes to encode
 * @param  input_length
 *         number of bytes to take from input buffer to encode
 * @param  output_buffer
 *         pointer to buffer to write encoded bytes to
 * @param  output_buffer_size
 *         the maximum size of the output buffer
 * @param  add_trailing_zero 
 *         when this is true, a zero byte will be appended
 *         to the output written to output_buffer. 
 * @return Number of bytes written to buffer output_buffer. 
 *         If output buffer may be to small (because it cannot 
 *         hold the maximum possible (i.e. worst case) number of 
 *         COBS encoded bytes, return 0. This signifies
 *         an error condition. No data was encoded in this case.
 *         A return value of 0 cannot occurr during normal operation
 *         because COBS adds at least one byte of overhead.
 * @note   Encoding time is more or less proportional to input buffer size.
 *         Depending on compiler optimization settings, encoding will take 
 *         about 0.34 to 0.41 microseconds per message byte on SAMD21 at 48
 *         MHz CPU clock or 0.12 to 0.14 microseconds per input byte on
 *         SAMD51 at 120 MHz.
 */
size_t encodeCOBS(uint8_t *input_buffer, size_t input_length, uint8_t *output_buffer, size_t output_buffer_size, bool add_trailing_zero) {
    // error checking: 
    // If size of output buffer is too small to receive the worst-case-sized
    // output of a COBS encoding, return 0.
    // During normal operation, the output size is at least 1 --> error 
    // condition can be detected reliably.
    if (output_buffer_size < getCOBSBufferSize(input_length, add_trailing_zero)) {
        return 0;
    }
    
    // pointer to memory address of last byte NOT to encode
    uint8_t *input_buffer_end = input_buffer + input_length;
    
    // Pointer into destination array where to store the extra bytes.
    // We scan forward, thus we have to remember where to put the 
    // proper code when we finally encounter a value of zero (or a run
    // of more than 254 non-zero values). 
    // The very first byte of the output will be a code byte. Initialize accordingly.
    uint8_t *code_ptr = output_buffer;
    
    // needed for output size calculation
    uint8_t *output_start = output_buffer;
    
    // Move pointer to output one step ahead. The first encoded byte 
    // will be at this position.
    output_buffer++;
    
    // Initial code is at least 0x01 - there must be never a code
    // of 0x00, as we want to eliminate all zeros from the stream,
    // after all.
    // This variable increments for each value which is not 0x00.
    uint8_t code = 0x01;
    
    while (input_buffer < input_buffer_end) {
        if (*input_buffer == zero_byte) {
            FinishBlock(code);
        }
        else {
            *output_buffer = *input_buffer; // copy value verbatim
            output_buffer++;                // select next slot in output
            code++;                         // advance code
            // if code gets too large, create a new block
            if (code == 0xFF) FinishBlock(code);
        }
        // advance pointer into source array
        input_buffer++;
    }
    FinishBlock(code);
    // optionally write a closing 0x00 as final delimiter to the output stream
    if (add_trailing_zero) {
        *output_buffer = zero_byte; 
        output_buffer++;
    }
    return static_cast<size_t>((output_buffer-output_start)-1); 
}

/**
 * @brief  Decode a buffer of bytes encoded with the COBS algorithm.
 * @param  encoded_input 
 *         Pointer to buffer with COBS encoded bytes to decode. The 
 *         buffer MUST contain a zero byte at the end of the COBS
 *         encoded stream.
 * @param  max_input_length
 *         Maximum number of bytes to take from input buffer to encode.
 *         This can be the maximum number of bytes the buffer can hold if
 *         and only if the encoded bytes contain a trailing zero byte as 
 *         delimiter. The COBS algorithm then figures out by itself when 
 *         decoding is finished.
 *         If the encoded byte stream does not contain a trailing zero
 *         byte as delimiter, this must be the exact number of bytes 
 *         to decode.
 *         However, it is always better to specify the exact number of 
 *         bytes one expects to decode. A malformed code byte in the encoded 
 *         stream might otherwise cause missing the trailing zero byte, resulting
 *         in decoding garbage.
 * @param  decoded_output
 *         Pointer to buffer into which to write the decoded bytes.
 *         If the size of decoded_output is equal or greater than 
 *         max_input_length, the decoding operation is safe, as the
 *         size of decoded output is at least one byte less than the input.
 * @param  max_output_length
 *         Maximum number of bytes the output buffer can hold. If this
 *         buffer is too small to hold the worst-case decoded size (i.e.
 *         smaller than one byte less than the input stream), nothing is
 *         written to the output buffer and 0 is returned. 
 * @return Number of bytes written to buffer decoded_output. 
 *         A number of 0 written bytes signals an error condition.
 * @note   This function does not check, if there really are no zero bytes 
 *         within the COBS-encoded blocks (except as delimiter at the end),
 *         i.e. that we have a valid and pure COBS-encode byte stream.
 *         This is a design decision. It is better to decode the whole stream
 *         than only delivering a part of the decoding. At least the decoded 
 *         byte stream will have the expected/correct length (more or less). 
 *         Integrity checking (i.e. hashing, CRCs, etc.) must be performed
 *         afterwards on the decoded bytes, if necessary.
 * @note   Decoding is more or less proportional to message size and at
 *         least twice as fast as endoding.
 *         Depending on compiler optimization settings, decoding will take 
 *         about 0.13 to 0.22 microseconds per message byte on SAMD21 at 48
 *         MHz CPU clock or 0.02 to 0.0.07 microseconds per input byte on
 *         SAMD51 at 120 MHz.
 */
size_t decodeCOBS(uint8_t *encoded_input, size_t max_input_length, uint8_t *decoded_output, size_t max_output_length) {
    // sanity checking buffer sizes
    if (max_input_length == 0 || max_output_length == 0) {
        return 0;
    }
    // output_buffer must be able to hold at least (max_input_length - 1) bytes.
    if ( max_output_length < (max_input_length - 1)) {
        return 0;
    }
    
    // memory address of starting element in output buffer
    // needed to calculate the number of decoded bytes
    uint8_t *output_buffer_start =  decoded_output;
    
    // memory address of byte *after* the last byte to decode
    uint8_t *input_buffer_end = encoded_input + max_input_length;

    // Note: This is not really safe!! 
    // If input data is not well-formed and there are zero bytes within
    // the byte stream (other than the delimiter byte) and/or the code byte
    // got corrupted, there could be *encoded_input == 0 during inner loop.
    // Not really a problem. We prevent the potential buffer overflow, but do
    // nothing against corrupted data.
    while (encoded_input < input_buffer_end && *encoded_input != 0) {
        uint8_t code = *encoded_input;
        // Sanity-check value of code to prevent buffer overflow.
        // If code is manipulated/corrupted and too big, it would be possible to read more bytes 
        // than specified by max_input_length.
        // --> Limit code to the rest of the available bytes in buffer.
        if (code > input_buffer_end - encoded_input) {
            code = input_buffer_end - encoded_input;
        }
        encoded_input++;
        // copy code-1 number of bytes verbatim from input to output
        for (uint8_t i=1; i < code; i++) {
            *decoded_output = *encoded_input;
            encoded_input++;
            decoded_output++;
        }
/*
        // Alternative implementation using memcpy instead of loop.
        // This might be slightly faster, but with keeping the loop version it
        // it is possible to use the same buffer as input and output using offsets.
        uint8_t code_minus_1 = code - 1;
        memcpy(decoded_output, encoded_input, code_minus_1);
        encoded_input += code_minus_1;
        decoded_output += code_minus_1;
*/
        // Add zero to output... but only when code is not special value of 0xFF. 
        // 0xFF means, that there were at least 254 consecutive not-zero bytes.
        // --> Do NOT add a zero to the output stream!
        if (code < 0xFF) {
            *decoded_output = zero_byte;
            decoded_output++;
        }
    } // end of while(), decoding complete
    return static_cast<size_t>(decoded_output - output_buffer_start - 1);
}
