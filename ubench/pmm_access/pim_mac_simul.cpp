#include "pim_mac_simul.h"
#define USE_MODIFIED_MAC_UNIT

MultiplierData multiplyBfloat16(const BF16& in_A, const BF16& in_B) {
    // Extract sign, exponent, and mantissa
    bool in_A_sign = in_A.sign();
    uint8_t in_A_exp = in_A.exp();
    uint16_t in_A_mant = in_A.mant();
    
    bool in_B_sign = in_B.sign();
    uint8_t in_B_exp = in_B.exp();
    uint16_t in_B_mant = in_B.mant();
    
    // Temporary wires and variables
    bool check_zero;
    uint16_t exp_add_temp;
    int16_t exp_bias_sub;
    
    // assign check_zero = (in_A_exp == 8'b0 || in_B_exp == 8'b0) "|| exp_bias_sub[8];"
    check_zero = (in_A_exp == 0 || in_B_exp == 0);    
    
    // wire mul_sign = (check_zero) ? 1'b0 : in_A_sign ^ in_B_sign;
    bool mul_sign = check_zero ? false : (in_A_sign ^ in_B_sign);
    
    // assign exp_add_temp = in_A_exp + in_B_exp;
    exp_add_temp = in_A_exp + in_B_exp;

    // assign exp_bias_sub = exp_add_temp - 8'b0111_1111;
    exp_bias_sub = exp_add_temp - 0x7F;
    
    // wire [7:0]  mul_exp = (check_zero) ? 8'b00000000 : exp_bias_sub[7:0];
    uint8_t mul_exp = (check_zero || ((exp_bias_sub & 0x100) == 0x100)) ? 0 : exp_bias_sub & 0xFF;
    
    // wire [11:0] mul_HH = in_A_mant[7:0] * in_B_mant[7:4]; //8x4;
    uint16_t mul_HH = (in_A_mant & 0xFF) * ((in_B_mant >> 4) & 0xF);

    // wire [7:0]  mul_HL = in_A_mant[7:4] * in_B_mant[3:0]; //4x4;
    uint16_t mul_HL = ((in_A_mant >> 4) & 0xF) * (in_B_mant & 0xF); // 4x4 multiplication
    uint16_t mul_mant = mul_HH + mul_HL;    
    uint8_t exp_control = mul_exp & 0x7; // 3-bit
    uint8_t exp_compare = (mul_exp >> 3) & 0x1F; // 5-bit
    
    // Output assignment
    MultiplierData result;
    result.sign = mul_sign;
    // result.exp = mul_exp;
    result.exp = exp_compare;
    result.exp_control = exp_control;
    result.mant = check_zero ? 0 : mul_mant;
    
    return result;
}

float convertToFP32(const MultiplierData& result) {
    uint32_t fp32 = 0;
    uint8_t carry_out_flag = 0;

    MultiplierData result_temp;
    result_temp.sign =  result.sign;
    result_temp.mant = result.mant;
    result_temp.exp = (result.exp << 3) | result.exp_control;    
    
    // Handle zero case
    if (result_temp.exp == 0) {        
        return (float) fp32; // Returns 0
    }    

    //  Norlization: Incrementing the exponent if overflow occured in Mul stage
    if ((result_temp.mant & 0x800) == 0x800) carry_out_flag = 1; 
    
    // Normalize mantissa
    uint32_t normalized_mant = result_temp.mant << (13 - carry_out_flag); // Convert 11-bit mantissa to 23-bit mantissa    
    uint8_t normalized_exp = result_temp.exp + carry_out_flag; // Propagate exponent from BF16 to FP32
    
    // Pack sign, exponent, and mantissa into a 32-bit value
    fp32 |= (result_temp.sign & 0x1) << 31;         // Set sign bit
    fp32 |= (normalized_exp & 0xFF) << 23;     // Set exponent bits (8 bits)
    fp32 |= normalized_mant & 0x7FFFFF;        // Set mantissa bits (23 bits)
    assert((fp32 & 0xFFF) == 0);

    float result_to_float = reinterpret_cast<float&>(fp32);    
    
    return result_to_float;
}

