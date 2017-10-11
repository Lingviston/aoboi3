#include <iostream>
#include "stdio.h"
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
			} else {
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
				} else if (!cache_line.is_valid()) {
					//printf("%lld miss empty line\n", block_index);
				    best_initialized = true;
					best = cache_line;
					best_index = block_index;
				} else if (!best_initialized) {
					//printf("%lld miss no best\n", block_index);
				    best_initialized = true;
					best = cache_line;
					best_index = block_index;
				} else if (best.is_valid() && cache_line.stamp < best.stamp) {
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
}

int main(int argc, char* argv[])
{

	long long cache_size;
	long long associativity;
	long long cache_line_size;
	long long n;
	
	scanf("%lld %lld %lld %lld", &cache_size, &associativity, &cache_line_size, &n);
	
	Cache* cache = new Cache(cache_size, associativity, cache_line_size);
	
	for(int i = 0; i < n; ++i) {
		long long address;
		
		scanf("%lld", &address);
		
		cache->offer(address);
	}
	
	printf("%lld %lld", cache->cache_hits, cache->cache_misses);
	
	delete cache;
}