#pragma once
#include <vector>

namespace mg {

template <class T> class FreeList {
public:
  /// Creates a new free list.
  FreeList();

  /// Inserts an element to the free list and returns an index to it.
  int insert(const T &element);

  // Removes the nth element from the free list.
  void erase(int n);

  // Removes all elements from the free list.
  void clear();

  // Returns the range of valid indices.
  int range() const;

  // Returns the nth element.
  T &operator[](int n);

  // Returns the nth element.
  const T &operator[](int n) const;

  // Reserves space for n elements.
  void reserve(int n);

  // Swaps the contents of the two lists.
  void swap(FreeList &other);

private:
  union FreeElement {
    T element;
    int next;
  };
  std::vector<FreeElement> data;
  int first_free;
};

template <class T> FreeList<T>::FreeList() : first_free(-1) {}

template <class T> int FreeList<T>::insert(const T &element) {
  if (first_free != -1) {
    const int index = first_free;
    first_free = data[first_free].next;
    data[index].element = element;
    return index;
  } else {
    FreeElement fe;
    fe.element = element;
    data.push_back(fe);
    return data.size() - 1;
  }
}

template <class T> void FreeList<T>::erase(int n) {
  assert(n >= 0 && n < data.size());
  data[n].next = first_free;
  first_free = n;
}

template <class T> void FreeList<T>::clear() {
  data.clear();
  first_free = -1;
}

template <class T> int FreeList<T>::range() const { return data.size(); }

template <class T> T &FreeList<T>::operator[](int n) { return data[n].element; }

template <class T> const T &FreeList<T>::operator[](int n) const { return data[n].element; }

template <class T> void FreeList<T>::reserve(int n) { data.reserve(n); }

template <class T> void FreeList<T>::swap(FreeList &other) {
  const int temp = first_free;
  data.swap(other.data);
  first_free = other.first_free;
  other.first_free = temp;
}

} // namespace mg