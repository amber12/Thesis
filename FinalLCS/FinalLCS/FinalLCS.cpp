// FinalLCS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

#include "string.h"
#include "intrin.h"
#include <omp.h>

int max(int a, int b)
{
	return (a > b) ? a : b;
}

void ReferenceLlcs(char *A, int m, char *B, int n)
{
	int** L = new int*[m + 1];

	for (int i = 0; i <= m; ++i)
	{
		L[i] = new int[n + 1];
	}

	//Following steps build L[m+1][n+1] in bottom up fashion. Note that L[i][j] contains length of LCS of X[0..i-1] and Y[0..j-1] 

	for (int i = 0; i <= m; i++)
	{
		for (int j = 0; j <= n; j++)
		{
			if (i == 0 || j == 0)
			{
				L[i][j] = 0;
			}
			else if (A[i - 1] == B[j - 1])
			{
				L[i][j] = L[i - 1][j - 1] + 1;
			}
			else
			{
				L[i][j] = max(L[i - 1][j], L[i][j - 1]);
			}
		}
	}

	// L[m][n] contains length of LCS for X[0..n-1] and Y[0..m-1]

	printf("Reference %d\r\n", L[m][n]);

	//	Cleanup memory

	for (int i = 0; i < m; ++i)
	{
		delete L[i];
	}

	delete L;
}

unsigned long long Count1s(int m, unsigned long long *V)
{
	int iterations = 1 + ((m - 1) >> 6);

	//	count 1s in final row

	unsigned long long Ln = 0;

	int p = 0;

	for (; p < (iterations - 1); p++)
	{
		Ln += __popcnt64(V[p]);
	}

	//  last word might have less than 64 bits in it

	for (int i = (p << 6); i < m; i++)
	{
		unsigned long long bitset = (V[i >> 6] >> (i % 64)) & 1;

		Ln += bitset;
	}

	return Ln;
}

void Algorithm1(char *B, unsigned long long *V, int n, int iterations, int diagonalLength, int blockSize, unsigned long long *PM)
{
	int *currentRow = new int[iterations];

	memset(currentRow, 0, iterations * sizeof(int));

	unsigned char *borrow = new unsigned char[n];

	memset(borrow, 0, n * sizeof(unsigned char));

	unsigned char *carrys = new unsigned char[n];

	memset(carrys, 1, n * sizeof(unsigned char));

	memset(V, 0x00, iterations * sizeof(unsigned long long));

	double start = omp_get_wtime();

	for (int i = 0; i < (iterations + diagonalLength); i++)
	{
#pragma omp parallel for schedule(static,1) num_threads(diagonalLength) 
		for (int p = i; p > i - diagonalLength; p--)
		{
			if ((p >= 0) && (p < iterations))
			{
				unsigned long long *pm = PM + (p << 2);

				unsigned long long localV = V[p];

				int lastRow = currentRow[p] + blockSize;

				if (lastRow > n)
				{
					lastRow = n;
				}

				unsigned long long partial;

				for (int j = currentRow[p]; j < lastRow; j++)
				{
					unsigned long long tString = pm[B[j]];

					unsigned long long x = localV | tString;

					char msb = localV >> 63;

					unsigned long long shift = (localV << 1) | (unsigned long long)carrys[j];

					carrys[j] = msb;

					borrow[j] = _subborrow_u64(borrow[j], x, shift, &partial);

					localV = x & (partial ^ x);
				}

				V[p] = localV;

				currentRow[p] += blockSize;
			}
		}
	}

	printf("Algorithm 1 - Time: \t %f : Threads %d\n", omp_get_wtime() - start, diagonalLength);

	delete carrys;

	delete borrow;

	delete currentRow;
}

void Algorithm2(char *B, unsigned long long *V, int n, int iterations, int diagonalLength, int blockSize, unsigned long long *PM)
{
	int *currentRow = new int[iterations];

	memset(currentRow, 0, iterations * sizeof(int));

	unsigned char *carrys = new unsigned char[n];

	memset(carrys, 0, n * sizeof(unsigned char));

	memset(V, 0xff, iterations * sizeof(unsigned long long));

	double start = omp_get_wtime();

	for (int i = 0; i < (iterations + diagonalLength); i++)
	{
#pragma omp parallel for schedule(static,1) num_threads(diagonalLength) 
		for (int p = i; p > i - diagonalLength; p--)
		{
			if ((p >= 0) && (p < iterations))
			{
				unsigned long long *pm = PM + (p << 2);

				unsigned long long localV = V[p];

				unsigned long long part1;

				int lastRow = currentRow[p] + blockSize;

				if (lastRow > n)
				{
					lastRow = n;
				}

				for (int j = currentRow[p]; j < lastRow; j++)
				{
					unsigned long long pmcalc = pm[B[j]];

					unsigned long long pre = localV & pmcalc;

					carrys[j] = _addcarry_u64(carrys[j], localV, pre, &part1);

					localV = part1 | (localV & ~pmcalc);
				}

				V[p] = localV;

				currentRow[p] += blockSize;
			}
		}
	}

	printf("Algorithm 2 - Time: \t %f : Threads %d\n", omp_get_wtime() - start, diagonalLength);

	delete carrys;

	delete currentRow;
}


