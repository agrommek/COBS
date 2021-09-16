/**
 * @file    cobs.cpp
 * @brief   Implementation file for the COBS library.
 * @author  Andreas Grommek
 * @version  1.1.0 (complete rewrite)
 * @date     2021-09-16
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
 * This library implements the COBS algorithm. 
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
 * @version  1.1.0 (complete rewrite)
 * @date     2021-09-16
 */
 
#include "cobs.h"

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
#define FinishBlock(X) (*code_ptr = (X), code_ptr = outptr++, code = 0x01 )

/**
 * @brief  Encode a buffer of bytes using the COBS algorithm and store
 *         the result in @b another buffer.
 * @param  inptr 
 *         pointer to buffer with bytes to encode
 * @param  inputlen
 *         number of bytes to take from input buffer to encode
 * @param  outptr
 *         pointer to buffer to write encoded bytes to
 * @param  outlen
 *         the maximum size of the output buffer
 * @param  add_trailing_zero 
 *         when this is true, a zero byte will be appended
 *         to the output written to output_buffer. 
 * @return Number of bytes written to buffer outptr. 
 *         If output buffer may be to small (because it cannot 
 *         hold the maximum possible (i.e. worst case) number of 
 *         COBS encoded bytes, return 0. This signifies
 *         an error condition. No data was encoded in this case.
 *         A return value of 0 cannot occurr during normal operation
 *         because COBS adds at least one byte of overhead.
 * @note   Encoding time is more or less proportional to input buffer size.
 */
size_t encodeCOBS(const uint8_t *inptr, size_t inputlen, uint8_t *outptr, size_t outlen, bool add_trailing_zero) {
    if (outlen < getCOBSBufferSize(inputlen, add_trailing_zero)) {
        return 0;
    }
    const uint8_t *inptr_end = inptr + inputlen;
    const uint8_t *output_start = outptr;
    uint8_t *code_ptr = outptr;
    outptr++;
    uint8_t code = 0x01;
    
    while (inptr < inptr_end) {
        if (*inptr == 0x00) {
            FinishBlock(code);
        }
        else {
            *outptr = *inptr; // copy value verbatim
            outptr++;         // select next slot in output
            code++;           // advance code
            // if code gets too large, create a new block
            // but only when we are not about to finish with a block of 254 consecutive non-zero bytes
            if (code == 0xFF && (inptr_end - inptr > 1)) FinishBlock(code);
        }
        inptr++;
    }
    *code_ptr = code;
    if (add_trailing_zero) {
        *outptr = 0x00; 
        outptr++;
    }
    return static_cast<size_t>((outptr-output_start));
}

/**
 * @brief  Decode a buffer of bytes encoded with the COBS algorithm and 
 *         store the result in @b another buffer.
 * @param  inptr 
 *         Pointer to buffer with COBS encoded bytes to decode. The 
 *         buffer MUST contain a zero byte at the end of the COBS
 *         encoded stream.
 * @param  inputlen
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
 * @param  outptr
 *         Pointer to buffer into which to write the decoded bytes.
 *         If the size of decoded_output is equal or greater than 
 *         inputlen, the decoding operation is safe, as the
 *         size of decoded output is at least one byte less than the input.
 * @param  outputlen
 *         Maximum number of bytes the output buffer can hold. If this
 *         buffer is too small to hold the worst-case decoded size (i.e.
 *         smaller than one byte less than the input stream), nothing is
 *         written to the output buffer and 0 is returned. 
 * @return Number of bytes written to buffer outptr. 
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
 */
size_t decodeCOBS(const uint8_t *inptr, size_t inputlen, uint8_t *outptr, size_t outputlen) {
    if (inputlen < 2 || outputlen == 0) {
        return 0;
    }
    if ( outputlen < (inputlen - 1)) {
        return 0;
    }
    
    const uint8_t *outptr_start =  outptr;
    const uint8_t *last_element = ( inptr[inputlen-1] == 0) ? 
        inptr + inputlen - 1 : 
        inptr + inputlen ;
    
    while (true) {
        // read code
        uint8_t code = *inptr;
        if (inptr + code > last_element) {
            code = last_element - inptr;
        }
        inptr++;
        // copy code-1 number of bytes verbatim from input to output
        for (uint8_t i=1; i < code; i++) {
            *outptr = *inptr;
            inptr++;
            outptr++;
        }
        // break when finished
        if (inptr >= last_element) break;
        // append 0 if necessary
        if (code < 0xFF) {
            *outptr = 0x00;
            outptr++;
        }
    } // end of while(), decoding complete
    return static_cast<size_t>(outptr - outptr_start);
}

/**
 * @brief  Decode a buffer of bytes encoded with the COBS algorithm and 
 *         store the result in the @b same buffer.
 * @param  inptr 
 *         Pointer to buffer with COBS encoded bytes to decode. The 
 *         buffer MUST contain a zero byte at the end of the COBS
 *         encoded stream.
 * @param  inputlen
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
 * @return Number of bytes written back to inptr. 
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
 */
size_t decodeCOBS_inplace(uint8_t *inptr, size_t inputlen) {
    if (inputlen < 2) return 0;
    
    uint8_t *outptr = inptr;
    const uint8_t *outptr_start =  inptr;
    const uint8_t *last_element = ( inptr[inputlen-1] == 0) ? 
        inptr + inputlen - 1 : 
        inptr + inputlen ;
    
    while (true) {
        // read code
        uint8_t code = *inptr;
        if (inptr + code > last_element) {
            code = last_element - inptr;
        }
        inptr++;
        // copy code-1 number of bytes verbatim from input to output
        for (uint8_t i=1; i < code; i++) {
            *outptr = *inptr;
            inptr++;
            outptr++;
        }
        // break when finished
        if (inptr >= last_element) break;
        // append 0 if necessary
        if (code < 0xFF) {
            *outptr = 0x00;
            outptr++;
        }
    } // end of while(), decoding complete
    return static_cast<size_t>(outptr - outptr_start);
}
