#include "PPMR.hpp"

PPMR::PPMR()
{
    model_root              = new ContextNode;
    model_root->leaf        = true;
    model_root->children    = {
        {0, nullptr},
        {1, nullptr}
    };
    
    front_queue = model_root;
    back_queue  = model_root;
    
    node_limit   = -1;
    order_limit = 0;
    
    node_counter = 1;
    leaf_counter = 1;
    
    encoded_size = 0;
    original_size = 0;
}

PPMR::~PPMR()
{
    delete model_root;
}

void PPMR::set_node_limit(long lim)
{
    node_limit = lim;
}

void PPMR::set_order_limit(int lim)
{
    order_limit = lim;
}


void PPMR::run(const std::string &path, const std::string &file)
{
    TimerMeasure START = Timer::now();
    
    // setup file reading
    std::ifstream fin(path + file, std::ifstream::binary);
    const long BUFFER_SIZE = 16*1024;
    std::string prev_buffer (BUFFER_SIZE, '\0');
    std::string buffer      (BUFFER_SIZE, '\0');
    
    original_size = std::filesystem::file_size(path + file) * 8L;
    char last_prev = '\0';
    hand.resize(order_limit+1, nullptr);
    
    // read file
    while (!fin.eof())
    {
        fin.read(buffer.data(), BUFFER_SIZE);
        int read = (int)fin.gcount();
        
        for (int i = -1; i < read - 1; i++)
        {
            double bits = next_encoding_size(i, last_prev, buffer, prev_buffer);
            encoded_size += bits;
            
            while (node_limit > 0 && node_counter > node_limit)
            {
                pop_leaf_queue();
                node_counter--;
            }
            
            last_prev = buffer[i+1];
        }
        
        prev_buffer.swap(buffer);
    }
    
    fin.close();
    
    TimerMeasure END = Timer::now();
    
    t_file = file;
    t_time = std::chrono::duration<double>(END - START).count();
}

double PPMR::next_encoding_size(int i, char last_prev, const std::string& buffer, const std::string& prev_buffer)
{
    bool                        predicted = false;
    double                      bits = 0;
    std::unordered_set<char>    exclusion;
    
    // update hand
    for (int k = (int)hand.size()-1; k > 0; k--)
    {
        if (hand[k-1] != nullptr && !hand[k-1]->leaf)
        {
            std::unordered_map<char, ContextNode*>::iterator it = hand[k-1]->children.find(last_prev);
            if (it != hand[k-1]->children.end())    hand[k] = it->second;
            else                                    hand[k] = nullptr;
        }
        
        else hand[k] = nullptr;
    }
    hand[0] = model_root;
    
    // Process each context, starting by the longest
    for (int k = order_limit; k >= 0; k--) if (hand[k] != nullptr) bits += process_context(hand[k], buffer[i+1], predicted, exclusion);
        
    if (!predicted) bits += std::log2( 256 - exclusion.size() );
    
    model_root->count++;
    return bits;
}

double PPMR::process_context(ContextNode *context, char pred, bool& predicted, std::unordered_set<char>& exclusion)
{
    double bits = 0;
    ContextNode* pred_node = nullptr;
    
    // Compute bit
    long context_pred_count = 0;
    long exclusion_count    = 0;
    
    std::unordered_map<char, ContextNode*>::iterator it = context->children.find(pred);
    if (!context->leaf)
    {
        if (it != context->children.end())
        {
            pred_node           = it->second;
            context_pred_count  = pred_node->count;
        }
        
        for (char ch : exclusion)
        {
            std::unordered_map<char, ContextNode*>::iterator itt = context->children.find(ch);
            if (itt != context->children.end()) exclusion_count += itt->second->count;
        }
    }
    
    bool found_pred = (!context->leaf) && (it != context->children.end());
    if (!found_pred) context_pred_count = context->children.size();
    
    
    if (!predicted && !context->leaf)
    {
        bits += std::log2( double(context->count_as_context + context->children.size() - exclusion_count) / double(context_pred_count) );
        predicted = found_pred;
    }
    
    // update model
    if (!predicted && !context->leaf) for (const auto&[ch, child] : context->children) exclusion.insert(child->ch);
    
    if (!found_pred)
    {
        node_counter++;
        
        pred_node = new ContextNode(pred);
        pred_node->parent = context;
        
        if (context->leaf) remove_leaf_queue(context);
        context->children.insert({pred, pred_node});
        
        push_back_leaf_queue(pred_node);
    }
    
    if (pred_node)
    {
        context->count_as_context++;
        pred_node->count++;
    }
    
    return bits;
}