void pim_mac_mul(short* A, short* B, short* C, int M, int N) {
    
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {            
            BF16 bf16_a(reinterpret_cast<uint16_t&>(A[i*N + j]));
            BF16 bf16_b(reinterpret_cast<uint16_t&>(B[i*N + j]));
            MultiplierData temp = multiplyBfloat16(bf16_a, bf16_b);
            C[i*N + j] = float_to_short(convertToFP32(temp));            
        }
    }
}
#ifdef USE_MODIFIED_MAC_UNIT
AccumulatorData addBfloat16(
    const MultiplierData& in,
    const AccumulatorData& acc    
) {        
    uint8_t exp_control = in.exp_control;
    // Mantissa alignment
    // assign mant_align_sft = (in_mant << exp_control);
    uint32_t mant_align_sft = in.mant << (exp_control + 8); //  KJI
    // assign mant_align = mant_align_sft[18:3];
    uint32_t mant_align = mant_align_sft >> 3;
    
    // Check exponent difference
    // wire [5:0] exp_diff = ({1'b0,in_exp} - {1'b0,in_acc_exp});
    int8_t exp_diff = static_cast<int8_t>(in.exp) - static_cast<int8_t>(acc.exp);
    
    // assign mul_mant_larger_than_acc_mant_2  = (in_exp > in_acc_exp) && !mul_mant_larger_than_acc_mant_1;
    
    // bool mul_mant_larger_than_acc_mant_2 = (in.exp > acc.exp) && (exp_diff != 1);
    bool mul_mant_larger_than_acc_mant_3 = (exp_diff > 2);  //  KJI
    bool mul_mant_larger_than_acc_mant_2 = (exp_diff == 2); //  KJI
    bool mul_mant_larger_than_acc_mant_1 = (exp_diff == 1);
    bool mul_mant_same = (exp_diff == 0);
    bool mul_mant_smaller_than_acc_mant_1 = (exp_diff == -1);
    bool mul_mant_smaller_than_acc_mant_2 = (exp_diff == -2);   //  KJI
    bool mul_mant_smaller_than_acc_mant_3 = (exp_diff < -2);    //  KJI
    // bool mul_mant_smaller_than_acc_mant_2 = (in.exp < acc.exp) && (exp_diff != -1);        
    
    // Select large value
    bool large_sign = (mul_mant_larger_than_acc_mant_3 || mul_mant_larger_than_acc_mant_2 || mul_mant_larger_than_acc_mant_1) ? in.sign : acc.sign;
    uint8_t large_exp = (mul_mant_larger_than_acc_mant_3 || mul_mant_larger_than_acc_mant_2 || mul_mant_larger_than_acc_mant_1) ? in.exp : acc.exp;
    uint32_t large_mant = (mul_mant_larger_than_acc_mant_3 || mul_mant_larger_than_acc_mant_2 || mul_mant_larger_than_acc_mant_1) ? mant_align : acc.mant;
    
    // Aligned mantissas
    // uint16_t mul_result_aligned_mant = (mul_mant_smaller_than_acc_mant_1) ? (mant_align >> 8) : mant_align;
    uint32_t mul_result_aligned_mant = mant_align;  //  KJI
    if (mul_mant_smaller_than_acc_mant_3) mul_result_aligned_mant = 0;   //  KJI
    else if (mul_mant_smaller_than_acc_mant_2) mul_result_aligned_mant = mant_align >> 16;   //  KJI
    else if (mul_mant_smaller_than_acc_mant_1) mul_result_aligned_mant = mant_align >> 8;   //  KJI
    
    // uint16_t acc_result_aligned_mant = (mul_mant_larger_than_acc_mant_1) ? (acc.mant >> 8) : acc.mant;
    uint32_t acc_result_aligned_mant = acc.mant;    //  KJI
    if (mul_mant_larger_than_acc_mant_3) acc_result_aligned_mant = 0;  //  KJI
    if (mul_mant_larger_than_acc_mant_2) acc_result_aligned_mant = acc.mant >> 16;  //  KJI
    else if (mul_mant_larger_than_acc_mant_1) acc_result_aligned_mant = acc.mant >> 8;  //  KJI
    
    bool diff_sign = in.sign ^ acc.sign;
    int32_t mantissa_ADD_temp;

    if (diff_sign) {
        if (in.sign) {                        
            mantissa_ADD_temp = -mul_result_aligned_mant + acc_result_aligned_mant;                                                
        } else {            
            mantissa_ADD_temp = mul_result_aligned_mant - acc_result_aligned_mant;            
        }
    } else {        
        mantissa_ADD_temp = mul_result_aligned_mant + acc_result_aligned_mant;
    }    
    
    int32_t mantissa_ADD = (diff_sign && mantissa_ADD_temp < 0) ? -mantissa_ADD_temp : mantissa_ADD_temp;        
    
    bool check_result_zero = (mantissa_ADD == 0);
    
    // Overflow handling
    bool mant_ovf = (mantissa_ADD & 0x1000000) && !diff_sign;   //  KJI
    
    uint8_t aligned_ovf_exp = large_exp + 1;
    uint32_t aligned_ovf_mant = (1 << 16) | ((mantissa_ADD >> 8) & 0xFFFF); //  KJI
    
    bool add_result_sign = diff_sign ? (mantissa_ADD_temp < 0) : in.sign;
    uint8_t add_result_exp = mant_ovf ? aligned_ovf_exp : large_exp;
    uint32_t add_result_mant = mant_ovf ? aligned_ovf_mant : (mantissa_ADD & 0xFFFFFF); //  KJI
    
    bool MAC_result_sign;
    uint8_t MAC_result_exp;
    uint32_t MAC_result_mant;   //  KJI
    
    // if (mul_mant_larger_than_acc_mant_3 || mul_mant_smaller_than_acc_mant_3) {  //  KJI
    //     MAC_result_sign = large_sign;
    //     MAC_result_exp = large_exp;
    //     MAC_result_mant = large_mant;
    // } else {
    MAC_result_sign = add_result_sign;
    MAC_result_exp = add_result_exp;
    MAC_result_mant = add_result_mant;
    // }
    
    AccumulatorData result;
    result.sign = MAC_result_sign;
    result.exp = MAC_result_exp;
    result.mant = MAC_result_mant;

    result.mant_overflow = mant_ovf ? true : false;
    result.exp_diff_1 = (mul_mant_larger_than_acc_mant_1 || mul_mant_smaller_than_acc_mant_1) ? true : false;
    result.exp_diff_2 = (mul_mant_larger_than_acc_mant_2 || mul_mant_smaller_than_acc_mant_2) ? true : false;

    return result;
}

