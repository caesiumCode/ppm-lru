#include "PPMC.hpp"

template<int MAXIMUM_ORDER>
PPMC<MAXIMUM_ORDER>::PPMC(int window)
{
    lru_window = window;
    
    for (int k = 0; k <= MAXIMUM_ORDER; k++)
    {
        model[k].clear();
        if (lru_window > 0)
        {
            if      (k == 0)    lru[k].set_capacity(1);
            else if (k == 1)    lru[k].set_capacity(256);
            else                lru[k].set_capacity(lru_window);
        }
    }
    
    original_size = 0;
    encoded_size = 0;
}

template<int MAXIMUM_ORDER>
void PPMC<MAXIMUM_ORDER>::run(const std::string &path, const std::string &file)
{
    // setup file reading
    std::ifstream fin(path + file, std::ifstream::binary);
    const long BUFFER_SIZE = 16*1024;
    std::string prev_buffer (BUFFER_SIZE, '\0');
    std::string buffer      (BUFFER_SIZE, '\0');
    
    original_size = std::filesystem::file_size(path + file) * 8;
    
    // read file
    while (!fin.eof())
    {
        fin.read(buffer.data(), BUFFER_SIZE);
        
        for (int i = -1; i < int(fin.gcount()) - 1; i++)
        {
            double bits = next_encoding_size(i, buffer, prev_buffer);
            encoded_size += bits;            
        }
        
        prev_buffer.swap(buffer);
    }
    
    fin.close();
}

template<int MAXIMUM_ORDER>
double PPMC<MAXIMUM_ORDER>::next_encoding_size(int i, const std::string& buffer, const std::string& prev_buffer)
{
    bool                        predicted = false;
    double                      bits = 0;
    std::unordered_set<char>    exclusion;
    
    int order_split = std::min(i+1, MAXIMUM_ORDER);
    
    /*
     prev_buffer      buffer
                              i i+1
     ________________   _______(_)________
               |_____   ______|
                context_string
     */
    if (prev_buffer[0] != '\0') for (int k = MAXIMUM_ORDER; k > order_split; k--)
    {
        std::string context_string = prev_buffer.substr(prev_buffer.size() - k + i + 1) + buffer.substr(0, i + 1);
                
        process_context(context_string, k, buffer[i+1], bits, predicted, exclusion);
    }
    
    /*
     buffer
                    i i+1
     ________________(_)______________
        |___________|
        context_string
     */
    for (int k = order_split; k >= 0; k--)
    {
        std::string context_string = buffer.substr(i - k + 1, k);
                
        process_context(context_string, k, buffer[i+1], bits, predicted, exclusion);
    }
    
    if (!predicted) bits += std::log2(256 - exclusion.size());
        
    return bits;
}

template<int MAXIMUM_ORDER>
void PPMC<MAXIMUM_ORDER>::process_context(const std::string& context_string, int order, char next_char, double& bits, bool& predicted, std::unordered_set<char>& exclusion)
{
    auto insertion_result = model[order].insert({context_string, Context()});
    bool new_context = insertion_result.second;
    Context& context = insertion_result.first->second;
    
    if (lru_window > 0)
    {
        std::string removed;
        bool deletion = lru[order].request(context_string, removed);
        
        if (deletion)
        {
            model[order].erase(removed);
            remove_old_context(removed, order);
        }
    }
    
    if (new_context)
    {
        context.total_count = 2;
        context.escape = 1;
        context.counts[next_char] = 1;
    }
    else
    {
        bits += compute_encoding(context, next_char, predicted, exclusion);
        
        update_context(context, next_char);
    }
}

template<int MAXIMUM_ORDER>
void PPMC<MAXIMUM_ORDER>::remove_old_context(const std::string& removed, int order)
{
    char        sub_pred    = removed.back();
    std::string prefix      = removed.substr(0, order-1);
    
    std::unordered_map<std::string, Context>::iterator hint = model[order-1].find(prefix);
    
    if (hint != model[order-1].end())
    {
        Context& sub_context = hint->second;
        std::unordered_map<char, long>::iterator hint2 = sub_context.counts.find(sub_pred);
        
        
        if (hint2 != sub_context.counts.end())
        {
            sub_context.total_count -= hint2->second + 1;
            sub_context.escape--;
            sub_context.counts.erase(hint2);
            
            if (sub_context.total_count == 0)
            {
                model[order-1].erase(hint);
                lru[order-1].erase(prefix);
                
                remove_old_context(prefix, order-1);
            }
        }
    }
}