void PPMR::tree_size(ContextNode* node, std::vector<layer>& layers, int depth)
{
    if (depth >= layers.size()) layers.push_back({0, 0, 0});

    layers[depth].distinct++;
    layers[depth].count += node->count;
    
    if (!node->leaf) for (const auto&[ch, child] : node->children) tree_size(child, layers, depth+1);
    else layers[depth].leaves++;
}

void PPMR::disp_stats()
{
    std::cout << std::setw(20) << "bpc" << std::setw(12) << encoded_size / original_size * 8.0 << std::endl;
    std::cout << std::setw(20) << "compression ratio" << std::setw(12) << encoded_size / original_size << std::endl;
 
    
    std::vector<layer> layers;
    tree_size(model_root, layers);
    
    
    std::cout << std::setw(20) << "context nodes" << std::setw(12) << node_counter << std::endl << std::endl;
    
    
    std::cout << std::setw(6) << "ORDER" << std::setw(20) << "NUMBER OF CONTEXTS" << std::endl;
    
    for (int k = 0; k < layers.size() - 1; k++) std::cout << std::setw(6) << k << std::setw(20) << layers[k].distinct << std::setw(20) << double(layers[k].count)/double(layers[k].distinct) << std::setw(20) << double(layers[k].leaves)/double(layers[k].distinct) << std::endl;
    
    int k = (int)layers.size()-1;
    std::cout << std::setw(6) << "(" + std::to_string(k) << std::setw(20) << layers[k].distinct << std::setw(20) << double(layers[k].count)/double(layers[k].distinct) << std::setw(20) << double(layers[k].leaves)/double(layers[k].distinct) << std::endl;
}

std::string PPMR::stats()
{
    std::string output;
    
    output += t_file + ",";
    output += std::to_string(order_limit) + ",";
    output += std::to_string(node_limit) + ",";
    output += std::to_string(encoded_size / original_size * 8.0) + ",";
    output += std::to_string(node_counter) + ",";
    output += std::to_string(t_time);
    
    return output;
}

void PPMR::disp_model()
{
    std::string stack;
    model_root->print(stack);
}


// leaf queue API
void PPMR::push_back_leaf_queue(ContextNode* node)
{
    leaf_counter++;
    
    node->leaf = true;
    
    if (back_queue == nullptr)
    {
        front_queue = node;
        back_queue  = node;
        
        node->children = {
            {0, nullptr},
            {1, nullptr}
        };
    }
    else
    {
        node->children = {
            {0, back_queue},
            {1, nullptr}
        };
        
        back_queue->children[1] = node;
        back_queue = node;
    }
}
void PPMR::push_front_leaf_queue(ContextNode* node)
{
    leaf_counter++;
    
    node->leaf = true;
    
    if (front_queue == nullptr)
    {
        front_queue = node;
        back_queue  = node;
        
        node->children = {
            {0, nullptr},
            {1, nullptr}
        };
    }
    else
    {
        node->children = {
            {0, nullptr},
            {1, front_queue}
        };
        
        front_queue->children[0] = node;
        front_queue = node;
    }
}

void PPMR::pop_leaf_queue()
{
    leaf_counter--;
    
    ContextNode* node   = front_queue;
    ContextNode* parent = front_queue->parent;
    
    parent->count_as_context -= node->count;
    
    // new front of the queue
    front_queue = front_queue->children[1];
    if (front_queue) front_queue->children[0] = nullptr;

    // Remove from parent's list
    parent->children.erase(node->ch);
    
    // free memory
    delete node;
    
    // Update former parent's status
    if (parent->children.empty()) push_front_leaf_queue(parent);
}

void PPMR::remove_leaf_queue(ContextNode* node)
{
    leaf_counter--;
    
    if (front_queue == back_queue)
    {
        front_queue = nullptr;
        back_queue  = nullptr;
    }
    else if (node == front_queue)
    {
        front_queue = front_queue->children[1];
        if (front_queue) front_queue->children[0] = nullptr;
    }
    else if (node == back_queue)
    {
        back_queue = back_queue->children[0];
        if (back_queue) back_queue->children[1] = nullptr;
    }
    else
    {
        node->children[0]->children[1] = node->children[1];
        node->children[1]->children[0] = node->children[0];
    }
    
    node->leaf = false;
    node->children.clear();
}
