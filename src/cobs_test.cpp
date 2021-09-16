/**
 * @file    cobs_test.cpp
 * @brief   Unit test for the COBS library.
 * @author  Andreas Grommek
 * @version 1.1.0 (complete rewrite)
 * @date    2021-09-16
 * 
 */

#include <iostream>
#include "cobs.h"
#include <string.h>

using namespace std;
/**
 * @brief  Check correctnes of COBS encoding and decoding functions.
 *         Used for unit test.
 */
void compare(uint8_t *plain, size_t plain_length, uint8_t *encoded, size_t encoded_length, bool with_trailing_zero=true) {

    const size_t result_maxlength = getCOBSBufferSize(plain_length, with_trailing_zero);
    cout << "allocating buffer of size " << std::dec << result_maxlength << endl;
    uint8_t resultbuffer[result_maxlength];
    size_t len;
    
    // calculate COBS encoding of input
    len = encodeCOBS(plain, plain_length, resultbuffer, sizeof(resultbuffer), with_trailing_zero);
    cout << "encoding to separate buffer: ";
    if ((encoded_length == len) && (memcmp(encoded, resultbuffer, len) == 0)) {
        cout << "OK" << endl;
    }
    else {
        cout << "failed!" << endl;
        cout << "length of calculated result: " << std::dec << len << endl;
        cout << "length of expected result:   " << std::dec << encoded_length << endl;
        cout << "calculated-expected: ";
        for (size_t i=0; i<len; i++) {
            cout << std::dec << i << ":" << std::hex << "0x" << (int)resultbuffer[i] << "-0x" << (int)encoded[i] << " ";
        }
        cout << endl;
    }
    
    // calculate COBS decoding of output in separate buffer
    len = decodeCOBS(encoded, encoded_length, resultbuffer, sizeof(resultbuffer));
    cout << "decoding to separate buffer: ";
    if ((plain_length == len) && (memcmp(plain, resultbuffer, len) == 0)) {
        cout << "OK" << endl;
    }
    else {
        cout << "failed!" << endl;
        cout << "length of calculated result: " << std::dec << len << endl;
        cout << "length of expected result:   " << std::dec << plain_length << endl;
        cout << "calculated-expected: ";
        for (size_t i=0; i<len; i++) {
            cout << std::dec << i << ":" << std::hex << "0x" << (int)resultbuffer[i] << "-0x" << (int)plain[i] << " ";
        }
        cout << endl;
    }    

    // calculate COBS decoding of output in same buffer
    len = decodeCOBS_inplace(encoded, encoded_length);
    cout << "decoding to same buffer:     ";
    if ((plain_length == len) && (memcmp(plain, encoded, len) == 0)) {
        cout << "OK" << endl;
    }
    else {
        cout << "failed!" << endl;
        cout << "length of calculated result: " << std::dec << len << endl;
        cout << "length of expected result:   " << std::dec << plain_length << endl;
        cout << "calculated-expected: ";
        for (size_t i=0; i<len; i++) {
            cout << std::dec << i << ":" << std::hex << "0x" << (int)encoded[i] << "-0x" << (int)plain[i] << " ";
        }
        cout << endl;
    }  
    return;
}

