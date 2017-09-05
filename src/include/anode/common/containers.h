
#pragma once

#include "anode.h"

#include <stack>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>



namespace anode {

template<typename T>
class gc_deque : public std::deque<T, gc_allocator<T>> { };

template<typename T>
class gc_stack : public std::stack<T, gc_deque<T>> {
public:
};

template<typename T>
class gc_vector : public std::vector<T, gc_allocator<T>> { };

template<typename TKey, typename TValue>
class gc_unordered_map : public std::unordered_map<TKey, TValue, std::hash<TKey>, std::equal_to<TKey>, gc_allocator<std::pair<TKey, TValue>>> { };

template<typename TValue>
class gc_unordered_set : public std::unordered_set<TValue, std::hash<TValue>, std::equal_to<TValue>, gc_allocator<std::pair<TValue, TValue>>> { };

}