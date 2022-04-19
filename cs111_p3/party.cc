#include <condition_variable>
#include <mutex>
#include <map>
#include <deque>
#include <iostream>

// The total number of Zodiac signs
static const int NUM_SIGNS = 12;

// Represents one party, which is capable of matching guests according
// to their zodiac signs.
class Party {
public:
    Party();
    std::string meet(std::string &my_name, int my_sign, int other_sign);

    class NameAndCV {
        public:
        NameAndCV(std::string name):name(name) {
        }
        std::string name;
        std::condition_variable curCV;
    };


    std::deque<NameAndCV *> nameAndCVArray[NUM_SIGNS][NUM_SIGNS];

    std::string targetSTarget;

private:
    // Synchronizes access to this structure.
    std::mutex mutex;
};

Party::Party()
    : mutex()
{    
    
}

std::string Party::meet(std::string &my_name, int my_sign, int other_sign)
{
    std::unique_lock lock(mutex);
    std::deque<NameAndCV *> &targetDeque = nameAndCVArray[other_sign][my_sign];
    std::deque<NameAndCV *> &shoudlStayDeque = nameAndCVArray[my_sign][other_sign];

    if (targetDeque.empty()) {
        NameAndCV newNameAndCV(my_name);

        shoudlStayDeque.push_back(&newNameAndCV);
        newNameAndCV.curCV.wait(lock);
        return targetSTarget;
    } else {
        NameAndCV *curNameAndCV = targetDeque.front();
        targetDeque.pop_front();
        targetSTarget = my_name;
        curNameAndCV->curCV.notify_one();
        return curNameAndCV->name;
    }
    return NULL;
}
