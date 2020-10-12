/* 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  Copyright Morten K. Schou
 */

/* 
 * File:   ptr_vector.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 12-10-2020.
 */

#ifndef AALWINES_PTR_VECTOR_H
#define AALWINES_PTR_VECTOR_H

#include <memory>
#include <vector>

template <typename Iterator, typename ValueType>
class ptr_iterator {
protected:
    static_assert(std::is_same_v<typename Iterator::value_type, std::unique_ptr<ValueType>>, "Iterator and ValueType not matching");
    using difference_type = typename Iterator::difference_type;

    Iterator inner;

public:
    constexpr ptr_iterator() noexcept : inner(Iterator()) { }
//    explicit ptr_iterator(Iterator&& i) noexcept : inner(std::move(i)) { }
    explicit ptr_iterator(const Iterator& i) noexcept : inner(i) { }

    // Forward iterator requirements
    ValueType& operator*() const noexcept { return *(inner->get()); }

    ValueType* operator->() const noexcept { return inner->get(); }

    ValueType* get() const noexcept { return inner->get(); }

    ptr_iterator& operator++() noexcept {
        ++inner;
        return *this;
    }

    ptr_iterator operator++(int) noexcept { return ptr_iterator(inner++); }

    // Bidirectional iterator requirements
    ptr_iterator& operator--() noexcept {
        --inner;
        return *this;
    }

    ptr_iterator operator--(int) noexcept { return ptr_iterator(inner--); }

    // Random access iterator requirements
    ValueType& operator[](difference_type n) const noexcept { return *(inner[n]); }

    ptr_iterator& operator+=(difference_type n) noexcept { inner += n; return *this; }

    ptr_iterator operator+(difference_type n) const noexcept { return ptr_iterator(inner + n); }

    ptr_iterator& operator-=(difference_type n) noexcept { inner -= n; return *this; }

    ptr_iterator operator-(difference_type n) const noexcept { return ptr_iterator(inner - n); }

    const Iterator& base() const noexcept { return inner; }
};

// Forward iterator requirements
template<typename IteratorL, typename IteratorR, typename ValueType>
inline bool
operator==(const ptr_iterator<IteratorL, ValueType>& lhs,
           const ptr_iterator<IteratorR, ValueType>& rhs)
noexcept { return lhs.base() == rhs.base(); }

template<typename Iterator, typename ValueType>
inline bool
operator==(const ptr_iterator<Iterator, ValueType>& lhs,
           const ptr_iterator<Iterator, ValueType>& rhs)
noexcept { return lhs.base() == rhs.base(); }

template<typename IteratorL, typename IteratorR, typename ValueType>
inline bool
operator!=(const ptr_iterator<IteratorL, ValueType>& lhs,
           const ptr_iterator<IteratorR, ValueType>& rhs)
noexcept { return lhs.base() != rhs.base(); }

template<typename Iterator, typename ValueType>
inline bool
operator!=(const ptr_iterator<Iterator, ValueType>& lhs,
           const ptr_iterator<Iterator, ValueType>& rhs)
noexcept { return lhs.base() != rhs.base(); }

template<typename IteratorL, typename IteratorR, typename ValueType>
inline bool
operator<(const ptr_iterator<IteratorL, ValueType>& lhs,
           const ptr_iterator<IteratorR, ValueType>& rhs)
noexcept { return lhs.base() < rhs.base(); }

template<typename Iterator, typename ValueType>
inline bool
operator<(const ptr_iterator<Iterator, ValueType>& lhs,
           const ptr_iterator<Iterator, ValueType>& rhs)
noexcept { return lhs.base() < rhs.base(); }

template<typename IteratorL, typename IteratorR, typename ValueType>
inline bool
operator>(const ptr_iterator<IteratorL, ValueType>& lhs,
          const ptr_iterator<IteratorR, ValueType>& rhs)
noexcept { return lhs.base() > rhs.base(); }

template<typename Iterator, typename ValueType>
inline bool
operator>(const ptr_iterator<Iterator, ValueType>& lhs,
          const ptr_iterator<Iterator, ValueType>& rhs)
noexcept { return lhs.base() > rhs.base(); }

template<typename IteratorL, typename IteratorR, typename ValueType>
inline bool
operator<=(const ptr_iterator<IteratorL, ValueType>& lhs,
          const ptr_iterator<IteratorR, ValueType>& rhs)
noexcept { return lhs.base() <= rhs.base(); }

template<typename Iterator, typename ValueType>
inline bool
operator<=(const ptr_iterator<Iterator, ValueType>& lhs,
          const ptr_iterator<Iterator, ValueType>& rhs)
noexcept { return lhs.base() <= rhs.base(); }

template<typename IteratorL, typename IteratorR, typename ValueType>
inline bool
operator>=(const ptr_iterator<IteratorL, ValueType>& lhs,
          const ptr_iterator<IteratorR, ValueType>& rhs)
noexcept { return lhs.base() >= rhs.base(); }

template<typename Iterator, typename ValueType>
inline bool
operator>=(const ptr_iterator<Iterator, ValueType>& lhs,
          const ptr_iterator<Iterator, ValueType>& rhs)
noexcept { return lhs.base() >= rhs.base(); }


template <typename T>
class ptr_vector {
private:
    using inner_t = typename std::vector<std::unique_ptr<T>>;
    inner_t inner;

    using iterator = ptr_iterator<typename inner_t::iterator, T>;
    using const_iterator = ptr_iterator<typename inner_t::const_iterator, T>;
    using size_type = typename inner_t::size_type;

public:
    ptr_vector() : inner() {}
    explicit ptr_vector(inner_t&& other) : inner(std::move(other)) {}

    iterator begin() noexcept { return iterator(inner.begin()); }
    iterator end() noexcept { return iterator(inner.end()); }
    const_iterator begin() const noexcept { return const_iterator(inner.begin()); }
    const_iterator end() const noexcept { return const_iterator(inner.end()); }
    const_iterator cbegin() const noexcept { return const_iterator(inner.cbegin()); }
    const_iterator cend() const noexcept { return const_iterator(inner.cend()); }

    template<typename... Args>
    T* emplace_back(Args&&... args) {
        return inner.emplace_back(std::make_unique<T>(std::forward<Args>(args)...)).get();
    }

    size_type size() const noexcept {
        return inner.size();
    }

    T* back() noexcept {
        return inner.back().get();
    }
    T* back() const noexcept {
        return inner.back().get();
    }
    void clear() noexcept {
        inner.clear();
    }

    void reserve(size_type n) {
        inner.reserve(n);
    }

    T* operator[](size_type n) noexcept {
        return inner[n].get();
    }

};


#endif //AALWINES_PTR_VECTOR_H
