#ifndef IBLEND_ALGORITHM_H
#define IBLEND_ALGORITHM_H

/**
 * \file IBlendAlgorithm.hpp
 * \brief Abstract interface for blend-generation algorithms.
 *
 * All blend algorithms must implement this interface so AppController
 * can swap strategies without changing any other code.
 */

#include <list>
#include "../ModelLayer/User.hpp"
#include "../ModelLayer/Blend.hpp"

/// Abstract interface for blend-generation algorithms.
class IBlendAlgorithm {
public:
    virtual ~IBlendAlgorithm() = default;

    /**
     * Generates a Blend from a collection of participants.
     * \param participants List of users whose Watch Later data is used as input.
     * \return A new Blend containing the algorithm's chosen video selection.
     */
    virtual Blend generateBlend(const std::list<User>& participants) = 0;
};

#endif
