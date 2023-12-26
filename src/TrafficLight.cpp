#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // perform queue modifications under lock
    std::unique_lock<std::mutex> uLock(_mutex);
    // wait for and receive new messages and pull them from the queue and received object is returned by the receive function 
    _cond.wait(uLock, [this] { return !_queue.empty(); });   // pass unique lock to condition variable

    // remove last message from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg;
}

// add a new message to the queue and afterwards send a notification
template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // perform queue modifications under lock
    std::lock_guard<std::mutex> uLock(_mutex);
    _queue.push_back(std::move(msg));       // add message to queue
    _cond.notify_one();                     // notify client after pushing new message to queue
}


/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // repeatedly calls the receive function on the message queue
    while(true) {
        TrafficLightPhase phase = _queue.receive();

        // if received message (TrafficLightPhase) is green, return
        if(phase == TrafficLightPhase::green)   return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // private method "cycleThroughPhases“ started in a thread (using threads queue from base class)
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // infinite loop that measures the time between two loop cycles and toggles the current phase of the traffic light between red and green and sends an update message
    // to the message queue using move semantics

    double cycleDuration = (rand()%3+4)*1000;   // random number between 4-6 converted to milliseconds
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;

    // init stop watch
    lastUpdate = std::chrono::system_clock::now();
    while(true) {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute time difference to stop watch
        long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
        if(timeSinceLastUpdate >= cycleDuration) {
            // toggle current phase of traffic light between red and green
            _currentPhase = (getCurrentPhase() == TrafficLightPhase::red) ? TrafficLightPhase::green : TrafficLightPhase::red;
            // send update message to queue
            _queue.send(std::move(_currentPhase));

            // reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();
        }
    }
}
