#pragma once
#include <stdexcept>

namespace vsc {

// Partial-pivot Gaussian elimination for dense N×N systems.
// A is overwritten with L (lower, unit diagonal implicit) and U (upper).
// pivot stores row permutation.
// Throws std::runtime_error if matrix is singular.
template <int N>
void lu_factor(double A[N][N], int pivot[N]) {
    for (int k = 0; k < N; ++k) {
        // find pivot
        int p = k;
        double best = 0.0;
        for (int i = k; i < N; ++i) {
            double v = A[i][k] < 0 ? -A[i][k] : A[i][k];
            if (v > best) { best = v; p = i; }
        }
        if (best < 1e-15) throw std::runtime_error("LuSolver: singular matrix");
        pivot[k] = p;
        if (p != k) {
            for (int j = 0; j < N; ++j) {
                double tmp = A[k][j]; A[k][j] = A[p][j]; A[p][j] = tmp;
            }
        }
        double inv = 1.0 / A[k][k];
        for (int i = k + 1; i < N; ++i) {
            A[i][k] *= inv;
            for (int j = k + 1; j < N; ++j)
                A[i][j] -= A[i][k] * A[k][j];
        }
    }
}

// Solve A*x = b using LU factors produced by lu_factor (A now holds L and U).
// pivot is the row permutation. Result written to x.
template <int N>
void lu_solve(const double LU[N][N], const int pivot[N], const double b[N], double x[N]) {
    double y[N];
    // apply permutation and forward substitution (L*y = P*b)
    for (int i = 0; i < N; ++i) y[i] = b[i];
    for (int k = 0; k < N; ++k) {
        double tmp = y[k]; y[k] = y[pivot[k]]; y[pivot[k]] = tmp;
    }
    for (int i = 0; i < N; ++i) {
        double s = y[i];
        for (int j = 0; j < i; ++j) s -= LU[i][j] * y[j];
        y[i] = s;
    }
    // back substitution (U*x = y)
    for (int i = N - 1; i >= 0; --i) {
        double s = y[i];
        for (int j = i + 1; j < N; ++j) s -= LU[i][j] * x[j];
        x[i] = s / LU[i][i];
    }
}

} // namespace vsc
