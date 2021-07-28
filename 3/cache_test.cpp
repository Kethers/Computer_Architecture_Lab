#include <iostream>
#include <time.h>
#include <Windows.h>
#include <cstdlib>
#include <vector>
#include <algorithm>

using namespace std;

#define ARRAY_SIZE (1 << 28) // test array size is 2^28=256MB
typedef unsigned char BYTE;	 // define BYTE as one-byte type

#define KB_LEN 10

BYTE t_array[ARRAY_SIZE];	 // test array
int L1_cache_size = 1 << 15; //32KB
int L2_cache_size = 1 << 18; //256KB
int L1_cache_block = 64;
int L2_cache_block = 64;
int L1_way_count = 8;
int L2_way_count = 4;
// int write_policy = 0; // 0 for write back ; 1 for write through

// have an access to arrays with L2 Data Cache'size to clear the L1 cache
void Clear_L1_Cache()
{
	memset(t_array, 0, L2_cache_size);
}

// have an access to arrays with ARRAY_SIZE to clear the L2 cache
void Clear_L2_Cache()
{
	memset(&t_array[L2_cache_size + 1], 0, ARRAY_SIZE - L2_cache_size);
}

int get_Max_index_in_Vec(const vector<int> &vec)
{
	int max_index = 0, max_diff = INT_MIN;
	for (int i = 0; i < vec.size(); i++)
	{
		if (vec[i] > max_diff)
		{
			max_diff = vec[i];
			max_index = i;
		}
	}
	return max_index;
}

int time_of_loop_array_traverse(const unsigned long long &test_array_size,
								const unsigned long long &step_len,
								const unsigned long long &time_loop_count)
{
	int temp = 0;
	int forward_index = 0;
	clock_t start = clock();
	for (int i = 0; i < time_loop_count; i++)
	{
		temp &= t_array[forward_index];
		forward_index += step_len;
		if (forward_index > test_array_size)
		{
			forward_index = 0;
		}
	}
	return clock() - start;
}

int L1_DCache_Size()
{
	cout << "L1_Data_Cache_Test" << endl;
	//add your own code

	Clear_L1_Cache();

	vector<int> time_diff;
	// unsigned int KB_len = 10;
	unsigned int begin_len = 3;
	unsigned long long test_array_size = 1 << (KB_LEN + begin_len);
	const int test_max_size = 128 * 1024; //128KB
	const int step_block_size = 64;
	const unsigned long long time_loop_count = 150000000;
	int this_time = 0, last_time = 0;
	int l1_dcache_size = 0;

	for (int j = 0; test_array_size <= test_max_size; j++, test_array_size <<= 1)
	{
		Clear_L1_Cache();

		this_time = time_of_loop_array_traverse(test_array_size, step_block_size, time_loop_count);
		if (last_time != 0)
		{
			time_diff.push_back(this_time - last_time);
		}
		last_time = this_time;

		cout << "Test_Array_Size = " << (test_array_size >> 10) << "KB\t"
			 << "Average access time = " << this_time << "ms" << endl;
	}

	int max_index = get_Max_index_in_Vec(time_diff);

	l1_dcache_size = 1 << (max_index + KB_LEN + begin_len);

	cout << "L1_Data_Cache_Size is " << (l1_dcache_size >> KB_LEN) << "KB" << endl;
	cout << "----------------------------------------------------------" << endl;
	cout << endl;

	Clear_L1_Cache();
	return l1_dcache_size;
}

