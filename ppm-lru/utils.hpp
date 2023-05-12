#ifndef utils_h
#define utils_h

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <cmath>
#include <filesystem>


using Timer         = std::chrono::high_resolution_clock;
using TimerMeasure  = std::chrono::time_point<Timer>;


struct Context
{
    Context()
    {
        total_count = 0;
        escape = 0;
        
        counts.clear();
    }
    
    long total_count;
    long escape;
    
    std::unordered_map<char, long> counts;
};

struct ContextNode
{
    Context context;
    
    std::unordered_map<char, ContextNode*> children;
};




#endif /* utils_h */
