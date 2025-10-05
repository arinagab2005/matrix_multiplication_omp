#include <iostream>
#include <vector>
#include <iomanip>
#include <cstdlib>
#include <string>
#include <omp.h>
#include <algorithm>

enum class InitMode { Ones, Rand, Identity };

static InitMode get_init_mode() {
    const char* s = std::getenv("INIT");
    if (!s) return InitMode::Ones;
    std::string v(s);
    if (v == "rand") return InitMode::Rand;
    if (v == "identity") return InitMode::Identity;
    return InitMode::Ones;
}

static void init_matrices(size_t M, size_t N, size_t K,
                          std::vector<double>& A, std::vector<double>& B) {
    InitMode mode = get_init_mode();
    if (mode == InitMode::Ones) {
        std::fill(A.begin(), A.end(), 1.0);
        std::fill(B.begin(), B.end(), 1.0);
    } else if (mode == InitMode::Rand) {
        srand(0);
        for (auto& x : A) x = rand() / static_cast<double>(RAND_MAX);
        for (auto& x : B) x = rand() / static_cast<double>(RAND_MAX);
    } else if (mode == InitMode::Identity) {
        for (size_t i = 0; i < A.size(); ++i)
            A[i] = (i % 1000) / 1000.0;
        std::fill(B.begin(), B.end(), 0.0);
        size_t d = std::min(N, K);
        for (size_t k = 0; k < d; ++k) B[k * K + k] = 1.0;
    }
}

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " M N K P\n";
        return 1;
    }
    
    const size_t M = std::strtoull(argv[1], nullptr, 10);
    const size_t N = std::strtoull(argv[2], nullptr, 10);
    const size_t K = std::strtoull(argv[3], nullptr, 10);
    const int P = std::atoi(argv[4]);
    //зафиксируем число потоков
    omp_set_num_threads(P);
    
    std::vector<double> A(M * N);
    std::vector<double> B(N * K);
    std::vector<double> C(M * K);
    
    init_matrices(M, N, K, A, B);
    
    double t0 = omp_get_wtime();
    #pragma omp parallel for collapse(2) schedule(static)
    for (long long i = 0; i < (long long)M; ++i)
        for (long long j = 0; j < (long long)K; ++j) {
            double sum = 0.0;
            for (long long k = 0; k < (long long)N; ++k)
                sum += A[i * N + k] * B[k * K + j];
            C[i * K + j] = sum;
        }
    double t1 = omp_get_wtime();

    std::cout << std::fixed << std::setprecision(6) << (t1 - t0) << "\n";

    long double checksum = 0.0;
    for (double x : C) checksum += x;
    std::cerr << "checksum=" << std::fixed << std::setprecision(6)
              << static_cast<double>(checksum) << "\n";
}

