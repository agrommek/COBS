# Library for Consistent Overhead Byte Stuffing (COBS)

## What is it for?

This library implements the COBS algorithm.

As the name says, the algorithm does "byte stuffing". Byte stuffing means, that a byte in a stream or buffer is consistently and reversably replaced by another pattern. This is useful, for example, if one has to transmit messages over a stream-like protocol and wants/needs to sepearate messages by a delimiter. Candidates for that are plain serial protocols (RS-232, UART, etc.) and TCP. The problem with the delimiter approach: What if the delimiter can occurr within the message itself? Byte stuffing algorithms solve that problem by replacing the "delimiting symbol" by other symbols at the cost of some overhead (both in terms of encoding/decoding time and number of bytes to transmit).

For mor information on the COBS algorithm, take a look at the [original paper](http://www.stuartcheshire.org/papers/COBSforToN.pdf) or the [Wikipedia article](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing).

## Overhead

The algorithm introduces an overhead of at least one byte. However, the worst-case-overhead is small and easily calculated beforehand. The worst case overhead is at most one byte for every 254 bytes encoded.

## Which platform does it run on?

The library was written (and and is used by me) mainly for the Arduino platform. There, I use it to send binary messages of different lengths over a serial connections.

However, the code is pure C++. There is nothing in the code that prevents the library from being used on any other platform. No Arduino-specific header files are used. No special compiler flags are needed. There is no dependence on endianness or something like that.

## Usage

### Encoding bytes

`encodeCOBS(uint8_t *input_buffer, size_t input_length, uint8_t *output_buffer, size_t output_buffer_size, bool add_trailing_zero=true))` 

Use the function `encodeCOBS()` to encode bytes in the array `input_buffer` and save the result in array `output_buffer`. The buffer sizes are needed for error checking. The boolean argument `add_trailing_zero` adds a zero byte (i.e. a delimiter byte) to the end of the encoded bytes. Usually, the algorithm does not add this delimiter. However, we would not do COBS encoding if we wouldn't want to send the data over some un-delimited connection. This flag is there for practical convenience.

The function returns the number of bytes written to the output buffer. A number of 0 bytes indicates an error condition.

### Decoding bytes

`size_t decodeCOBS(uint8_t *encoded_input, size_t max_input_length, uint8_t *decoded_output, size_t)`

Use the function `decodeCOBS()` to convert a buffer with COBS encoded bytes back to the original message. The algorithm notices by itself when the decoding is complete if and only if the COBS-encoded bytes in input_buffer are...

1. ... well-formed, i.e. not corrupted or manipulated
1. ... terminated by the separator byte (i.e. 0x00).

It is best to use the actual number of bytes to decode *in* input buffer as `max_input_length` rather than the total maximum size *of* the input buffer. The function will never write back more than `max_input_length` bytes to `decoded_output`.

The function returns the number of bytes written to the output buffer. A number of 0 bytes indicates an error condition, most probably due to a too-small output buffer.

### Helper functions

`size_t getCOBSBufferSize(size_t input_size, bool with_trailing_zero=true)`

Use the function `getCOBSBufferSize()` to calculate the necessary buffer size for the COBS-encoded output given the size of the message. COBS adds some bytes as overhead, thus the ouput buffer for the encoding must be larger than the original message. Depending on the exact message content, the overhead can be a little more or less. The boolean flag `with_trailing_zero` takes into account if we need an output buffer big enough to also hold an additional delimiter or not.

