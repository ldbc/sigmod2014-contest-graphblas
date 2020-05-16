#pragma once

#include <vector>

template<typename ElementT, typename Comparator>
class SmallestElementsContainer {
    std::vector<ElementT> elements;
    int sizeLimit;
    Comparator comparator;

public:
    explicit SmallestElementsContainer(int size_limit, Comparator comparator)
            : sizeLimit(size_limit), comparator(std::move(comparator)) {
        // reserve space for all elements and a newly inserted
        elements.reserve(sizeLimit + 1);
    }

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

    std::vector<ElementT> removeElements() {
        std::vector<ElementT> sortedElements;
        std::swap(sortedElements, elements);

        std::sort_heap(sortedElements.begin(), sortedElements.end(), comparator);

        return sortedElements;
    }
};

template<typename ElementT, typename Comparator = std::less<ElementT>>
auto makeSmallestElementsContainer(int size_limit, Comparator comparator = Comparator{}) {
    return SmallestElementsContainer<ElementT, Comparator>{size_limit, std::move(comparator)};
}
