#pragma once

#include <cstdint>
#include <cstring>
#include <atomic>

#ifdef _DEBUG
#include "YtcDbg.hpp"
#define new DBG_NEW
#endif
namespace Ytc
{
    class Exception
    {
    public:
        Exception(const wchar_t* desc) : description_(desc) {}
        const wchar_t* What() const { return description_; }
    private:
        const wchar_t* description_;
    };

    template<typename T>
    class String
    {
    public:
        static constexpr uint32_t MaxSize = uint32_t(-1);
        static constexpr uint32_t InvalidIndex = MaxSize;
        /// <summary>
        /// Calculate the zero-terminated string length
        /// </summary>
        /// <param name="buffer"></param>
        /// <returns></returns>
        static uint32_t CountChar(const T* buffer) noexcept
        {
            uint32_t result = 0;
            while (*buffer++) ++result;
            return result;
        }
        /// <summary>
        /// Compare two specified string objects and returns an integer that indicates their relative position in the sort order.
        /// </summary>
        /// <param name="s1"></param>
        /// <param name="s2"></param>
        /// <returns></returns>
        static int Compare(const String<T>& s1, const String<T>& s2) noexcept
        {
            const T* buffer1 = s1.Buffer();
            const T* buffer2 = s2.Buffer();
            if (buffer1 == buffer2) return 0;
            const uint32_t length = s1.length_ < s2.length_ ? s1.length_ : s2.length_;
            for (uint32_t i = 0; i < length; ++i)
            {
                int diff = buffer1[i] - buffer2[i];
                if (diff) return diff;
            }
            return s1.length_ - s2.length_;
        }

        static int Compare(const String<T>& s1, const T* s2) noexcept
        {
            if (!s2) return 1;
            const T* buffer1 = s1.Buffer();
            const uint32_t length1 = s1.Length();
            for (uint32_t i = 0; i < length1; ++i)
            {
                T c = *s2++;
                if (c)
                {
                    int diff = buffer1[i] - c;
                    if (diff) return diff;
                }
                else
                {
                    return 1;
                }
            }
            return *s2 ? -1 : 0;
        }

        constexpr String() noexcept : length_(0), bufferSize_(StaticBufferSize)
        {
            storage_.staticBuffer[0] = 0;
        }

        constexpr String(T value) noexcept : bufferSize_(StaticBufferSize)
        {
            storage_.staticBuffer[0] = value;
            length_  = value ? 1 : 0;
        }

        String(T value, uint32_t count)
        {
            if (value && count)
            {
                auto* beginPtr = InitializeCapacity(count);
                auto* endPtr = beginPtr + count;
                while (beginPtr != endPtr) *beginPtr++ = value;
                *beginPtr = 0;
            }
            else
            {
                length_ = 0;
                bufferSize_ = StaticBufferSize;
                storage_.staticBuffer[0] = 0;
            }
        }

        String(const T* buffer, uint32_t length)
        {
            if (!buffer) throw Exception(L"Null pointer is not a string!");
            CreateFrom(buffer, length);
        }

        String(const T* buffer) :String(buffer, CountChar(buffer))
        {
        }

        String(const String<T>& other) 
        {
            if (other.IsLong())
            {
                GetSharedFrom(other);
            }
            else
            {
                CreateFrom(other.Buffer(), other.Length());
            }
        }

        String(const String<T>& _string, uint32_t start, uint32_t count = MaxSize)
        {
            if (start < _string.Length())
            {
                int length = _string.Length() - start;
                if (length < count) count = length;
                CreateFrom(_string.Buffer() + start, length);
            }
            else
            {
                throw Exception(L"Range error!");
            }
        }

        String(String<T>&& _string) noexcept
        {
            MoveImpl(std::move(_string));
        }

        ~String()
        {
            Destroy();
        }

        void Assign(const T* buffer, uint32_t length)
        {
            auto* ptr = UninitializedCopy(buffer, length, Reserve(length));
            length_ = length;
            *ptr = 0;  
        }

        String& operator=(const String<T>& other)
        {
            if (this != &other)
            {
                if (other.IsLong())
                {
                    Destroy();
                    GetSharedFrom(other);
                }
                else
                {
                    Assign(other.Buffer(), other.Length());
                }
            }
            return *this;
        }

        String& operator=(String<T>&& _string) noexcept
        {
            if (this != &_string)
            {
                Destroy();
                MoveImpl(std::move(_string));
            }
            return *this;
        }

