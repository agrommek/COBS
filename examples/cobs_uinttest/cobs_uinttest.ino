/*
  Consisten Overhead Byte Stuffing (COBS)

  This example runs some COBS encoding and decoding on known byte patterns.
  Everything if fine when each tests reports "OK".
    
  This example code is in the public domain.
*/

#include "cobs.h"

// helper function to print byte-wise comparison of two buffers
void print_byte_comparison(const uint8_t *buf1, const uint8_t *buf2, size_t len) {
    for (size_t i=0; i<len; i++) {
        Serial.print(i, DEC);
        Serial.print(F(": 0x"));
        if (buf1[i] < 16) Serial.print(F("0"));
        Serial.print(buf1[i], HEX);
        Serial.print(F("-0x"));
        if (buf2[i] < 16) Serial.print(F("0"));
        Serial.print(buf2[i], HEX);
        Serial.println();
    } 
}

// helper function to test the functions encodeCOBS(), decodeCOBS() and decodeCOBS_inplace() from cobs.h
void run_COBS_test(uint8_t *plain, size_t plain_length, uint8_t *encoded, size_t encoded_length, bool with_trailing_zero=true) {
    const size_t result_maxlength = getCOBSBufferSize(plain_length, with_trailing_zero);
    
    Serial.print(F("allocating buffer of size "));
    Serial.print(result_maxlength, DEC);
    Serial.println();
    
    uint8_t resultbuffer[result_maxlength];
    size_t len;
    
    // calculate COBS encoding of input
    len = encodeCOBS(plain, plain_length, resultbuffer, sizeof(resultbuffer), with_trailing_zero);
    Serial.print(F("encoding to separate buffer: "));
    if ((encoded_length == len) && (memcmp(encoded, resultbuffer, len) == 0)) {
        Serial.println(F("OK"));
    }
    else {
        Serial.println(F("failed!"));
        Serial.print(F("length of calculated result: "));
        Serial.println(len, DEC);;
        Serial.print(F("length of expected result:   "));
        Serial.println(encoded_length, DEC);
        Serial.println(F("calculated-expected: "));
        print_byte_comparison(resultbuffer, encoded, len);
        Serial.println();
    }
    
    // calculate COBS decoding of output in separate buffer
    len = decodeCOBS(encoded, encoded_length, resultbuffer, sizeof(resultbuffer));
    Serial.print(F("decoding to separate buffer: "));
    if ((plain_length == len) && (memcmp(plain, resultbuffer, len) == 0)) {
        Serial.println(F("OK"));
    }
    else {
        Serial.println(F("failed!"));;
        Serial.print(F("length of calculated result: "));
        Serial.println(len, DEC);
        Serial.print(F("length of expected result:   "));
        Serial.println(plain_length, DEC);
        Serial.println(F("calculated-expected: "));
        print_byte_comparison(resultbuffer, plain, len);
        Serial.println();
    }

    // calculate COBS decoding of output in same buffer
    // make a copy of it before 
    uint8_t encoded_copy[encoded_length];
    memcpy(encoded_copy, encoded, encoded_length);
    
    len = decodeCOBS_inplace(encoded_copy, encoded_length);
    Serial.print(F("decoding to same buffer:     "));
    if ((plain_length == len) && (memcmp(plain, encoded_copy, len) == 0)) {
        Serial.println(F("OK"));
    }
    else {
        Serial.println(F("failed!"));
        Serial.print(F("length of calculated result: "));
        Serial.println(len, DEC);
        Serial.print(F("length of expected result:   "));
        Serial.println(plain_length);
        Serial.println(F("calculated-expected: "));
        print_byte_comparison(encoded_copy, plain, len);
        Serial.println();
    }
    Serial.println();
    return;
}


