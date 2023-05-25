#ifndef PPMR_hpp
#define PPMR_hpp

#include "utils.h"

class PPMR
{
public:
    PPMR();
    ~PPMR();
    
    void set_node_limit(long lim);
    void set_order_limit(int lim);
    
    void run(const std::string& path, const std::string& file);
    std::string stats();
    void disp_stats();
    void disp_model();
    
private:
    struct ContextNode
    {
        ContextNode()
        {
            ch                  = '\0';
            count               = 0;
            count_as_context    = 0;
            leaf                = false;
            parent              = nullptr;
            children.clear();
        }
        ContextNode(char _ch)
        {
            ch                  = _ch;
            count               = 0;
            count_as_context    = 0;
            leaf                = false;
            parent              = nullptr;
            children.clear();
        }
        ~ContextNode()
        {
            if (!leaf) for (const auto&[ch, child] : children) delete child;
        }
        
        void print(std::string& stack)
        {
            stack.push_back(ch);
            std::cout << stack << " " << count << std::endl;
            if (!leaf) for (const auto&[ch, child] : children) child->print(stack);
            stack.pop_back();
        }
        
        char ch;
        long count;
        long count_as_context;
        
        bool leaf;
        ContextNode* parent;
        std::unordered_map<char, ContextNode*> children;
    };
    
private:
    ContextNode* model_root;
    ContextNode* front_queue;
    ContextNode* back_queue;
    
    std::vector<ContextNode*> hand;
    
    double original_size;
    double encoded_size;
    
    long node_limit;
    int order_limit;
    
    long node_counter;
    long leaf_counter;
    
    std::string t_file;
    double t_time;
    
private:
    std::string get_context_string(int i, int order, const std::string& buffer, const std::string& prev_buffer);
    bool        find_context(const std::string& context_string, std::vector<ContextNode*>& context_list);
    double      next_encoding_size(int i, const std::string& buffer, const std::string& prev_buffer);
    double      process_context(ContextNode* context, char pred, bool& predicted, std::unordered_set<char>& exclusion);
    
    struct layer
    {
        long count;
        long distinct;
        long leaves;
    };
    void tree_size (ContextNode* node, std::vector<layer>& layers, int depth = 0);
    
private:
    // leaf queue API
    void push_front_leaf_queue(ContextNode* node);
    void push_back_leaf_queue(ContextNode* node);
    void pop_leaf_queue();
    void remove_leaf_queue(ContextNode* node);
    
    void delete_leaf_queue(ContextNode* node);
};

#endif /* PPMR_hpp */
