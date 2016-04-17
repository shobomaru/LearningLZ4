#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>
#include "lz4.h"

using namespace std;

// FNV hash algorithm

static const uint64_t FNV_OFFSET_BASIS_64 = 14695981039346656037U;
static const uint64_t FNV_PRIME_64 = 1099511628211LLU; 
int64_t fnv_1_hash_64(const uint8_t *bytes, size_t length)
{
	uint64_t hash;
	size_t i;

	hash = FNV_OFFSET_BASIS_64;
	for (i = 0; i < length; ++i) {
		hash = (FNV_PRIME_64 * hash) ^ (bytes[i]);
	}

	return hash;
}

int main(int argc, char** argv)
{
	vector<char> input, output;
	int64_t hash1, hash2;
	int sizeInput = 0, sizeCompressed = 0;

	// Read uncompressed data, and generate hash

	ifstream ifs("_binary.ex_", ios::binary);
	if (!ifs)
		return 1;

	ifs.seekg(0, ios::end);
	sizeInput = (int)ifs.tellg();
	input.resize(sizeInput);
	ifs.seekg(0);
	ifs.read(input.data(), input.size());
	ifs.close();
	hash1 = fnv_1_hash_64((uint8_t*)input.data(), input.size());

	cout << "Input size = " << input.size() << endl;
	cout << "Input hash = " << hex << hash1 << endl;

	// Compress

	const int MessageMaxBytes = 8192;

	{
		LZ4_stream_t lz4body;
		LZ4_resetStream(&lz4body);
		LZ4_stream_t* const lz4 = &lz4body;

		const int sizeMsg = LZ4_COMPRESSBOUND(MessageMaxBytes);
		char temp[sizeof(int) + sizeMsg];
		int posIn = 0;

		char src1[2][MessageMaxBytes];
		int x = 0;
		(src1);
		(x);
		for (;;)
		{
#if 1 // Sample code method

			// Caclurate source size
			auto sizeIn = min(MessageMaxBytes, (int)input.size() - posIn);
			if (sizeIn <= 0)
				break;
			// Copy source to temporary buffer
			copy(input.data() + posIn, input.data() + posIn + sizeIn, &src1[x][0]);
			// LZ4
			int size = LZ4_compress_fast_continue(lz4, &src1[x][0], &temp[sizeof(int)], sizeIn, sizeMsg, 1);
			if (size <= 0)
				return 3;
			// Insert block size
			auto* p = reinterpret_cast<char*>(&size);
			copy(p, p + sizeof(int), &temp[0]);
			// Save
			for_each(temp, temp + sizeof(int) + size, [&](auto& a) { output.push_back(a); });
			// Advance input pointer
			posIn += sizeIn;
			// Swap temporary buffers
			x ^= 1;

#else // Avoid copying method

			// Caclurate source size
			auto sizeIn = min(MessageMaxBytes, (int)input.size() - posIn);
			if (sizeIn <= 0)
				break;
			// LZ4
			int size = LZ4_compress_fast_continue(lz4, input.data() + posIn, &temp[sizeof(int)], sizeIn, sizeMsg, 1);
			if (size <= 0)
				return 3;
			// Insert block size
			auto* p = reinterpret_cast<char*>(&size);
			copy(p, p + sizeof(int), &temp[0]);
			// Save
			for_each(temp, temp + sizeof(int) + size, [&](auto& a) { output.push_back(a); });
			// Advance input pointer
			posIn += sizeIn;

#endif
		}

		sizeCompressed = (int)output.size();
	}

	cout << "Compress size = " << dec << output.size() << endl;
	cout << "Compress hash = " << hex << fnv_1_hash_64((uint8_t*)output.data(), output.size()) << endl;

	// Destroy original data

	input.clear();
	input.shrink_to_fit();
	swap(input, output);

	// Decompress

	// Make sure not to realloc because LZ4 needs to read previous decompressed data
	output.resize(sizeInput);

	{
		LZ4_streamDecode_t lz4body;
		LZ4_setStreamDecode(&lz4body, nullptr, 0);
		LZ4_streamDecode_t* const lz4 = &lz4body;

		int posIn = 0;
		int posOut = 0;

		while (posIn < sizeCompressed)
		{
			// Read block size
			int sizeIn = 0;
			copy(&input[posIn], &input[posIn + sizeof(int)], reinterpret_cast<char*>(&sizeIn));
			posIn += sizeof(int);
			// LZ4
			int size = LZ4_decompress_safe_continue(lz4, &input[posIn], &output[posOut], sizeIn, MessageMaxBytes);
			if (size <= 0)
				return 5;
			// Advance pointers
			posIn += sizeIn;
			posOut += size;
		}

		output.resize(posOut);
	}

	cout << "Decompress size = " << dec << output.size() << endl;

	// Validate

	hash2 = fnv_1_hash_64((uint8_t*)output.data(), output.size());
	cout << "Decompress hash = " << hex << hash2 << endl;

	if (sizeInput == output.size() && hash1 == hash2)
		cout << "Success!" << endl;
	else
		cout << "Failed." << endl;

	return 0;
}
