#include <iostream>
#include <cstdint>
#include <bitset>
#include "util.h"
#include <cassert>
// #include "convert_numeric.h"
#include <cstring>

struct BF16 {
    uint16_t value;
    
    BF16(uint16_t val) : value(val) {}
    
    bool sign() const {
        return (value >> 15) & 0x1;
    }
    
    uint8_t exp() const {
        return (value >> 7) & 0xFF;
    }
    
    uint16_t mant() const {
        return 0x80 | (value & 0x7F); // {1'b1, in_A[6:0]}
    }
};

struct AccumulatorData {
    bool sign;
    uint8_t exp; // 5-bit
    uint32_t mant; // 16-bit
    // int16_t MAR;  // 8-bit mantissa auxiliary register
    bool mant_overflow;
    bool exp_diff_1;
    bool exp_diff_2;
};

struct MultiplierData {
    bool sign;
    // uint8_t exp;
    uint8_t exp;    //  exp_compare
    uint8_t exp_control;
    uint16_t mant;
};

MultiplierData multiplyBfloat16(const BF16& in_A, const BF16& in_B);
float convertToFP32(const MultiplierData& result);
void pim_mac_mul(short* A, short* B, short* C, int M, int N);

AccumulatorData addBfloat16(const MultiplierData& in, const AccumulatorData& acc);
short normalize(const AccumulatorData& in);
float normalize_to_fp32(const AccumulatorData& in);