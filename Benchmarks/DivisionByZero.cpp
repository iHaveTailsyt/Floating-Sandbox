#include "Utils.h"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t Size = 500000;

std::vector<float> MakeDivisors(size_t count)
{
    std::vector<float> divisors(count);

    for (size_t i = 0; i < count / 2; ++i)
    {
        divisors.push_back(0.0f);
    }

    for (size_t i = 0; i < count / 2; ++i)
    {
        divisors.push_back(1.0f);
    }

    return divisors;
}

static void DivisionByZero_Check(benchmark::State& state) 
{
    auto floats = MakeFloats(Size);
    std::vector<float> results(Size);
    auto divisors = MakeDivisors(10);
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            float result = 0.0f;
            float const divisor = divisors[i % 10];
            if (divisor != 0.0f)
            {
                result = floats[i] / divisor;
            }

            results.push_back(result);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(DivisionByZero_Check);

static void DivisionByZero_Approx(benchmark::State& state)
{
    auto floats = MakeFloats(Size);
    std::vector<float> results(Size);
    auto divisors = MakeDivisors(10);
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            float result = 0.0f;
            float const divisor = divisors[i % 10];
            result = floats[i] / std::max(divisor, std::numeric_limits<float>::min());

            results.push_back(result);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(DivisionByZero_Approx);