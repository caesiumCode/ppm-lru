#ifndef LRU_hpp
#define LRU_hpp

#include <string>
#include <unordered_map>
#include <iostream>
#include <functional>
#include <vector>

template<typename T>
struct ListNode
{
    T value;
    
    ListNode<T>* prev;
    ListNode<T>* next;
};

template<typename T>
class LRU
{
public:
    LRU();
    LRU(const LRU<T>& lru);
    LRU(int capacity);
    ~LRU();
    
    void set_capacity(int capacity);
    
    int             get_size();
    std::vector<T>  get_queue();
        
    friend void swap(LRU<T>& first, LRU<T>& second);
    LRU<T>& operator=(LRU<T> lru);
    
    bool request(const T& value, T& removed);
    void erase(const T& value);
    
private:
    int m_capacity;
    int m_size;
    
    ListNode<T>* m_block;
    ListNode<T>* m_front;
    ListNode<T>* m_back;
    
    std::unordered_map<T, ListNode<T>*> m_map;
    
private:
    void detach(ListNode<T>* node);
    void attach(ListNode<T>* node);
};

#endif /* LRU_hpp */