        String& operator=(T c)
        {
            auto* ptr = storage_.staticBuffer;
            if (IsHeapAllocated())
            {
                if (storage_.variableBuffer.Sharing())
                {
                    storage_.variableBuffer.DecRef();
                    bufferSize_ = StaticBufferSize;;
                }
                else
                {
                    ptr = storage_.variableBuffer.ptr;
                }
            }
            *ptr = c;
            *(++ptr) = 0;
            length_ = c ? 1 : 0;
            return *this;
        }

        const T* Buffer() const noexcept
        {
            return IsHeapAllocated() ? storage_.variableBuffer.ptr : storage_.staticBuffer;
        }
        /// <summary>
        /// Return the count of characters.
        /// </summary>
        /// <returns></returns>
        uint32_t Length() const noexcept
        {
            return length_;
        }

        bool IsEmpty() const noexcept
        {
            return Length() == 0;
        }
        /// <summary>
        /// Retrieves a substring from this instance.
        /// </summary>
        /// <param name="start">The zero-absed starting character position of a substring in this instance.</param>
        /// <param name="length">The number of characters in the substring.</param>
        /// <returns></returns>
        String<T> SubString(uint32_t start, uint32_t length = MaxSize) const
        {
            if (start < length_)
            {
                const uint32_t max_len = length_ - start;
                return String(Buffer() + start, length <= max_len ? length : max_len);
            }
            throw Exception(L"The argument<start> is out of range!");
        }
        /// <summary>
        /// Reports the zero-based index of the first occurrence of a specified character.
        /// </summary>
        /// <param name="c">The specified character.</param>
        /// <returns>index of the character</returns>
        uint32_t IndexOf(T c) const noexcept
        {
            const T* buffer = Buffer();
            for (uint32_t i = 0; i < length_; ++i)
            {
                if (buffer[i] == c) return i;
            }
            return InvalidIndex;
        }
        /// <summary>
        /// Reports the zero-based index of the first occurrence of a specified string.
        /// </summary>
        /// <param name="value">The string to seek.</param>
        /// <returns>index of the string</returns>
        uint32_t IndexOf(const String<T>& value) const noexcept
        {
            if (value.IsEmpty()) return 0;
            uint32_t length = value.length_;
            const T* _buffer = value.Buffer();
            const T* my_buffer = Buffer();
            for (uint32_t start = 0; length <= length_ - start; ++start)
            {
                uint32_t i = start;
                for (uint32_t j = 0; j < length && my_buffer[i] == _buffer[j]; ++j)
                {
                    ++i;
                }
                if ((i - start) == length)
                {
                    return start;
                }
            }
            return InvalidIndex;
        }
        /// <summary>
        /// Report the zero-based index position of the last occurrence of a specified character within this instance.
        /// </summary>
        /// <param name="value">The specified character</param>
        /// <returns>the index of value</returns>
        uint32_t LastIndexOf(T value) const noexcept
        {
            const T* buffer = Buffer();
            for (uint32_t i = length_ - 1; i != InvalidIndex; --i)
            {
                if (buffer[i] == value) return i;
            }
            return InvalidIndex;
        }
        /// <summary>
        /// Report the zero-based index position of the last occurrence of a specified string within this instance.
        /// </summary>
        /// <param name="value">The specified string</param>
        /// <returns>the index of value</returns>
        uint32_t LastIndexOf(const String<T>& value) const noexcept
        {
            if (IsEmpty()) return InvalidIndex;
            if (value.IsEmpty()) return length_ - 1;
            uint32_t length = value.length_;
            const T* buffer = value.Buffer();
            const T* my_buffer = Buffer();
            for (uint32_t start = length_ - 1; length <= (start + 1); --start)
            {
                uint32_t i = start;
                for (uint32_t j = length - 1; j != InvalidIndex && my_buffer[i] == buffer[j]; --j)
                {
                    --i;
                }
                if ((start - i) == length) return i + 1;
            }
            return InvalidIndex;
        }

