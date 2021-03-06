//
// Created by howorang on 08.05.17.
//

#include "ChatServer.h"
#include "Ice/Ice.h"


void ChatServerI::LogIn(const UserPrx & userPrx, const ::Ice::Current & current) {
    if((std::find(userNames.begin(), userNames.end(), userPrx -> ice_getIdentity().name) != userNames.end())) {
        UserAlreadyRegistered e;
        throw e;
    }
    userNames.push_back(userPrx -> ice_getIdentity().name);
    users.push_back(userPrx);
}

//TO DO Optimize
UserPrx ChatServerI::getUserByName(const ::std::string & name, const ::Ice::Current & current) {
    std::vector<UserPrx>::iterator it;
    for (it = users.begin();it < users.end(); it++) {
        UserPrx userPrx = userPrx.checkedCast(*it);
        if (userPrx ->getName() == name) {
            return userPrx;
        }
    }
    return NULL;
}

Groups ChatServerI::GroupList(const ::Ice::Current &) {
    return groups;
}

GroupServerPrx ChatServerI::getGroupServerByName(const ::std::string & name, const ::Ice::Current &) {
    Groups::iterator it;
    for (it = groups.begin();it < groups.end(); it++) {
        GroupServerPrx groupServerPrx = groupServerPrx.checkedCast(*it);
        if (groupServerPrx ->ice_getIdentity().name == name) {
            return groupServerPrx;
        }
    }
    NameDoesNotExist e;
    throw e;
}

void ChatServerI::CreateGroup(const ::std::string & name, const ::Ice::Current &) {
    if (groupExists(name)) {
        NameAlreadyExists e;
        throw e;
    }

    GroupServerManagerPrx groupServerManagerPrx = getLeastLoadedServerManager();
    GroupServerPrx groupServerPrx = groupServerManagerPrx -> CreateGroup(name);
    groupServersManagers[groupServerManagerPrx].groupCount++;
    groupServersManagers[groupServerManagerPrx].groupNames.push_back(name);
    groups.push_back(groupServerPrx);
}

void ChatServerI::DeleteGroup(const ::std::string & name, const ::Ice::Current & current) {

    if (!groupExists(name)){
        NameDoesNotExist e;
        throw e;
    }

    GroupServerManagerPrx managerPrx = getHostingServerManager(name);
    managerPrx -> DeleteGroup(name);
    groupServersManagers[managerPrx].groupCount--;
    std::vector<std::string> groupNames = groupServersManagers[managerPrx].groupNames;
    groupServersManagers[managerPrx].groupNames.erase( std::remove( groupNames.begin(), groupNames.end(), name ), groupNames.end() );
}

void ChatServerI::registerServer(const GroupServerManagerPrx & groupServerManagerPrx, const ::Ice::Current & current) {
    if (groupServersManagers.find(groupServerManagerPrx) != groupServersManagers.end()) {
        ServerAlreadyRegistered e;
        throw e;
    }

    current.adapter ->createProxy(groupServerManagerPrx -> ice_getIdentity());



    GroupServerManagerLoad managerLoad;
    managerLoad.groupCount = 0;
    groupServersManagers[groupServerManagerPrx] = managerLoad;
}


void ChatServerI::unregisterServer(const GroupServerManagerPrx & groupServerManagerPrx, const ::Ice::Current & current) {
    if (groupServersManagers.find(groupServerManagerPrx) == groupServersManagers.end()) {
        ServerDoesNotExist e;
        throw e;
    }
    groupServersManagers.erase(groupServerManagerPrx);
}

GroupServerManagerPrx ChatServerI::getLeastLoadedServerManager() {
    std::map<GroupServerManagerPrx, GroupServerManagerLoad>::iterator it;

    std::pair<GroupServerManagerPrx, GroupServerManagerLoad> smallestEntry =
            *std::min_element(groupServersManagers.begin(), groupServersManagers.end(), comparator);
    return smallestEntry.first;


}


bool comparator(std::pair<GroupServerManagerPrx, ChatServerI::GroupServerManagerLoad> i,
                std::pair<GroupServerManagerPrx, ChatServerI::GroupServerManagerLoad> j) {
    return i.second.groupCount < j.second.groupCount;
}

GroupServerManagerPrx ChatServerI::getHostingServerManager(std::string groupName) {
    std::map<GroupServerManagerPrx, GroupServerManagerLoad>::iterator it;

    for (it = groupServersManagers.begin();it != groupServersManagers.end(); ++it) {

        std::pair<GroupServerManagerPrx, GroupServerManagerLoad> groupServerManagerEntry;
        std::vector<std::string> groupNames = groupServerManagerEntry.second.groupNames;
        std::vector<std::string>::iterator name_it;

        for (name_it = groupNames.begin(); name_it < groupNames.end(); name_it++) {
            if (*name_it == groupName) {
                GroupServerManagerPrx prx = groupServerManagerEntry.first;
                return prx;
            }
        }
    }
    return NULL;
}

bool ChatServerI::groupExists(std::string name) {
    return getHostingServerManager(name) != NULL;
}