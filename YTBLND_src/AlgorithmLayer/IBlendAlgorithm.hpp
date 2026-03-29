#ifndef IBLEND_ALGORITHM_H
#define IBLEND_ALGORITHM_H

#include <list>
#include "../ModelLayer/User.hpp"
#include "../ModelLayer/Blend.hpp"

class IBlendAlgorithm {
public:
    virtual ~IBlendAlgorithm() = default;
    virtual Blend generateBlend(const std::list<User>& participants) = 0;
};

#endif
