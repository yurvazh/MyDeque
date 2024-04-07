//Created by Yury Vazhenin
#include <iostream>
#include <type_traits>
#include <unordered_set>
#include <algorithm>
#include <numeric>
#include <random>
#include <cassert>
#include <deque>
#include <iterator>
#include <compare>



template<typename T>
class Deque {
    static const size_t blockSize = 32;
    size_t size_, capacity_;
    T** arr = nullptr;
    size_t first_block, first_elem;
private:
    template<bool IsConst>
    struct base_iterator {
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using reference = std::conditional_t<IsConst, const T&, T&>;
        using value_type = std::remove_reference_t<reference>;
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = ptrdiff_t;

        T** data_pointer;
        int blockNumber, elemNumber;

        base_iterator(const base_iterator<false>& other): data_pointer(other.data_pointer)
            , blockNumber(other.blockNumber), elemNumber(other.elemNumber) {}

        base_iterator& operator=(const base_iterator<false>& other) {
            data_pointer = other.data_pointer;
            blockNumber = other.blockNumber;
            elemNumber = other.elemNumber;
            return *this;
        }

        base_iterator(T** data, int blockNumber, int elemNumber) : data_pointer(data)
            , blockNumber(blockNumber), elemNumber(elemNumber) {}

        base_iterator(T** data, int absolute) : data_pointer(data)
            ,  blockNumber(absolute / blockSize), elemNumber(absolute % blockSize) {}

        reference operator*() const {
            return data_pointer[blockNumber][elemNumber];
        }

        pointer operator->() const {
            return &data_pointer[blockNumber][elemNumber];
        }

        base_iterator& operator++() {
            elemNumber = (elemNumber + 1) % blockSize;
            if (!elemNumber) {
                blockNumber++;
            }
            return *this;
        }

        base_iterator operator++(int) {
            base_iterator copy = *this;
            ++*this;
            return copy;
        }

        base_iterator& operator--() {
            if (elemNumber == 0) {
                blockNumber--;
                elemNumber = blockSize;
            }
            elemNumber--;
            return *this;
        }

        base_iterator operator--(int) {
            base_iterator copy = *this;
            --*this;
            return copy;
        }

        base_iterator& operator+=(int d) {
            int absoluteNumber = blockNumber * blockSize + elemNumber + d;
            blockNumber = absoluteNumber / blockSize;
            elemNumber = absoluteNumber % blockSize;
            return *this;
        }

        base_iterator operator+(int d) const{
            base_iterator copy = *this;
            copy += d;
            return copy;
        }

        base_iterator& operator-=(int d) {
            this->operator+=(-d);
            return *this;
        }

        base_iterator operator-(int d) const {
            base_iterator copy = *this;
            copy -= d;
            return copy;
        }

        base_iterator prev() const {
            base_iterator copy = *this;
            --copy;
            return copy;
        }

        base_iterator next() const {
            base_iterator copy = *this;
            ++copy;
            return copy;
        }

        difference_type operator-(const base_iterator& other) const {
            return -((other.blockNumber - blockNumber) * blockSize  + other.elemNumber - elemNumber);
        }

        std::strong_ordering operator<=>(const base_iterator& other) const {
            int diff = *this - other;
            if (!diff)
                return std::strong_ordering::equal;
            return (diff > 0 ? std::strong_ordering::greater : std::strong_ordering::less);
        }

        bool operator==(const base_iterator&) const = default;

    private:
        void change_data(T** new_data) {
            data_pointer = new_data;
        }

        void change_block(int new_block) {
            blockNumber = new_block;
        }

        friend void Deque<T>::insert(base_iterator<false>, const T&);
    };
public:
    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<iterator>;

    Deque() : size_(0), capacity_(0), arr(nullptr), first_block(0), first_elem(0) {};

    Deque (int n) : size_(n), capacity_(0) , first_block(0), first_elem(0){
        reserve((n + blockSize - 1) / blockSize);
        first_block = 0;
        iterator tmp = begin();
        try {
            for (; tmp != end(); ++tmp) {
                new (&*tmp) T();
            }
        } catch (...) {
            clear_memory(begin(), tmp);
            throw;
        }
    }

    Deque (int n, const T& value) : size_(n), capacity_(0), first_block(0), first_elem(0) {
        reserve((n + blockSize - 1) / blockSize);
        first_block = 0;
        iterator tmp = begin();
        try {
            for (; tmp != end(); ++tmp) {
                new(&*tmp) T(value);
            }
        } catch (...) {
            clear_memory(begin(), tmp);
            throw;
        }
    }

    Deque (const Deque& other) : size_(other.size_), capacity_(0), first_block(other.first_block), first_elem(other.first_elem) {
        reserve(other.capacity_);
        first_block = other.first_block;
        first_elem = other.first_elem;
        iterator tmp = begin();
        const_iterator other_tmp = other.begin();
        try {
            for (; tmp != end(); ++tmp, ++other_tmp) {
                new(&*tmp) T(*other_tmp);
            }
        } catch (...) {
            clear_memory(begin(), tmp);
            throw;
        }
    }

    void swap(Deque& other) {
        T** help_arr = arr;
        arr = other.arr;
        other.arr = help_arr;
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        std::swap(first_block, other.first_block);
        std::swap(first_elem, other.first_elem);
    }

    Deque& operator=(Deque other) {
        swap(other);
        return *this;
    }

