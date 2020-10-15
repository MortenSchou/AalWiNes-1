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

#include <iterator>
#include <memory>
#include <vector>

/**
 * The ptr_vector is a convinience abstraction over std::vector<std::unique_ptr<T>>.
 * The ptr_iterator follows the pointer, so ordinary references to T are used in range-for loop etc.
 * Element access on the ptr_vector returns a raw pointer to T.
 */

namespace utils::details {
    template <typename _inner_iterator, typename T>
    class ptr_iterator {
    private:
        static_assert(std::is_same_v<std::remove_const_t<typename _inner_iterator::value_type>, std::unique_ptr<std::remove_const_t<T>>>, "_inner_iterator and T not matching");
        using _iterator_type = std::iterator<std::random_access_iterator_tag, T>;
        _inner_iterator _inner;
    public:
        using iterator_category = typename _iterator_type::iterator_category;
        using value_type        = typename _iterator_type::value_type;
        static_assert(std::is_same_v<value_type, T>, "value_type and T not matching");
        using difference_type   = typename _iterator_type::difference_type;
        using pointer           = typename _iterator_type::pointer;
        using reference         = typename _iterator_type::reference;

        constexpr ptr_iterator() noexcept : _inner(_inner_iterator()) { }
        explicit ptr_iterator(_inner_iterator&& i) noexcept : _inner(std::move(i)) { }
        explicit ptr_iterator(const _inner_iterator& i) noexcept : _inner(i) { }

        // Allow iterator to const_iterator conversion
        ptr_iterator(const ptr_iterator<typename std::vector<std::unique_ptr<std::remove_const_t<T>>>::iterator, std::remove_const_t<T>>& i) noexcept : _inner(i.base()) { }

        //pointer get() const noexcept { return _inner->get(); }
        reference operator*() const noexcept { return *(_inner->get()); }
        pointer operator->() const noexcept { return _inner->get(); }

        // Forward iterator requirements
        ptr_iterator& operator++() noexcept { ++_inner; return *this; }
        ptr_iterator operator++(int) noexcept { return ptr_iterator(_inner++); }
        template <typename _iteratorR, typename U>
        bool operator==(const ptr_iterator<_iteratorR, U>& rhs) const noexcept { return base() == rhs.base(); }
        template <typename _iteratorR, typename U>
        bool operator!=(const ptr_iterator<_iteratorR, U>& rhs) const noexcept { return base() != rhs.base(); }

        // Bidirectional iterator requirements
        ptr_iterator& operator--() noexcept { --_inner; return *this; }
        ptr_iterator operator--(int) noexcept { return ptr_iterator(_inner--); }

        // Random access iterator requirements
        reference operator[](difference_type n) const noexcept { return *_inner[n]; }
        ptr_iterator& operator+=(difference_type n) noexcept { _inner += n; return *this; }
        ptr_iterator operator+(difference_type n) const noexcept { return ptr_iterator(_inner + n); }
        ptr_iterator& operator-=(difference_type n) noexcept { _inner -= n; return *this; }
        ptr_iterator operator-(difference_type n) const noexcept { return ptr_iterator(_inner - n); }
        template <typename _iteratorR, typename U>
        difference_type operator-(const ptr_iterator<_iteratorR, U>& rhs) const noexcept { return base() - rhs.base(); }
        template <typename _iteratorR, typename U>
        bool operator<(const ptr_iterator<_iteratorR, U>& rhs) const noexcept { return base() < rhs.base(); }
        template <typename _iteratorR, typename U>
        bool operator>(const ptr_iterator<_iteratorR, U>& rhs) const noexcept { return base() > rhs.base(); }
        template <typename _iteratorR, typename U>
        bool operator<=(const ptr_iterator<_iteratorR, U>& rhs) const noexcept { return base() <= rhs.base(); }
        template <typename _iteratorR, typename U>
        bool operator>=(const ptr_iterator<_iteratorR, U>& rhs) const noexcept { return base() >= rhs.base(); }

