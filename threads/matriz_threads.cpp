#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <string>

using namespace std;

void compute_row(const vector<vector<int>> &A, const vector<vector<int>> &B,
                 vector<vector<int>> &Result, int row)
{
    int colsB = B[0].size();
    int colsA = A[0].size();

    for (int j = 0; j < colsB; j++)
    {
        int sum = 0;
        for (int k = 0; k < colsA; k++)
        {
            sum += A[row][k] * B[k][j];
        }
        Result[row][j] = sum;
    }
}

bool read_matrix(ifstream &file, vector<vector<int>> &M, int &rows, int &cols)
{
    string line;
    if (!getline(file, line))
        return false;
    rows = stoi(line.substr(2));

    getline(file, line);
    cols = stoi(line.substr(2));

    M.assign(rows, vector<int>(cols));
    for (int i = 0; i < rows; i++)
    {
        getline(file, line);
        stringstream ss(line);
        for (int j = 0; j < cols; j++)
            ss >> M[i][j];
    }
    return true;
}

int main()
{
    ifstream file("input.txt");
    if (!file)
    {
        cerr << "Error opening input file.\n";
        return 1;
    }

    vector<vector<int>> A, B;
    int rowsA, colsA, rowsB, colsB;

    if (!read_matrix(file, A, rowsA, colsA) || !read_matrix(file, B, rowsB, colsB))
    {
        cerr << "Error reading matrices.\n";
        return 1;
    }

    if (colsA != rowsB)
    {
        cerr << "Matrices have incompatible dimensions.\n";
        return 1;
    }

    vector<vector<int>> Result(rowsA, vector<int>(colsB, 0));
    vector<thread> threads;

    for (int i = 0; i < rowsA; i++)
    {
        threads.emplace_back(compute_row, cref(A), cref(B), ref(Result), i);
    }

    for (auto &t : threads)
        t.join();

    for (int i = 0; i < rowsA; i++)
    {
        for (int j = 0; j < colsB; j++)
        {
            cout << Result[i][j] << (j + 1 == colsB ? '\n' : ' ');
        }
    }

    return 0;
}