template<int MAXIMUM_ORDER>
double PPMC<MAXIMUM_ORDER>::compute_encoding(Context& context, char next_char, bool& predicted, std::unordered_set<char>& exclusion)
{
    double bits = 0;
    
    if (!predicted)
    {
        if (context.counts.contains(next_char))
        {
            long exclusion_count = 0;
            for (char sym : exclusion) if (context.counts.contains(sym)) exclusion_count += context.counts[sym];
            
            bits += std::log2( double(context.total_count - exclusion_count) / double(context.counts[next_char]) );
            
            predicted = true;
        }
        else
        {
            long exclusion_count = 0;
            for (char sym : exclusion) if (context.counts.contains(sym)) exclusion_count += context.counts[sym];
            
            bits += std::log2( double(context.total_count - exclusion_count) / double(context.escape) );
            
            for (const auto&[ch, count] : context.counts) exclusion.insert(ch);
        }
    }
    
    return bits;
}

template<int MAXIMUM_ORDER>
void PPMC<MAXIMUM_ORDER>::update_context(Context& context, char next_char)
{
    bool inserted = context.counts.insert({next_char, 1}).second;
    
    if (inserted)
    {
        context.total_count += 2;
        context.escape++;
    }
    else
    {
        context.counts[next_char]++;
        context.total_count++;
    }
}

template<int MAXIMUM_ORDER>
std::string PPMC<MAXIMUM_ORDER>::clean(std::string str)
{
    for (char& ch : str) ch = clean(ch);
    return str;
}

template<int MAXIMUM_ORDER>
char PPMC<MAXIMUM_ORDER>::clean(char ch)
{
    return ch == '\n' ? 'N' : ch;
}


template<int MAXIMUM_ORDER>
void PPMC<MAXIMUM_ORDER>::disp_stats()
{
    std::cout << std::setw(20) << "bpc" << std::setw(12) << encoded_size / original_size * 8 << std::endl;
    std::cout << std::setw(20) << "compression rate" << std::setw(12) << encoded_size / original_size << std::endl;
    
    long num_context = 0;
    for (int k = 0; k <= MAXIMUM_ORDER; k++) for (auto&[context_string, context] : model[k]) num_context += context.counts.size();
    
    std::cout << std::setw(20) << "context inst" << std::setw(12) << num_context << std::endl << std::endl;
    
    for (int k = 0; k <= MAXIMUM_ORDER; k++)
    {
        double mean_num_pred = 0;
        double mean_esc = 0;
        for (auto&[context_string, context] : model[k])
        {
            mean_num_pred += context.counts.size();
            mean_esc += double(context.escape) / double(context.total_count);
        }
        
        std::cout << "ORDER" << std::setw(3) << k;
        std::cout << std::setw(12) << model[k].size();
        std::cout << std::setw(12) << mean_num_pred / double(model[k].size());
        std::cout << std::setw(12) << mean_esc / double(model[k].size()) << std::endl;
    }
}

template<int MAXIMUM_ORDER>
void PPMC<MAXIMUM_ORDER>::disp_model()
{
    for (int k = 0; k <= MAXIMUM_ORDER; k++)
    {
        std::cout << "ORDER " << k << std::endl;
        
        for (const auto& [key, val] : model[k])
        {
            std::cout << std::setw(MAXIMUM_ORDER+1) << clean(key) << " -> " << std::setw(4) << "ESC" << std::setw(8) << val.escape << std::setw(10) << val.escape << "/" << val.total_count << std::endl;
            for (const auto&[ch, count] : val.counts)
            {
                std::cout << std::setw(MAXIMUM_ORDER+1) << "" << " -> " << std::setw(4) << (ch == '\n' ? 'N' : ch) << std::setw(8) << count << std::setw(10) << count << "/" << val.total_count << std::endl;
            }
            
            std::cout << std::endl;
        }
    }
}
