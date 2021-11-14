#pragma once

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

}
