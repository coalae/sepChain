#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>

template <typename Key, size_t N = 7>
class ADS_set
{
public:
    class Iterator;
    using value_type = Key;
    using key_type = Key;
    using reference = key_type&;
    using const_reference = const key_type&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = Iterator;
    using const_iterator = Iterator;
    //using key_compare = std::less<key_type>;   // B+-Tree
    using key_equal = std::equal_to<key_type>; // Hashing
    using hasher = std::hash<key_type>;        // Hashing

private:
    float max_lf {0.7};
    enum class Mode
    {
        free, inuse, end
    };
    struct Element
    {
        value_type key;
        Element * nextElement = nullptr;
        Mode mode = Mode::free;
        Element *nextElementOfChain = nullptr;
    };
    size_t tableSize {0};
    size_t elementSize {0};
    Element *table {nullptr};

    size_type hash_idx(const key_type& k) const
    {
        return hasher {}(k) % tableSize;
    }

    Element* insertReal(const key_type& key)
    {
        if (count(key) == 0)
        {
            int position = hash_idx(key);
            if (table[position].mode != Mode::inuse)
            {
                table[position].key = key;
                table[position].mode = Mode::inuse;
            }
            else
            {
                Element * temp = new Element();
                temp->key = table[position].key;
                temp->nextElement = table[position].nextElement;
                temp->mode = table[position].mode;
                temp->nextElementOfChain = table[position].nextElementOfChain;
                table[position].key = key;
                table[position].nextElement = temp;
                table[position].mode = Mode::inuse;
            }
            elementSize++;
            return table+position;
        }
        return nullptr;
    }

    void rehash(size_type n)
    {
        Element *oldTable {table};

        size_t oldTableSize {tableSize};
        size_t newTableSize = std::max(std::max(n, size_type(elementSize / max_lf)),N);
        table = new Element[newTableSize+1];
        table[newTableSize].mode = Mode::end;
        tableSize = newTableSize;
        elementSize = 0;
        for (int i = 0 ; i < newTableSize; i++)
        {
            if (i < newTableSize)
                table[i].nextElementOfChain = &table[i+1];
        }
        for (int i = 0 ; i < oldTableSize; i++)
        {
            if (oldTable[i].mode == Mode::inuse)
            {

                Element * temp = &oldTable[i];
                Element * prev = nullptr;
                bool firstElement = true;
                while(temp != nullptr)
                {
                    prev=temp;
                    if(prev->mode == Mode::inuse)
                        insertReal(prev->key);
                    temp = temp->nextElement;
                    if (firstElement)
                        firstElement = false;
                    else
                        delete prev;
                }
            }
        }
        delete[] oldTable;
    }

    void reserve(size_type n)
    {
        if (n > tableSize * max_lf)
        {
            size_type new_max_sz {tableSize};
            while (n > new_max_sz * max_lf) new_max_sz = new_max_sz * 2 + 1;
            rehash(new_max_sz);
        }
    }

    Element *find_pos(const key_type& key) const
    {
        int position = hash_idx(key);
        if (table[position].mode == Mode::inuse)
        {
            if (key_equal {}(key, table[position].key))
                return table+position;
            Element * temp = table[position].nextElement;
            while(temp != nullptr && temp->mode == Mode::inuse)
            {
                if (key_equal {}(key, temp->key))
                    return temp;
                temp = temp->nextElement;
            }

        }
        return nullptr;
    }


public:
    ADS_set()
    {
        tableSize = 0;
        rehash(N);
    }

    ADS_set(std::initializer_list<key_type> ilist) : ADS_set {}
    {
        insert(ilist);

    }
    template<typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set {}
    {
        insert(first, last);
    }
    ADS_set(const ADS_set& other)
    {
        rehash(other.tableSize);
        for (const auto& k: other)
            insertReal(k);
    }

    ~ADS_set()
    {
        for(int i = 0; i < tableSize; i++)
        {
            Element * currentElement {table[i].nextElement};
            Element * prevElement {nullptr};
            while(currentElement != nullptr)
            {
                prevElement=currentElement;
                currentElement=currentElement->nextElement;
                delete prevElement;
                prevElement = nullptr;
            }

        }
        delete[] table;
    }

    ADS_set& operator=(const ADS_set& other)
    {
        if (this == &other) return *this;
        clear();
        reserve(other.elementSize);
        for (const auto& k: other)
            insertReal(k);
        return *this;
    }
    ADS_set& operator=(std::initializer_list<key_type> ilist)
    {
        clear();
        insert(ilist);
        return *this;
    }

    size_type size() const
    {
        return elementSize;
    }
    bool empty() const
    {
        if (elementSize == 0)
            return true;
        return false;
    }

    size_type count(const key_type& key) const
    {
        int position = hash_idx(key);
        if (table[position].mode == Mode::inuse && key_equal {}(key, table[position].key))
            return 1;
        Element * temp = table[position].nextElement;
        while(temp != nullptr)
        {
            if (temp->mode == Mode::inuse && key_equal {}(temp->key, key))
                return 1;
            temp = temp->nextElement;
        }
        return 0;
    }

