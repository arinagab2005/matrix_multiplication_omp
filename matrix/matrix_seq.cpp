#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <string>
#include <algorithm>

enum class InitMode { Ones, Rand, Identity };

/*
 Ones - Все элементы матрицы 1.0
 Rand - Случайные числа от 0 до 1
 Identity - Единичная матрица
 */

static InitMode get_init_mode() {
    const char* s = std::getenv("INIT");
    if (!s) return InitMode::Ones;
    std::string v(s);
    if (v == "rand") return InitMode::Rand;
    if (v == "identity") return InitMode::Identity;
    return InitMode::Ones;
}

static void init_matrices(size_t M, size_t N, size_t K, std::vector<double> &A, std::vector<double> &B) {
    InitMode mode = get_init_mode();
    
    if (mode == InitMode::Ones) {
        std::fill(A.begin(), A.end(), 1.0);
        std::fill(B.begin(), B.end(), 1.0);
    } else if (mode == InitMode::Rand) {
        srand(0);
        for (auto& x : A) x = rand() / static_cast<double>(RAND_MAX);
        for (auto& x : B) x = rand() / static_cast<double>(RAND_MAX);
    } else if (mode == InitMode::Identity) {
        for (size_t i = 0; i < A.size(); ++i) {
            A[i] = (i % 1000) / 1000.0;
        }
        std::fill(B.begin(), B.end(), 0.0);
        size_t d = std::min(N, K);
        for (size_t k = 0; k < d; ++k) {
            B[k * K + k] = 1.0;
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << "M N K P\n";
        return 1;
    }
    const size_t M = std::strtoull(argv[1], nullptr, 10);
    const size_t N = std::strtoull(argv[2], nullptr, 10);
    const size_t K = std::strtoull(argv[3], nullptr, 10);
    
    try {
        std::vector<double> A(M * N);
        std::vector<double> B(N * K);
        std::vector<double> C(M * K);

        init_matrices(M, N, K, A, B);
        
        auto t0 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < M; ++i) {
            for (size_t j = 0; j < K; ++j) {
                double sum = 0.0;
                for (size_t k = 0; k < N; ++k) {
                    sum += A[i * N + k] * B[k * K + j];
                }
                C[i * K + j] = sum;
            }
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double> dt = t1 - t0;
        std::cout << std::fixed <<std::setprecision(6) << dt.count() << std::endl;
        
        //контрольная сумма - сумма всех элементов С
        long double checksum = 0.0;
        for (double x : C) checksum += x;
        std::cerr << "checksum = " << std::fixed << std::setprecision(6) << static_cast<double>(checksum) << std::endl;
    } catch(...) {
        std::cerr << "Error\n";
        return 2;
    }
}