int L2_Cache_Size()
{
	cout << "L2_Data_Cache_Test" << endl;
	//add your own code

	Clear_L2_Cache();
	vector<int> time_diff;
	// unsigned int KB_len = 10;
	unsigned int begin_len = 6;
	unsigned long long test_array_size = 1 << (KB_LEN + begin_len);
	const int test_max_size = 1024 * 1024; //1024KB
	const int step_block_size = 64;
	const unsigned long long time_loop_count = 150000000;
	int this_time = 0, last_time = 0;
	int l2_cache_size = 0;

	for (int j = 0; test_array_size <= test_max_size; j++, test_array_size <<= 1)
	{
		Clear_L2_Cache();

		this_time = time_of_loop_array_traverse(test_array_size, step_block_size, time_loop_count);
		if (last_time != 0)
		{
			time_diff.push_back(this_time - last_time);
		}
		last_time = this_time;

		cout << "Test_Array_Size = " << (test_array_size >> 10) << "KB\t"
			 << "Average access time = " << this_time << "ms" << endl;
	}

	int max_index = get_Max_index_in_Vec(time_diff);
	l2_cache_size = 1 << (max_index + KB_LEN + begin_len);

	cout << "L2_Data_Cache_Size is " << (l2_cache_size >> KB_LEN) << "KB" << endl;
	cout << "----------------------------------------------------------" << endl;
	cout << endl;

	Clear_L2_Cache();
	return l2_cache_size;
}

int L1_DCache_Block()
{
	cout << "L1_DCache_Block_Test" << endl;
	//add your own code

	Clear_L1_Cache();

	vector<int> time_diff;
	const int l1_cache_size = 1 << 15;
	unsigned int begin_len = 3;
	const unsigned long long time_loop_count = 300000000;
	int this_time = 0, last_time = 0;
	int l1_dcache_block_size = 0;
	vector<int> blocks = {8, 16, 32, 64, 128};

	for (auto const block_len : blocks)
	{
		this_time = time_of_loop_array_traverse(L1_cache_size, block_len, time_loop_count);
		if (last_time != 0)
		{
			time_diff.push_back(this_time - last_time);
		}
		last_time = this_time;

		cout << "Test_Block_Size = " << block_len << "B\t"
			 << "Average access time = " << this_time << "ms" << endl;
	}

	int max_index = get_Max_index_in_Vec(time_diff);
	l1_dcache_block_size = 1 << (begin_len + max_index + 1);

	cout << "L1_DBlock_Size is " << l1_dcache_block_size << "B" << endl;
	cout << "----------------------------------------------------------" << endl;
	cout << endl;

	Clear_L1_Cache();
	return l1_dcache_block_size;
}

int L2_Cache_Block()
{
	cout << "L2_Cache_Block_Test" << endl;
	//add your own code

	Clear_L2_Cache();

	vector<int> time_diff;
	const int l1_cache_size = 1 << 15;
	unsigned int begin_len = 3;
	vector<int> blocks = {8, 16, 32, 64, 128};
	const unsigned long long time_loop_count = 300000000;
	int this_time = 0, last_time = 0;
	int l2_cache_block_size = 0;

	for (auto const block_len : blocks)
	{
		this_time = time_of_loop_array_traverse(L2_cache_size, block_len, time_loop_count);
		if (last_time != 0)
		{
			time_diff.push_back(this_time - last_time);
		}
		last_time = this_time;

		cout << "Test_Block_Size = " << block_len << "B\t"
			 << "Average access time = " << this_time << "ms" << endl;
	}

	int max_index = get_Max_index_in_Vec(time_diff);
	l2_cache_block_size = 1 << (begin_len + max_index + 1);

	cout << "L2 Block_Size is " << l2_cache_block_size << "B" << endl;
	cout << "----------------------------------------------------------" << endl;
	cout << endl;

	Clear_L2_Cache();
	return l2_cache_block_size;
}

int test_L1_DCache_Way_Count(const int test_array_size,
							 const int block_num,
							 const unsigned long long &time_loop_count)
{
	int block_size = test_array_size / block_num;
	int temp = 0;
	int start;

	start = clock();
	for (int i = 0; i < time_loop_count / block_num; i++)
	{
		for (int j = 0; j < block_num; j += 2)
		{
			t_array[j * block_size] = ((t_array[j * block_size] + 13) * 7) % 11;
		}
	}
	return clock() - start;
}

