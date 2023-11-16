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
#include <iomanip>
#include <queue>

using namespace std::chrono;

struct Person {
  std::string name;
  std::string surname;
  std::string fatherland;
};

struct Event {
  std::chrono::system_clock::time_point created; 
  std::chrono::system_clock::time_point expires;
  std::string description;
};

struct EventComparator {
  bool operator() (const Event& e1, const Event& e2) const {
    return e1.expires < e2.expires;
  }
};

struct Birthday {
  std::chrono::system_clock::time_point date; 
  Person full_name; 
  int age;

  bool operator<(const Birthday &other) const {
    return date < other.date;
  }
};

class Diary {
private:
  std::multiset<Event, EventComparator> events;
  std::set<Birthday> birthdays;
  std::thread event_worker;
  std::atomic<bool> working;
  std::mutex mtx_events;
  std::mutex mtx_birthdays;

void EventWorker() {
    while (working) {
    std::this_thread::sleep_for(std::chrono::seconds(60));
      {
        std::lock_guard<std::mutex> lg(mtx_events);
        auto now = std::chrono::system_clock::now();
        for(auto it = events.begin(); it != events.end(); ){
          if(now >= it->expires) {
            it = events.erase(it);
          } else {
            ++it;
          }
        }
    }
    // Add mechanism for birthdays
   {
  std::lock_guard<std::mutex> lg(mtx_birthdays);
  auto today = std::chrono::system_clock::now();
  for(auto it = birthdays.begin(); it != birthdays.end(); ){
    std::time_t birthday_time_t = std::chrono::system_clock::to_time_t(it->date);
    std::tm birthday_tm = *std::localtime(&birthday_time_t);
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(today);
    std::tm now_tm = *std::localtime(&now_time_t);

    // Check if today is a birthday
    if (now_tm.tm_mday == birthday_tm.tm_mday && now_tm.tm_mon == birthday_tm.tm_mon) {
      std::cout << "Birthday Alert!! " << it->full_name.name << " is becoming " << it->age+1 << " today.\n";
      Birthday updated_birthday = *it;
      updated_birthday.age++; // increment age
      it = birthdays.erase(it); // remove the old value
      birthdays.insert(updated_birthday); // add the updated one
    } else {
      ++it;
    }
  }
 }
    }
 }

 

public:
  Diary() : working(true), event_worker(&Diary::EventWorker, this) {}

  /**
  * The destructor stops the worker thread.
  */
  ~Diary() {
    working = false;
    event_worker.join();
  }

  /**
  * Adds an event to the diary.
  * @param expires Expiration time of the event.
  * @param description Description of the event.
  * @return Whether the event was successfully added.
  */
  bool AddEvent(const std::chrono::system_clock::time_point& expires, const std::string& description) {
    Event e;
    e.created = std::chrono::system_clock::now();
    e.expires = expires;
    e.description = description;
    std::lock_guard<std::mutex> lg(mtx_events);
    events.emplace(e);
    return true;
  }

  /**
  * Adds a birthday to the diary.
  * @param date Date of birth.
  * @param full_name Full name of the person.
  * @param age The person's current age.
  * @return Whether the birthday was successfully added.
  */
  bool AddBirthday(const std::chrono::system_clock::time_point& date, const Person& full_name, int age) {
    if(full_name.name.empty() || full_name.surname.empty() || full_name.fatherland.empty() || age < 0) {
      return false;
    }
    
    Birthday b;
    b.date = date;
    b.full_name = full_name;
    b.age = age;

    std::lock_guard<std::mutex> lg(mtx_birthdays);
    return birthdays.insert(b).second; // вернет false, если такой же Birthday уже существует
  }

   std::multiset<Event, EventComparator> GetEvents() {
    std::lock_guard<std::mutex> lg(mtx_events);
    return events;
  }

  std::set<Birthday> GetBirthdays() {
    std::lock_guard<std::mutex> lg(mtx_birthdays);
    return birthdays;
  }
  
  bool SaveToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file) {
      return false;
    }

    // Function to convert time_point to string
    auto TimePointToTimeT = [](std::chrono::system_clock::time_point tp) {
        return std::chrono::system_clock::to_time_t(tp);
    };

    auto TimePointToString = [&TimePointToTimeT](std::chrono::system_clock::time_point tp) {
        auto tt = TimePointToTimeT(tp);
        auto tm = *std::localtime(&tt);
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    };
    
    // Save all events.
    for (const auto& e : GetEvents()) {
      file << "Event created at " << TimePointToString(e.created) 
           << ", expires at " << TimePointToString(e.expires)
           << ", description: " << e.description << '\n';
    }

    // Save all birthdays.
    for (const auto& b : GetBirthdays()) {
      file << "Birthday of " << b.full_name.name << " " << b.full_name.surname << " " << b.full_name.fatherland
           << ", born at " << TimePointToString(b.date)
           << ", age " << b.age << '\n';
    }

    return true;
 }

  /**
  * Runs the interactive mode allowing the user to interact with the diary.
 */
 void runInteractiveMode() {
    int choice = 0;
    
    // Keep running the interactive mode until the user selects the exit option
    while (choice != 3) {
        std::cout << "Select an option:\n"
                  << "1. Add Event\n"
                  << "2. Add Birthday\n"
                  << "3. Exit\n";
        std::cin >> choice;

        if (choice == 1) {
            // Enter event details
            std::string description;
            std::time_t expires;

            std::cout << "Enter event description: ";
            std::cin.ignore();
            std::getline(std::cin, description);
                    
            std::cout << "Enter event expiry time (in seconds from now): ";
            std::cin >> expires;
                    
            auto expires_timepoint = std::chrono::system_clock::now() + std::chrono::seconds(expires);
            this->AddEvent(expires_timepoint, description);
        } else if (choice == 2) {
            // Enter person details
            std::string name, surname, fatherland;
            std::time_t birth_date;

            std::cout << "Enter person's name: ";
            std::cin >> name;

            std::cout << "Enter person's surname: ";
            std::cin >> surname;

            std::cout << "Enter person's fatherland: ";
            std::cin >> fatherland;

            std::cout << "Enter person's birth date (in seconds from now): ";
            std::cin >> birth_date;

            Person p;
            p.name = name;
            p.surname = surname;
            p.fatherland = fatherland;

            auto birth_date_timepoint = std::chrono::system_clock::now() + std::chrono::seconds(birth_date); // if the birthday is in the future
            this->AddBirthday(birth_date_timepoint, p, 0);  // age is set to 0 assuming the person is not born yet
        }
    }
}


  void Print() {
  auto TimePointToTimeT = [](std::chrono::system_clock::time_point tp) {
    return std::chrono::system_clock::to_time_t(tp);
  };

  auto TimePointToString = [&TimePointToTimeT](std::chrono::system_clock::time_point tp) {
    auto tt = TimePointToTimeT(tp);
    auto tm = *std::localtime(&tt);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
  };


  for (const auto& e : GetEvents()) {
    std::cout << "Event created at " << TimePointToString(e.created)
      << ", expires at " << TimePointToString(e.expires)
      << ", description: " << e.description << '\n';
  }

  for (const auto& b : GetBirthdays()) {
    std::cout << "Birthday of " << b.full_name.name << " " << b.full_name.surname << " " << b.full_name.fatherland
      << ", born at " << TimePointToString(b.date)
      << ", age " << b.age << '\n';
  }
}

};

int main() {
    Diary my_diary;
    my_diary.runInteractiveMode(); /* after choice one of the three option, choice option 3 as well */
    my_diary.Print();  
    return 0;
}

