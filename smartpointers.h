#ifndef SMART_POINTER_SMARTPOINTERS_H
#define SMART_POINTER_SMARTPOINTERS_H

#include <iostream>
#include <memory>

template <typename T>
class UniquePtr {
private:
    T *object;

public:
    explicit UniquePtr() noexcept : object{nullptr} {}

    UniquePtr(const UniquePtr &right) = delete;

    UniquePtr <T> &operator=(const UniquePtr &right) = delete;

    explicit UniquePtr(T *right) noexcept : object{right} {}

    UniquePtr(T &right) noexcept : object{right} {}

    UniquePtr(UniquePtr <T> &&right) {
        object = right.object;
        right.object = nullptr;
    }

    template<typename U>
    UniquePtr(UniquePtr<U>&& right) noexcept {
        object = right.object;
        right.object = nullptr;
    }

    UniquePtr <T> &operator=(UniquePtr <T> &&right) noexcept {
        if (object)
            delete object;

        object = right.object;
        right.object = nullptr;

        return *this;
    }

    ~UniquePtr() noexcept {
        delete object;
    }

    typename std::add_lvalue_reference <T>::type operator*() const {
        return *get();
    }

    T *operator->() const noexcept {
        return get();
    }

    T *get() const noexcept {
        return object;
    }

    T *release() noexcept {
        T *copy = object;
        object = nullptr;
        return copy;
    }

    template <typename U>
    void reset(U *pointer = nullptr) noexcept {
        if (object != nullptr)
            delete object;

        object = static_cast<T *> (pointer);
    }

    void swap(UniquePtr <T> &other) noexcept {
        auto temp = other.object;
        other.object = object;
        object = temp;
    }
};
class ReferenceCounter {
private:
    size_t count;
    size_t weak;

public:
    explicit ReferenceCounter() noexcept : count{0}, weak{0} {}

    void add() noexcept {
        ++count;
    }

    size_t remove() noexcept {
        return --count;
    }

    auto get() const noexcept {
        return count;
    }

    void addWeak() noexcept {
        ++weak;
    }

    size_t removeWeak() noexcept {
        return --weak;
    }

    auto getWeak() const noexcept {
        return weak;
    }
};

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {
private:
    friend class WeakPtr <T>;

    T *object;
    ReferenceCounter *reference;

    void deleter() noexcept {
        if (object && !reference->remove()) {
            delete object;
            if (!reference->getWeak())
                delete reference;
        }
    }

public:
    explicit SharedPtr() noexcept : object{nullptr}, reference{nullptr} {}

    explicit SharedPtr(T *pointer) noexcept : object{pointer}, reference{new ReferenceCounter{}} {
        reference->add();
    }

    SharedPtr(const SharedPtr <T> &right) noexcept : object{right.object}, reference{right.reference} {
        if (reference)
            reference->add();
    }

    SharedPtr(const WeakPtr <T> &right) noexcept : object{right.pointer}, reference{right.reference} {
        if (reference)
            reference->add();
    }

    SharedPtr(SharedPtr <T> &&right) noexcept {
        object = right.object;
        reference = right.reference;

        right.object = nullptr;
        right.reference = nullptr;
    }

    ~SharedPtr() noexcept {
        deleter();
    }

    SharedPtr <T> &operator=(const SharedPtr <T> &right) noexcept {
        if (this == &right)
            return *this;

        deleter();

        object = right.object;
        reference = right.reference;
        reference->add();

        return *this;
    }

    SharedPtr <T> &operator=(SharedPtr <T> &&right) noexcept {
        if (this == &right)
            return *this;

        deleter();

        object = right.object;
        reference = right.reference;

        right.object = nullptr;
        right.reference = nullptr;

        return *this;
    }

    typename std::add_lvalue_reference <T>::type operator*() const {
        return *get();
    }

    T *operator->() const noexcept {
        return get();
    }

    T *get() const noexcept {
        return object;
    }

    size_t use_count() const noexcept {
        return (reference) ? reference->get() : 0;
    }

    template <typename U>
    void reset(U *pointer = nullptr) noexcept {
        deleter();

        if (pointer) {
            object = static_cast<T *> (pointer);

            reference = new ReferenceCounter{};
            reference->add();
        } else {
            object = nullptr;
            reference = nullptr;
        }

    }

    void reset() noexcept {
        deleter();

        object = nullptr;
        reference = nullptr;
    }

    void swap(SharedPtr <T> &other) noexcept {
        auto temp = other.object;
        other.object = object;
        object = temp;

        auto ref = other.reference;
        other.reference = reference;
        reference = ref;
    }
};

template <typename T> class WeakPtr {
private:
    friend class SharedPtr <T>;

    T *pointer;
    ReferenceCounter *reference;

    void deleter() noexcept {
        if (reference && !reference->get() && !reference->removeWeak())
            delete reference;
    }

public:
    explicit WeakPtr() noexcept : pointer{nullptr}, reference{nullptr} {}

    WeakPtr(const SharedPtr <T> &from) noexcept : pointer{from.object}, reference{from.reference} {
        reference->addWeak();
    }

    WeakPtr(const WeakPtr <T> &right) noexcept : pointer{right.pointer}, reference{right.reference} {
        reference->addWeak();
    }

    WeakPtr(WeakPtr <T> &&right) noexcept {
        pointer = right.pointer;
        reference = right.reference;

        right.pointer = nullptr;
        right.reference = nullptr;
    }

    ~WeakPtr() noexcept {
        deleter();
    }

    WeakPtr <T> &operator=(const WeakPtr <T> &right) noexcept {
        if (this == &right)
            return *this;

        deleter();

        pointer = right.pointer;
        reference = right.reference;
        reference->addWeak();

        return *this;
    }

    WeakPtr <T> &operator=(const SharedPtr <T> &right) noexcept {
        deleter();

        pointer = right.object;
        reference = right.reference;
        reference->addWeak();

        return *this;
    }

    WeakPtr <T> &operator=(WeakPtr <T> &&right) noexcept {
        if (this == &right)
            return *this;

        deleter();

        pointer = right.pointer;
        reference = right.reference;

        right.pointer = nullptr;
        right.reference = nullptr;

        return *this;
    }

    bool expired() const noexcept {
        return !use_count();
    }

    size_t use_count() const noexcept {
        return (reference) ? reference->get() : 0;
    }

    SharedPtr <T> lock() const noexcept {
        return expired() ? SharedPtr <T> {} : SharedPtr <T> {*this};
    }

    void reset() noexcept {
        deleter();

        pointer = nullptr;
        reference = nullptr;
    }

    void swap(WeakPtr <T> &other) noexcept {
        auto temp = other.pointer;
        other.pointer = pointer;
        pointer = temp;

        auto ref = other.reference;
        other.reference = reference;
        reference = ref;
    }
};

#endif //SMART_POINTER_SMARTPOINTERS_H