int main() {
    const bool with_trailing_zero = true;
    const size_t sub = (with_trailing_zero) ? 0 : 1;
    
    // 10 examples from https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing#Encoding_examples
    
    uint8_t input1[] = {0x00};
    uint8_t output1[] = {0x01, 0x01, 0x00};

    uint8_t input2[] = {0x00, 0x00};
    uint8_t output2[] = {0x01, 0x01, 0x01, 0x00};

    
    uint8_t input3[] = {0x11, 0x22, 0x00, 0x33};
    uint8_t output3[] = {0x03, 0x11, 0x22, 0x02, 0x33, 0x00};

    uint8_t input4[] = {0x11, 0x22, 0x33, 0x44};
    uint8_t output4[] = {0x05, 0x11, 0x22, 0x33, 0x44, 0x00};    
    
    uint8_t input5[] = {0x11, 0x00, 0x00, 0x00};
    uint8_t output5[] = {0x02, 0x11, 0x01, 0x01, 0x01, 0x00};   

    // 0x01 .. 0xFE --> input ends with 254 consecutive bytes --> optimized end of decode!!
    uint8_t input6[254];
    for (size_t i=0; i<sizeof(input6); i++) input6[i] = i+1;
    uint8_t output6[256];
    output6[0] = 0xFF;
    for (size_t i=1; i<=254; i++) output6[i] = i;
    output6[255] = 0x00; // trailing zero

    // 0x00 .. 0xFE --> 255 bytes
    uint8_t input7[255];
    for (size_t i=0; i<255; i++) input7[i] = i;
    uint8_t output7[257];
    output7[0] = 0x01;
    output7[1] = 0xFF;
    for (size_t i=2; i<=255; i++) output7[i] = i-1;
    output7[256] = 0x00; // trailing zero
    
    // 0x01 .. 0xFF --> 255 bytes, all non-zero
    uint8_t input8[255];
    for (size_t i=0; i<255; i++) input8[i] = i+1;
    uint8_t output8[258];
    output8[0] = 0xFF;   // code byte
    for (size_t i=1; i<255; i++) output8[i] = i; // 254 data bytes, 0x01 .. 0xFE
    output8[255] = 0x02; // code byte
    output8[256] = 0xFF; // last data byte
    output8[257] = 0x00; // trailing zero
    
    // 0x02 .. 0xFF, 0x00 --> 255 bytes, last byte is a zero
    uint8_t input9[255];
    for (size_t i=0; i<254; i++) input9[i] = i+2; // 0x02..0xFF
    input9[254] = 0x00;
    uint8_t output9[258];
    output9[0] = 0xFF; // code byte
    for (size_t i=1; i<255; i++) output9[i] = i+1; // 254 data bytes, 0x02..0xFF
    output9[255] = 0x01; // overhead code byte
    output9[256] = 0x01; // regular code byte
    output9[257] = 0x00; // trailing zero
    
    // 0x03..0xFF, 0x00, 0x01 --> 255 bytes
    uint8_t input10[255];
    for (size_t i=0; i<253; i++) input10[i] = i+3; // 0x03..0xFF, 253 bytes
    input10[253] = 0x00;
    input10[254] = 0x01;
    uint8_t output10[257];
    output10[0] = 0xFE; // code byte
    for (size_t i=1; i<254; i++) output10[i] = i+2; // 0x03..0xFF, 253 bytes
    output10[254] = 0x02; // code byte
    output10[255] = 0x01; // data byte
    output10[256] = 0x00; // trailing zero
    
    // from COBS paper: http://www.stuartcheshire.org/papers/COBSforToN.pdf
    uint8_t input11[] =  {0x45, 0x00, 0x00, 0x2C, 0x4C, 0x79, 0x00, 0x00, 0x40, 0x06, 0x4F, 0x37};
    uint8_t output11[] = {0x02, 0x45, 0x01, 0x04, 0x2C, 0x4C, 0x79, 0x01, 0x05, 0x40, 0x06, 0x4F, 0x37, 0x00};
    
    /* run tests */
    
    cout << endl << "checking example 1:" << endl;
    compare(input1, sizeof(input1), output1, sizeof(output1)-sub, with_trailing_zero);

    cout << endl << "checking example 2:" << endl;
    compare(input2, sizeof(input2), output2, sizeof(output2)-sub, with_trailing_zero);

    cout << endl << "checking example 3:" << endl;
    compare(input3, sizeof(input3), output3, sizeof(output3)-sub, with_trailing_zero);
    
    cout << endl << "checking example 4:" << endl;
    compare(input4, sizeof(input4), output4, sizeof(output4)-sub, with_trailing_zero);

    cout << endl << "checking example 5:" << endl;
    compare(input5, sizeof(input5), output5, sizeof(output5)-sub, with_trailing_zero);
    
    cout << endl << "checking example 6:" << endl;
    compare(input6, sizeof(input6), output6, sizeof(output6)-sub, with_trailing_zero);
    
    cout << endl << "checking example 7:" << endl;
    compare(input7, sizeof(input7), output7, sizeof(output7)-sub, with_trailing_zero);
    
    cout << endl << "checking example 8:" << endl;
    compare(input8, sizeof(input8), output8, sizeof(output8)-sub, with_trailing_zero);

    cout << endl << "checking example 9:" << endl;
    compare(input9, sizeof(input9), output9, sizeof(output9)-sub, with_trailing_zero);
    
    cout << endl << "checking example 10:" << endl;
    compare(input10, sizeof(input10), output10, sizeof(output10)-sub, with_trailing_zero);

    cout << endl << "checking example 11:" << endl;
    compare(input11, sizeof(input11), output11, sizeof(output11)-sub, with_trailing_zero);

    return 0;
}