int L1_DCache_Way_Count()
{
	cout << "L1_DCache_Way_Count" << endl;
	//add your own code

	Clear_L1_Cache();

	const int test_array_size = 2 * L1_cache_size;
	// const int block_num_of_L1_cache = L1_cache_size / L1_cache_block;
	// int block_num_in_cache_set = 2;
	// int set_of_L1_cache = block_num_of_L1_cache / block_num_in_cache_set;
	// int block_num_of_mem_set = set_of_L1_cache;
	const unsigned long long time_loop_count = 150000000;
	int this_time = 0, last_time = 0;
	vector<int> time_diff;
	int l1_dcache_way_count = 0;

	int block_num = 4;

	for (int i = 0; i < 8; i++, block_num <<= 1)
	{
		Clear_L1_Cache();

		this_time = test_L1_DCache_Way_Count(test_array_size, block_num, time_loop_count);
		if (last_time != 0)
		{
			time_diff.push_back(this_time - last_time);
		}
		last_time = this_time;

		cout << "L1_DCache_Way_Count_Test = " << block_num << "\t"
			 << "Average access time = " << this_time << "ms" << endl;
	}

	int max_index = get_Max_index_in_Vec(time_diff);
	l1_dcache_way_count = 1 << (max_index + 1);

	cout << "L1_DCache_Way_Count is " << l1_dcache_way_count << endl;
	cout << "----------------------------------------------------------" << endl;
	cout << endl;

	Clear_L1_Cache();
	return l1_dcache_way_count;
}

int test_L2_Cache_Way_Count(const int test_array_size,
							const int block_num,
							const unsigned long long &time_loop_count)
{
	int block_size = test_array_size / block_num;
	int temp = 0;
	int index = 0;
	int offset = 0;
	int start;

	start = clock();
	for (int i = 0; i < time_loop_count / block_num; i++)
	{
		for (int j = 0; j < block_num; j += 2)
		{
			index = j * block_size + offset;
			t_array[index] = ((t_array[index] + 13) * 7) % 11;
		}
		offset += 4 * 1024; //4KB
		if (offset > block_size)
		{
			offset = 0;
		}
	}
	return clock() - start;
}

int L2_Cache_Way_Count()
{
	cout << "L2_Cache_Way_Count" << endl;
	//add your own code

	Clear_L2_Cache();

	const int test_array_size = 2 * L2_cache_size;
	const unsigned long long time_loop_count = 150000000;
	int block_num = 4;
	int this_time = 0;
	vector<int> time_diff;
	int l2_cache_way_count = 0;

	for (int i = 0; i < 7; i++, block_num <<= 1)
	{
		Clear_L2_Cache();

		this_time = test_L2_Cache_Way_Count(test_array_size, block_num, time_loop_count);
		time_diff.push_back(this_time);

		cout << "L2_Cache_Way_Count_Test = " << block_num << "\t"
			 << "Average access time = " << this_time << "ms" << endl;
	}

	int max_index = get_Max_index_in_Vec(time_diff);
	l2_cache_way_count = 1 << (max_index);

	cout << "L2_Cache_Way_Count is " << l2_cache_way_count << endl;
	cout << "----------------------------------------------------------" << endl;
	cout << endl;

	Clear_L2_Cache();
	return l2_cache_way_count;
}

//int Cache_Write_Policy() {
//	cout << "Cache_Write_Policy" << endl;
//	//add your own code
//}
//
//void Check_Swap_Method() {
//	cout << "L1_Check_Replace_Method" << endl;
//	//add your own code
//}

int main()
{
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	L1_cache_size = L1_DCache_Size();
	L2_cache_size = L2_Cache_Size();
	L1_cache_block = L1_DCache_Block();
	L2_cache_block = L2_Cache_Block();
	L1_way_count = L1_DCache_Way_Count();
	L2_way_count = L2_Cache_Way_Count();

	// write_policy = Cache_Write_Policy();
	// Check_Swap_Method();
	// system("pause");
	return 0;
}
