#include "stdafx.h"
#include <iostream>
#include <random>
#include <ctime>
#include <fstream>
#include <deque>


//__declspec(align(16))
// __restrict
namespace {

	struct CacheLine
	{
		long long cache_line_number = -1;
		long long stamp = -1;

		bool is_valid() {
			return cache_line_number != -1 && stamp != -1;
		}
	};

	class Cache {
	private:
		long long _cache_size;
		long long _associativity;
		long long _line_size;
		long long _block_lines_count;
		long long _index = 0;
		CacheLine* _cache;

	public:

		long long cache_misses = 0;
		long long cache_hits = 0;

		Cache(long long cache_size, long long associativity, long long line_size) {
			_cache_size = cache_size;
			_associativity = associativity;
			_line_size = line_size;

			_block_lines_count = cache_size / line_size / associativity;
			_cache = new CacheLine[_block_lines_count * _associativity];
		}

		void offer(long long address) {
			_index++;
			long long cache_line_number = address / _line_size;
			long long cache_line_number_block = cache_line_number % _block_lines_count;

			long long block_index = get_best_cache_block_index(cache_line_number, cache_line_number_block);

			//printf("%lld %lld %lld %lld\n", block_index, cache_line_number_block, _cache[block_index*_block_lines_count + cache_line_number_block].cache_line_number, cache_line_number);
			if (_cache[block_index*_block_lines_count + cache_line_number_block].cache_line_number == cache_line_number) {
				cache_hits += 1;
			}
			else {
				cache_misses += 1;
			}

			_cache[block_index*_block_lines_count + cache_line_number_block].cache_line_number = cache_line_number;
			_cache[block_index*_block_lines_count + cache_line_number_block].stamp = _index;
		}

		long long get_best_cache_block_index(long long cache_line_number, long long cache_line_number_block) {
			CacheLine best = CacheLine();
			long long best_index = 0;
			long long index = 0;
			bool best_initialized = false;
			for (long long block_index = 0; block_index < _associativity; ++block_index) {
				CacheLine cache_line = _cache[block_index*_block_lines_count + cache_line_number_block];

				//printf("Best is %lld %lld ", best.cache_line_number, best.stamp);
				if (cache_line.cache_line_number == cache_line_number) {
					best_initialized = true;
					best = cache_line;
					best_index = block_index;
					//printf("%lld hit\n", block_index);
					break;
				}
				else if (!cache_line.is_valid()) {
					//printf("%lld miss empty line\n", block_index);
					best_initialized = true;
					best = cache_line;
					best_index = block_index;
				}
				else if (!best_initialized) {
					//printf("%lld miss no best\n", block_index);
					best_initialized = true;
					best = cache_line;
					best_index = block_index;
				}
				else if (best.is_valid() && cache_line.stamp < best.stamp) {
					//printf("%lld miss better found\n", block_index);
					best_initialized = true;
					best = cache_line;
					best_index = block_index;
				}
			}

			return best_index;
		}

		~Cache() {
			delete _cache;
		}
	};

