#ifndef CHANNEL_H
#define CHANNEL_H

#include <string>
#include<list>

class Channel {
    private:
        std::string channelID, displayName;
        std::list<std::string> categories;
};
#endif