        /// <summary>
        /// Returns a value indicating whether a specified character occurs within this string.
        /// </summary>
        /// <param name="value">The specified character</param>
        /// <returns> true if the value occurs; otherwise, false</returns>
        bool Contains(T value) const noexcept
        {
            return IndexOf(value) != InvalidIndex;
        }
        /// <summary>
        /// Returns a value indicating whether a specified string occurs within this string.
        /// </summary>
        /// <param name="value">The specified string</param>
        /// <returns> true if the value occurs; otherwise, false</returns>
        bool Contains(const String<T>& value) const noexcept
        {
            return IndexOf(value) != InvalidIndex;
        }
        /// <summary>
        /// Returns a new string in which a specified number of characters in the current instance beginning at a specified position have been deleted.
        /// </summary>
        /// <param name="start">specified zero-based position</param>
        /// <param name="count">number of characters</param>
        /// <returns>a new string instance</returns>
        String<T> Remove(uint32_t start, uint32_t count) const
        {
            if (start < length_ && (start + count) <= length_)
            {
                auto* myBuffer = Buffer();
                uint32_t newLength = length_ - count;
                String<T> newString;
                auto* ptr = UninitializedCopy(myBuffer, start - 0, newString.InitializeCapacity(newLength));
                uint32_t rightStart = start + count;
                ptr = UninitializedCopy(myBuffer + rightStart, length_ - rightStart, ptr);
                *ptr = 0;
                return newString;
            }
            else
            {
                throw Exception(L"The argument is out of range for this instance!");
            }
        }

        /// <summary>
        /// Returns a copy of this string converted to uppercase.
        /// </summary>
        /// <returns>a new string instance</returns>
        String<T> ToUpper() const
        {
            String<T> newString(Buffer(), Length());
            T* buffer = const_cast<T*>(newString.Buffer());
            for (uint32_t i = 0; i < length_; ++i)
            {
                T c = buffer[i];
                if (c >= 'a' && c <= 'z')
                {
                    buffer[i] = c - DistanceOfUpperLower;
                }
            }
            return newString;
        }
        /// <summary>
        /// Returns a copy of this string converted to lowercase.
        /// </summary>
        /// <returns>a new string instance</returns>
        String<T> ToLower() const
        {
            String<T> newString(Buffer(), Length());
            T* buffer = const_cast<T*>(newString.Buffer());
            for (uint32_t i = 0; i < length_; ++i)
            {
                T c = buffer[i];
                if (c >= 'A' && c <= 'Z')
                {
                    buffer[i] = c + DistanceOfUpperLower;
                }
            }
            return newString;
        }

        static constexpr auto DistanceOfUpperLower = 'a' - 'A';

        String<T>& operator+=(const String<T>& value)
        {
            Append(value.Buffer(), value.Length());
            return *this;
        }

        String<T>& operator+=(const T* buffer)
        {
            Append(buffer, CountChar(buffer));
            return *this;
        }
        /// <summary>
        /// Add some characters to the tail, which may expand current instance 
        /// </summary>
        /// <param name="buffer">c-style string, it will crash if this param is nullptr</param>
        /// <param name="length">count of characters</param>
        void Append(const T* buffer, uint32_t length)
        {
            if (length)
            {
                T* tail = UninitializedCopy(buffer, length, Expand(length));
                *tail = 0;
            }
        }
        /// <summary>
        /// Add some characters to the tail, which may expand current instance
        /// </summary>
        /// <param name="value">a specified character</param>
        /// <param name="count">number of the character</param>
        void Append(T value, uint32_t count = 1)
        {
            if (value && count)
            {
                T* tail = Expand(count);
                for (; count-- != 0; ++tail)
                {
                    *tail = value;
                }
                *tail = 0;
            }
        }

        String<T>& operator+=(T value)
        {
            Append(value);
            return *this;
        }

        friend String<T> operator+(const String<T>& lhs, const String<T>& rhs)
        {
            return ConstructByConcat(lhs.Buffer(), lhs.length_, rhs.Buffer(), rhs.length_);
        }

        friend String<T> operator+(const String<T>& lhs, T rhs)
        {
            if (!rhs) return lhs;
            String<T> newString;
            auto* buffer = UninitializedCopy(lhs.Buffer(), lhs.Length(), newString.InitializeCapacity(lhs.Length() + 1));
            *buffer = rhs;
            *(++buffer) = 0;
            return newString;
        }

        friend String<T> operator+(T lhs, const String<T>& rhs)
        {
            if (!lhs) return rhs;
            String<T> newString;
            auto* buffer = newString.InitializeCapacity(lhs.Length() + 1);
            *buffer = lhs;
            UninitializedCopy(rhs.Buffer(), rhs.Length(), ++buffer);
            *buffer = 0;
            return newString;
        }

        friend String<T> operator+(const String<T>& lhs, const T* rhs)
        {
            auto rhsLen = CountChar(rhs);
            if (rhsLen == 0) return lhs;
            return ConstructByConcat(lhs.Buffer(), lhs.length_, rhs, rhsLen);
        }