short normalize(const AccumulatorData& in) {
    
    short result = 0;
    AccumulatorData out;
    out.sign = in.sign;

    if (in.mant & (1 << 23)) {
        out.exp = (in.exp + 1) << 3;
        out.mant = in.mant >> 16;
    } else if (in.mant & (1 << 22)) {
        out.exp = in.exp << 3 | 0x7;
        out.mant = in.mant >> 15;        
    } else if (in.mant & (1 << 21)) {
        out.exp = in.exp << 3 | 0x6;
        out.mant = in.mant >> 14;        
    } else if (in.mant & (1 << 20)) {
        out.exp = in.exp << 3 | 0x5;
        out.mant = in.mant >> 13;        
    } else if (in.mant & (1 << 19)) {
        out.exp = in.exp << 3 | 0x4;
        out.mant = in.mant >> 12;        
    } else if (in.mant & (1 << 18)) {
        out.exp = in.exp << 3 | 0x3;
        out.mant = in.mant >> 11;        
    } else if (in.mant & (1 << 17)) {
        out.exp = in.exp << 3 | 0x2;
        out.mant = in.mant >> 10;        
    } else if (in.mant & (1 << 16)) {
        out.exp = in.exp << 3 | 0x1;
        out.mant = in.mant >> 9;        
    } else if (in.mant & (1 << 15)) {
        out.exp = in.exp << 3;
        out.mant = in.mant >> 8;
    } else if (in.mant & (1 << 14)) {
        out.exp = (in.exp - 1) << 3 | 0x7;
        out.mant = in.mant >> 7;
    } else if (in.mant & (1 << 13)) {
        out.exp = (in.exp - 1) << 3 | 0x6;
        out.mant = in.mant >> 6;        
    } else if (in.mant & (1 << 12)) {
        out.exp = (in.exp - 1) << 3 | 0x5;
        out.mant = in.mant >> 5;        
    } else if (in.mant & (1 << 11)) {
        out.exp = (in.exp - 1) << 3 | 0x4;
        out.mant = in.mant >> 4;        
    } else if (in.mant & (1 << 10)) {
        out.exp = (in.exp - 1) << 3 | 0x3;
        out.mant = in.mant >> 3;        
    } else if (in.mant & (1 << 9)) {
        out.exp = (in.exp - 1) << 3 | 0x2;
        out.mant = in.mant >> 2;        
    } else if (in.mant & (1 << 8)) {
        out.exp = (in.exp - 1) << 3 | 0x1;
        out.mant = in.mant >> 1;        
    } else if (in.mant & (1 << 7)) {
        out.exp = (in.exp - 1) << 3;
        out.mant = in.mant;        
    } else if (in.mant & (1 << 6)) {
        out.exp = (in.exp - 2) << 3 | 0x7;
        out.mant = in.mant << 1;        
    } else if (in.mant & (1 << 5)) {
        out.exp = (in.exp - 2) << 3 | 0x6;
        out.mant = in.mant << 2;
    } else if (in.mant & (1 << 4)) {
        out.exp = (in.exp - 2) << 3 | 0x5;
        out.mant = in.mant << 3;
    } else if (in.mant & (1 << 3)) {
        out.exp = (in.exp - 2) << 3 | 0x4;
        out.mant = in.mant << 4;
    } else if (in.mant & (1 << 2)) {
        out.exp = (in.exp - 2) << 3 | 0x3;
        out.mant = in.mant << 5;
    } else if (in.mant & (1 << 1)) {
        out.exp = (in.exp - 2) << 3 | 0x2;
        out.mant = in.mant << 6;
    } else if (in.mant & (1 << 0)) {
        out.exp = (in.exp - 2) << 3 | 0x1;
        out.mant = in.mant << 7;
    } else {
        out.exp = in.exp << 3;
        out.mant = 0;        
    }

    result |= (out.sign & 0x1) << 15;
    result |= (out.exp & 0xFF) << 7;
    result |= (out.mant & 0x7F);

    return result;
}

