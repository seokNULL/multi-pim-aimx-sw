#include "result_checker.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <iterator>

using namespace std;

struct comp {
    comp(int idx_, float cpu_, float aim_): idx(idx_), cpu(cpu_), aim(aim_) {
        diff = abs(cpu - aim);
        uint16_t cpu_bit = reinterpret_cast<uint16_t*>(&cpu)[1];
        uint16_t aim_bit = reinterpret_cast<uint16_t*>(&aim)[1];
        uint16_t diff_bit = reinterpret_cast<uint16_t*>(&diff)[1];
        cpu_hex = cpu_bit;
        aim_hex = aim_bit;
        diff_hex = diff_bit;
        if (cpu_bit == 0)
            bit_diff = 0;
        else
            bit_diff = cpu_bit > aim_bit? cpu_bit - aim_bit: aim_bit - cpu_bit;
    }

    int idx;
    float cpu;
    float aim;    
    float diff;
    uint16_t cpu_hex;
    uint16_t aim_hex;
    uint16_t diff_hex;
    uint16_t bit_diff;
};

struct comp_with_aim_model {
    comp_with_aim_model(int idx_, float cpu_, float aim_model_, float aim_)
    : idx(idx_), cpu(cpu_), aim_model(aim_model_), aim(aim_) {
        diff = abs(aim_model - aim);
        uint16_t aim_model_bit = reinterpret_cast<uint16_t*>(&aim_model)[1];
        uint16_t aim_bit = reinterpret_cast<uint16_t*>(&aim)[1];
        if (aim_model_bit == 0)
            bit_diff = 0;
        else
            bit_diff = aim_model_bit > aim_bit?
                aim_model_bit - aim_bit: aim_bit - aim_model_bit;
    }

    int idx;
    float cpu;
    float aim_model;
    float aim;
    float diff;
    uint16_t bit_diff;
};

std::ostream& operator<<(std::ostream& os, const comp_with_aim_model& val) {
    os << setw(5) << val.idx << "\t"
        << "cpu: " << setw(12) << val.cpu << "\t"
        << "aim_mdl: " << setw(10) << val.aim_model << "\t"
        << "aim: " << setw(12) << val.aim << "\t"
        << "diff: " << val.diff << "\t"
        << "upper 16 bits diff: " << setw(5) << val.bit_diff;
    return os;
}

std::ostream& operator<<(std::ostream& os, const comp& val) {
    os << setw(5) << val.idx << "\t" 
        // << "cpu: " << "\t" << "0x" << std::hex << val.cpu_hex << "(" << std::dec << val.cpu << ")" << "\t"
        // << "pim: " << "\t" << "0x" << std::hex << val.aim_hex << "(" << std::dec << val.aim << ")" << "\t"
        // << "diff: " << std::dec << val.diff << "\t"
        // << "upper 16 bits diff: " << setw(5) << "0x" << std::hex << val.bit_diff << std::dec;
        << "pim:" << "\t" << "0x" << std::hex << val.cpu_hex << "(" << std::dec << val.cpu << ")" << "\t"
        << "pim simul:" << "\t" << "0x" << std::hex << val.aim_hex << "(" << std::dec << val.aim << ")" << "\t"
        << "diff:" << std::dec << val.diff << "\t"
        << "upper 16 bits diff:" << setw(5) << "0x" << std::hex << val.bit_diff << std::dec;
    return os;
}

void ResultChecker::compare_results(const std::vector<float> &cpu_output,
    const std::vector<float> &aim_output) {
    assert(cpu_output.size() == aim_output.size());
    vector<comp> comps;
    
    int k = 3;
    for (size_t i = 0; i < cpu_output.size(); ++i) {
        comps.emplace_back(i, cpu_output[i], aim_output[i]);
    }

    cout << fixed << setprecision(4);
    // copy(comps.begin(), comps.end(), ostream_iterator<comp>(cout, "\n"));
    cout << endl
        << k << " Outputs with the largest diff" << endl;
    partial_sort(comps.begin(), comps.begin() + k, comps.end(),
        [](const comp& a, const comp& b) {
            return a.diff > b.diff;
        }
    );
    copy(comps.begin(), comps.begin() + k, ostream_iterator<comp>(cout, "\n"));

    cout << endl
        << k << " Outputs with the largest upper 16 bits diff" << endl;
    partial_sort(comps.begin(), comps.begin() + 5, comps.end(),
        [](const comp& a, const comp& b) {
            return a.bit_diff > b.bit_diff;
        }
    );
    copy(comps.begin(), comps.begin() + k, ostream_iterator<comp>(cout, "\n"));

    cout << endl
        << k << " Outputs with the largest upper 16 bits diff and "
        << "cpu value larger than 1" << endl;
    partial_sort(comps.begin(), comps.begin() + k, comps.end(),
        [](const comp& a, const comp& b) {
            if (abs(a.cpu) > 1) {
                if (abs(b.cpu) > 1)
                    return a.bit_diff > b.bit_diff;
                else
                    return true;
            }
            else {
                if (abs(b.cpu) > 1)
                    return false;
                else
                    return a.bit_diff > b.bit_diff;
            }
        }
    );
    copy(comps.begin(), comps.begin() + k, ostream_iterator<comp>(cout, "\n"));
}

void ResultChecker::compare_results_with_aim_model(const std::vector<float> &cpu_output,
    const std::vector<float> &aim_model_output,
    const std::vector<float> &aim_output)
{
    assert(cpu_output.size() == aim_output.size());

    vector<comp_with_aim_model> comps;    
    int k = 3;
    for (size_t i = 0; i < cpu_output.size(); ++i) {
        comps.emplace_back(i, cpu_output[i], aim_model_output[i], aim_output[i]);
    }

    cout << fixed << setprecision(4);
    copy(comps.begin(), comps.end(), ostream_iterator<comp_with_aim_model>(cout, "\n"));
    cout << endl
        << k << " Outputs with the largest diff" << endl;
    partial_sort(comps.begin(), comps.begin() + k, comps.end(),
        [](const comp_with_aim_model& a, const comp_with_aim_model& b) {
            return a.diff > b.diff;
        }
    );
    copy(comps.begin(), comps.begin() + k, ostream_iterator<comp_with_aim_model>(cout, "\n"));

    cout << endl
        << k << " Outputs with the largest upper 16 bits diff" << endl;
    partial_sort(comps.begin(), comps.begin() + 5, comps.end(),
        [](const comp_with_aim_model& a, const comp_with_aim_model& b) {
            return a.bit_diff > b.bit_diff;
        }
    );
    copy(comps.begin(), comps.begin() + k, ostream_iterator<comp_with_aim_model>(cout, "\n"));

    cout << endl
        << k << " Outputs with the largest upper 16 bits diff and "
        << "cpu value larger than 1" << endl;
    partial_sort(comps.begin(), comps.begin() + k, comps.end(),
        [](const comp_with_aim_model& a, const comp_with_aim_model& b) {
            if (abs(a.cpu) > 1) {
                if (abs(b.cpu) > 1)
                    return a.bit_diff > b.bit_diff;
                else
                    return true;
            }
            else {
                if (abs(b.cpu) > 1)
                    return false;
                else
                    return a.bit_diff > b.bit_diff;
            }
        }
    );
    copy(comps.begin(), comps.begin() + k, ostream_iterator<comp_with_aim_model>(cout, "\n"));
}
