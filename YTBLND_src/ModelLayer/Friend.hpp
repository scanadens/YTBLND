#ifndef FRIEND_H
#define FRIEND_H

#include <string>

class Friend {
    private:
        std::string userID;
        std::string displayName;
        std::string email;

    public:
        Friend(const std::string& userID,
               const std::string& displayName,
               const std::string& email);

        // Getters
        std::string getUserID()     const;
        std::string getDisplayName() const;
        std::string getEmail()      const;

        // Setters
        void setDisplayName(const std::string& displayName);
        void setEmail(const std::string& email);

        std::string toString() const;
};

#endif