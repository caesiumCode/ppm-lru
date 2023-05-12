#ifndef PPMC_hpp
#define PPMC_hpp

#include "utils.hpp"
#include "LRU.cpp"
#include "LRU.hpp"

template<int MAXIMUM_ORDER>
class PPMC
{
public:
    PPMC(int window = -1);
    
    void run(const std::string& path, const std::string& file);
    void disp_stats();
    
private:
    std::array<std::unordered_map<std::string, Context>, MAXIMUM_ORDER + 1> model;
    std::array<LRU<std::string>, MAXIMUM_ORDER + 1> lru;
    
    double  original_size;
    double  encoded_size;
    
    int     lru_window;
    
private:
    double  next_encoding_size  (int i, const std::string& buffer, const std::string& prev_buffer);
    void    process_context     (const std::string& context_string, int order, char next_char, double& bits, bool& predicted, std::unordered_set<char>& exclusion);
    void    remove_old_context  (const std::string& removed_string, int order);
    double  compute_encoding    (Context& context, char next_char, bool& predicted, std::unordered_set<char>& exclusion);
    void    update_context      (Context& context, char next_char);
    
    std::string clean(std::string str);
    char        clean(char ch);
    void        disp_model();
};

#endif /* PPMC_hpp */
