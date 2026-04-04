#ifndef IBLEND_ALGORITHM_H
#define IBLEND_ALGORITHM_H

/**
 * \file IBlendAlgorithm.hpp
 * \author Jasmine Kumar
 * \brief Abstract interface for blend-generation algorithms.
 *
 * All blend algorithms must implement this interface so AppController
 * can swap strategies without changing any other code.
 */

#include <list>
#include <string>
#include "../ModelLayer/User.hpp"
#include "../ModelLayer/Blend.hpp"

/** Abstract interface for blend-generation algorithms. */
/**
 * \class IBlendAlgorithm
 * \brief IBlendAlgorithm class declaration.
 */
class IBlendAlgorithm {
public:
    virtual ~IBlendAlgorithm() = default;

    /**
     * Generates a Blend from a collection of participants.
     * \param participants List of users whose Watch Later data is used as input.
     * \param title        User-facing name for the blend (default: empty).
     * \return A new Blend containing the algorithm's chosen video selection.
     */
    virtual Blend generateBlend(const std::list<User>& participants,
                                const std::string& title = "") = 0;
};

#endif
