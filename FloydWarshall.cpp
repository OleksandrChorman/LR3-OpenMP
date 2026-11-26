#include <iostream>
#include <vector>
#include <iomanip>
#include <omp.h>
#include <string>
#include <sstream>
#include <random>
#include <limits>
#include <algorithm>

using namespace std;

const long long INF = 1000000000LL;
const long long NEG_INF = -INF;

using Matrix = vector<vector<long long>>;

// ============================================================
// MATRIX GENERATION
// ============================================================

// Create 10 predefined 7x7 test matrices.
// These are deterministic matrices generated from different seeds.
Matrix generatePresetMatrix(int seed)
{
    int n = 7;
    Matrix m(n, vector<long long>(n, INF));
    vector<int> potential(n);

    mt19937 gen(seed);
    uniform_int_distribution<int> pDist(-8, 8);
    uniform_int_distribution<int> wDist(1, 15);
    uniform_int_distribution<int> chance(0, 99);

    for (int i = 0; i < n; i++)
    {
        potential[i] = pDist(gen);
        m[i][i] = 0;
    }

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (i != j && chance(gen) < 45)
            {
                int base = wDist(gen);
                m[i][j] = base + potential[j] - potential[i];
            }
        }
    }

    return m;
}

vector<Matrix> createTests()
{
    vector<Matrix> tests;

    for (int i = 1; i <= 10; i++)
        tests.push_back(generatePresetMatrix(i * 100));

    return tests;
}

// Random matrix generation without negative cycles
Matrix generateRandomGraph(int n, mt19937& gen)
{
    Matrix m(n, vector<long long>(n, INF));
    vector<int> potential(n);

    uniform_int_distribution<int> pDist(-15, 15);
    uniform_int_distribution<int> wDist(0, 20);
    uniform_int_distribution<int> chance(0, 99);

    for (int i = 0; i < n; i++)
    {
        potential[i] = pDist(gen);
        m[i][i] = 0;
    }

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (i != j && chance(gen) < 35)
            {
                int base = wDist(gen);
                m[i][j] = base + potential[j] - potential[i];
            }
        }
    }

    return m;
}

// ============================================================
// MANUAL MATRIX INPUT
// ============================================================

bool readWeight(const string& input, long long& value)
{
    if (input == "INF" || input == "inf" || input == "Inf")
    {
        value = INF;
        return true;
    }

    try
    {
        size_t pos;
        value = stoll(input, &pos);

        if (pos != input.size())
            return false;

        if (value <= -1000000 || value >= 1000000)
            return false;

        return true;
    }
    catch (...)
    {
        return false;
    }
}

Matrix enterMatrixManually()
{
    int n;

    cout << "\nEnter matrix size (2-250): ";

    while (!(cin >> n) || n < 2 || n > 250)
    {
        cout << "Invalid size. Enter a number from 2 to 250: ";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    Matrix m(n, vector<long long>(n, INF));

    cout << "\nEnter matrix values row by row.\n";
    cout << "Use INF for missing edges.\n";
    cout << "Diagonal elements must be 0.\n\n";

    for (int i = 0; i < n; i++)
    {
        cout << "Row " << i + 1 << ": ";

        for (int j = 0; j < n; j++)
        {
            string input;
            long long value;

            while (true)
            {
                cin >> input;

                if (readWeight(input, value))
                    break;

                cout << "Invalid value. Enter a number or INF: ";
            }

            if (i == j && value != 0)
            {
                cout << "Diagonal must be 0. Value replaced with 0.\n";
                value = 0;
            }

            m[i][j] = value;
        }
    }

    cout << "\nManual matrix entered successfully!\n";
    cout << "Matrix size: " << n << " x " << n << "\n";

    return m;
}

// ============================================================
// NEXT MATRIX
// ============================================================

Matrix initializeNext(const Matrix& dist)
{
    int n = (int)dist.size();

    Matrix next(n, vector<long long>(n, -1));

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (i == j)
                next[i][j] = i;
            else if (dist[i][j] != INF)
                next[i][j] = j;
        }
    }

    return next;
}

// ============================================================
// NEGATIVE CYCLE DETECTION
// ============================================================

bool hasNegativeCycle(const Matrix& dist)
{
    for (int i = 0; i < (int)dist.size(); i++)
    {
        if (dist[i][i] < 0)
            return true;
    }

    return false;
}

