#include "YtcError.hpp"

#include <memory>
namespace Ytc
{
    template<typename T>
    using Ref = std::shared_ptr<T>;


    template<typename T, typename...Args>
    inline Ref<T> MakeRef(Args&&...args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

}