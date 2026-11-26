
#include <iostream>
#include <vector>
#include <random>
#include <omp.h>
#include <iomanip>
#include <cmath>
#include <clocale>

using namespace std;

// Sequential calculation
void sequentialCalculation(
    const vector<double>& A,
    const vector<double>& B,
    vector<double>& C)
{
    for (int i = 0; i < A.size(); i++)
    {
        double x = A[i] * B[i];

        C[i] = 1.8 * x * x - 0.6 * x + 0.05;
    }
}

// Parallel calculation using OpenMP
void parallelCalculation(
    const vector<double>& A,
    const vector<double>& B,
    vector<double>& C,
    int threads)
{
#pragma omp parallel for num_threads(threads)
    for (int i = 0; i < A.size(); i++)
    {
        double x = A[i] * B[i];

        C[i] = 1.8 * x * x - 0.6 * x + 0.05;
    }
}

int main()
{
    setlocale(LC_ALL, "English");

    int N;

    cout << "==========================================" << endl;
    cout << "LABORATORY WORK 3" << endl;
    cout << "Parallel polynomial calculation using OpenMP" << endl;
    cout << "Variant 16" << endl;
    cout << "==========================================" << endl;

    cout << "\nEnter array size N (10000 - 1000000): ";
    cin >> N;

    // Input validation
    if (cin.fail() || N < 10000 || N > 1000000)
    {
        cout << "Error: invalid array size!" << endl;
        return 1;
    }

    // Create arrays
    vector<double> A(N);
    vector<double> B(N);
    vector<double> Cseq(N);
    vector<double> Cpar(N);

    // Random number generator
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(0.0, 1.0);

    // Fill arrays with random values
    for (int i = 0; i < N; i++)
    {
        A[i] = dist(gen);
        B[i] = dist(gen);
    }

    cout << "\nArrays generated successfully!" << endl;

    // Number of repetitions for more accurate measurements
    const int repeats = 5;

    // Sequential calculation
    double sequentialTime = 0.0;

    for (int r = 0; r < repeats; r++)
    {
        double start = omp_get_wtime();

        sequentialCalculation(A, B, Cseq);

        double finish = omp_get_wtime();

        sequentialTime += finish - start;
    }

    sequentialTime /= repeats;

    cout << fixed << setprecision(8);

    cout << "\n==========================================" << endl;
    cout << "SEQUENTIAL EXECUTION" << endl;
    cout << "==========================================" << endl;

    cout << "Average execution time: "
        << sequentialTime << " seconds" << endl;

    // Parallel calculations
    int threadCounts[] = { 2, 4, 8 };

    cout << "\n==========================================" << endl;
    cout << "PARALLEL EXECUTION" << endl;
    cout << "==========================================" << endl;

    cout << left << setw(15) << "Threads"
        << setw(20) << "Time (sec)"
        << setw(20) << "Speedup" << endl;

    // Store the last parallel result for verification
    double lastParallelTime = 0.0;

    for (int threads : threadCounts)
    {
        double parallelTime = 0.0;

        for (int r = 0; r < repeats; r++)
        {
            double start = omp_get_wtime();

            parallelCalculation(A, B, Cpar, threads);

            double finish = omp_get_wtime();

            parallelTime += finish - start;
        }

        parallelTime /= repeats;

        double speedup = sequentialTime / parallelTime;

        cout << left << setw(15) << threads
            << setw(20) << parallelTime
            << setw(20) << speedup << endl;

        lastParallelTime = parallelTime;
    }

    // Verification of sequential and parallel results
    double maxDifference = 0.0;

    for (int i = 0; i < N; i++)
    {
        double difference = abs(Cseq[i] - Cpar[i]);

        if (difference > maxDifference)
        {
            maxDifference = difference;
        }
    }

    cout << "\n==========================================" << endl;
    cout << "RESULT VERIFICATION" << endl;
    cout << "==========================================" << endl;

    cout << "Maximum difference: " << maxDifference << endl;

    if (maxDifference < 1e-12)
    {
        cout << "Sequential and parallel results are correct!" << endl;
    }
    else
    {
        cout << "Error: results do not match!" << endl;
    }

    // Display the first 5 input and output elements
    cout << "\n==========================================" << endl;
    cout << "FIRST 5 ELEMENTS FOR MANUAL VERIFICATION" << endl;
    cout << "==========================================" << endl;

    cout << "Polynomial: C[i] = 1.8*(A[i]*B[i])^2"
        << " - 0.6*(A[i]*B[i]) + 0.05" << endl;

    cout << setprecision(10);

    for (int i = 0; i < 5; i++)
    {
        cout << "\nElement [" << i << "]" << endl;
        cout << "A[" << i << "] = " << A[i] << endl;
        cout << "B[" << i << "] = " << B[i] << endl;
        cout << "C[" << i << "] = " << Cseq[i] << endl;
    }

    // Calculate checksum of the result array
    double checksum = 0.0;

    for (int i = 0; i < N; i++)
    {
        checksum += Cseq[i];
    }

    cout << "\n==========================================" << endl;
    cout << "CHECKSUM" << endl;
    cout << "==========================================" << endl;

    cout << "Sum of all C elements: " << checksum << endl;

    cout << "\n==========================================" << endl;
    cout << "PROGRAM COMPLETED SUCCESSFULLY" << endl;
    cout << "==========================================" << endl;

    return 0;
}
