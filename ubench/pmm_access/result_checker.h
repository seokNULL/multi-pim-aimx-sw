#pragma once

#include <vector>

class ResultChecker {
public:
    static void compare_results(const std::vector<float> &cpu_output,
        const std::vector<float> &aim_output);

    static void compare_results_with_aim_model(
        const std::vector<float> &cpu_output,
        const std::vector<float> &aim_model_output,
        const std::vector<float> &aim_output);
};