void reportNegativeCycle(const Matrix& dist)
{
    cout << "\n========================================\n";
    cout << "WARNING: NEGATIVE CYCLE DETECTED!\n";
    cout << "========================================\n";

    cout << "Negative cycle vertices: ";

    bool first = true;

    for (int i = 0; i < (int)dist.size(); i++)
    {
        if (dist[i][i] < 0)
        {
            if (!first)
                cout << ", ";

            cout << i + 1;
            first = false;
        }
    }

    cout << "\n";
    cout << "Shortest paths affected by negative cycles\n";
    cout << "may not have finite values.\n\n";
}

// ============================================================
// FLOYD-WARSHALL SEQUENTIAL
// ============================================================

void floydSequential(Matrix& dist, Matrix& next)
{
    int n = (int)dist.size();

    for (int k = 0; k < n; k++)
    {
        vector<long long> row = dist[k];
        vector<long long> col(n);
        vector<long long> nextCol(n);

        for (int i = 0; i < n; i++)
        {
            col[i] = dist[i][k];
            nextCol[i] = next[i][k];
        }

        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                if (col[i] == INF || row[j] == INF)
                    continue;

                long long candidate = col[i] + row[j];

                if (candidate < dist[i][j])
                {
                    dist[i][j] = candidate;
                    next[i][j] = nextCol[i];
                }
            }
        }
    }
}

// ============================================================
// FLOYD-WARSHALL PARALLEL
// ============================================================

double floydParallel(
    Matrix& dist,
    Matrix& next,
    int threads,
    vector<double>& threadTimes)
{
    int n = (int)dist.size();

    vector<long long> row(n);
    vector<long long> col(n);
    vector<long long> nextCol(n);

    double totalStart = omp_get_wtime();

#pragma omp parallel num_threads(threads) shared(dist, next, row, col, nextCol, threadTimes)
    {
        int tid = omp_get_thread_num();
        double threadStart = omp_get_wtime();

        for (int k = 0; k < n; k++)
        {
#pragma omp single
            {
                for (int i = 0; i < n; i++)
                {
                    col[i] = dist[i][k];
                    nextCol[i] = next[i][k];
                    row[i] = dist[k][i];
                }
            }

#pragma omp for schedule(static)
            for (int i = 0; i < n; i++)
            {
                for (int j = 0; j < n; j++)
                {
                    if (col[i] == INF || row[j] == INF)
                        continue;

                    long long candidate = col[i] + row[j];

                    if (candidate < dist[i][j])
                    {
                        dist[i][j] = candidate;
                        next[i][j] = nextCol[i];
                    }
                }
            }
        }

        threadTimes[tid] = omp_get_wtime() - threadStart;
    }

    return omp_get_wtime() - totalStart;
}

// ============================================================
// NEGATIVE CYCLE PROCESSING
// ============================================================

void processNegativeCycles(Matrix& dist, Matrix& next)
{
    int n = (int)dist.size();
    vector<int> negativeVertices;

    for (int k = 0; k < n; k++)
    {
        if (dist[k][k] < 0)
            negativeVertices.push_back(k);
    }

    Matrix reachable = dist;

    for (int k : negativeVertices)
    {
        for (int i = 0; i < n; i++)
        {
            if (reachable[i][k] == INF)
                continue;

            for (int j = 0; j < n; j++)
            {
                if (reachable[k][j] != INF)
                {
                    dist[i][j] = NEG_INF;
                    next[i][j] = -2;
                }
            }
        }
    }
}

// ============================================================
// MATRIX OUTPUT
// ============================================================

void printMatrix(
    const Matrix& matrix,
    string title,
    bool isNextMatrix = false)
{
    int n = (int)matrix.size();
    int cellWidth = 7;

    cout << "\n" << title << "\n\n";

    cout << setw(5) << " ";

    for (int j = 0; j < n; j++)
        cout << setw(cellWidth) << j + 1;

    cout << "\n";

    for (int i = 0; i < n; i++)
    {
        cout << setw(3) << i + 1 << " |";

        for (int j = 0; j < n; j++)
        {
            long long value = matrix[i][j];

            if (isNextMatrix)
            {
                if (value == -1)
                    cout << setw(cellWidth) << "-";
                else if (value == -2)
                    cout << setw(cellWidth) << "-INF";
                else
                    cout << setw(cellWidth) << value + 1;
            }
            else
            {
                if (value == INF)
                    cout << setw(cellWidth) << "INF";
                else if (value == NEG_INF)
                    cout << setw(cellWidth) << "-INF";
                else
                    cout << setw(cellWidth) << value;
            }
        }

        cout << "\n";
    }
}