float normalize_to_fp32(const AccumulatorData& in) {
    
    uint32_t result = 0;
    AccumulatorData out;
    out.sign = in.sign;    

    if (in.mant & (1 << 23)) {
        out.exp = (in.exp + 1) << 3;
        out.mant = in.mant >> 8;
    } else if (in.mant & (1 << 22)) {
        out.exp = in.exp << 3 | 0x7;
        out.mant = in.mant >> 7;
    } else if (in.mant & (1 << 21)) {
        out.exp = in.exp << 3 | 0x6;
        out.mant = in.mant >> 6;
    } else if (in.mant & (1 << 20)) {
        out.exp = in.exp << 3 | 0x5;
        out.mant = in.mant >> 5;
    } else if (in.mant & (1 << 19)) {
        out.exp = in.exp << 3 | 0x4;
        out.mant = in.mant >> 4;
    } else if (in.mant & (1 << 18)) {
        out.exp = in.exp << 3 | 0x3;
        out.mant = in.mant >> 3;
    } else if (in.mant & (1 << 17)) {
        out.exp = in.exp << 3 | 0x2;
        out.mant = in.mant >> 2;
    } else if (in.mant & (1 << 16)) {
        out.exp = in.exp << 3 | 0x1;
        out.mant = in.mant >> 1;
    } else if (in.mant & (1 << 15)) {
        out.exp = in.exp << 3;
        out.mant = in.mant >> 0;
    } else if (in.mant & (1 << 14)) {
        out.exp = (in.exp - 1) << 3 | 0x7;
        out.mant = in.mant << 1;
    } else if (in.mant & (1 << 13)) {
        out.exp = (in.exp - 1) << 3 | 0x6;
        out.mant = in.mant << 2;        
    } else if (in.mant & (1 << 12)) {
        out.exp = (in.exp - 1) << 3 | 0x5;
        out.mant = in.mant << 3;        
    } else if (in.mant & (1 << 11)) {
        out.exp = (in.exp - 1) << 3 | 0x4;
        out.mant = in.mant << 4;        
    } else if (in.mant & (1 << 10)) {
        out.exp = (in.exp - 1) << 3 | 0x3;
        out.mant = in.mant << 5;        
    } else if (in.mant & (1 << 9)) {
        out.exp = (in.exp - 1) << 3 | 0x2;
        out.mant = in.mant << 6;        
    } else if (in.mant & (1 << 8)) {
        out.exp = (in.exp - 1) << 3 | 0x1;
        out.mant = in.mant << 7;        
    } else if (in.mant & (1 << 7)) {
        out.exp = (in.exp - 1) << 3;
        out.mant = in.mant << 8;        
    } else if (in.mant & (1 << 6)) {
        out.exp = (in.exp - 2) << 3 | 0x7;
        out.mant = in.mant << 9;        
    } else if (in.mant & (1 << 5)) {
        out.exp = (in.exp - 2) << 3 | 0x6;
        out.mant = in.mant << 10;
    } else if (in.mant & (1 << 4)) {
        out.exp = (in.exp - 2) << 3 | 0x5;
        out.mant = in.mant << 11;
    } else if (in.mant & (1 << 3)) {
        out.exp = (in.exp - 2) << 3 | 0x4;
        out.mant = in.mant << 12;
    } else if (in.mant & (1 << 2)) {
        out.exp = (in.exp - 2) << 3 | 0x3;
        out.mant = in.mant << 13;
    } else if (in.mant & (1 << 1)) {
        out.exp = (in.exp - 2) << 3 | 0x2;
        out.mant = in.mant << 14;
    } else if (in.mant & (1 << 0)) {
        out.exp = (in.exp - 2) << 3 | 0x1;
        out.mant = in.mant << 15;
    } else {
        out.exp = in.exp << 3;
        out.mant = 0;        
    }

    result |= (out.sign & 0x1) << 31;
    result |= (out.exp & 0xFF) << 23;
    result |= (out.mant & 0x7FFF) << 8;

    float value = reinterpret_cast<float&>(result);

    return value;
}
#else
AccumulatorData addBfloat16(
    const MultiplierData& in,
    const AccumulatorData& acc    
) {        
    uint8_t exp_control = in.exp_control;
    // Mantissa alignment
    // assign mant_align_sft = (in_mant << exp_control);
    uint32_t mant_align_sft = in.mant << exp_control;
    // assign mant_align = mant_align_sft[18:3];
    uint16_t mant_align = mant_align_sft >> 3;
    
    // Check exponent difference
    // wire [5:0] exp_diff = ({1'b0,in_exp} - {1'b0,in_acc_exp});
    int8_t exp_diff = static_cast<int8_t>(in.exp) - static_cast<int8_t>(acc.exp);
    
    // assign mul_mant_larger_than_acc_mant_2  = (in_exp > in_acc_exp) && !mul_mant_larger_than_acc_mant_1;
    bool mul_mant_larger_than_acc_mant_2 = (in.exp > acc.exp) && (exp_diff != 1);
    bool mul_mant_larger_than_acc_mant_1 = (exp_diff == 1);
    bool mul_mant_same = (exp_diff == 0);
    bool mul_mant_smaller_than_acc_mant_1 = (exp_diff == -1);
    bool mul_mant_smaller_than_acc_mant_2 = (in.exp < acc.exp) && (exp_diff != -1);        
    
    // Select large value
    bool large_sign = (mul_mant_larger_than_acc_mant_2 || mul_mant_larger_than_acc_mant_1) ? in.sign : acc.sign;
    uint8_t large_exp = (mul_mant_larger_than_acc_mant_2 || mul_mant_larger_than_acc_mant_1) ? in.exp : acc.exp;
    uint16_t large_mant = (mul_mant_larger_than_acc_mant_2 || mul_mant_larger_than_acc_mant_1) ? mant_align : acc.mant;
    
    // Aligned mantissas
    uint16_t mul_result_aligned_mant = (mul_mant_smaller_than_acc_mant_1) ? (mant_align >> 8) : mant_align;
    uint16_t acc_result_aligned_mant = (mul_mant_larger_than_acc_mant_1) ? (acc.mant >> 8) : acc.mant;
    
    bool diff_sign = in.sign ^ acc.sign;
    int32_t mantissa_ADD_temp;    

    if (diff_sign) {
        if (in.sign) {                        
            mantissa_ADD_temp = -mul_result_aligned_mant + acc_result_aligned_mant;                                                
        } else {            
            mantissa_ADD_temp = mul_result_aligned_mant - acc_result_aligned_mant;            
        }
    } else {        
        mantissa_ADD_temp = mul_result_aligned_mant + acc_result_aligned_mant;
    }    
    
    int32_t mantissa_ADD = (diff_sign && mantissa_ADD_temp < 0) ? -mantissa_ADD_temp : mantissa_ADD_temp;        
    
    bool check_result_zero = (mantissa_ADD == 0);
    
    // Overflow handling
    bool mant_ovf = (mantissa_ADD & 0x10000) && !diff_sign;    
    
    uint8_t aligned_ovf_exp = large_exp + 1;
    uint16_t aligned_ovf_mant = (1 << 8) | ((mantissa_ADD >> 8) & 0xFF);
    
    bool add_result_sign = diff_sign ? (mantissa_ADD_temp < 0) : in.sign;
    uint8_t add_result_exp = mant_ovf ? aligned_ovf_exp : large_exp;
    uint16_t add_result_mant = mant_ovf ? aligned_ovf_mant : (mantissa_ADD & 0xFFFF);
    
    bool MAC_result_sign;
    uint8_t MAC_result_exp;
    uint16_t MAC_result_mant;
    
    if (mul_mant_larger_than_acc_mant_2 || mul_mant_smaller_than_acc_mant_2) {
        MAC_result_sign = large_sign;
        MAC_result_exp = large_exp;
        MAC_result_mant = large_mant;
    } else {
        MAC_result_sign = add_result_sign;
        MAC_result_exp = add_result_exp;
        MAC_result_mant = add_result_mant;
    }
    
    AccumulatorData result;
    result.sign = MAC_result_sign;
    result.exp = MAC_result_exp;
    result.mant = MAC_result_mant;

    result.mant_overflow = mant_ovf ? true : false;
    result.exp_diff_1 = (mul_mant_larger_than_acc_mant_1 || mul_mant_smaller_than_acc_mant_1) ? true : false;
    result.exp_diff_2 = (mul_mant_larger_than_acc_mant_2 || mul_mant_smaller_than_acc_mant_2) ? true : false;

    return result;
}

