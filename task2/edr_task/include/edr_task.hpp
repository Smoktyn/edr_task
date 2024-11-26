#pragma once
#include <time.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <tuple>
#include <algorithm>
#include <fstream>

#include <json.hpp>

struct event_time_t {
    int year, month, day, hour, minute, second;
    long long microsecond;

    void parse_time(const std::string& time) {
        std::istringstream ss(time);
        char delimiter;

        if (!(ss >> year >> delimiter >> month >> delimiter >> day) ||
            !(ss.ignore(1) >> hour >> delimiter >> minute >> delimiter >> second) ||
            !(ss.ignore(1) >> microsecond)) {
            throw std::runtime_error("Invalid time format");
        }
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << year << "-"
            << std::setw(2) << std::setfill('0') << month << "-"
            << std::setw(2) << std::setfill('0') << day << " "
            << std::setw(2) << std::setfill('0') << hour << ":"
            << std::setw(2) << std::setfill('0') << minute << ":"
            << std::setw(2) << std::setfill('0') << second << "."
            << std::setw(6) << std::setfill('0') << microsecond;
        return oss.str();
    }

    bool operator<(const event_time_t& other) const {
        return std::tie(year, month, day, hour, minute, second, microsecond) <
            std::tie(other.year, other.month, other.day, other.hour, other.minute, other.second, other.microsecond);
    }
};

struct event_t {
    event_time_t time;
    std::string event_type{};
    std::string process{};
    std::string target{};

    void parse_event(const nlohmann::json& json) {
        if (json.contains("time")) {
            time.parse_time(json["time"]);
        }

        static const std::vector<std::string> event_types = { "exec", "fork", "create", "open" };
        for (const auto& et : event_types) {
            if (json["event"].contains(et)) {
                event_type = et;
                break;
            }
        }

        if (!event_type.empty()) {
            get_process_path(json);
            parse_event_details(json);
        }
    }

    bool operator<(const event_t& other) const {
        return time < other.time;
    }

private:
    void get_process_path(const nlohmann::json& json) {
        if (json["process"].contains("executable")) {
            process = json["process"]["executable"]["path"];
        }
    }

    void parse_event_details(const nlohmann::json& json) {
        if (event_type == "exec") {
            parse_exec_target(json);
        }
        else if (event_type == "fork") {
            parse_fork_target(json);
        }
        else if (event_type == "create") {
            parse_create_target(json);
        }
        else if (event_type == "open") {
            parse_open_target(json);
        }
    }

    void parse_exec_target(const nlohmann::json& json) {
        if (json["event"]["exec"].contains("target")) {
            target = json["event"]["exec"]["target"]["executable"]["path"];
        }
    }

    void parse_fork_target(const nlohmann::json& json) {
        if (json["event"]["fork"].contains("child")) {
            target = json["event"]["fork"]["child"]["executable"]["path"];
        }
    }

    void parse_open_target(const nlohmann::json& json) {
        if (json["event"]["open"].contains("file")) {
            target = json["event"]["open"]["file"]["path"];
        }
    }

    void parse_create_target(const nlohmann::json& json) {
        if (json["event"]["create"].contains("destination")) {
            target = json["event"]["create"]["destination"]["existing_file"]["path"];
        }
    }
};

class CParser {
public:
    void parse(std::ifstream& fs) {
        std::string line;
        while (std::getline(fs, line)) {
            try {
                nlohmann::json parsedJson = nlohmann::json::parse(line);
                event_t tmp;
                tmp.parse_event(parsedJson);
                events.push_back(tmp);
            }
            catch (const std::exception& e) {
                std::cerr << "Error parsing line: " << e.what() << std::endl;
            }
        }
        std::sort(events.begin(), events.end());

        auto grouped_events = group_by_process();
        write_grouped_events(grouped_events);
    }

private:
    std::map<std::string, std::vector<event_t>> group_by_process() {
    std::map<std::string, std::vector<event_t>> groupedEvents;
        for (const auto& e : events) {
            groupedEvents[e.process].push_back(e);
        }
        return groupedEvents;
    }

    void write_grouped_events(std::map<std::string, std::vector<event_t>>& grouped_events) {
        int choice{ 0 };
        std::cout << "[1] Print to console\n[2] Save to file\n";
        std::cin >> choice;

        switch (choice) {
        case 1:
            print_grouped_events(grouped_events);
            break;
        case 2:
            std::string filename;
            std::cout << "Eneter filename" << std::endl;;
            std::cin >> filename;
            filename += ".txt";

            save_events_to_file(filename, grouped_events);
            break;
        }
    }

    std::string center(const std::string& s, int width) {
        int len = s.length();
        if (len >= width) return s;
        int leftPadding = (width - len) / 2;
        int rightPadding = width - len - leftPadding;
        return std::string(leftPadding, ' ') + s + std::string(rightPadding, ' ');
    }

    void print_grouped_events(const std::map<std::string, std::vector<event_t>>& grouped_events) {
        const int timeWidth = 30;
        const int processWidth = 40;
        const int eventWidth = 12;
        const int targetWidth = 50;

        for (const auto& [process, _events] : grouped_events) {
            std::cout << std::left
                << std::setw(timeWidth) << "Time"
                << std::setw(processWidth) << "Process"
                << std::setw(eventWidth) << "Event"
                << std::setw(targetWidth) << "Target" << "\n";
            std::cout << std::string(timeWidth + processWidth + eventWidth + targetWidth, '-') << "\n";

            for (const auto& e : _events) {
                std::cout << std::left
                    << std::setw(timeWidth) << e.time.to_string()
                    << std::setw(processWidth) << e.process
                    << std::setw(eventWidth) << e.event_type
                    << std::setw(targetWidth) << e.target << "\n";
            }
            std::cout << "\n\n";
        }
    }

    void save_events_to_file(const std::string& filename, const std::map<std::string, std::vector<event_t>>& grouped_events) const {
        const int timeWidth = 35;
        const int processWidth = 60;
        const int eventWidth = 10;
        const int targetWidth = 190;
        const char separator = '|';

        auto center = [](const std::string& s, int width) {
            int len = s.length();
            if (len >= width) return s;
            int leftPadding = (width - len) / 2;
            int rightPadding = width - len - leftPadding;
            return std::string(leftPadding, ' ') + s + std::string(rightPadding, ' ');
            };

        std::ofstream ofs(filename);
        if (!ofs) {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }

        for (const auto& [process, _events] : grouped_events) {
            ofs << std::left
                << std::setw(timeWidth) << "Time"
                << std::setw(processWidth) << "Process"
                << std::setw(eventWidth) << "Event"
                << std::setw(targetWidth) << "Target" << "\n";
            ofs << std::string(timeWidth + processWidth + eventWidth + targetWidth, '-') << "\n";

            for (const auto& e : _events) {
                ofs << std::left
                    << std::setw(timeWidth) << e.time.to_string()
                    << std::setw(processWidth) << e.process
                    << std::setw(eventWidth) << e.event_type
                    << std::setw(targetWidth) << e.target << "\n";
            }
            ofs << "\n\n";
        }
    }

private:
    std::vector<event_t> events;
};