void setup() {

    Serial.begin(115200);
    while (!Serial);

    Serial.println(F("\nStarting COBS unit test...\n"));

    // run each test twice, first with trailing zeros on encoded stream, then without.
    bool with_zero[] = {true, false};

    for (uint8_t i=0; i<2; i++) {
        
        bool with_trailing_zero = with_zero[i];

        if (with_trailing_zero) {
            Serial.println(F("\n *** Running tests WITH trailing zero on encoded data ***\n"));
        }
        else {
            Serial.println(F("\n *** Running tests WITHOUT trailing zero on encoded data ***\n"));
        }

        // Note: Each example is written WITH trailing zero byte. For the tests WITHOUT trailing zero byte, the
        // output stream is simply truncated by one byte.

        // run 10 examples from https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing#Encoding_examples
    
        // example 01
        {
            uint8_t input[] = {0x00};
            uint8_t output[] = {0x01, 0x01, 0x00};
            Serial.println(F("Running example 01:"));
            run_COBS_test(input, sizeof(input), output, sizeof(output)-i, with_trailing_zero);
        }
    
        // example 02
        {
            uint8_t input[] = {0x00, 0x00};
            uint8_t output[] = {0x01, 0x01, 0x01, 0x00};
            Serial.println(F("Running example 02:"));
            run_COBS_test(input, sizeof(input), output, sizeof(output)-i, with_trailing_zero);
        }
        
        // example 03
        {
            uint8_t input[] = {0x11, 0x22, 0x00, 0x33};
            uint8_t output[] = {0x03, 0x11, 0x22, 0x02, 0x33, 0x00};
            Serial.println(F("Running example 03:"));
            run_COBS_test(input, sizeof(input), output, sizeof(output)-i, with_trailing_zero);
        }
        
        // example 04
        {
            uint8_t input[] = {0x11, 0x22, 0x33, 0x44};
            uint8_t output[] = {0x05, 0x11, 0x22, 0x33, 0x44, 0x00};
            Serial.println(F("Running example 04:"));
            run_COBS_test(input, sizeof(input), output, sizeof(output)-i, with_trailing_zero);
        }
        
        // example 05
        {
            uint8_t input[] = {0x11, 0x00, 0x00, 0x00};
            uint8_t output[] = {0x02, 0x11, 0x01, 0x01, 0x01, 0x00};   
            Serial.println(F("Running example 05:"));
            run_COBS_test(input, sizeof(input), output, sizeof(output)-i, with_trailing_zero);
        }
        
        // example 06
        {
            // 0x01 .. 0xFE --> input ends with 254 consecutive bytes --> optimized end of decode!!
            uint8_t input[254];
            for (size_t i=0; i<sizeof(input); i++) input[i] = i+1;
            
            uint8_t output[256];
            output[0] = 0xFF;
            for (size_t i=1; i<=254; i++) output[i] = i;
            output[255] = 0x00; // trailing zero

            Serial.println(F("Running example 06:"));
            run_COBS_test(input, sizeof(input), output, sizeof(output)-i, with_trailing_zero);
        }
        
        // example 07
        {
            // 0x00 .. 0xFE --> 255 bytes
            uint8_t input[255];
            for (size_t i=0; i<255; i++) input[i] = i;
            
            uint8_t output[257];
            output[0] = 0x01;
            output[1] = 0xFF;
            for (size_t i=2; i<=255; i++) output[i] = i-1;
            output[256] = 0x00; // trailing zero

            Serial.println(F("Running example 07:"));
            run_COBS_test(input, sizeof(input), output, sizeof(output)-i, with_trailing_zero);
        }
        
        // example 08
        {
            // 0x01 .. 0xFF --> 255 bytes, all non-zero
            uint8_t input[255];
            for (size_t i=0; i<255; i++) input[i] = i+1;
            
            uint8_t output[258];
            output[0] = 0xFF;   // code byte
            for (size_t i=1; i<255; i++) output[i] = i; // 254 data bytes, 0x01 .. 0xFE
            output[255] = 0x02; // code byte
            output[256] = 0xFF; // last data byte
            output[257] = 0x00; // trailing zero

            Serial.println(F("Running example 08:"));
            run_COBS_test(input, sizeof(input), output, sizeof(output)-i, with_trailing_zero);
        }
    
        // example 09
        {
            // 0x02 .. 0xFF, 0x00 --> 255 bytes, last byte is a zero
            uint8_t input[255];
            for (size_t i=0; i<254; i++) input[i] = i+2; // 0x02..0xFF
            input[254] = 0x00;
    
            uint8_t output[258];
            output[0] = 0xFF; // code byte
            for (size_t i=1; i<255; i++) output[i] = i+1; // 254 data bytes, 0x02..0xFF
            output[255] = 0x01; // overhead code byte
            output[256] = 0x01; // regular code byte
            output[257] = 0x00; // trailing zero

            Serial.println(F("Running example 09:"));
            run_COBS_test(input, sizeof(input), output, sizeof(output)-i, with_trailing_zero);
        }
           
        // example 10
        {
            // 0x03..0xFF, 0x00, 0x01 --> 255 bytes
            uint8_t input[255];
            for (size_t i=0; i<253; i++) input[i] = i+3; // 0x03..0xFF, 253 bytes
            input[253] = 0x00;
            input[254] = 0x01;
            
            uint8_t output[257];
            output[0] = 0xFE; // code byte
            for (size_t i=1; i<254; i++) output[i] = i+2; // 0x03..0xFF, 253 bytes
            output[254] = 0x02; // code byte
            output[255] = 0x01; // data byte
            output[256] = 0x00; // trailing zero

            Serial.println(F("Running example 10:"));
            run_COBS_test(input, sizeof(input), output, sizeof(output)-i, with_trailing_zero);
        }
    
        // run example from COBS paper: http://www.stuartcheshire.org/papers/COBSforToN.pdf
        // example 11 
        {
            uint8_t input[] =  {0x45, 0x00, 0x00, 0x2C, 0x4C, 0x79, 0x00, 0x00, 0x40, 0x06, 0x4F, 0x37};
            uint8_t output[] = {0x02, 0x45, 0x01, 0x04, 0x2C, 0x4C, 0x79, 0x01, 0x05, 0x40, 0x06, 0x4F, 0x37, 0x00};
            Serial.println(F("Running example 11:"));
            run_COBS_test(input, sizeof(input), output, sizeof(output)-i, with_trailing_zero);
        }
    }
    Serial.println(F("COBS unit test done!"));
    
}

void loop() {}