short normalize(const AccumulatorData& in) {
    
    short result = 0;
    AccumulatorData out;
    out.sign = in.sign;

    if (in.mant & (1 << 15)) {
        out.exp = (in.exp + 1) << 3;
        out.mant = in.mant >> 8;        
    } else if (in.mant & (1 << 14)) {
        out.exp = in.exp << 3 | 0x7;
        out.mant = in.mant >> 7;        
    } else if (in.mant & (1 << 13)) {
        out.exp = in.exp << 3 | 0x6;
        out.mant = in.mant >> 6;        
    } else if (in.mant & (1 << 12)) {
        out.exp = in.exp << 3 | 0x5;
        out.mant = in.mant >> 5;        
    } else if (in.mant & (1 << 11)) {
        out.exp = in.exp << 3 | 0x4;
        out.mant = in.mant >> 4;        
    } else if (in.mant & (1 << 10)) {
        out.exp = in.exp << 3 | 0x3;
        out.mant = in.mant >> 3;        
    } else if (in.mant & (1 << 9)) {
        out.exp = in.exp << 3 | 0x2;
        out.mant = in.mant >> 2;        
    } else if (in.mant & (1 << 8)) {
        out.exp = in.exp << 3 | 0x1;
        out.mant = in.mant >> 1;        
    } else if (in.mant & (1 << 7)) {
        out.exp = in.exp << 3;
        out.mant = in.mant;        
    } else if (in.mant & (1 << 6)) {
        out.exp = (in.exp - 1) << 3 | 0x7;
        out.mant = in.mant << 1;
    } else if (in.mant & (1 << 5)) {
        out.exp = (in.exp - 1) << 3 | 0x6;
        out.mant = in.mant << 2;
    } else if (in.mant & (1 << 4)) {
        out.exp = (in.exp - 1) << 3 | 0x5;
        out.mant = in.mant << 3;
    } else if (in.mant & (1 << 3)) {
        out.exp = (in.exp - 1) << 3 | 0x4;
        out.mant = in.mant << 4;
    } else if (in.mant & (1 << 2)) {
        out.exp = (in.exp - 1) << 3 | 0x3;
        out.mant = in.mant << 5;
    } else if (in.mant & (1 << 1)) {
        out.exp = (in.exp - 1) << 3 | 0x2;
        out.mant = in.mant << 6;
    } else if (in.mant & (1 << 0)) {
        out.exp = (in.exp - 1) << 3 | 0x1;
        out.mant = in.mant << 7;
    } else {
        out.exp = in.exp << 3;
        out.mant = 0;        
    }

    result |= (out.sign & 0x1) << 15;
    result |= (out.exp & 0xFF) << 7;
    result |= (out.mant & 0x7F);

    return result;
}

