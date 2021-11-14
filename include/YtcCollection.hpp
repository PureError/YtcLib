#pragma once

#include "YtcMemory.hpp"
#include <cstdint>
namespace Ytc
{

    template<typename T>
    using NonReferenceType = std::remove_reference_t<T>;

    template<typename T>
    inline NonReferenceType<T>&& Move(T&& value) noexcept
    {
        return std::move(value);
    }

    class Disposable
    {
    public:
        virtual ~Disposable() {}
    };


    template<typename T>
    class IEnumerator : public Disposable
    {
    public:
        virtual bool MoveNext() = 0;
        virtual T& Current() = 0;
        virtual void Reset() = 0;
    };

    template<typename T>
    class IEnumerable : public Disposable
    {
    public:
        virtual Ref<IEnumerator<T>> GetEnumerator() = 0;
    };


    template<typename T>
    class ICollection : public IEnumerable<T>
    {
    public:
        /// <summary>
        /// Gets the number of elements contained in the ICollection.
        /// </summary>
        /// <returns>number of elements</returns>
        virtual uint32_t Count() const = 0;
        /// <summary>
        /// Gets a value indicating whether access to the ICollection is synchronized (thread safe).
        /// </summary>
        /// <returns>true if it is synchronized otherwise false</returns>
        virtual bool IsSynchronized() const { return false; }

    };

    template<typename T>
    class List : public ICollection<T>
    {
    public:
        static constexpr int InvalidIndex = -1;
        class Enumerator : public IEnumerator<T>
        {
        public:
            Enumerator(List<T>& list) : list_(list), index_(InvalidIndex)
            {
            }

            ~Enumerator()
            {
            }

            bool MoveNext() override
            {
                return ++index_ < static_cast<int>(list_.Count());
            }
            T& Current() override
            {
                return list_[index_];
            }
            void Reset() override
            {
                index_ = InvalidIndex;
            }
        private:
            List<T>& list_;
            int index_;
        };

        List() noexcept : buffer_(nullptr), count_(0), capacity_(0)
        {
        }

        List(const List& other)
        {
            if (other.count_)
            {
                const auto bytesCount = other.count_ * sizeof(T);
                buffer_ = static_cast<T*>(malloc(bytesCount));
                std::uninitialized_copy(other.buffer_, other.buffer_ + other.count_, buffer_);
                capacity_ = count_ = other.count_;
            }
            else
            {
                buffer_ = nullptr;
                capacity_ = count_ = 0;
            }
        }

        List(List&& other)
        {
            memcpy(this, &other, sizeof(*this));
            other.buffer_ = nullptr;
            other.capacity_ = 0;
            other.count_ = 0;
        }

        ~List()
        {
            if (buffer_)
            {
                Clear();
                free(buffer_);
            }
        }

        List<T>& operator=(const List<T>& other)
        {
            if (this != &other)
            {
                Clear();
                if (capacity_ < other.count_)
                {
                    if (buffer_)
                    {
                        free(buffer_);
                    }
                    new (this)List<T>(other);
                }
                else
                {
                    T* src = other.buffer_;
                    if (count_ < other.count_)
                    {
                        T* copied = src + count_;
                        T* dest = std::copy(src, copied, buffer_);
                        std::uninitialized_copy(copied, src + other.count_, dest);
                    }
                    else
                    {
                        std::copy(src, src + other.count_, buffer_);
                        Discard(other.count_, count_);
                    }
                    count_ = other.count_;
                }
            }
            return *this;
        }

        List<T>& operator=(List<T>&& other)
        {
            if (this != &other)
            {
                SwapWith(other);
            }
            return *this;
        }

        void Add(const T& item)
        {
            uint32_t newCount = count_ + 1;
            if (capacity_ < newCount)
            {
                Reserve(newCount + (newCount >> 1));
            }
            new (buffer_ + count_) T(item);
            count_ = newCount;
        }

        void SwapWith(List<T>& other)
        {
            std::swap(buffer_, other.buffer_);
            std::swap(capacity_, other.capacity_);
            std::swap(count_, other.count_);
        }

        void AddRange(Ref<IEnumerable<T>> collection)
        {
            while (collection->MoveNext())
            {
                Add(collection->Current());
            }
        }

        void Clear()
        {
            Discard(0, count_);
            count_ = 0;
        }

        int IndexOf(const T& item) const
        {
            for (int i = 0; i < count_; ++i)
            {
                if (buffer_[i] == item)
                {
                    return i;
                }
            }
            return InvalidIndex;
        }

        bool Contains(const T& item) const
        {
            return IndexOf(item) != InvalidIndex;
        }

        Ref<IEnumerator<T>> GetEnumerator() override
        {
            return MakeRef<Enumerator>(*this);
        }

        uint32_t Count() const override
        {
            return count_;
        }

        T& operator[](int index)
        {
            return buffer_[index];
        }

        const T& operator[](int index) const
        {
            return buffer_[index];
        }
    private:
        void Reserve(uint32_t size)
        {
            ReallocImpl(size, std::is_trivially_copy_constructible<T>());
            capacity_ = size;
        }

        void ReallocImpl(size_t size, std::true_type)
        {
            buffer_ = static_cast<T*>(realloc(buffer_, size * sizeof(T)));
        }

        void ReallocImpl(size_t size, std::false_type)
        {
            T* newBuffer = static_cast<T*>(malloc(sizeof(T) * size));
            std::uninitialized_copy(buffer_, buffer_ + count_, newBuffer);
            buffer_ = newBuffer;
        }
        
        void Discard(uint32_t start, uint32_t count)
        {
            DiscardImpl(start, count, std::is_trivially_destructible<T>());
        }

        void DiscardImpl(uint32_t start, uint32_t count, std::true_type) {}
        void DiscardImpl(uint32_t start, uint32_t count, std::false_type)
        {
            for (; start < count; ++start)
            {
                buffer_[start].~T();
            }
        }

        T* buffer_;
        uint32_t count_;
        uint32_t capacity_;
    };
}