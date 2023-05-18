#include "PPMC.hpp"

PPMC::PPMC()
{
    maximum_order   = 0;
    original_size   = 0;
    encoded_size    = 0;
}

void PPMC::set_order(int order)
{
    maximum_order = order;
}

void PPMC::run(const std::string &path, const std::string &file)
{
    // setup file reading
    std::ifstream fin(path + file, std::ifstream::binary);
    const long BUFFER_SIZE = 16*1024;
    std::string prev_buffer (BUFFER_SIZE, '\0');
    std::string buffer      (BUFFER_SIZE, '\0');
    
    original_size = std::filesystem::file_size(path + file) * 8L;
    
    // read file
    while (!fin.eof())
    {
        fin.read(buffer.data(), BUFFER_SIZE);
        int read = (int)fin.gcount();
        
        for (int i = -1; i < read - 1; i++)
        {
            double bits = next_encoding_size(i, buffer, prev_buffer);
            encoded_size += bits;
        }
        
        prev_buffer.swap(buffer);
    }
    
    fin.close();
}

double PPMC::next_encoding_size(int i, const std::string& buffer, const std::string& prev_buffer)
{
    bool                        predicted = false;
    double                      bits = 0;
    std::unordered_set<char>    exclusion;
    
    int order_split = std::min(i+1, maximum_order);
    
    /*
     prev_buffer      buffer
                              i i+1
     ________________   _______(_)________
               |_____   ______|
                context_string
     */
    if (prev_buffer[0] != '\0') for (int k = maximum_order; k > order_split; k--)
    {
        std::string context_string = prev_buffer.substr(prev_buffer.size() - k + i + 1) + buffer.substr(0, i + 1);
                
        process_context(context_string, buffer[i+1], bits, predicted, exclusion);
    }
    
    /*
     buffer
                    i i+1
     ________________(_)______________
        |___________|
        context_string
     */
    for (int k = order_split; k > 0; k--)
    {
        std::string context_string = buffer.substr(i - k + 1, k);
                
        process_context(context_string, buffer[i+1], bits, predicted, exclusion);
    }
    
    model_root.count++;
    process_context("", buffer[i+1], bits, predicted, exclusion);
    
    
    if (!predicted) bits += std::log2(256 - exclusion.size());
            
    return bits;
}

void PPMC::process_context(const std::string& context_string, char pred, double& bits, bool& predicted, std::unordered_set<char>& exclusion)
{
    // Find context
    int             order       = (int)context_string.size();
    ContextNode*    context     = &model_root;
    for (int k = 0; k < order; k++)
    {
        int i = 0;
        while (context->children[i]->ch != context_string[k]) i++;
        
        context = context->children[i].get();
    }
    std::vector<std::unique_ptr<ContextNode>>& predictions = context->children;
    
    // Compute bit
    int pred_i = (int)predictions.size();
    long count_exclusion = 0;
    long count_context   = 0;
    
    for (int i = 0; i < predictions.size(); i++)
    {
        char ch = predictions[i]->ch;
        
        if (ch == pred)             pred_i = i;
        //if (exclusion.contains(ch)) count_exclusion += predictions[i]->count;
        count_context += predictions[i]->count;
    }
        
    bool found_prediction = (pred_i != predictions.size());
    
    long count_context_pred;
    if (found_prediction)   count_context_pred = predictions[pred_i]->count;
    else                    count_context_pred = predictions.size();
    
    if (!predicted && !predictions.empty())
    {
        bits        += std::log2(double(count_context + predictions.size() - count_exclusion)/double(count_context_pred));
        predicted   = found_prediction;
    }
    
    // Update model
    //if (!predicted) for (int i = 0; i < predictions.size(); i++) exclusion.insert(predictions[i]->ch);
    
    if (!found_prediction)
    {
        predictions.emplace_back(new ContextNode(pred));
        pred_i = (int)predictions.size()-1;
    }
    
    predictions[pred_i]->count++;
}

void tree_size(ContextNode* node, std::vector<long>& layers, int depth)
{
    layers[depth]++;
    
    for (std::unique_ptr<ContextNode>& child : node->children) tree_size(child.get(), layers, depth+1);
}

void PPMC::disp_stats()
{
    std::cout << std::setw(20) << "bpc" << std::setw(12) << double(encoded_size) / double(original_size) * 8.0 << std::endl;
    std::cout << std::setw(20) << "compression ratio" << std::setw(12) << double(encoded_size) / double(original_size) << std::endl;
 
    
    std::vector<long> context_counter(maximum_order+2, 0);
    tree_size(&model_root, context_counter, 0);
    long sum = 0;
    for (long c : context_counter) sum += c;
    
    std::cout << std::setw(20) << "context nodes" << std::setw(12) << sum << std::endl << std::endl;
    std::cout << std::setw(6) << "ORDER" << std::setw(20) << "NUMBER OF CONTEXTS" << std::endl;
    
    for (int k = 0; k <= maximum_order; k++) std::cout << std::setw(6) << k << std::setw(20) << context_counter[k] << std::endl;
    
    std::cout << std::setw(6) << "(" + std::to_string(maximum_order+1) << std::setw(20) << context_counter[maximum_order+1] << ")" << std::endl;
}

