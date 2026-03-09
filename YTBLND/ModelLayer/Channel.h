#ifndef CHANNEL_H
#define CHANNEL_H

#include <string>
#include<list>

class Channel {
    private:
        std::string channelID;
        std::string displayName;
        std::list<std::string> categories;

    public:
       Channel(const std::string& channelID,
                const std::string& displayName,
                const std::list<std::string>& categories);

        // Getters 
        std::string getChannelID()    const;
        std::string getDisplayName()  const;
        std::list<std::string> getCategories() const;
};
#endif