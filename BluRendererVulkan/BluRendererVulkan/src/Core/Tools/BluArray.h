#ifndef BLUARRAY_H
#define BLUARRAY_H
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

template <typename T>
class BluArray {
 public:
  BluArray() : data_(nullptr), size_(0), capacity_(0) {}
  
  explicit BluArray(size_t initial_size)
      : size_(initial_size), capacity_(initial_size) {
    data_ = new T[capacity_];
  }

  BluArray(const BluArray& other)
      : size_(other.size_), capacity_(other.capacity_) {
    data_ = new T[capacity_];
    std::copy(other.data_, other.data_ + size_, data_);
  }

  BluArray(BluArray&& other) noexcept
      : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
  }

  ~BluArray() { delete[] data_; }

  // Copy assignment operator
  BluArray& operator=(const BluArray& other) {
    if (this != &other) {
      BluArray temp(other);
      swap(*this, temp);
    }
    return *this;
  }

  // Move assignment operator
  BluArray& operator=(BluArray&& other) noexcept {
    if (this != &other) {
      delete[] data_;
      data_ = other.data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      other.data_ = nullptr;
      other.size_ = 0;
      other.capacity_ = 0;
    }
    return *this;
  }

  // Element access
  T& operator[](size_t index) {
    if (index >= size_) {
      throw std::out_of_range("Index out of range");
    }
    return data_[index];
  }

  const T& operator[](size_t index) const {
    if (index >= size_) {
      throw std::out_of_range("Index out of range");
    }
    return data_[index];
  }

  // Size and capacity
  size_t size() const { return size_; }
  size_t capacity() const { return capacity_; }

  // Resize the array
  void resize(size_t new_size) {
    if (new_size > capacity_) {
      reserve(new_size);
    }
    size_ = new_size;
  }

  // Reserve memory
  void reserve(size_t new_capacity) {
    if (new_capacity > capacity_) {
      T* new_data = new T[new_capacity];
      std::copy(data_, data_ + size_, new_data);
      delete[] data_;
      data_ = new_data;
      capacity_ = new_capacity;
    }
  }

  // Push back element
  void push_back(const T& value) {
    if (size_ == capacity_) {
      reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    }
    data_[size_++] = value;
  }

  // Pop back element
  void pop_back() {
    if (size_ > 0) {
      --size_;
    }
  }

  // Clear the array
  void clear() { size_ = 0; }

 private:
  T* data_;
  size_t size_;
  size_t capacity_;
};

#endif