float normalize_to_fp32(const AccumulatorData& in) {
    
    uint32_t result = 0;
    AccumulatorData out;
    out.sign = in.sign;
    uint8_t in_mant_lsb_buf = 0;

    if (in.mant & (1 << 15)) {
        out.exp = (in.exp + 1) << 3;
        out.mant = in.mant >> 8;
        in_mant_lsb_buf = in.mant & 0xFF;
    } else if (in.mant & (1 << 14)) {
        out.exp = in.exp << 3 | 0x7;
        out.mant = in.mant >> 7;
        in_mant_lsb_buf = in.mant & 0x7F;
        in_mant_lsb_buf <<= 1;        
    } else if (in.mant & (1 << 13)) {
        out.exp = in.exp << 3 | 0x6;
        out.mant = in.mant >> 6;
        in_mant_lsb_buf = in.mant & 0x3F;
        in_mant_lsb_buf <<= 2;        
    } else if (in.mant & (1 << 12)) {
        out.exp = in.exp << 3 | 0x5;
        out.mant = in.mant >> 5;
        in_mant_lsb_buf = in.mant & 0x1F;
        in_mant_lsb_buf <<= 3;        
    } else if (in.mant & (1 << 11)) {
        out.exp = in.exp << 3 | 0x4;
        out.mant = in.mant >> 4;
        in_mant_lsb_buf = in.mant & 0xF;
        in_mant_lsb_buf <<= 4;        
    } else if (in.mant & (1 << 10)) {
        out.exp = in.exp << 3 | 0x3;
        out.mant = in.mant >> 3;
        in_mant_lsb_buf = in.mant & 0x7;
        in_mant_lsb_buf <<= 5;        
    } else if (in.mant & (1 << 9)) {
        out.exp = in.exp << 3 | 0x2;
        out.mant = in.mant >> 2;
        in_mant_lsb_buf = in.mant & 0x3;
        in_mant_lsb_buf <<= 6;
        in_mant_lsb_buf |= (in.MAR >> 2) & 0x3F;
    } else if (in.mant & (1 << 8)) {
        out.exp = in.exp << 3 | 0x1;
        out.mant = in.mant >> 1;
        in_mant_lsb_buf = in.mant & 0x1;
        in_mant_lsb_buf <<= 7;        
    } else if (in.mant & (1 << 7)) {
        out.exp = in.exp << 3;
        out.mant = in.mant;        
    } else if (in.mant & (1 << 6)) {
        out.exp = (in.exp - 1) << 3 | 0x7;
        out.mant = in.mant << 1;        
    } else if (in.mant & (1 << 5)) {
        out.exp = (in.exp - 1) << 3 | 0x6;
        out.mant = in.mant << 2;        
    } else if (in.mant & (1 << 4)) {
        out.exp = (in.exp - 1) << 3 | 0x5;
        out.mant = in.mant << 3;        
    } else if (in.mant & (1 << 3)) {
        out.exp = (in.exp - 1) << 3 | 0x4;
        out.mant = in.mant << 4;        
    } else if (in.mant & (1 << 2)) {
        out.exp = (in.exp - 1) << 3 | 0x3;
        out.mant = in.mant << 5;        
    } else if (in.mant & (1 << 1)) {
        out.exp = (in.exp - 1) << 3 | 0x2;
        out.mant = in.mant << 6;        
    } else if (in.mant & (1 << 0)) {
        out.exp = (in.exp - 1) << 3 | 0x1;
        out.mant = in.mant << 7;        
    } else {
        out.exp = in.exp << 3;
        out.mant = 0;
    }

    result |= (out.sign & 0x1) << 31;
    result |= (out.exp & 0xFF) << 23;
    result |= (out.mant & 0x7F) << 16;
    result |= (in_mant_lsb_buf & 0xFF) << 8;

    float value = reinterpret_cast<float&>(result);

    return value;
}
#endif