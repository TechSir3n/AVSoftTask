#include <iostream>
#include <cstring>
#include <chrono>
#include <ctime>
#include <vector>
#include <algorithm>
#include <fstream>
#include <thread>
#include <set>


#include <chrono>
#include <string>
#include <set>
#include <fstream>
#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <algorithm>

using namespace std::chrono;

struct Person
{
  std::string name;      
  std::string surname;    
  std::string fatherland; 
};

struct Event
{
  std::chrono::system_clock::time_point created;   // time create
  std::chrono::system_clock::time_point expires;   // time finish
  std::string description; 
};

struct Birthday
{
  std::chrono::system_clock::time_point endDate;  // date finish
  Person personInfo; // info about person
  int age;

  bool operator<(const Birthday &other) const {
    // This example compares just by endDate, but you could use any criteria you want
    return endDate < other.endDate;
  }
};

class Diary final
{
private:
  std::set<Birthday> st;
  std::vector<Event> events;
  std::atomic<bool> exit_flag{ false };
  std::thread* age_updater_thread{ nullptr };
  std::mutex mtx; // to ensure thread safety on events operations 

public:
  
  Diary() {
      // using detach as requested but consider managing it via shared_ptr or similar structure
      age_updater_thread = new std::thread([&] { this->age_updater(); });
      age_updater_thread->detach();
    }
  
  ~Diary() {
      delete age_updater_thread;
  }

  void addEvent(const std::chrono::system_clock::time_point &expiryDate, const std::string &description) {
    Event newEvent;
    
    newEvent.created = std::chrono::system_clock::now();
    newEvent.expires = expiryDate;
    newEvent.description = description;

    //mutex for synchronous access to events 
    std::lock_guard<std::mutex> lock(mtx);
    events.push_back(newEvent);
  }

  bool addBirthday(const Birthday &newBirthday) {
    //mutex is not necessary here, as events within single thread (main) 
    if (st.count(newBirthday) > 0) {
      return false;
    }

    st.insert(newBirthday);
    return true;
  }

  bool saveFileEvents(const Event &event) {
    std::ofstream file("output.txt", std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open file::[saveFileEvents]";
        return false;
    }

    time_t created_time_t = std::chrono::system_clock::to_time_t(event.created);
    time_t expired_time_t = std::chrono::system_clock::to_time_t(event.expires);

    file << "Описание: " << event.description << std::endl;
    file << "Дата создания: " << std::ctime(&created_time_t);
    file << "Дата окончания: " << std::ctime(&expired_time_t);

    return true;
  }

  void age_updater() {
    while (!exit_flag) {
      auto now = std::chrono::system_clock::now();
      for (auto it : st) {
        if (std::chrono::system_clock::to_time_t(it.endDate) == std::chrono::system_clock::to_time_t(now)) {
          it.age += 1;
        }
      }
      std::this_thread::sleep_for(std::chrono::hours(24));
    }
  }

  void removeExpiredEvents() {

    auto now = std::chrono::system_clock::now(); 

    // Remove all expired events
    std::lock_guard<std::mutex> lock(mtx);
    events.erase(
        std::remove_if(events.begin(), events.end(),
                       [&](const Event& event)
                       { return event.expires < now; }),
        events.end());
  }

  void outputEvent(const Event &event) {
    time_t created_time_t = std::chrono::system_clock::to_time_t(event.created);
    time_t expired_time_t = std::chrono::system_clock::to_time_t(event.expires);
  
    std::cout << "Описание: " << event.description << std::endl;
    std::cout << "Дата создания: " << std::ctime(&created_time_t);
    std::cout << "Дата окончания: " << std::ctime(&expired_time_t);
  }

  void outputEventToday() {
    auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    auto today = std::chrono::system_clock::from_time_t(86400*(tt/86400));

    std::vector<Event> eventsToday;
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::copy_if(events.begin(), events.end(),
                         std::back_inserter(eventsToday),
                         [&](const Event& event)
                         { return event.created >= today && event.created < today + std::chrono::hours(24); });
    }

    for(auto& event : eventsToday) {
      outputEvent(event);
    }
  }

  void stopUpdatingAges() {
      exit_flag = true;
  }
};



int main() {
    // 1. Create a diary
    Diary myDiary;

    // 2. Create a new event
    auto eventExpiryDate = std::chrono::system_clock::now() + std::chrono::hours(48); // event expires in two days
    std::string eventDescription = "Meeting with friends.";

    // Adding the event to the diary
    myDiary.addEvent(eventExpiryDate, eventDescription);

    // 3. Create a birthday
    Birthday bd;
    bd.endDate = std::chrono::system_clock::now() + std::chrono::hours(8760); // next birthday in a year
    bd.personInfo = {"John", "Doe", "Sr."}; // First name, Last name and Fatherland respectively
    bd.age = 30; // let's say John is 30 years old

    // Adding the birthday to the diary
    myDiary.addBirthday(bd);

    // 4. Output the event of today
    myDiary.outputEventToday();

    // 5. Stop age updating for birthdays
    myDiary.stopUpdatingAges();

   Event ev;
ev.created = std::chrono::system_clock::now();
ev.expires = eventExpiryDate;
ev.description = eventDescription;

// Save the event to the file
myDiary.saveFileEvents(ev);

    return 0;
}
