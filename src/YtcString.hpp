#pragma once

#include <cstdint>
#include <cstring>
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
			if (!buffer) return 0;
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

		String() noexcept = default;

		String(T c)  noexcept
		{
			if (c)
			{
				storage_.static_buffer[0] = c;
				length_ = 1;
			}
		}

		String(const T* buffer, uint32_t length)
		{
			if (buffer)
			{
				InitializeFrom(buffer, length);
			}
		}

		String(const T* buffer) :String(buffer, CountChar(buffer))
		{
		}

		String(const String<T>& _string)
		{
			if (_string.Length() < MaxStaticBufferSize)
			{
				memcpy(this, &_string, sizeof(_string));
			}
			else
			{
				length_ = _string.Length();
				storage_.buffer_size = length_ + 1;
				storage_.variable_buffer = new T[storage_.buffer_size];
				memcpy(storage_.variable_buffer, _string.storage_.variable_buffer, storage_.buffer_size * sizeof(T));
			}
		}
		String(const String<T>& _string, uint32_t start, uint32_t count = MaxSize)
		{
			if (start < _string.Length())
			{
				int length = _string.Length() - start;
				if (length < count) count = length;
				InitializeFrom(_string.Buffer() + start, length);
			}
		}
		String(String<T>&& _string) noexcept
		{
			memcpy(this, &_string, sizeof(_string));
			_string.storage_.static_buffer[0] = 0;
			_string.length_ = 0;
		}

		~String()
		{
			if (Length() >= MaxStaticBufferSize)
			{
				delete[] storage_.variable_buffer;
			}
		}

		String& operator=(const String<T>& _string)
		{
			if (this != &_string)
			{
				if (_string.Length() < MaxStaticBufferSize)
				{
					if (Length() >= MaxStaticBufferSize)
					{
						delete[] storage_.variable_buffer;
					}
					memcpy(this, &_string, sizeof(_string));
				}
				else
				{
					length_ = _string.Length();
					if (storage_.buffer_size < length_)
					{
						delete[] storage_.variable_buffer;
						storage_.buffer_size = length_ + 1;
						storage_.variable_buffer = new T[storage_.buffer_size];
					}
					memcpy(storage_.variable_buffer, _string.storage_.variable_buffer, sizeof(T) * length_);
					storage_.variable_buffer[length_] = 0;
				}
			}
			return *this;
		}
		String& operator=(String<T>&& _string)
		{
			if (this != &_string)
			{
				if (Length() >= MaxStaticBufferSize)
				{
					delete[] storage_.variable_buffer;
				}
				memcpy(this, &_string, sizeof(_string));
				_string.storage_.static_buffer[0] = 0;
				_string.length_ = 0;
			}
			return *this;
		}

		String& operator=(T c)
		{
			if (Length() >= MaxStaticBufferSize)
			{
				delete[] storage_.variable_buffer;
			}
			storage_.static_buffer[0] = c;
			length_ = c ? 1 : 0;
			return *this;
		}

		const T* Buffer() const noexcept
		{
			return Length() < MaxStaticBufferSize ? storage_.static_buffer : storage_.variable_buffer;
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
		String SubString(uint32_t start, uint32_t length = MaxSize) const
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

		bool operator==(const String<T>& _string) const noexcept
		{
			return !Compare(*this, _string);
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
		void InitializeFrom(const T* buffer, uint32_t length)
		{
			T* dest_buffer = nullptr;
			if (length < MaxStaticBufferSize)
			{
				dest_buffer = storage_.static_buffer;
			}
			else
			{
				storage_.buffer_size = length + 1;
				storage_.variable_buffer = new T[storage_.buffer_size];
				dest_buffer = storage_.variable_buffer;
			}
			memcpy(dest_buffer, buffer, sizeof(T) * length);
			dest_buffer[length] = 0;
			length_ = length;
		}
		static constexpr uint32_t MaxStaticBufferSize = 16;
		union Storage
		{
			struct
			{
				uint32_t buffer_size;
				T* variable_buffer;
			};
			T static_buffer[MaxStaticBufferSize];
		};
		Storage storage_ = {};
		uint32_t length_ = 0;
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