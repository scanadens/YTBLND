#ifndef IBLEND_ALGORITHM_H
#define IBLEND_ALGORITHM_H

#include <list>
#include "../ModelLayer/User.h"
#include "../ModelLayer/Blend.h"

class IBlendAlgorithm {
public:
    virtual ~IBlendAlgorithm() = default;
    virtual Blend generateBlend(const std::list<User>& participants) = 0;
};

#endif
