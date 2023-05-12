#include "LRU.hpp"

template<typename T>
LRU<T>::LRU()
: LRU<T>::LRU(1)
{
    
}
template<typename T>
LRU<T>::LRU(const LRU<T>& lru)
: m_capacity(lru.m_capacity)
{
    m_size = lru.m_size;
    
    m_block = new ListNode<T>[m_capacity];
    m_map.clear();
    
    for (int i = 0; i < m_capacity; i++)
    {
        m_block[i].value        = lru.m_block[i].value;
        m_block[i].prev         = nullptr;
        m_block[i].next         = nullptr;
        m_map[m_block[i].value] = &m_block[i];
        
        if (lru.m_front == &lru.m_block[i]) m_front = &m_block[i];
        if (lru.m_back  == &lru.m_block[i]) m_back  = &m_block[i];
    }
    
    for (int i = 0; i < m_capacity; i++)
    {
        if (lru.m_block[i].prev) m_block[i].prev = m_map[lru.m_block[i].prev->key];
        if (lru.m_block[i].next) m_block[i].next = m_map[lru.m_block[i].next->key];
    }
}

template<typename T>
LRU<T>::LRU(int capacity)
: m_capacity(capacity)
{
    m_size = 0;
    m_map.clear();
    
    m_block = new ListNode<T>[m_capacity];
    m_front = nullptr;
    m_back  = nullptr;
    
    for (int i = 0; i < m_capacity; i++)
    {
        m_block[i].prev = nullptr;
        m_block[i].next = nullptr;
    }
}

template<typename T>
LRU<T>::~LRU()
{
    delete [] m_block;
}

template<typename T>
void LRU<T>::set_capacity(int capacity)
{
    delete [] m_block;
    
    m_capacity = capacity;
    m_size = 0;
    m_map.clear();
    
    m_block = new ListNode<T>[m_capacity];
    m_front = nullptr;
    m_back  = nullptr;
    
    for (int i = 0; i < m_capacity; i++)
    {
        m_block[i].prev = nullptr;
        m_block[i].next = nullptr;
    }
}

template<typename T>
int LRU<T>::get_size()
{
    return m_size;
}

template<typename T>
std::vector<T> LRU<T>::get_queue()
{
    std::vector<T> queue;
    ListNode<T>* node = m_front;
    
    while (node != nullptr)
    {
        queue.push_back(node->value);
        node = node->next;
    }
    
    return queue;
}


template<typename Key, typename T>
void swap(LRU<T>& first, LRU<T>& second)
{
    std::swap(first.m_capacity, second.m_capacity);
    std::swap(first.m_size,     second.m_size);
    std::swap(first.m_block,    second.m_block);
    std::swap(first.m_front,    second.m_front);
    std::swap(first.m_back,     second.m_back);
}

template<typename T>
LRU<T>& LRU<T>::operator=(LRU<T> lru)
{
    swap(*this, lru);
    return *this;
}

template<typename T>
bool LRU<T>::request(const T& value, T& removed)
{
    typedef typename std::unordered_map<T, ListNode<T>*>::iterator map_iterator;
    map_iterator hint = m_map.find(value);
    
    bool hit = (hint != m_map.end());
    bool deletion = false;
        
    if (hit)
    {
        ListNode<T>* node = hint->second;
        
        detach(node);
        attach(node);
    }
    else
    {
        ListNode<T>* node;
        
        if (m_size < m_capacity)
        {
            node = &m_block[m_size];

            m_size++;
        }
        else
        {
            node    = m_back;
            removed = node->value;
            
            detach(node);
            m_map.erase(node->value);
            deletion = true;
        }
        
        node->value     = value;
        m_map[value]    = node;
        
        attach(node);
    }
    
    return deletion;
}

template<typename T>
void LRU<T>::erase(const T& value)
{
    typedef typename std::unordered_map<T, ListNode<T>*>::iterator map_iterator;
    map_iterator hint = m_map.find(value);
    
    if (hint != m_map.end())
    {
        ListNode<T>* node       = hint->second;
        ListNode<T>* back_node  = &m_block[m_size-1];
        
        detach(node);
        m_map.erase(node->value);
        
        if (node != back_node)
        {
            node->value = back_node->value;
            node->prev  = back_node->prev;
            node->next  = back_node->next;
            if (back_node->prev) back_node->prev->next = node;
            if (back_node->next) back_node->next->prev = node;
            
            if (back_node == m_front)   m_front = node;
            if (back_node == m_back)    m_back  = node;
            
            m_map[node->value] = node;
        }
        
        m_size--;
    }
}

template<typename T>
void LRU<T>::detach(ListNode<T> *node)
{
    if (node == m_back)  m_back  = node->prev;
    if (node == m_front) m_front = node->next;
    
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
}

template<typename T>
void LRU<T>::attach(ListNode<T> *node)
{
    node->prev = nullptr;
    node->next = m_front;
    
    if (m_front) m_front->prev = node;
    
    if (m_size == 1) m_back = node;
    
    m_front = node;
}
