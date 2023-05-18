#ifndef PPMC_hpp
#define PPMC_hpp

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
#include <list>
#include <memory>
#include <queue>

struct ContextNode
{
    ContextNode()
    {
        ch       = ' ';
        count    = 0;
        children.clear();
    }
    ContextNode(char _ch)
    {
        ch       = _ch;
        count    = 0;
        children.clear();
    }
    
    char ch;
    long count;
    
    bool leaf;
    std::vector<std::unique_ptr<ContextNode>> children;
};

class PPMC
{
public:
    PPMC();
    
    void set_order(int order);
    
    void run(const std::string& path, const std::string& file);
    void disp_stats();
    
private:
    int         maximum_order;
    ContextNode model_root;
    
    long original_size;
    long encoded_size;
        
private:
    double  next_encoding_size  (int i, const std::string& buffer, const std::string& prev_buffer);
    void    process_context     (const std::string& context_string, char pred, double& bits, bool& predicted, std::unordered_set<char>& exclusion);

};

#endif /* PPMC_hpp */