        const _inner_iterator& base() const noexcept { return _inner; }
    };
    template <typename _iterator, typename T>
    inline ptr_iterator<_iterator, T> operator+(typename ptr_iterator<_iterator, T>::difference_type n, const ptr_iterator<_iterator, T>& i) noexcept {
        return ptr_iterator<_iterator, T>(i.base() + n);
    }
}

namespace utils {
    template <typename T>
    class ptr_vector {
    private:
        using inner_t = typename std::vector<std::unique_ptr<T>>;
        inner_t _inner;

    public:
        using value_type = T;
        using size_type = typename inner_t::size_type;
        using difference_type = typename inner_t::difference_type;
        using iterator = details::ptr_iterator<typename inner_t::iterator, T>;
        using const_iterator = details::ptr_iterator<typename inner_t::const_iterator, const T>;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        ptr_vector() : _inner() {}
        explicit ptr_vector(inner_t&& other) : _inner(std::move(other)) {
            // The semantics of ptr_vector does not permit nullptrs. Iteration will not be safe.
            _inner.erase(std::remove_if(_inner.begin(), _inner.end(), [](const std::unique_ptr<T>& p){ return p == nullptr; }), _inner.end());
        }

        T* at(size_type i) { return _inner.at(i).get(); }
        const T* at(size_type i) const {return _inner.at(i).get() ;}
        T* operator[](size_type i) noexcept { return _inner[i].get(); }
        const T* operator[](size_type i) const noexcept {return _inner[i].get() ;}
        T* front() noexcept { return _inner.front().get(); }
        const T* front() const noexcept { return _inner.front().get(); }
        T* back() noexcept { return _inner.back().get(); }
        const T* back() const noexcept { return _inner.back().get(); }

        iterator begin() noexcept { return iterator(_inner.begin()); }
        iterator end() noexcept { return iterator(_inner.end()); }
        const_iterator begin() const noexcept { return cbegin(); }
        const_iterator end() const noexcept { return cend(); }
        const_iterator cbegin() const noexcept { return const_iterator(_inner.cbegin()); }
        const_iterator cend() const noexcept { return const_iterator(_inner.cend()); }
        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rbegin() const noexcept { return crbegin(); }
        const_reverse_iterator rend() const noexcept { return crend(); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        size_type size() const noexcept { return _inner.size(); }
        [[nodiscard]] bool empty() const noexcept { return _inner.empty(); }
        size_type max_size() const noexcept { return _inner.max_size(); }
        void reserve(size_type n) { _inner.reserve(n); }
        size_type capacity() const noexcept { return _inner.capacity(); }
        void shrink_to_fit() { _inner.shrink_to_fit(); }

        void clear() noexcept { _inner.clear(); }
        // insert
        template <typename... Args>
        iterator emplace(const_iterator pos, Args&&... args) {
            return iterator(_inner.emplace(pos.base(), std::make_unique<T>(std::forward<Args>(args)...)));
        }
        // iterator erase(const_iterator pos) { return iterator(_inner.erase(pos.base())); };
        // iterator erase(const_iterator first, const_iterator last) { return iterator(_inner.erase(first.base(), last.base())); }
        // pushback
        T* move_back(std::unique_ptr<T>&& p) {
            return _inner.emplace_back(std::move(p)).get();
        }
        template <typename... Args>
        T* emplace_back(Args&&... args) {
            return _inner.emplace_back(std::make_unique<T>(std::forward<Args>(args)...)).get();
        }
        // void pop_back() noexcept { _inner.pop_back(); }
        // resize
        void swap(ptr_vector& other) noexcept { _inner.swap(other); }

        inner_t& inner() { return _inner; }
        const inner_t& inner() const { return _inner; }
    };

}



#endif //AALWINES_PTR_VECTOR_H
