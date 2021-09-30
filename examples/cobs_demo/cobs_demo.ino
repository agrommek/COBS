/*
  Consisten Overhead Byte Stuffing (COBS)

  This example shows how to encode and decode an array of bytes using the COBS
  algorithm.

  The example does several things:
    - show the usage of the library functions
    - verify the correct working of the library function, i.e. can be used as
      a unit test
    - runs a benchmark for encoding and decoding functions
    
  This example code is in the public domain.
*/

#include "cobs.h"

// determine the message size in bytes
const size_t MESSAGE_SIZE = 64;

// The message buffer is filled with pseudo-random bytes.
// Chose the random seed so that the message contains some zero bytes
// to better observer the effect of the COBS encoding.
//const uint32_t RANDOM_SEED = 1252; // for AVR
const uint32_t RANDOM_SEED = 1241; // for ARM

// Use this value as the maximum generated value in randomly generated
// message. The lower this value is chosen, the more zeros are contained
// in the message.
const uint8_t MAX_RANDOM_VALUE = 255;

// define if you want to add a trailing zero byte or not
const bool ADD_ZERO_BYTE = false;

// A little helper function to print out byte buffers in a readable format:
// Print HEX numbers with leading zero, separated by space, 16 bytes in a row
void print_buffer(uint8_t *buf, size_t len) {
    for (size_t i=0; i<len; i++) {
        if (buf[i] < 16) {
            Serial.print("0");
        }
        Serial.print(buf[i], HEX);
        Serial.print(" ");
        if ((i+1) % 16 == 0) Serial.println();
    }
    Serial.println();
}