void Algorithm3(char *B, unsigned long long *V, int n, int iterations, int diagonalLength, int blockSize, unsigned long long *PM)
{
	int *currentRow = new int[iterations];

	memset(currentRow, 0, iterations * sizeof(int));

	unsigned char *borrow = new unsigned char[n];

	memset(borrow, 0, n * sizeof(unsigned char));

	unsigned char *carrys = new unsigned char[n];

	memset(carrys, 0, n * sizeof(unsigned char));

	memset(V, 0xff, iterations * sizeof(unsigned long long));

	double start = omp_get_wtime();
	for (int i = 0; i < (iterations + diagonalLength); i++)
	{
#pragma omp parallel for schedule(static,1) num_threads(diagonalLength) 
		for (int p = i; p > i - diagonalLength; p--)
		{
			if ((p >= 0) && (p < iterations))
			{
				unsigned long long *pm = PM + (p << 2);

				unsigned long long localV = V[p];
				unsigned long long t1;
				unsigned long long p1;

				int lastRow = currentRow[p] + blockSize;

				if (lastRow > n)
				{
					lastRow = n;
				}
				for (int j = currentRow[p]; j < lastRow; j++)
				{
					unsigned long long tStr = pm[B[j]];

					unsigned long long y = localV & tStr;
					carrys[j] = _addcarry_u64(carrys[j], localV, y, &t1);
					borrow[j] = _subborrow_u64(borrow[j], localV, y, &p1);
					localV = t1 | p1;
				}
				V[p] = localV;

				currentRow[p] += blockSize;
			}
		}
	}
	printf("Algorithm 3 - Time: \t %f : Threads %d\n", omp_get_wtime() - start, diagonalLength);

	delete carrys;

	delete borrow;

	delete currentRow;
}

void Llcs(char *A, int m, char *B, int n, int threads)
{
	int iterations = 1 + ((m - 1) >> 6);

	//  allocate a preanalaysis matrixof the alphabet size

	unsigned long long *PM = new unsigned long long[4 * iterations];

	memset(PM, 0, sizeof(unsigned long long) * 4 * iterations);

	//  set the appropriate bits in the matrix

	for (char c = 0; c <= 4; c++)
	{
		for (int i = 0; i < m; i++)
		{
			if (c == A[i])
			{
				//  divide by 64 to get correct word and then multiply by 4 to get correct group

				PM[c + ((i >> 6) << 2)] |= (1ULL << (i % 64));
			}
		}
	}

	//  run the bitwise algorithm (note we are limited to a string of 64 bits here because of the long long

	unsigned long long *V = new unsigned long long[iterations];


	int blockSize = 1 + ((n - 1) / threads);

	Algorithm1(B, V, n, iterations, threads, blockSize, PM);

	printf("Algorithm 1 %d\r\n", Count1s(m, V));

	Algorithm2(B, V, n, iterations, threads, blockSize, PM);

	printf("Algorithm 2 %d\r\n", m - Count1s(m, V));

	Algorithm3(B, V, n, iterations, threads, blockSize, PM);

	printf("Algorithm 3 %d\r\n", m - Count1s(m, V));

	//	Cleanup memory

	delete PM;

	delete V;
}

int main()
{
	//  setup test strings

	//const char* testA = "survey"; // "ttgatacat"; // 
	//const char *testA="GTCTTACATCCGTTCG";
	//const char *testA = "aaagtgacctagcccg";
	const char *testA = "CCTTAAGAAAGAATTCTCTGCGTCATGATGATTGCAGGCAAGTCTAGATTTTCAGATAAGCGGCTCCACGACCTTAATATATGACTTCAGGG";
	//const char *testA = "GCCGAAGTGCAAACCCTGTGAGCCCGTTCCCGAGTCTGATGCGCGGATTTTTAAAATAGCGGGTGACGGCGACAGACATTCGAGGTAGTTAG";

	int mLength = strlen(testA);

	//const char* testB = "surgery";// "gaataagacc"; 
	//const char* testB = "TAGCTTAAGATCTTGT";
	//const char* testB = "tccagatg";
	const char* testB = "TTCTAGGAAGGTTACGAGTATTGCTTTGCACGTTCTAATCAGGTTCAGGGGTGTCAAAACGAGAAGTAGCACACAAATCATGCGACAGAGGT";
	//const char *testB = "AGCAACTGAGGTCGGGAGACCATCAATGAGCATCCAACCTCGGCTCAATGTTGGCGTAGTAAACTGGTGAGGGGGGGCTAAGCTAGACCGTA";

	int nLength = strlen(testB);

	//  initialize the restricted alphabet by offseting string


	int m = 1000000;
	int n = 1000000;

	//int m = 1000000000;
	//int n = 640;

	char* A = new char[m];

	memset(A, 0, m);



	for (int i = 0; i < m; i++)
	{
		switch (testA[i % mLength])
		{
		case 'A':
		case 'a':
			A[i] = 0;
			break;
		case 'C':
		case 'c':
			A[i] = 1;
			break;
		case 'G':
		case 'g':
			A[i] = 2;
			break;
		case 'T':
		case 't':
			A[i] = 3;
			break;
		}
	}


	char* B = new char[n];

	memset(B, 0, n);

	for (int i = 0; i < n; i++)
	{
		switch (testB[i % nLength])
		{
		case 'A':
		case 'a':
			B[i] = 0;
			break;
		case 'C':
		case 'c':
			B[i] = 1;
			break;
		case 'G':
		case 'g':
			B[i] = 2;
			break;
		case 'T':
		case 't':
			B[i] = 3;
			break;
		}
	}

	//  perfom the length longest common string calculation

	//ReferenceLlcs(A, m, B, n);

	for (int threads = 1; threads <= 10; threads++)
	{
		Llcs(A, m, B, n, threads);
	}

	getchar();

	delete A;

	delete B;

	return 0;
}