    iterator find(const key_type& key) const
    {
        if (Element *pos {find_pos(key)})
            return const_iterator {pos};
        return end();
    }
    void clear()
    {

        for (int i = 0 ; i < tableSize; i++)
        {
            if (table[i].mode != Mode::free){
                table[i].mode = Mode::free;
                elementSize--;
            }
            Element * currentElement {table[i].nextElement};
            Element * prevElement {nullptr};

            while(currentElement != nullptr)
            {
                prevElement = currentElement;
                currentElement = currentElement->nextElement;
                prevElement->mode = Mode::free;
                delete prevElement;
                elementSize--;
            }
            table[i].nextElement = nullptr;

        }
    }
    void swap(ADS_set& other)
    {
        std::swap(elementSize, other.elementSize);
        std::swap(tableSize, other.tableSize);
        std::swap(max_lf, other.max_lf);
        std::swap(table, other.table);
    }

    void insert(std::initializer_list<key_type> ilist)
    {
        for (const key_type key : ilist)
            insertReal(key);
    }
    std::pair<iterator,bool> insert(const key_type& key)
    {
        if (Element *pos {find_pos(key)})
            return std::make_pair(const_iterator {pos}, false);
        reserve(elementSize+1);
        return std::make_pair(const_iterator {insertReal(key)}, true);
    }


    template<typename InputIt> void insert(InputIt first, InputIt last)
    {

        for (auto it = first; it != last; ++it)
        {
            if (!find_pos(*it))
            {
                reserve(elementSize+1);
                insertReal(*it);
            }
        }
    }

    size_type erase(const key_type& key)
    {
        int position = hash_idx(key);
        if (table[position].mode == Mode::inuse)
        {
            Element *currentElement = &table[position];
            Element *nextElement = currentElement->nextElement;
            if (key_equal {}(key, currentElement->key) && currentElement->mode != Mode::free){
                if (nextElement != nullptr){
                    currentElement->key = nextElement->key;
                    currentElement->mode = nextElement->mode;
                    currentElement->nextElementOfChain = nextElement->nextElementOfChain;
                    currentElement->nextElement = nextElement->nextElement;
                    delete nextElement;
                }else{
                    currentElement->mode = Mode::free;
                }
                elementSize--;
                return 1;
            }


            while(nextElement != nullptr)
            {
                 if (key_equal {}(key, nextElement->key) && nextElement->mode != Mode::free){
                        if (nextElement->nextElement != nullptr){
                            currentElement->nextElement = nextElement->nextElement;
                        }else{
                            currentElement->nextElementOfChain = nextElement->nextElementOfChain;
                            currentElement->nextElement = nullptr;
                        }
                        delete nextElement;
                        elementSize--;
                        return 1;
                }
                currentElement = nextElement;
                nextElement = nextElement->nextElement;
            }

        }
        return 0;
    }
    const_iterator begin() const
    {
        return const_iterator {table};
    }
    const_iterator end() const
    {
        return const_iterator {table+tableSize};
    }

    void dump(std::ostream& o = std::cerr) const
    {
        for (int i = 0 ; i < tableSize; i++)
        {
            o << i << ") ";
            if (table[i].mode == Mode::inuse)
            {
                o << table[i].key;
                Element * temp = table[i].nextElement;
                while(temp != nullptr && temp->mode == Mode::inuse)
                {
                    o << "-->";
                    o << temp->key;
                    temp = temp->nextElement;
                }

            }
            o << "\n";
        }
    }

    friend bool operator==(const ADS_set& lhs, const ADS_set& rhs)
    {
        if (lhs.elementSize != rhs.elementSize)
            return false;
        for (const auto& k: lhs)
            if (!rhs.find_pos(k))
                return false;
        return true;

    }
    friend bool operator!=(const ADS_set& lhs, const ADS_set& rhs)
    {
        return !(lhs==rhs);
    }

};

template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator
{
private:
    Element* pos;

    void skip_free()
    {

        while(pos->mode != Mode::inuse && pos->mode != Mode::end)
        {
            if (pos->nextElement == nullptr)
                pos = pos->nextElementOfChain;
        }

    }

public:
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type&;
    using pointer = const value_type*;
    using iterator_category = std::forward_iterator_tag;

    explicit Iterator(Element * pos = nullptr) : pos {pos} { if (pos) skip_free(); }
    reference operator*() const
    {
        return pos->key;
    }
    pointer operator->() const
    {
        return &pos->key;
    }
    Iterator& operator++()
    {

        if (pos->mode != Mode::end)
        {
            if (pos->nextElement != nullptr)
            {
                pos = pos->nextElement;
            }
            else
            {
                pos = pos->nextElementOfChain;

            }
            skip_free();
        }
        return *this;
    }
    Iterator operator++(int)
    {
        auto rc = *this;
        ++*this;
        return rc;
    }
    friend bool operator==(const Iterator& lhs, const Iterator& rhs)
    {
        return lhs.pos == rhs.pos;
    }
    friend bool operator!=(const Iterator& lhs, const Iterator& rhs)
    {
        return lhs.pos != rhs.pos;
    }
};

template <typename Key, size_t N> void swap(ADS_set<Key,N>& lhs, ADS_set<Key,N>& rhs)
{
    lhs.swap(rhs);
}

#endif // ADS_SET_H