void setup() {
    
    Serial.begin(115200);
    while(!Serial);

    Serial.println("\n\nStarting COBS test...");

    // Allocate buffer for unencoded data, i.e. the plain message.
    uint8_t message_buffer[MESSAGE_SIZE] = {0};
    const size_t message_buffer_size = sizeof(message_buffer);

    // Determine necessary size of buffer to hold the COBS encoded data
    const size_t ENCODED_MESSAGE_SIZE = getCOBSBufferSize(MESSAGE_SIZE, ADD_ZERO_BYTE);

    // Allocate buffer big enough to hold result of COBS encoding.
    uint8_t cobs_buffer[ENCODED_MESSAGE_SIZE] = {0};
    const size_t cobs_buffer_size = sizeof(cobs_buffer);

    // Allocate buffer big enough to hold original message.
    // Make it waaaay too big for testing, so it can hold the data if something goes wrong.
    // Ususally, a size of MESSAGE_SIZE is sufficient.
    uint8_t decoded_buffer[2 * MESSAGE_SIZE] = {0};
    const size_t decoded_buffer_size = sizeof(decoded_buffer);

    // Create message by filling some pseudo-random bytes into message buffer.
    // Select random seed above, so there are some zero bytes in the message.
    randomSeed(RANDOM_SEED);
    size_t number_of_zeros = 0;
    for (size_t i=0; i<message_buffer_size; i++) {
        message_buffer[i] = random(0, static_cast<uint32_t>(MAX_RANDOM_VALUE)+1);
        if (message_buffer[i] == 0) number_of_zeros++;
    }

    // show content of message
    Serial.println();
    Serial.print("content of message_buffer/message (");
    Serial.print(message_buffer_size, DEC);
    Serial.print(" bytes):");
    Serial.println();
    print_buffer(message_buffer, message_buffer_size);

    // show the number of zeros in the message
    // every zero in the message gets replaces by COBS encoding
    Serial.print("message_buffer contains ");
    Serial.print(number_of_zeros, DEC);
    Serial.print(" zero byte(s).");
    Serial.println();
    
    // encode message with COBS, write resulting bytes to cobs_buffer
    // benchmark the encoding time in the process
    Serial.println();
    if (ADD_ZERO_BYTE) {
        Serial.println("Encoding message WITH trailing zero byte.");
    }
    else {
        Serial.println("Encoding message WITHOUT trailing zero byte.");
    }

    // declare variable to hold time stamps for benchmarking
    uint32_t timestamp_start;
    uint32_t timestamp_stop;
    uint32_t delta;
    
    timestamp_start = micros();
    size_t encoded_size = encodeCOBS(message_buffer, message_buffer_size, cobs_buffer, cobs_buffer_size, ADD_ZERO_BYTE);
    timestamp_stop = micros();
    delta = timestamp_stop - timestamp_start;

    /*
     * Note for not-so-experienced C/C++ programmers: 
     * 
     * The following form to call encodeCOBS() is 100% equivalent:
     * 
     *   encodeCOBS(&message_buffer[0], message_buffer_size, &cobs_buffer[0], cobs_buffer_size, ADD_ZERO_BYTE);
     * 
     * Using this form makes it possible to start reading from and writeing to buffer positions other then 0. 
     */


    // print results of encoding
    Serial.print("content of cobs_buffer (");
    Serial.print(encoded_size, DEC);
    Serial.print(" bytes of data, ");
    Serial.print(cobs_buffer_size, DEC);
    Serial.print(" bytes max):");
    Serial.println();
    print_buffer(cobs_buffer, encoded_size);
    Serial.print("Encoding took ");
    Serial.print(delta, DEC);
    Serial.print(" microseconds. That is ");
    Serial.print( ((float)delta)/((float)message_buffer_size), 4);
    Serial.print(" microseconds per message byte.");
    Serial.println();

    // select if we want to tell the algorithm the exact length of the encoded byte stream
    // or if we want it to figure out the end by itself.
    // The algrorithm can *ONLY* figure out the end by itself if a trailing zero byte was added during encode.
    size_t input_buffer_size;
    input_buffer_size = encoded_size;
//    input_buffer_size = cobs_buffer_size; // send whole buffer, try to find trailing zero automatically

    // decode COBS data to plain message again
    // benchmark the decoding time in the process
    Serial.println();
    timestamp_start = micros();
    size_t decoded_size = decodeCOBS(cobs_buffer, input_buffer_size, decoded_buffer, decoded_buffer_size);
    timestamp_stop = micros();
    delta = timestamp_stop - timestamp_start;
    Serial.print("content of decoded_buffer (");
    // print results
    Serial.print(decoded_size, DEC);
    Serial.print(" bytes of data, ");
    Serial.print(decoded_buffer_size, DEC);
    Serial.print(" bytes max):");
    Serial.println();
    print_buffer(decoded_buffer, decoded_size);
    Serial.print("Decoding took ");
    Serial.print(delta, DEC);
    Serial.print(" microseconds. That is ");
    Serial.print( ((float)delta)/((float)decoded_size), 4);
    Serial.print(" microseconds per message byte.");
    Serial.println();

    // check validity - input and output sizes
    Serial.println();
    if (message_buffer_size == decoded_size) {
        Serial.println("Sizes of unencoded and decoded buffers match. Good!");
    }
    else {
        Serial.println("Sizes of unencoded and decoded buffers do NOT match. This is not good...");
    }

    // check byte for byte
    bool buffers_have_same_content = true;
    for (size_t i=0; i<decoded_size; i++) {
        if (message_buffer[i] != decoded_buffer[i]) {
            buffers_have_same_content = false;
            Serial.print("Buffer elements ");
            Serial.print(i, DEC);
            Serial.print(" do NOT match: 0x");
            if (message_buffer[i] < 16) Serial.print("0");
            Serial.print(message_buffer[i], HEX);
            Serial.print(" and 0x");
            if (decoded_buffer[i] < 16) Serial.print("0");
            Serial.print(decoded_buffer[i], HEX);
            Serial.println();
        }
    }
    if (buffers_have_same_content) {
        Serial.println("Original message and decoded bufffer are identical. Good!");
    }
    else {
        Serial.println("Original message and decoded buffers are NOT identical. This is not good...");
    }

} // end of setup()

// do nothing in loop()
void loop() {
    delay(10);
}