    iterator begin() {
        return iterator(arr, first_block, first_elem);
    }

    const_iterator begin() const {
        return cbegin();
    }

    const_iterator cbegin() const {
        return const_iterator(arr, first_block, first_elem);
    }

    iterator end() {
        return iterator(arr, blockSize * first_block + first_elem + size_);
    }

    const_iterator end() const{
        return cend();
    }

    const_iterator cend() const {
        return const_iterator(arr, blockSize * first_block + first_elem + size_);
    }

    reverse_iterator rbegin() {
        return reverse_iterator(iterator(arr, blockSize * first_block + first_elem + size_));
    }

    const_reverse_iterator rbegin() const {
        return crbegin();
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(iterator(arr, blockSize * first_block + first_elem + size_));
    }

    reverse_iterator rend() {
        return reverse_iterator(iterator(arr, blockSize * first_block + first_elem));
    }

    const_reverse_iterator rend() const{
        return crend();
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(iterator(arr, blockSize * first_block + first_elem));
    }

    size_t size() const {
        return size_;
    }

    T& operator[] (size_t index) {
        index += first_block * blockSize + first_elem;
        return arr[index / blockSize][index % blockSize];
    }

    const T& operator[] (size_t index) const {
        index += first_block * blockSize + first_elem;
        return arr[index / blockSize][index % blockSize];
    }

    void push_back (const T& value) {
        size_t old_capacity = capacity_;
        T** old_arr = nullptr;
        size_t old_first_block = first_block;
        if (full())
            old_arr = reserve((capacity_ > 0) ? capacity_ * 3 : 1);
        try {
            new (&*end()) T(value);
        } catch (...) {
            if (old_arr) {
                std::swap(arr, old_arr);
                std::swap(capacity_, old_capacity);
                delete[] old_arr;
            }
            first_block = old_first_block;
            throw;
        }
        if (old_arr)
            delete[] old_arr;
        ++size_;
    }

    void push_front(const T& value) {
        size_t old_capacity = capacity_;
        T** old_arr = nullptr;
        size_t old_first_block = first_block;
        if (first_block == 0 && first_elem == 0) {
            if (capacity_ == 0) {
                old_arr = reserve(2);
                first_block++;
            } else {
                old_arr = reserve(capacity_ * 3);
            }
        }
        iterator it = begin();
        --it;
        try {
            new (&*it) T(value);
        } catch (...) {
            if (old_arr) {
                std::swap(arr, old_arr);
                std::swap(capacity_, old_capacity);
                delete[] old_arr;
            }
            first_block = old_first_block;
            throw;
        }
        if (old_arr)
            delete[] old_arr;
        if (!first_elem) {
            --first_block;
            first_elem = blockSize;
        }
        --first_elem;
        ++size_;
    }

    void pop_front() {
        iterator it = begin();
        it->~T();
        if (++first_elem == blockSize) {
            first_elem = 0;
            ++first_block;
        }
        --size_;
    }

    void pop_back() {
        iterator it = end();
        (--it)->~T();
        --size_;
    }

    T& at(size_t index) {
        if (index < 0 || index >= size())
            throw std::out_of_range("Wrong index");
        return *(begin() + index);
    }

    const T& at(size_t index) const{
        if (index < 0 || index >= size())
            throw std::out_of_range("Wrong index");
        return *(begin() + index);
    }

    void insert(iterator place, const T& value) {
        if (full()) {
            int old_capacity = capacity_;
            delete[] reserve(capacity_ ? 3 * capacity_ : 1);
            place.change_data(arr);
            place.change_block(place.blockNumber + old_capacity);
        }
        iterator help = end();
        for (; help != place; --help) {
            *help = *(help.prev());
        }
        *place = value;
        ++size_;
    }

    void erase (iterator place) {
        --size_;
        iterator help = place;
        for (; help != end(); ++help) {
            *help = *(help.next());
        }
        end()->~T();
    }

    ~Deque() {
        clear_memory(begin(), end());
    }
private:
    bool full() const {
        return (capacity_ * blockSize == size_ + first_block * blockSize + first_elem);
    }

    T** reserve (size_t n) {
        if ((capacity_ == 0) && (n > 1)) {
            reserve(1);
        }
        if (capacity_ >= n)
            return nullptr;
        size_t new_capacity = std::max(3 * capacity_, n);
        T** new_arr = new T*[new_capacity];
        size_t i = 0;
        try {
            for (; i < new_capacity; i++) {
                if (i < capacity_ || i >= 2 * capacity_) {
                    new_arr[i] = reinterpret_cast<T*>(new char[blockSize * sizeof(T)]{});
                } else {
                    new_arr[i] = arr[i - capacity_];
                }
            }
        } catch (...) {
            for (size_t index = 0; index < i; index++) {
                if (index < capacity_ || index >= 2 * capacity_) {
                    delete[] reinterpret_cast<char*>(new_arr[index]);
                }
            }
            delete[] new_arr;
            throw;
        }
        first_block += capacity_;
        capacity_ = new_capacity;
        std::swap(arr, new_arr);
        return new_arr;
    }

    void clear_memory (iterator left, iterator right) {
        for (; left != right; ++left) {
            left->~T();
        }
        for (size_t i = 0; i < capacity_; ++i) {
            delete[] reinterpret_cast<char*> (arr[i]);
        }
        delete[] arr;
    }
};