        friend String<T> operator+(const T* lhs, const String<T>& rhs)
        {
            auto lhsLen = CountChar(lhs);
            if (lhsLen == 0) return rhs;
            return ConstructByConcat(lhs, lhsLen, rhs.Buffer(), rhs.length_);
        }

        bool operator==(const String<T>& _string) const noexcept
        {
            return Length() == _string.Length() && !Compare(*this, _string);
        }

        bool operator!=(const String<T>& _string) const noexcept
        {
            return !(*this == _string);
        }

        bool operator>(const String<T>& _string) const noexcept
        {
            return Compare(*this, _string) > 0;
        }

        bool operator<=(const String<T>& _string) const noexcept
        {
            return !(*this > _string);
        }

        bool operator<(const String<T>& _string) const noexcept
        {
            return Compare(*this, _string) < 0;
        }

        bool operator>=(const String<T>& _string) const noexcept
        {
            return !(*this < _string);
        }

        bool operator==(const T* _string) const noexcept
        {
            return !Compare(*this, _string);
        }

        bool operator!=(const T* _string) const noexcept
        {
            return !(*this == _string);
        }

        bool operator>(const T* _string) const noexcept
        {
            return Compare(*this, _string) > 0;
        }

        bool operator<=(const T* _string) const noexcept
        {
            return !(*this > _string);
        }

        bool operator<(const T* _string) const noexcept
        {
            return Compare(*this, _string) < 0;
        }

        bool operator>=(const T* _string) const noexcept
        {
            return !(*this < _string);
        }

    private:

        struct VariableBuffer
        {
            using RefCounter = std::atomic_uint32_t;
            mutable RefCounter* refCount; // COW for long string
            T* ptr;

            bool IsRefCountBased() const noexcept
            {
                return refCount != nullptr;
            }

            void IncRef() const
            {
                if (!IsRefCountBased())
                {
                    refCount = CreateRefCounter();
                }

                ++(*refCount);
            }

            void DecRef() const
            {
                --(*refCount);
            }

            void CheckRefCount()
            {
                if (*refCount == 0)
                {
                    DestroyRefCounter(refCount);
                    delete[] ptr;
                }
            }

            bool Sharing() const noexcept
            {
                return IsRefCountBased() && RefCount() > 1;
            }

            uint32_t RefCount() const
            {
                return *refCount;
            }
        private:
            static auto* CreateRefCounter()
            {
                return new RefCounter(1);
            }

            static void DestroyRefCounter(RefCounter* counter)
            {
                delete counter;
            }
        };
        
        static constexpr uint32_t StaticBufferSize = 16;
        static constexpr uint32_t MinLongStringLength = 256;

        static T* UninitializedCopy(const T* source, uint32_t count, T* dest)
        {
            memcpy(dest, source, count * sizeof(T));
            return dest + count;
        }

        bool IsHeapAllocated() const noexcept
        {
            return bufferSize_ > StaticBufferSize;
        }

        bool IsLong() const noexcept
        {
            return Length() >= MinLongStringLength;
        }

        T* InitializeCapacity(uint32_t length)
        {
            length_ = length;
            if (length < StaticBufferSize)
            {
                bufferSize_ = StaticBufferSize;
                return storage_.staticBuffer;
            }
            bufferSize_ = length + 1;
            auto* ptr = new T[bufferSize_];
            storage_.variableBuffer.ptr = ptr;
            storage_.variableBuffer.refCount = nullptr;
            length_ = length;
            return ptr;
        }

        T* Reserve(uint32_t n)
        {
            uint32_t bufferSizeRequired = n + 1;
            if (IsHeapAllocated())
            {
                if (storage_.variableBuffer.Sharing())
                {
                    storage_.variableBuffer.DecRef();
                    if (n < StaticBufferSize)
                    {
                        bufferSize_ = StaticBufferSize;
                        return storage_.staticBuffer;
                    }
                    storage_.variableBuffer.ptr = new T[bufferSizeRequired];
                    storage_.variableBuffer.refCount = nullptr;
                    bufferSize_ = bufferSizeRequired;
                    return storage_.variableBuffer.ptr;
                }
                else
                {
                    if (bufferSize_ < bufferSizeRequired) 
                    {
                        delete[] storage_.variableBuffer.ptr;
                        storage_.variableBuffer.ptr = new T[bufferSizeRequired];
                        bufferSize_ = bufferSizeRequired;
                    }
                    return storage_.variableBuffer.ptr;
                }
            }
            
            if (n < StaticBufferSize)
            {
                return storage_.staticBuffer;
            }
            storage_.variableBuffer.ptr = new T[bufferSizeRequired];
            storage_.variableBuffer.refCount = nullptr;
            bufferSize_ = bufferSizeRequired;
            return storage_.variableBuffer.ptr;
        }

