/**
 * @file    cobs.h
 * @brief   Header file for the COBS library
 * @author  Andreas Grommek
 * @version 1.0.0
 * @date    2021-05-23
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

#ifndef ConsistentOverheadByteStuffing_h
#define ConsistentOverheadByteStuffing_h

/* 
 * Do not include Arduino.h to keep library usable for other platforms.
 * Include only standard C++ headers instead.
*/
#include <stddef.h>  // needed for size_t data type
#include <stdint.h>  // needed for uint8_t data type

size_t getCOBSBufferSize(size_t input_size,
                         bool   with_trailing_zero=true);

size_t encodeCOBS(uint8_t *input_buffer,
                  size_t   input_length, 
                  uint8_t *output_buffer,
                  size_t   output_buffer_size,
                  bool     add_trailing_zero=true);

size_t decodeCOBS(uint8_t *encoded_input, 
                  size_t   max_input_length,
                  uint8_t *decoded_output,
                  size_t   max_output_length);
#endif
