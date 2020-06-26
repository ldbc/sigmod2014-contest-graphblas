#pragma once

#include <vector>

/// The collection holds the k smallest elements from the elements inserted.
/// \tparam ElementT type of elements to hold
/// \tparam Comparator type of comparator
template<typename ElementT, typename Comparator>
class SmallestElementsContainer {
public:
    std::vector<ElementT> elements;
    int sizeLimit;
    Comparator comparator;

    /// You can use makeSmallestElementsContainer&lt;ElementT&gt;(size_limit, comparator) instead.
    explicit SmallestElementsContainer(int size_limit, Comparator comparator)
            : sizeLimit(size_limit), comparator(std::move(comparator)) {
        // reserve space for all elements and a newly inserted
        elements.reserve(sizeLimit + 1);
    }

    /// Add new element if it is smaller than the biggest one (or the container holds less elements than the size limit).
    /// \param newElement the new candidate to be inserted
    void add(ElementT newElement) {
        // if we haven't reached the size limit or the new element is less than the greatest in the container
        if (elements.size() < sizeLimit || comparator(newElement, elements.front())) {
            elements.push_back(std::move(newElement));
            std::push_heap(elements.begin(), elements.end(), comparator);

            if (elements.size() > sizeLimit) {
                std::pop_heap(elements.begin(), elements.end(), comparator);
                elements.pop_back();
            }
        }
    }

    size_t size() const{
        return elements.size();
    }

    ElementT const &max() const{
        return elements.front();
    }

    /// Return all elements of the container in ascending order. Afterwards the container is empty.
    /// \return a sorted std::vector containing the smallest elements from the elements inserted
    std::vector<ElementT> removeElements() {
        std::vector<ElementT> sortedElements;
        std::swap(sortedElements, elements);

        std::sort_heap(sortedElements.begin(), sortedElements.end(), comparator);

        return sortedElements;
    }
};

/// Returns a new SmallestElementsContainer.
/// \tparam ElementT type of elements in the container
/// \param size_limit the size of the container, above which only the smallest elements are kept
/// \param comparator the comparator to define order among the elements (default: std::less&lt;&gt;).
///        Use std::greater&lt;&gt; to reverse the order and keep the largest elements.
/// \return a new empty SmallestElementsContainer
///
/// \see transformComparator
template<typename ElementT, typename Comparator = std::less<ElementT>>
auto makeSmallestElementsContainer(int size_limit, Comparator comparator = Comparator{}) {
    return SmallestElementsContainer<ElementT, Comparator>{size_limit, std::move(comparator)};
}
