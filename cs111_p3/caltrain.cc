#include <condition_variable>
#include <mutex>
#include <iostream>

class Station {
public:
    Station();
    void load_train(int count);
    void wait_for_train();
    void seated();
    
private:
    // Synchronizes access to all information in this object.
    std::mutex mutex;

    //code added
    int passengers_waiting;
    int seatsLeft;
    int passengers_boarding;
    bool train_in_station;
    std::condition_variable cv1;
    std::condition_variable cv2;
};

Station::Station()
    : mutex()
{
    std::unique_lock lock(mutex);
    //code added
    passengers_waiting = 0;
    train_in_station = false;
    passengers_boarding = 0;
}

void Station::load_train(int available)
{
    std::unique_lock lock(mutex);
    seatsLeft = available;
    train_in_station = true;
    if (available == 0) {
        return;
    }

    for(int i = 0; i < available; i++) {
        cv2.notify_one();
    }

    while((seatsLeft > 0 && passengers_waiting > 0) || passengers_boarding > 0) {
        cv1.wait(lock);
    }
    return;
}

void Station::wait_for_train()
{
    std::unique_lock lock(mutex);
    passengers_waiting++;
    while(train_in_station == false || seatsLeft == 0) {
        cv2.wait(lock);
    }
    seatsLeft--;
    passengers_waiting--;
    passengers_boarding++;
    return;
}

void Station::seated()
{
    std::unique_lock lock(mutex);
    passengers_boarding--;
    cv1.notify_one(); 
    return;
}