        T* Expand(uint32_t extraLength)
        {
            uint32_t newLength = length_ + extraLength;
            uint32_t minBufferSize = newLength + 1;
            T* buffer = nullptr;
            if (IsHeapAllocated())
            {
                if (storage_.variableBuffer.Sharing())
                {
                    assert(newLength >= StaticBufferSize);
                    buffer = new T[minBufferSize];
                    T* tail = UninitializedCopy(storage_.variableBuffer.ptr, length_, buffer);
                    storage_.variableBuffer.DecRef();
                    storage_.variableBuffer.ptr = buffer;
                    storage_.variableBuffer.refCount = nullptr;
                    bufferSize_ = minBufferSize;
                    buffer = tail;
                }
                else
                {
                    if (bufferSize_ < minBufferSize)
                    {
                        buffer = new T[minBufferSize];
                        T* tail = UninitializedCopy(storage_.variableBuffer.ptr, length_, buffer);
                        delete[] storage_.variableBuffer.ptr;
                        storage_.variableBuffer.ptr = buffer;
                        bufferSize_ = minBufferSize;
                        buffer = tail;
                    }
                    else
                    {
                        buffer = storage_.variableBuffer.ptr + length_;
                    }
                }
            }
            else if (StaticBufferSize < minBufferSize)
            {
                buffer = new T[minBufferSize];
                T* tail = UninitializedCopy(storage_.staticBuffer, length_, buffer);
                storage_.variableBuffer.ptr = buffer;
                storage_.variableBuffer.refCount = nullptr;
                bufferSize_ = minBufferSize;
                buffer = tail;
            }
            else
            {
                buffer = storage_.staticBuffer + length_;
            }
            length_ = newLength;
            return buffer;
        }

        void ShallowCopyFrom(const String<T>& other)
        {
            memcpy(this, &other, sizeof(other));
        }

        void MoveImpl(String<T>&& other)
        {
            ShallowCopyFrom(other);
            other.storage_.staticBuffer[0] = 0;
            other.length_ = 0;
            other.bufferSize_ = StaticBufferSize;
        }

        void GetSharedFrom(const String<T>& other)
        {
            other.storage_.variableBuffer.IncRef();
            ShallowCopyFrom(other);
        }

        void CreateFrom(const T* buffer, uint32_t length)
        {
            auto* ptr = UninitializedCopy(buffer, length, InitializeCapacity(length));
            *ptr = 0;
        }

        void Destroy()
        {
            if (IsHeapAllocated())
            {
                if (storage_.variableBuffer.IsRefCountBased())
                {
                    storage_.variableBuffer.DecRef();
                    storage_.variableBuffer.CheckRefCount();
                }
                else
                {
                    delete[] storage_.variableBuffer.ptr;
                }
                bufferSize_ = StaticBufferSize;
            }
            length_ = 0;
        }

        static String<T> ConstructByConcat(const T* s1, uint32_t len1, 
                                           const T* s2, uint32_t len2)
        {
            String<T> newString;
            T* buffer = newString.InitializeCapacity(len1 + len2);
            buffer = UninitializedCopy(s1, len1, buffer);
            buffer = UninitializedCopy(s2, len2, buffer);
            *buffer = 0;
            return newString;
        }

        union Storage
        {
            VariableBuffer variableBuffer;
            T staticBuffer[StaticBufferSize];
        };
        Storage storage_;
        uint32_t length_;
        uint32_t bufferSize_;
    };

    template<typename T>
    bool operator==(const T* s1, const String<T>& s2) noexcept
    {
        return s2 == s1;
    }
    template<typename T>
    bool operator!=(const T* s1, const String<T>& s2) noexcept
    {
        return s2 != s1;
    }
    template<typename T>
    bool operator<(const T* s1, const String<T>& s2) noexcept
    {
        return s2 > s1;
    }
    template<typename T>
    bool operator>=(const T* s1, const String<T>& s2) noexcept
    {
        return s2 <= s1;
    }
    template<typename T>
    bool operator>(const T* s1, const String<T>& s2) noexcept
    {
        return s2 < s1;
    }
    template<typename T>
    bool operator<=(const T* s1, const String<T>& s2) noexcept
    {
        return s2 >= s1;
    }

    using AString = String<char>;
    using WString = String<wchar_t>;
}