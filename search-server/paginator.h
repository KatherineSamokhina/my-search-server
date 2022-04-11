#pragma once

#include <fstream>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange() = default;

    IteratorRange(Iterator begin, Iterator end)
        : begin_(begin)
        , end_(end)
    {}

    Iterator begin() const {
        return begin_;
    }
    Iterator end() const {
        return end_;
    }
    size_t size() {
        return distance(begin_, end_);
    }

private:
    Iterator begin_;
    Iterator end_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator() = default;
    Paginator(Iterator begin, Iterator end, size_t size) {
        if (size == 0 || distance(begin, end) == 0)
            throw std::invalid_argument("Can't create object with such parameters");
        while (distance(begin, end) > (int)size) {
            pages_.push_back(IteratorRange(begin, begin + size));
            advance(begin, size);
        }
        if (distance(begin, end) != 0) {
            pages_.push_back(IteratorRange(begin, end));
        }
    }

    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }
    size_t size() {
        return pages_.size();
    }
private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& range) {
    for (const auto& it : range) {
        os << it;
    }
    return os;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}