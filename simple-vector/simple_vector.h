#pragma once

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>

#include "array_ptr.h"

struct Reserve {
  size_t capacity;
};
struct Reserve Reserve(size_t capacity_to_reserve) {
  return {capacity_to_reserve};
}

template <typename Type> class SimpleVector {
public:
  using Iterator = Type *;
  using ConstIterator = const Type *;

  SimpleVector() noexcept = default;

  // Создаёт вектор из size элементов, инициализированных значением по
  // умолчанию
  explicit SimpleVector(size_t size)
      : items_(size), size_(size), capacity_(size) {}

  explicit SimpleVector(struct Reserve capacity)
      : items_(capacity.capacity), size_(0), capacity_(capacity.capacity) {}

  // Создаёт вектор из size элементов, инициализированных значением value
  SimpleVector(size_t size, const Type &value)
      : items_(size), size_(size), capacity_(size) {
    for (size_t i = 0; i < size; i++) {
      items_.Get()[i] = value;
    }
  }

  // Создаёт вектор из std::initializer_list
  SimpleVector(std::initializer_list<Type> init)
      : items_(init.size()), size_(init.size()), capacity_(init.size()) {
    std::copy(init.begin(), init.end(), items_.Get());
  }

  // Возвращает количество элементов в массиве
  size_t GetSize() const noexcept { return size_; }

  // Возвращает вместимость массива
  size_t GetCapacity() const noexcept { return capacity_; }

  // Сообщает, пустой ли массив
  bool IsEmpty() const noexcept { return size_ == 0; }

  // Возвращает ссылку на элемент с индексом index
  Type &operator[](size_t index) noexcept { return items_[index]; }

  // Возвращает константную ссылку на элемент с индексом index
  const Type &operator[](size_t index) const noexcept { return items_[index]; }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  Type &At(size_t index) {
    if (index >= size_) {
      throw std::out_of_range("problem");
    }
    return items_[index];
  }

  // Возвращает константную ссылку на элемент с индексом index
  // Выбрасывает исключение std::out_of_range, если index >= size
  const Type &At(size_t index) const {
    if (index >= size_) {
      throw std::out_of_range("problem");
    }
    return items_[index];
  }

  // Обнуляет размер массива, не изменяя его вместимость
  void Clear() noexcept { Resize(0); }

  // Изменяет размер массива.
  // При увеличении размера новые элементы получают значение по умолчанию для
  // типа Type
  void Resize(size_t new_size) {
    if (new_size <= capacity_) {
      if (new_size > size_) {
        std::generate(begin() + new_size, end(), []() { return Type{}; });
      }
      size_ = new_size;
      return;
    }
    ArrayPtr<Type> new_items(new_size);

    std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()),
              new_items.Get());

    items_.swap(new_items);
    size_ = new_size;
    capacity_ = new_size;
  }

  // Возвращает итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  Iterator begin() noexcept { return size_ ? &items_[0] : nullptr; }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  Iterator end() noexcept { return size_ ? &items_[size_] : nullptr; }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator begin() const noexcept { return cbegin(); }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator end() const noexcept { return cend(); }

  // Возвращает константный итератор на начало массива
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cbegin() const noexcept { return size_ ? &items_[0] : nullptr; }

  // Возвращает итератор на элемент, следующий за последним
  // Для пустого массива может быть равен (или не равен) nullptr
  ConstIterator cend() const noexcept {
    return size_ ? &items_[size_] : nullptr;
  }

  ~SimpleVector() {}

  SimpleVector(const SimpleVector &other) : SimpleVector(other.size_) {
    if (!other.IsEmpty()) {
      std::copy(other.begin(), other.end(), items_.Get());
    }
  }

  SimpleVector(SimpleVector &&other) noexcept
      : items_(std::move(other.items_)), size_(other.size_),
        capacity_(other.capacity_) {
    other.size_ = 0;
    other.capacity_ = 0;
  }

  SimpleVector &operator=(const SimpleVector &rhs) {
    if (this == &rhs) {
      return *this;
    }
    ArrayPtr<Type> new_items(rhs.size_);
    std::copy(rhs.begin(), rhs.end(), new_items.Get());
    using namespace std;
    items_.swap(new_items);
    size_ = rhs.size_;
    capacity_ = rhs.size_;
    return *this;
  }

  // Добавляет элемент в конец вектора
  // При нехватке места увеличивает вдвое вместимость вектора
  void PushBack(const Type &item) {
    if (size_ < capacity_) {
      items_[size_++] = item;
      return;
    }
    size_t size = size_;
    Resize(size_ ? size_ * 2 : 1);
    items_[size++] = item;
    size_ = size;
  }
  void PushBack(Type &&item) {
    if (size_ < capacity_) {
      items_[size_++] = std::move(item);
      return;
    }
    size_t size = size_;
    Resize(size_ ? size_ * 2 : 1);
    items_[size++] = std::move(item);
    size_ = size;
  }
  // Вставляет значение value в позицию pos.
  // Возвращает итератор на вставленное значение
  // Если перед вставкой значения вектор был заполнен полностью,
  // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью
  // 0 стать равной 1
  Iterator Insert(ConstIterator pos, const Type &value) {
    if (size_ == 0) {
      PushBack(value);
      return begin();
    } else {
      size_t index = pos - begin();
      if (size_ == capacity_) {
        size_t size = size_;
        Resize(capacity_ * 2);
        size_ = size;
      }
      if (index == size_ - 1) {
        PushBack(value);
      } else {
        std::copy_backward(begin() + index, end() - 1, end());
        items_[index] = value;
        size_++;
      }
      return begin() + index;
    }
  }

  Iterator Insert(ConstIterator pos, Type &&value) {
    if (size_ == 0) {
      PushBack(std::move(value));
      return begin();
    } else {
      size_t index = pos - begin();
      if (size_ == capacity_) {
        size_t size = size_;
        Resize(capacity_ * 2);
        size_ = size;
      }
      if (index == size_ - 1) {
        PushBack(std::move(value));
      } else {

        std::move_backward(begin() + index, end(), end() + 1);
        items_[index] = std::move(value);
        size_++;
      }
      return begin() + index;
    }
  }

  void PopBack() noexcept { size_--; }

  // Удаляет элемент вектора в указанной позиции
  Iterator Erase(ConstIterator pos) {
    size_t index = pos - begin();
    if (index == size_ - 1) {
      PopBack();
      return end();
    }
    std::move(begin() + index + 1, end(), begin() + index);
    size_--;
    return begin() + index;
  }

  // Обменивает значение с другим вектором
  void swap(SimpleVector &other) noexcept {
    using namespace std;
    items_.swap(other.items_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
  }

  void Reserve(size_t new_capacity) {
    if (new_capacity < capacity_) {
      return;
    }
    size_t size = size_;
    Resize(new_capacity);

    size_ = size;
  }

private:
  ArrayPtr<Type> items_;
  size_t size_ = 0;
  size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type> &lhs,
                       const SimpleVector<Type> &rhs) {
  if (lhs.GetSize() != rhs.GetSize()) {
    return false;
  }
  if ((lhs.begin() == nullptr) != (rhs.begin() == nullptr)) {
    return false;
  }
  return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type> &lhs,
                       const SimpleVector<Type> &rhs) {
  return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type> &lhs,
                      const SimpleVector<Type> &rhs) {
  if ((lhs.begin() == nullptr) != (rhs.begin() == nullptr)) {
    return false;
  }
  return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
                                      rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type> &lhs,
                       const SimpleVector<Type> &rhs) {
  return !(lhs > rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type> &lhs,
                      const SimpleVector<Type> &rhs) {
  return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type> &lhs,
                       const SimpleVector<Type> &rhs) {
  return !(lhs < rhs);
}
