
#pragma once

#include "anode.h"

#include <stack>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>



namespace anode {

template<typename T>
class gc_ref_deque : public std::deque<std::reference_wrapper<T>, gc_allocator<std::reference_wrapper<T>>> { };

template<typename T>
class gc_vector : public std::vector<T, gc_allocator<T>> { };

template<typename T>
class gc_stack : public std::stack<T, gc_allocator<T>> { };

template<typename T>
class gc_ref_vector : public std::vector<std::reference_wrapper<T>, gc_allocator<std::reference_wrapper<T>>> { };

template<typename TKey, typename TValue>
class gc_unordered_map : public std::unordered_map<TKey, TValue, std::hash<TKey>, std::equal_to<TKey>, gc_allocator<std::pair<TKey, TValue>>> { };

template<typename TKey, typename TValue>
class gc_ref_unordered_map : public std::unordered_map<TKey, std::reference_wrapper<TValue>, std::hash<TKey>, std::equal_to<TKey>, gc_allocator<std::pair<TKey, std::reference_wrapper<TValue>>>> { };

template<typename TValue>
class gc_unordered_set : public std::unordered_set<TValue, std::hash<TValue>, std::equal_to<TValue>, gc_allocator<std::pair<TValue, TValue>>> { };

}