	void MultSimple(const float* __restrict a, const float* __restrict b, float* __restrict c, int n, Cache &cache)
	{
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				cache.offer(reinterpret_cast<long long>(c + i * n + j));
				c[i * n + j] = 0.f;
				for (int k = 0; k < n; ++k) {
					cache.offer(reinterpret_cast<long long>(c + i * n + j));
					cache.offer(reinterpret_cast<long long>(c + i * n + j));
					cache.offer(reinterpret_cast<long long>(a + i*n + k));
					cache.offer(reinterpret_cast<long long>(b + k * n + j));
					c[i * n + j] += a[i * n + k] * b[k * n + j];
				}
			}
		}
	}

	void MultSimpleBlock(const float* __restrict a, const float* __restrict b, float* __restrict c, int n, Cache &cache)
	{
		const int BLOCK = 10;
		for (int i = 0; i < n; i += BLOCK) {
			for (int j = 0; j < n; j += BLOCK) {
				for (int k = 0; k < n; k += BLOCK) {
					for (int ib = 0; ib < BLOCK && i + ib < n; ++ib) {
						for (int jb = 0; jb < BLOCK && j + jb < n; ++jb) {
							if (k == 0) {
								cache.offer(reinterpret_cast<long long>(c + (i + ib) * n + (j + jb)));
								c[(i + ib) * n + (j + jb)] = 0.f;
							}
							for (int kb = 0; k + kb < n && kb < BLOCK; ++kb) {
								cache.offer(reinterpret_cast<long long>(c + (i + ib) * n + j + jb));
								cache.offer(reinterpret_cast<long long>(c + (i + ib) * n + j + jb));
								cache.offer(reinterpret_cast<long long>(a + (i + ib) * n + k + kb));
								cache.offer(reinterpret_cast<long long>(b + (k + kb) * n + j + jb));
								c[(i + ib) * n + j + jb] += a[(i + ib) * n + k + kb] * b[(k + kb) * n + j + jb];
							}
						}
					}
				}
			}
		}
	}

	void MultSwappedLoops(const float* __restrict a, const float* __restrict b, float * __restrict c, int n, Cache &cache)
	{
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				cache.offer(reinterpret_cast<long long>(c + i * n + j));
				c[i * n + j] = 0.f;
			}

			for (int k = 0; k < n; ++k) {
				for (int j = 0; j < n; ++j) {
					cache.offer(reinterpret_cast<long long>(c + i * n + j));
					cache.offer(reinterpret_cast<long long>(c + i * n + j));
					cache.offer(reinterpret_cast<long long>(a + i * n + k));
					cache.offer(reinterpret_cast<long long>(b + k * n + j));
					c[i * n + j] += a[i * n + k] * b[k * n + j];
				}
			}
		}
	}

	void FillRandom(float* a, int n)
	{
		std::default_random_engine eng;
		std::uniform_real_distribution<float> dist;

		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				a[i * n + j] = dist(eng);
			}
		}
	}
}

int main(int argc, char* argv[])
{
	std::ofstream data_collector;
	data_collector.open("1040_float_cache_misses_L3_new_code.csv");

	const int cache_size = 8388608;
	const int associativity = 16;
	const int cache_line_size = 64;

	int n = 1040;

	std::cerr << "n: " << n << '\n';


	float* a = new float[n * n];
	float* b = new float[n * n];
	float* c = new float[n * n];

	FillRandom(a, n);
	FillRandom(b, n);

	{
		Cache* cache = new Cache(cache_size, associativity, cache_line_size);

		const auto startTime = std::clock();
		MultSimple(a, b, c, n, *cache);
		const auto endTime = std::clock();

		double time = double(endTime - startTime) / CLOCKS_PER_SEC;
		std::cerr << "timeSimple: " << time << '\n';

		std::cerr << cache->cache_hits << "," << cache->cache_misses << '\n';
		data_collector << cache->cache_hits << ", " << cache->cache_misses << '\n';
		delete cache;
	}

	{
		Cache* cache = new Cache(cache_size, associativity, cache_line_size);

		const auto startTime = std::clock();
		MultSimpleBlock(a, b, c, n, *cache);
		const auto endTime = std::clock();

		double time = double(endTime - startTime) / CLOCKS_PER_SEC;
		std::cerr << "timeBlock: " << time << '\n';


		std::cerr << cache->cache_hits << "," << cache->cache_misses << '\n';
		data_collector << cache->cache_hits << ", " << cache->cache_misses << '\n';
		delete cache;
	}

	{
		Cache* cache = new Cache(cache_size, associativity, cache_line_size);

		const auto startTime = std::clock();
		MultSwappedLoops(a, b, c, n, *cache);
		const auto endTime = std::clock();

		double time = double(endTime - startTime) / CLOCKS_PER_SEC;
		std::cerr << "timeSwapped: " << time << '\n';


		std::cerr << cache->cache_hits << "," << cache->cache_misses << '\n';
		data_collector << cache->cache_hits << ", " << cache->cache_misses << '\n';
		delete cache;
	}

	delete[] a;
	delete[] b;
	delete[] c;
	
	data_collector.close();
}