// ============================================================
// PATH OUTPUT
// ============================================================

void printPath(
    const Matrix& dist,
    const Matrix& next,
    int from,
    int to)
{
    cout << "\nPath from vertex " << from + 1
        << " to vertex " << to + 1 << ":\n";

    if (dist[from][to] == INF)
    {
        cout << "There is no path.\n";
        return;
    }

    if (dist[from][to] == NEG_INF)
    {
        cout << "No finite shortest path: negative cycle detected.\n";
        return;
    }

    cout << "Distance: " << dist[from][to] << "\n";
    cout << "Route: ";

    int current = from;
    cout << current + 1;

    for (int step = 0; step < (int)dist.size(); step++)
    {
        if (current == to)
            break;

        current = (int)next[current][to];

        if (current < 0 || current >= (int)dist.size())
        {
            cout << "\nPath reconstruction error.\n";
            return;
        }

        cout << " -> " << current + 1;
    }

    cout << "\n";
}

// ============================================================
// MATRIX COMPARISON
// ============================================================

bool compareMatrices(const Matrix& A, const Matrix& B)
{
    if (A.size() != B.size())
        return false;

    for (int i = 0; i < (int)A.size(); i++)
    {
        for (int j = 0; j < (int)A.size(); j++)
        {
            if (A[i][j] != B[i][j])
                return false;
        }
    }

    return true;
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    omp_set_dynamic(0);

    mt19937 gen((unsigned int)time(nullptr));

    vector<Matrix> tests = createTests();

    cout << "========================================\n";
    cout << "LABORATORY WORK 3 - TASK 2\n";
    cout << "Floyd-Warshall Algorithm + OpenMP\n";
    cout << "========================================\n";

    int menuChoice;

    cout << "\nMENU:\n";
    cout << "1 - Select one of 10 predefined matrices\n";
    cout << "2 - Generate a random matrix\n";
    cout << "3 - Enter matrix manually\n";
    cout << "0 - Exit\n";
    cout << "Your choice: ";

    if (!(cin >> menuChoice) ||
        menuChoice < 0 || menuChoice > 3)
    {
        cout << "Invalid menu choice!\n";
        return 1;
    }

    if (menuChoice == 0)
        return 0;

    Matrix initial;

    if (menuChoice == 1)
    {
        int testNumber;

        cout << "Select test matrix (1-10): ";

        if (!(cin >> testNumber) ||
            testNumber < 1 || testNumber > 10)
        {
            cout << "Invalid test number!\n";
            return 1;
        }

        initial = tests[testNumber - 1];

        cout << "Matrix " << testNumber << " selected.\n";
    }
    else if (menuChoice == 2)
    {
        int n;

        cout << "Enter matrix size (2-250): ";

        if (!(cin >> n) || n < 2 || n > 250)
        {
            cout << "Invalid matrix size!\n";
            return 1;
        }

        cout << "\nGenerating random matrix...\n";

        bool generated = false;

        for (int attempt = 0; attempt < 100; attempt++)
        {
            Matrix candidate = generateRandomGraph(n, gen);

            Matrix checkDist = candidate;
            Matrix checkNext = initializeNext(checkDist);

            floydSequential(checkDist, checkNext);

            if (!hasNegativeCycle(checkDist))
            {
                initial = candidate;
                generated = true;
                break;
            }

            cout << "Negative cycle detected. Regenerating...\n";
        }

        if (!generated)
        {
            cout << "Failed to generate a matrix without negative cycles.\n";
            return 1;
        }

        cout << "Matrix generated successfully!\n";
        cout << "Matrix size: " << n << " x " << n << "\n";
    }
    else if (menuChoice == 3)
    {
        initial = enterMatrixManually();

        Matrix checkDist = initial;
        Matrix checkNext = initializeNext(checkDist);

        floydSequential(checkDist, checkNext);

        if (hasNegativeCycle(checkDist))
        {
            reportNegativeCycle(checkDist);
        }
        else
        {
            cout << "\nNo negative cycle detected.\n";
        }
    }

    int mode;

    cout << "\nCALCULATION MODE:\n";
    cout << "1 - Shortest paths between all vertices\n";
    cout << "2 - Shortest path between two vertices\n";
    cout << "Your choice: ";

    if (!(cin >> mode) || (mode != 1 && mode != 2))
    {
        cout << "Invalid mode!\n";
        return 1;
    }

    int from = -1, to = -1;
    int n = (int)initial.size();

    if (mode == 2)
    {
        cout << "Enter starting vertex (1-" << n << "): ";

        if (!(cin >> from) || from < 1 || from > n)
        {
            cout << "Invalid starting vertex!\n";
            return 1;
        }

        cout << "Enter ending vertex (1-" << n << "): ";

        if (!(cin >> to) || to < 1 || to > n)
        {
            cout << "Invalid ending vertex!\n";
            return 1;
        }

        from--;
        to--;
    }

    int outputChoice;

    cout << "\nOUTPUT OPTIONS:\n";
    cout << "1 - Display DIST and NEXT matrices once\n";
    cout << "2 - Skip matrix output\n";
    cout << "Your choice: ";

    if (!(cin >> outputChoice) ||
        (outputChoice != 1 && outputChoice != 2))
    {
        cout << "Invalid output choice!\n";
        return 1;
    }

    bool showMatrices = (outputChoice == 1);

    // ========================================================
    // SEQUENTIAL EXECUTION
    // ========================================================

    Matrix seqDist = initial;
    Matrix seqNext = initializeNext(seqDist);

    double seqStart = omp_get_wtime();

    floydSequential(seqDist, seqNext);

    double seqTime = omp_get_wtime() - seqStart;

    bool seqNegativeCycle = hasNegativeCycle(seqDist);

    if (seqNegativeCycle)
        reportNegativeCycle(seqDist);

    processNegativeCycles(seqDist, seqNext);

    cout << fixed << setprecision(8);

    cout << "\n========================================\n";
    cout << "SEQUENTIAL EXECUTION (1 THREAD)\n";
    cout << "========================================\n";
    cout << "Execution time: " << seqTime << " seconds\n";

    // Output matrices only once
    if (showMatrices && mode == 1)
    {
        printMatrix(seqDist, "DISTANCE MATRIX (DIST)");
        printMatrix(seqNext, "NEXT MATRIX", true);
    }

    if (mode == 2)
        printPath(seqDist, seqNext, from, to);

    // ========================================================
    // PARALLEL EXECUTION
    // ========================================================

    int threadCounts[] = { 2, 4, 8 };

    double bestTime = seqTime;
    string bestMode = "Sequential (1 thread)";
    double bestSpeedup = 1.0;

    cout << "\n========================================\n";
    cout << "PARALLEL EXECUTION\n";
    cout << "========================================\n";

    for (int threads : threadCounts)
    {
        Matrix parDist = initial;
        Matrix parNext = initializeNext(parDist);

        vector<double> threadTimes(threads, 0.0);

        double totalTime = floydParallel(
            parDist,
            parNext,
            threads,
            threadTimes
        );

        bool parallelNegativeCycle = hasNegativeCycle(parDist);

        processNegativeCycles(parDist, parNext);

        cout << "\n----------------------------------------\n";
        cout << "Threads: " << threads << "\n";
        cout << "----------------------------------------\n";

        for (int i = 0; i < threads; i++)
        {
            cout << "Thread " << i << " time: "
                << threadTimes[i] << " seconds\n";
        }

        cout << "Total parallel time: "
            << totalTime << " seconds\n";

        double speedup = seqTime / totalTime;

        cout << "Speedup: " << speedup << "\n";

        if (mode == 2)
            printPath(parDist, parNext, from, to);

        if (compareMatrices(seqDist, parDist) &&
            compareMatrices(seqNext, parNext))
        {
            cout << "DIST verification: CORRECT\n";
        }
        else
        {
            cout << "DIST verification: FAILED\n";
        }

        if (totalTime < bestTime)
        {
            bestTime = totalTime;
            bestSpeedup = speedup;
            bestMode = to_string(threads) + " threads";
        }
    }

    // ========================================================
    // PERFORMANCE COMPARISON
    // ========================================================

    cout << "\n========================================\n";
    cout << "PERFORMANCE COMPARISON\n";
    cout << "========================================\n";
    cout << "Sequential time: " << seqTime << " seconds\n";
    cout << "Best mode: " << bestMode << "\n";
    cout << "Best execution time: " << bestTime << " seconds\n";
    cout << "Best speedup: " << bestSpeedup << "\n";

    cout << "\n========================================\n";
    cout << "PROGRAM COMPLETED SUCCESSFULLY\n";
    cout << "========================================\n";

    return 0;
}