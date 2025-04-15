#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <curses.h>
#include <openssl/sha.h>
#include <ctime>
#include <map>
#include <set>
#include <numeric>
#include <sstream>



// Константы
const std::string DEFAULT_ADMIN_LOGIN = "admin";
const std::string DEFAULT_ADMIN_PASSWORD = "admin123";
const std::string MSG_LOGIN_EXISTS = "Логин уже существует!";
const std::string MSG_SUCCESS = "Операция выполнена успешно!";
const std::string MSG_CONFIRM_DELETE = "Вы действительно хотите удалить запись? (y/n): ";
const std::string MSG_INVALID_INPUT = "Неверный ввод!";
const std::string MSG_NOT_FOUND = "Данные не найдены!";
const int ROLE_ADMIN = 1;
const int ROLE_USER = 0;
const int ACCESS_PENDING = 0;
const int ACCESS_APPROVED = 1;
const int ACCESS_BLOCKED = 2;


// Структура для учетной записи
struct Account {
    std::string login;
    std::string salted_hash_password;
    std::string salt;
    int role;
    int access;
};

// Структура для командировки
struct Trip {
    std::string employee;
    int year;
    int month;
    int duration_days;
    std::string city;
    double daily_expense;
};

// **UtilityService** - Утилитные функции для хеширования паролей и генерации соли
class UtilityService {
public:
    static std::string generateSalt() {
        std::string salt = "salt" + std::to_string(rand() % 10000);
        return salt;
    }

    static std::string hashPassword(const std::string& password, const std::string& salt) {
        std::string input = password + salt;
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
        std::string hashed;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            char hex[3];
            sprintf(hex, "%02x", hash[i]);
            hashed += hex;
        }
        return hashed;
    }
};

// **AccountService** - Управление учетными записями и аутентификация
class AccountService {
private:
    const std::string ACCOUNTS_FILE = "accounts.txt";
    Account* currentUser = nullptr;

public:
    ~AccountService() {
        delete currentUser;
    }

    bool authenticate(const std::string& login, const std::string& password) {
        auto accounts = readAccounts();
        for (auto& acc : accounts) {
            if (acc.login == login) {
                std::string hashed = UtilityService::hashPassword(password, acc.salt);
                if (hashed == acc.salted_hash_password && acc.access == ACCESS_APPROVED) {
                    delete currentUser; // Очистка предыдущего пользователя
                    currentUser = new Account(acc);
                    return true;
                }
            }
        }
        return false;
    }

    void registerUser(const std::string& login, const std::string& password, int role, int access) {
        auto accounts = readAccounts();
        for (const auto& acc : accounts) {
            if (acc.login == login) return;
        }
        Account newAcc{login, "", UtilityService::generateSalt(), role, access};
        newAcc.salted_hash_password = UtilityService::hashPassword(password, newAcc.salt);
        accounts.push_back(newAcc);
        writeAccounts(accounts);
    }

    std::vector<Account> readAccounts() {
        std::vector<Account> accounts;
        std::ifstream file(ACCOUNTS_FILE);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                std::stringstream ss(line);
                std::vector<std::string> tokens;
                std::string token;
                while (std::getline(ss, token, ';')) {
                    tokens.push_back(token);
                }
                if (tokens.size() == 5) {
                    try {
                        accounts.push_back({
                            tokens[0],                  // login
                            tokens[1],                  // salted_hash_password
                            tokens[2],                  // salt
                            std::stoi(tokens[3]),       // role
                            std::stoi(tokens[4])        // access
                        });
                    } catch (const std::exception& e) {
                        continue; // Пропускаем некорректные строки
                    }
                }
            }
            file.close();
        }
        return accounts;
    }

    void writeAccounts(const std::vector<Account>& accounts) {
        std::ofstream file(ACCOUNTS_FILE);
        if (file.is_open()) {
            for (const auto& acc : accounts) {
                file << acc.login << ";" << acc.salted_hash_password << ";" << acc.salt << ";"
                     << acc.role << ";" << acc.access << "\n";
            }
            file.close();
        }
    }

    Account* getCurrentUser() const { return currentUser; }
    void clearCurrentUser() { delete currentUser; currentUser = nullptr; }
};

// **TripService** - Управление командировками
class TripService {
private:
    const std::string TRIPS_FILE = "trips.txt";

public:
    std::vector<Trip> readTrips() {
        std::vector<Trip> trips;
        std::ifstream file(TRIPS_FILE);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                std::stringstream ss(line);
                std::vector<std::string> tokens;
                std::string token;
                while (std::getline(ss, token, ';')) {
                    tokens.push_back(token);
                }
                if (tokens.size() == 6) {
                    try {
                        trips.push_back({
                            tokens[0],                  // employee
                            std::stoi(tokens[1]),       // year
                            std::stoi(tokens[2]),       // month
                            std::stoi(tokens[3]),       // duration_days
                            tokens[4],                  // city
                            std::stod(tokens[5])        // daily_expense
                        });
                    } catch (const std::exception& e) {
                        continue; // Пропускаем некорректные строки
                    }
                }
            }
            file.close();
        }
        return trips;
    }

    void writeTrips(const std::vector<Trip>& trips) {
        std::ofstream file(TRIPS_FILE);
        if (file.is_open()) {
            for (const auto& trip : trips) {
                file << trip.employee << ";" << trip.year << ";" << trip.month << ";"
                     << trip.duration_days << ";" << trip.city << ";" << trip.daily_expense << "\n";
            }
            file.close();
        }
    }

    void addTrip(const Trip& trip) {
        auto trips = readTrips();
        trips.push_back(trip);
        writeTrips(trips);
    }

    void editTrip(int index, const Trip& newTrip) {
        auto trips = readTrips();
        if (index >= 0 && index < trips.size()) {
            trips[index] = newTrip;
            writeTrips(trips);
        }
    }

    void deleteTrip(int index) {
        auto trips = readTrips();
        if (index >= 0 && index < trips.size()) {
            trips.erase(trips.begin() + index);
            writeTrips(trips);
        }
    }

 std::vector<Trip> searchTrips(int criterion, const std::string& query) {
    auto trips = readTrips();
    std::vector<Trip> result;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    for (const auto& trip : trips) {
        std::string field;
        if (criterion == 1) {
            field = trip.employee;
        } else if (criterion == 2) {
            field = trip.city;
        } else if (criterion == 3) {
            field = std::to_string(trip.month);
        }
        std::string lowerField = field;
        std::transform(lowerField.begin(), lowerField.end(), lowerField.begin(), ::tolower);
        if (lowerField.find(lowerQuery) != std::string::npos) {
            result.push_back(trip);
        }
    }
    return result;
}

    double calculateTotalExpenses(int year, int month) {
        auto trips = readTrips();
        double total = 0.0;
        for (const auto& trip : trips) {
            if (trip.year == year && trip.month == month) {
                total += trip.duration_days * trip.daily_expense;
            }
        }
        return total;
    }

    std::vector<std::pair<std::string, int>> getCityVisitFrequency(int start_year, int start_month, int end_year, int end_month) {
        auto trips = readTrips();
        std::map<std::string, int> cityCount;
        for (const auto& trip : trips) {
            if ((trip.year > start_year || (trip.year == start_year && trip.month >= start_month)) &&
                (trip.year < end_year || (trip.year == end_year && trip.month <= end_month))) {
                cityCount[trip.city]++;
            }
        }
        std::vector<std::pair<std::string, int>> result(cityCount.begin(), cityCount.end());
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            return a.second > b.second; // Сортировка по убыванию частоты посещений
        });
        return result;
    }
};

// **UIService** - Управление пользовательским интерфейсом
class UIService {
private:
    AccountService& accountService;
    TripService& tripService;

public:
    UIService(AccountService& accService, TripService& tripSvc)
        : accountService(accService), tripService(tripSvc) {}

    std::string inputString(int y, int x, const std::string& prompt, bool mask = false, int maxLength = -1) {
        mvprintw(y, x, prompt.c_str());
        std::string input;
        int ch;
        while ((ch = getch()) != '\n') {
            if (ch == '/' && input.empty()) return "/"; // Выход только если строка пустая
            if (ch == KEY_BACKSPACE || ch == 8) {
                if (!input.empty()) {
                    input.pop_back();
                    int pos = x + prompt.length() + input.length();
                    mvprintw(y, pos, " ");
                    move(y, pos);
                }
            } else if (ch >= 32 && ch <= 126 && (maxLength == -1 || input.length() < maxLength)) {
                input += static_cast<char>(ch);
                int pos = x + prompt.length() + input.length() - 1;
                mvprintw(y, pos, mask ? "*" : "%c", ch);
                move(y, pos + 1);
            }
            refresh();
        }
        return input;
    }

    int inputInt(int y, int x, const std::string& prompt, int maxLength = -1) {
        while (true) {
            std::string input = inputString(y, x, prompt, false, maxLength);
            if (input == "/") return -1; // Возвращаем -1 как индикатор отмены
            try {
                return std::stoi(input);
            } catch (...) {
                mvprintw(y + 1, x, "Ошибка: введите целое число!");
                refresh();
                napms(1000);
                mvprintw(y + 1, x, std::string(40, ' ').c_str());
                move(y, x + prompt.length());
                refresh();
            }
        }
    }

    double inputDouble(int y, int x, const std::string& prompt, int maxLength = -1) {
        while (true) {
            std::string input = inputString(y, x, prompt, false, maxLength);
            if (input == "/") return -1.0; // Возвращаем -1.0 как индикатор отмены
            try {
                return std::stod(input);
            } catch (...) {
                mvprintw(y + 1, x, "Ошибка: введите число!");
                refresh();
                napms(1000);
                mvprintw(y + 1, x, std::string(40, ' ').c_str());
                move(y, x + prompt.length());
                refresh();
            }
        }
    }

    void loginScreen() {
        int height, width;
        getmaxyx(stdscr, height, width);
        clear();
        mvprintw(0, 0, "=== Авторизация ===");
        std::string login = inputString(2, 0, "Логин: ", false, 20);
        if (login == "/") {
            mvprintw(height - 1, 0, "\"Вход отменен!\"");
            refresh();
            napms(1000);
            return;
        }
        std::string password = inputString(4, 0, "Пароль: ", true, 20);
        if (accountService.authenticate(login, password)) {
            mvprintw(height - 2, 0, "Вход выполнен успешно!");
        } else {
            mvprintw(height - 2, 0, "Неверный логин или пароль!");
        }
        mvprintw(height - 1, 0, "Нажмите любую клавишу...");
        refresh();
        getch();
    }

void registerScreen(bool byAdmin = false) {
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();
    mvprintw(0, 0, "=== Регистрация ===");
    std::string login = inputString(2, 0, "Логин: ", false, 20);
    if (login == "/") {
        mvprintw(height - 1, 0, "\"Регистрация отменена!\"");
        refresh();
        napms(1000);
        return;
    }
    if (login.empty()) {
        mvprintw(height - 1, 0, "Логин не может быть пустым!");
        refresh();
        napms(1000);
        return;
    }

    auto accounts = accountService.readAccounts();
    bool loginExists = false;
    for (const auto& acc : accounts) {
        if (acc.login == login) {
            loginExists = true;
            break;
        }
    }
    if (loginExists) {
        mvprintw(height - 1, 0, "Логин уже занят!");
        refresh();
        napms(1000);
        return;
    }

    std::string password = inputString(4, 0, "Пароль: ", true, 20);
    if (password == "/") {
        mvprintw(height - 1, 0, "\"Регистрация отменена!\"");
        refresh();
        napms(1000);
        return;
    }
    int role = byAdmin ? ROLE_USER : ROLE_USER;
    int access = byAdmin ? ACCESS_APPROVED : ACCESS_PENDING;
    accountService.registerUser(login, password, role, access);
    mvprintw(height - 2, 0, byAdmin ? MSG_SUCCESS.c_str() : "Ваш запрос отправлен администратору.");
    mvprintw(height - 1, 0, "Нажмите любую клавишу...");
    refresh();
    getch();
}



void viewTripsScreen() {
    auto trips = tripService.readTrips();
    int height, width;
    getmaxyx(stdscr, height, width);
    if (trips.empty()) {
        clear();
        mvprintw(0, 0, "=== Просмотр командировок ===");
        mvprintw(2, 0, "\"Командировки отсутствуют!\"");
        mvprintw(height - 1, 0, "Нажмите любую клавишу...");
        refresh();
        getch();
        return;
    }

    int dataRows = height - 5, currentPage = 0, sortColumn = -1;
    bool ascending = true;
    while (true) {
        clear();
        mvprintw(0, 0, "=== Просмотр командировок ===");
        int totalPages = (trips.size() + dataRows - 1) / dataRows;
        int startIdx = currentPage * dataRows;
        int endIdx = std::min(startIdx + dataRows, static_cast<int>(trips.size()));
        
        int colWidths[] = {4, 30, 4, 2, 2, 20, 9}; // №, Ф.И.О., Год, Мс, Дн, Город, Расх/день
        int totalWidth = std::accumulate(colWidths, colWidths + 7, 0) + 6; // +6 для разделителей "|"
        if (totalWidth > width) {
            // Уменьшаем только столбец "Ф.И.О.", сохраняя остальные
            colWidths[1] = width - (colWidths[0] + colWidths[2] + colWidths[3] + colWidths[4] + colWidths[5] + colWidths[6] + 6);
            if (colWidths[1] < 10) colWidths[1] = 10; // Минимальная ширина для читаемости
        }
        
        std::string header = "№   |Ф.И.О.                        |Год |Мс|Дн|Город               |Расх/день";
        mvprintw(2, 0, header.substr(0, width).c_str());
        mvprintw(3, 0, std::string(std::min(totalWidth, width), '-').c_str());

        int y = 4;
        for (int i = startIdx; i < endIdx && y < height - 1; ++i) {
            Trip& t = trips[i];
            std::string num = std::to_string(i + 1) + std::string(colWidths[0] - std::to_string(i + 1).length(), ' ');
            std::string emp = t.employee + std::string(std::max(0, static_cast<int>(colWidths[1]) - static_cast<int>(t.employee.length())), ' ');

            std::string year = std::to_string(t.year) + std::string(colWidths[2] - std::to_string(t.year).length(), ' ');
            std::string month = std::to_string(t.month) + std::string(colWidths[3] - std::to_string(t.month).length(), ' ');
            std::string dur = std::to_string(t.duration_days) + std::string(colWidths[4] - std::to_string(t.duration_days).length(), ' ');

            std::string city = t.city + std::string(std::max(0, static_cast<int>(colWidths[5]) - static_cast<int>(t.city.length())), ' ');

            std::string exp = std::to_string(t.daily_expense) + std::string(std::max(0, colWidths[6] - static_cast<int>(std::to_string(t.daily_expense).length())), ' ');
            exp = exp.substr(0, colWidths[6]);
            
            std::string line = num + "|" + emp + "|" + year + "|" + month + "|" + dur + "|" + city + "|" + exp;
            mvprintw(y++, 0, line.substr(0, width).c_str());
        }

        mvprintw(height - 1, 0, ("Стрелки влево/вправо - страницы | 1-6 - сортировка | Страница " + 
                                 std::to_string(currentPage + 1) + " из " + std::to_string(totalPages)).c_str());
        refresh();

        int ch = getch();
        if (ch == '/') break;
        else if (ch == KEY_RIGHT && currentPage < totalPages - 1) currentPage++;
        else if (ch == KEY_LEFT && currentPage > 0) currentPage--;
        else if (ch >= '1' && ch <= '6') {
            int newSort = ch - '0';
            if (newSort == sortColumn) ascending = !ascending;
            else { sortColumn = newSort; ascending = true; }
            std::sort(trips.begin(), trips.end(), [&](const Trip& a, const Trip& b) {
                switch (sortColumn) {
                    case 1: return ascending ? a.employee < b.employee : a.employee > b.employee;
                    case 2: return ascending ? a.year < b.year : a.year > b.year;
                    case 3: return ascending ? a.month < b.month : a.month > b.month;
                    case 4: return ascending ? a.duration_days < b.duration_days : a.duration_days > b.duration_days;
                    case 5: return ascending ? a.city < b.city : a.city > b.city;
                    case 6: return ascending ? a.daily_expense < b.daily_expense : a.daily_expense > b.daily_expense;
                    default: return false;
                }
            });
            currentPage = std::min(currentPage, static_cast<int>((trips.size() + dataRows - 1) / dataRows - 1));
        }
    }
}



    void addTripScreen() {
        int height, width;
        getmaxyx(stdscr, height, width);
        clear();
        mvprintw(0, 0, "=== Добавление командировки ===");
        Trip trip;
        trip.employee = inputString(2, 0, "Ф.И.О. сотрудника (макс. 30): ", false, 30);
        if (trip.employee == "/" || trip.employee.empty()) {
            mvprintw(height - 1, 0, "Добавление отменено!");
            refresh();
            napms(1000);
            return;
        }
        trip.year = inputInt(4, 0, "Год: ", 4);
        if (trip.year == -1) {
            mvprintw(height - 1, 0, "Добавление отменено!");
            refresh();
            napms(1000);
            return;
        }
        trip.month = inputInt(6, 0, "Месяц (1-12): ", 2);
        if (trip.month == -1) {
            mvprintw(height - 1, 0, "Добавление отменено!");
            refresh();
            napms(1000);
            return;
        }
        if (trip.month < 1 || trip.month > 12) {
            mvprintw(8, 0, "Неверный месяц!");
            refresh();
            napms(1000);
            return;
        }
        trip.duration_days = inputInt(8, 0, "Длительность командировки в днях: ", 2);
        if (trip.duration_days == -1) {
            mvprintw(height - 1, 0, "Добавление отменено!");
            refresh();
            napms(1000);
            return;
        }
        if (trip.duration_days <= 0) {
            mvprintw(10, 0, "Длительность должна быть больше 0!");
            refresh();
            napms(1000);
            return;
        }
        trip.city = inputString(10, 0, "Город (макс. 20): ", false, 20);
        if (trip.city == "/" || trip.city.empty()) {
            mvprintw(height - 1, 0, "Добавление отменено!");
            refresh();
            napms(1000);
            return;
        }
        trip.daily_expense = inputDouble(12, 0, "Сумма расходов на день: ", 9);
        if (trip.daily_expense == -1.0) {
            mvprintw(height - 1, 0, "Добавление отменено!");
            refresh();
            napms(1000);
            return;
        }
        if (trip.daily_expense <= 0) {
            mvprintw(14, 0, "Сумма должна быть больше 0!");
            refresh();
            napms(1000);
            return;
        }
        tripService.addTrip(trip);
        mvprintw(height - 1, 0, MSG_SUCCESS.c_str());
        refresh();
        napms(1000);
    }




void editTripScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    auto trips = tripService.readTrips();

    if (trips.empty()) {
        clear();
        mvprintw(0, 0, "=== Редактирование командировки ===");
        mvprintw(2, 0, "Командировки отсутствуют!");
        mvprintw(height - 1, 0, "Нажмите любую клавишу...");
        refresh();
        getch();
        return;
    }

    int dataRows = height - 8, currentPage = 0;
    while (true) {
        clear();
        mvprintw(0, 0, "=== Редактирование командировки ===");
        int totalPages = (trips.size() + dataRows - 1) / dataRows;
        int startIdx = currentPage * dataRows;
        int endIdx = std::min(startIdx + dataRows, static_cast<int>(trips.size()));

        int colWidths[] = {4, 30, 4, 2, 2, 12, 9};
        int totalWidth = std::accumulate(colWidths, colWidths + 7, 0) + 6;
        if (totalWidth > width) colWidths[1] = width - (totalWidth - colWidths[1]);

        std::string header = "№   |Ф.И.О.                          |Год |Мс|Дн|Город          |Расх/день";
        mvprintw(2, 0, header.substr(0, width).c_str());
        mvprintw(3, 0, std::string(totalWidth, '-').c_str());

        int y = 4;
        for (int i = startIdx; i < endIdx && y < height - 4; ++i) {
            Trip& t = trips[i];
            std::string num = std::to_string(i + 1) + std::string(colWidths[0] - std::to_string(i + 1).length(), ' ');
            std::string emp = t.employee.substr(0, colWidths[1]) + std::string(colWidths[1] - std::min(t.employee.length(), static_cast<size_t>(colWidths[1])), ' ');
            std::string year = std::to_string(t.year) + std::string(colWidths[2] - std::to_string(t.year).length(), ' ');
            std::string month = std::to_string(t.month) + std::string(colWidths[3] - std::to_string(t.month).length(), ' ');
            std::string dur = std::to_string(t.duration_days) + std::string(colWidths[4] - std::to_string(t.duration_days).length(), ' ');
            std::string city = t.city.substr(0, colWidths[5]) + std::string(colWidths[5] - std::min(t.city.length(), static_cast<size_t>(colWidths[5])), ' ');
            std::string exp = std::to_string(t.daily_expense).substr(0, colWidths[6]) + std::string(colWidths[6] - std::min(std::to_string(t.daily_expense).length(), static_cast<size_t>(colWidths[6])), ' ');
            
            std::string line = num + "|" + emp + "|" + year + "|" + month + "|" + dur + "|" + city + "|" + exp;
            mvprintw(y++, 0, line.substr(0, width).c_str());
        }

        mvprintw(height - 3, 0, ("Стрелки влево/вправо - страницы | 'e' - редактировать командировку | Страница " +
                                 std::to_string(currentPage + 1) + " из " + std::to_string(totalPages)).c_str());
        mvprintw(height - 2, 0, "Нажмите '/' для выхода");
        mvprintw(height - 1, 0, " ");
        refresh();

        int ch = getch();
        if (ch == '/') {
            refresh();
            return;
        } else if (ch == KEY_RIGHT && currentPage < totalPages - 1) {
            currentPage++;
        } else if (ch == KEY_LEFT && currentPage > 0) {
            currentPage--;
        } else if (ch == 'e') {
            std::string indexStr = inputString(height - 4, 0, "Введите номер командировки: ");
            if (indexStr == "/") {
                mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
                refresh();
                napms(1000);
                return;
            }
            int index;
            try {
                index = std::stoi(indexStr) - 1;
            } catch (...) {
                mvprintw(height - 1, 0, "\"Ошибка: введите корректный номер!\"");
                refresh();
                napms(1000);
                return;
            }

            if (index >= 0 && index < trips.size()) {
                Trip& trip = trips[index];
                clear();
                mvprintw(0, 0, "=== Редактирование командировки ===");
                mvprintw(2, 0, "Текущие данные командировки:");
                std::string currentData = std::to_string(index + 1) + " | " + trip.employee + " | " + std::to_string(trip.year) + " | " + std::to_string(trip.month) + " | " +
                                          std::to_string(trip.duration_days) + " | " + trip.city + " | " + std::to_string(trip.daily_expense);
                mvprintw(4, 0, currentData.substr(0, width).c_str());
                mvprintw(6, 0, "Введите новые данные (Enter для сохранения текущего значения):");

                std::string newEmployee = inputString(8, 0, "Новое Ф.И.О. (макс. 30): ", false, 30);
                if (newEmployee == "/") {
                    mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
                    refresh();
                    napms(1000);
                    return;
                }
                trip.employee = newEmployee.empty() ? trip.employee : newEmployee;

                std::string yearStr = inputString(9, 0, "Новый год: ", false, 4);
                if (yearStr == "/") {
                    mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
                    refresh();
                    napms(1000);
                    return;
                }
                if (!yearStr.empty()) {
                    try {
                        trip.year = std::stoi(yearStr);
                    } catch (...) {
                        mvprintw(height - 1, 0, "\"Ошибка: год оставлен без изменений!\"");
                        refresh();
                        napms(1000);
                    }
                }

                std::string monthStr = inputString(10, 0, "Новый месяц (1-12): ", false, 2);
                if (monthStr == "/") {
                    mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
                    refresh();
                    napms(1000);
                    return;
                }
                if (!monthStr.empty()) {
                    try {
                        int newMonth = std::stoi(monthStr);
                        if (newMonth >= 1 && newMonth <= 12) {
                            trip.month = newMonth;
                        } else {
                            mvprintw(height - 1, 0, "\"Ошибка: месяц оставлен без изменений!\"");
                            refresh();
                            napms(1000);
                        }
                    } catch (...) {
                        mvprintw(height - 1, 0, "\"Ошибка: месяц оставлен без изменений!\"");
                        refresh();
                        napms(1000);
                    }
                }

                std::string durationStr = inputString(11, 0, "Новая длительность (дни): ", false, 2);
                if (durationStr == "/") {
                    mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
                    refresh();
                    napms(1000);
                    return;
                }
                if (!durationStr.empty()) {
                    try {
                        trip.duration_days = std::stoi(durationStr);
                    } catch (...) {
                        mvprintw(height - 1, 0, "\"Ошибка: длительность оставлена без изменений!\"");
                        refresh();
                        napms(1000);
                    }
                }

                std::string cityStr = inputString(12, 0, "Новый город (макс. 12): ", false, 12);
                if (cityStr == "/") {
                    mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
                    refresh();
                    napms(1000);
                    return;
                }
                if (!cityStr.empty()) {
                    trip.city = cityStr;
                }

                std::string expenseStr = inputString(13, 0, "Новая сумма расходов на день: ", false, 9);
                if (expenseStr == "/") {
                    mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
                    refresh();
                    napms(1000);
                    return;
                }
                if (!expenseStr.empty()) {
                    try {
                        trip.daily_expense = std::stod(expenseStr);
                    } catch (...) {
                        mvprintw(height - 1, 0, "\"Ошибка: сумма расходов оставлена без изменений!\"");
                        refresh();
                        napms(1000);
                    }
                }

                tripService.editTrip(index, trip);
                mvprintw(height - 1, 0, MSG_SUCCESS.c_str());
                refresh();
                napms(1000);
                return;
            } else {
                mvprintw(height - 1, 0, MSG_NOT_FOUND.c_str());
                refresh();
                napms(1000);
            }
        }
    }
}



    // void deleteTripScreen() {
    //     int height, width;
    //     getmaxyx(stdscr, height, width);
    //     clear();
    //     mvprintw(0, 0, "=== Удаление командировки ===");
    //     std::string indexStr = inputString(2, 0, "Введите номер командировки: ");
    //     if (indexStr == "/") return;
    //     int index;
    //     try {
    //         index = std::stoi(indexStr) - 1;
    //     } catch (...) {
    //         mvprintw(height - 1, 0, "Ошибка: введите корректный номер!");
    //         refresh();
    //         napms(1000);
    //         return;
    //     }
    //     auto trips = tripService.readTrips();
    //     if (index >= 0 && index < trips.size()) {
    //         mvprintw(4, 0, MSG_CONFIRM_DELETE.c_str());
    //         refresh();
    //         if (getch() == 'y') {
    //             tripService.deleteTrip(index);
    //             mvprintw(height - 1, 0, MSG_SUCCESS.c_str());
    //         }
    //     } else {
    //         mvprintw(height - 1, 0, MSG_NOT_FOUND.c_str());
    //     }
    //     refresh();
    //     napms(1000);
    // }

void deleteTripScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    auto trips = tripService.readTrips();

    if (trips.empty()) {
        clear();
        mvprintw(0, 0, "=== Удаление командировки ===");
        mvprintw(2, 0, "Командировки отсутствуют!");
        mvprintw(height - 1, 0, "Нажмите любую клавишу...");
        refresh();
        getch();
        return;
    }

    int dataRows = height - 8, currentPage = 0;
    while (true) {
        clear();
        mvprintw(0, 0, "=== Удаление командировки ===");
        int totalPages = (trips.size() + dataRows - 1) / dataRows;
        int startIdx = currentPage * dataRows;
        int endIdx = std::min(startIdx + dataRows, static_cast<int>(trips.size()));

        int colWidths[] = {4, 30, 4, 2, 2, 12, 9};
        int totalWidth = std::accumulate(colWidths, colWidths + 7, 0) + 6;
        if (totalWidth > width) colWidths[1] = width - (totalWidth - colWidths[1]);

        std::string header = "№   |Ф.И.О.                          |Год |Мс|Дн|Город          |Расх/день";
        mvprintw(2, 0, header.substr(0, width).c_str());
        mvprintw(3, 0, std::string(totalWidth, '-').c_str());

        int y = 4;
        for (int i = startIdx; i < endIdx && y < height - 4; ++i) {
            Trip& t = trips[i];
            std::string num = std::to_string(i + 1) + std::string(colWidths[0] - std::to_string(i + 1).length(), ' ');
            std::string emp = t.employee.substr(0, colWidths[1]) + std::string(colWidths[1] - std::min(t.employee.length(), static_cast<size_t>(colWidths[1])), ' ');
            std::string year = std::to_string(t.year) + std::string(colWidths[2] - std::to_string(t.year).length(), ' ');
            std::string month = std::to_string(t.month) + std::string(colWidths[3] - std::to_string(t.month).length(), ' ');
            std::string dur = std::to_string(t.duration_days) + std::string(colWidths[4] - std::to_string(t.duration_days).length(), ' ');
            std::string city = t.city.substr(0, colWidths[5]) + std::string(colWidths[5] - std::min(t.city.length(), static_cast<size_t>(colWidths[5])), ' ');
            std::string exp = std::to_string(t.daily_expense).substr(0, colWidths[6]) + std::string(colWidths[6] - std::min(std::to_string(t.daily_expense).length(), static_cast<size_t>(colWidths[6])), ' ');
            
            std::string line = num + "|" + emp + "|" + year + "|" + month + "|" + dur + "|" + city + "|" + exp;
            mvprintw(y++, 0, line.substr(0, width).c_str());
        }

        mvprintw(height - 3, 0, ("Стрелки влево/вправо - страницы | 'd' - удалить командировку | Страница " +
                                 std::to_string(currentPage + 1) + " из " + std::to_string(totalPages)).c_str());
        mvprintw(height - 2, 0, "Нажмите '/' для выхода");
        mvprintw(height - 1, 0, " ");
        refresh();

        int ch = getch();
        if (ch == '/') {
            refresh();
            return;
        } else if (ch == KEY_RIGHT && currentPage < totalPages - 1) {
            currentPage++;
        } else if (ch == KEY_LEFT && currentPage > 0) {
            currentPage--;
        } else if (ch == 'd') {
            std::string indexStr = inputString(height - 4, 0, "Введите номер командировки: ");
            if (indexStr == "/") {
                mvprintw(height - 4, 0, "Удаление отменено!                                  ");
                refresh();
                napms(1000);
                return;
            }
            int index;
            try {
                index = std::stoi(indexStr) - 1;
            } catch (...) {
                mvprintw(height - 1, 0, "\"Ошибка: введите корректный номер!\"");
                refresh();
                napms(1000);
                return;
            }

            if (index >= 0 && index < trips.size()) {
                clear();
                mvprintw(0, 0, "=== Удаление командировки ===");
                mvprintw(2, 0, "Данные удаляемой командировки:");
                
                int miniColWidths[] = {4, 30, 4, 2, 2, 12, 9};
                int miniTotalWidth = std::accumulate(miniColWidths, miniColWidths + 7, 0) + 6;
                if (miniTotalWidth > width) miniColWidths[1] = width - (miniTotalWidth - miniColWidths[1]);

                std::string miniHeader = "№   |Ф.И.О.                          |Год |Мс|Дн|Город          |Расх/день";
                mvprintw(4, 0, miniHeader.substr(0, width).c_str());
                mvprintw(5, 0, std::string(miniTotalWidth, '-').c_str());

                Trip& t = trips[index];
                std::string num = std::to_string(index + 1) + std::string(miniColWidths[0] - std::to_string(index + 1).length(), ' ');
                std::string emp = t.employee.substr(0, miniColWidths[1]) + std::string(miniColWidths[1] - std::min(t.employee.length(), static_cast<size_t>(miniColWidths[1])), ' ');
                std::string year = std::to_string(t.year) + std::string(miniColWidths[2] - std::to_string(t.year).length(), ' ');
                std::string month = std::to_string(t.month) + std::string(miniColWidths[3] - std::to_string(t.month).length(), ' ');
                std::string dur = std::to_string(t.duration_days) + std::string(miniColWidths[4] - std::to_string(t.duration_days).length(), ' ');
                std::string city = t.city.substr(0, miniColWidths[5]) + std::string(miniColWidths[5] - std::min(t.city.length(), static_cast<size_t>(miniColWidths[5])), ' ');
                std::string exp = std::to_string(t.daily_expense).substr(0, miniColWidths[6]) + std::string(miniColWidths[6] - std::min(std::to_string(t.daily_expense).length(), static_cast<size_t>(miniColWidths[6])), ' ');
                
                std::string line = num + "|" + emp + "|" + year + "|" + month + "|" + dur + "|" + city + "|" + exp;
                mvprintw(6, 0, line.substr(0, width).c_str());

                mvprintw(height - 2, 0, MSG_CONFIRM_DELETE.c_str());
                mvprintw(height - 1, 0, "Нажмите 'y' для подтверждения, любую другую клавишу для отмены");
                refresh();

                int confirm = getch();
                if (confirm == 'y') {
                    tripService.deleteTrip(index);
                    mvprintw(height - 3, 0, MSG_SUCCESS.c_str());
                    refresh();
                    napms(1000);
                    return;
                } else {
                    mvprintw(height - 3, 0, "Удаление отменено!");
                    refresh();
                    napms(1000);
                    return;
                }
            } else {
                mvprintw(height - 1, 0, MSG_NOT_FOUND.c_str());
                refresh();
                napms(1000);
            }
        }
    }
}


    void searchTripsScreen() {
        int height, width;
        getmaxyx(stdscr, height, width);
        clear();
        mvprintw(0, 0, "=== Поиск командировок ===");
        mvprintw(2, 0, "1. По Ф.И.О. сотрудника | 2. По городу | 3. По месяцу");
        refresh();
        int ch = getch();
        if (ch == '/' || ch < '1' || ch > '3') return;
        int criterion = ch - '0';
        std::string query = inputString(4, 0, "Введите запрос: ");
        if (query == "/") return;

        auto trips = tripService.searchTrips(criterion, query);
        if (trips.empty()) {
            mvprintw(6, 0, MSG_NOT_FOUND.c_str());
            mvprintw(height - 1, 0, "Нажмите любую клавишу...");
            refresh();
            getch();
            return;
        }

        int dataRows = height - 5, currentPage = 0;
        while (true) {
            clear();
            mvprintw(0, 0, "=== Поиск командировок ===");
            int totalPages = (trips.size() + dataRows - 1) / dataRows;
            int startIdx = currentPage * dataRows;
            int endIdx = std::min(startIdx + dataRows, static_cast<int>(trips.size()));
            
            int colWidths[] = {4, 30, 4, 2, 2, 12, 9};
            int totalWidth = std::accumulate(colWidths, colWidths + 7, 0) + 6;
            if (totalWidth > width) colWidths[1] = width - (totalWidth - colWidths[1]);
            
            std::string header = "№   |Ф.И.О.                          |Год |Мс|Дн|Город          |Расх/день";
            mvprintw(2, 0, header.substr(0, width).c_str());
            mvprintw(3, 0, std::string(totalWidth, '-').c_str());

            int y = 4;
            for (int i = startIdx; i < endIdx && y < height - 1; ++i) {
                Trip& t = trips[i];
                std::string num = std::to_string(i + 1) + std::string(colWidths[0] - std::to_string(i + 1).length(), ' ');
                std::string emp = t.employee.substr(0, colWidths[1]) + std::string(colWidths[1] - std::min(t.employee.length(), static_cast<size_t>(colWidths[1])), ' ');
                std::string year = std::to_string(t.year) + std::string(colWidths[2] - std::to_string(t.year).length(), ' ');
                std::string month = std::to_string(t.month) + std::string(colWidths[3] - std::to_string(t.month).length(), ' ');
                std::string dur = std::to_string(t.duration_days) + std::string(colWidths[4] - std::to_string(t.duration_days).length(), ' ');
                std::string city = t.city.substr(0, colWidths[5]) + std::string(colWidths[5] - std::min(t.city.length(), static_cast<size_t>(colWidths[5])), ' ');
                std::string exp = std::to_string(t.daily_expense).substr(0, colWidths[6]) + std::string(colWidths[6] - std::min(std::to_string(t.daily_expense).length(), static_cast<size_t>(colWidths[6])), ' ');
                
                std::string line = num + "|" + emp + "|" + year + "|" + month + "|" + dur + "|" + city + "|" + exp;
                mvprintw(y++, 0, line.substr(0, width).c_str());
            }

            mvprintw(height - 1, 0, ("Стрелки влево/вправо - страницы | Страница " + 
                                     std::to_string(currentPage + 1) + " из " + std::to_string(totalPages)).c_str());
            refresh();

            ch = getch();
            if (ch == '/') break;
            else if (ch == KEY_RIGHT && currentPage < totalPages - 1) currentPage++;
            else if (ch == KEY_LEFT && currentPage > 0) currentPage--;
        }
    }

 void manageAccountsScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    auto accounts = accountService.readAccounts();
    if (accounts.empty()) {
        clear();
        mvprintw(0, 0, "=== Управление учетными записями ===");
        mvprintw(2, 0, "Учетные записи отсутствуют!");
        mvprintw(height - 1, 0, "Нажмите любую клавишу...");
        refresh();
        getch();
        return;
    }

    int dataRows = height - 8, currentPage = 0;
    while (true) {
        clear();
        mvprintw(0, 0, "=== Управление учетными записями ===");
        int totalPages = (accounts.size() + dataRows - 1) / dataRows;
        int startIdx = currentPage * dataRows;
        int endIdx = std::min(startIdx + dataRows, static_cast<int>(accounts.size()));

        int colWidths[] = {4, 20, 12, 14};
        int totalWidth = std::accumulate(colWidths, colWidths + 4, 0) + 3;
        if (totalWidth > width) colWidths[1] = width - (totalWidth - colWidths[1]);

        std::string header = "   № |Логин               |Админ      |Заблокирован";
        mvprintw(2, 0, header.substr(0, width).c_str());
        mvprintw(3, 0, std::string(totalWidth, '-').c_str());

        int y = 4;
        for (int i = startIdx; i < endIdx && y < height - 4; ++i) {
            Account& a = accounts[i];
            std::string num = std::to_string(i + 1) + std::string(colWidths[0] - std::to_string(i + 1).length() - 1, ' ') + "|";
            std::string login = a.login.substr(0, colWidths[1] - 1) + std::string(colWidths[1] - std::min(a.login.length(), static_cast<size_t>(colWidths[1] - 1)), ' ') + "|";
            std::string role = (a.role ? "1 - да " : "0 - нет") + std::string(colWidths[2] - 7, ' ') + "|";
            std::string access = (a.access == ACCESS_BLOCKED ? "1 - да " : "0 - нет");
            std::string line = num + login + role + access;
            mvprintw(y++, 0, line.substr(0, width).c_str());
        }

        mvprintw(height - 3, 0, "1. Заблокировать/Разблокировать | 2. Дать/Снять админку | 3. Удалить | 4. Редактировать");
        mvprintw(height - 2, 0, ("Стрелки влево/вправо - страницы | Страница " + 
                                 std::to_string(currentPage + 1) + " из " + std::to_string(totalPages)).c_str());
        mvprintw(height - 1, 0, "Нажмите '/' для выхода");
        refresh();

        int ch = getch();
        if (ch == '/') break;
        else if (ch == KEY_RIGHT && currentPage < totalPages - 1) currentPage++;
        else if (ch == KEY_LEFT && currentPage > 0) currentPage--;
        else if (ch >= '1' && ch <= '4') {
            std::string indexStr = inputString(height - 4, 0, "Введите номер записи: ");
            if (indexStr == "/") continue;
            int index;
            try {
                index = std::stoi(indexStr) - 1;
            } catch (...) {
                mvprintw(height - 1, 0, "Ошибка: введите корректный номер!");
                refresh();
                napms(1000);
                continue;
            }
            if (index >= 0 && index < accounts.size() && accounts[index].login != accountService.getCurrentUser()->login) {
                if (ch == '1') {
                    accounts[index].access = (accounts[index].access == ACCESS_BLOCKED) ? ACCESS_APPROVED : ACCESS_BLOCKED;
                    accountService.writeAccounts(accounts);
                } else if (ch == '2') {
                    accounts[index].role = !accounts[index].role;
                    accountService.writeAccounts(accounts);
                } else if (ch == '3') {
                    mvprintw(height - 2, 0, MSG_CONFIRM_DELETE.c_str());
                    refresh();
                    if (getch() == 'y') {
                        accounts.erase(accounts.begin() + index);
                        accountService.writeAccounts(accounts);
                    }
                } else if (ch == '4') {
                    clear();
                    mvprintw(0, 0, "=== Редактирование учетной записи ===");
                    mvprintw(2, 0, "Текущие данные учетной записи:");
                    std::string currentData = std::to_string(index + 1) + " | " + accounts[index].login + " | Админ: " + (accounts[index].role ? "да" : "нет") + " | Заблокирован: " + (accounts[index].access == ACCESS_BLOCKED ? "да" : "нет");
                    mvprintw(4, 0, currentData.substr(0, width).c_str());
                    mvprintw(6, 0, "Введите новые данные (Enter для сохранения текущего значения):");

                    std::string newLogin = inputString(8, 0, "Новый логин (макс. 20): ", false, 20);
                    if (newLogin == "/") {
                        mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
                        refresh();
                        napms(1000);
                        continue;
                    }
                    if (!newLogin.empty()) {
                        bool loginExists = false;
                        for (const auto& acc : accounts) {
                            if (acc.login == newLogin && acc.login != accounts[index].login) {
                                loginExists = true;
                                break;
                            }
                        }
                        if (loginExists) {
                            mvprintw(height - 1, 0, MSG_LOGIN_EXISTS.c_str());
                            refresh();
                            napms(1000);
                            continue;
                        }
                        accounts[index].login = newLogin;
                    }

                    std::string roleStr = inputString(10, 0, "Админ (0 - нет, 1 - да): ");
                    if (roleStr == "/") {
                        mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
                        refresh();
                        napms(1000);
                        continue;
                    }
                    if (!roleStr.empty()) {
                        try {
                            int role = std::stoi(roleStr);
                            if (role == 0 || role == 1) {
                                accounts[index].role = role;
                            } else {
                                mvprintw(height - 1, 0, "\"Ошибка: роль оставлена без изменений!\"");
                                refresh();
                                napms(1000);
                            }
                        } catch (...) {
                            mvprintw(height - 1, 0, "\"Ошибка: роль оставлена без изменений!\"");
                            refresh();
                            napms(1000);
                        }
                    }

                    std::string accessStr = inputString(12, 0, "Заблокирован (0 - нет, 1 - да): ");
                    if (accessStr == "/") {
                        mvprintw(height - 1, 0, "\"Редактирование отменено!\"");
                        refresh();
                        napms(1000);
                        continue;
                    }
                    if (!accessStr.empty()) {
                        try {
                            int access = std::stoi(accessStr);
                            if (access == 0 || access == 1) {
                                accounts[index].access = access ? ACCESS_BLOCKED : ACCESS_APPROVED;
                            } else {
                                mvprintw(height - 1, 0, "\"Ошибка: статус блокировки оставлен без изменений!\"");
                                refresh();
                                napms(1000);
                            }
                        } catch (...) {
                            mvprintw(height - 1, 0, "\"Ошибка: статус блокировки оставлен без изменений!\"");
                            refresh();
                            napms(1000);
                        }
                    }

                    accountService.writeAccounts(accounts);
                    mvprintw(height - 1, 0, MSG_SUCCESS.c_str());
                    refresh();
                    napms(1000);
                }
                accounts = accountService.readAccounts();
                totalPages = (accounts.size() + dataRows - 1) / dataRows;
                currentPage = std::min(currentPage, totalPages - 1);
            }
        }
    }
}

    void pendingRequestsScreen() {
        int height, width;
        getmaxyx(stdscr, height, width);
        auto accounts = accountService.readAccounts();
        std::vector<Account> pending;
        for (const auto& acc : accounts) {
            if (acc.access == ACCESS_PENDING) pending.push_back(acc);
        }
        if (pending.empty()) {
            clear();
            mvprintw(0, 0, "=== Запросы на подтверждение ===");
            mvprintw(2, 0, "Нет запросов!");
            mvprintw(height - 1, 0, "Нажмите любую клавишу...");
            refresh();
            getch();
            return;
        }

        int dataRows = height - 5, currentPage = 0;
        while (true) {
            clear();
            mvprintw(0, 0, "=== Запросы на подтверждение ===");
            int totalPages = (pending.size() + dataRows - 1) / dataRows;
            int startIdx = currentPage * dataRows;
            int endIdx = std::min(startIdx + dataRows, static_cast<int>(pending.size()));

            mvprintw(2, 0, "   № |Логин               ");
            mvprintw(3, 0, "-------------------------");
            int y = 4;
            for (int i = startIdx; i < endIdx && y < height - 1; ++i) {
                mvprintw(y++, 0, ("%3d |" + pending[i].login).c_str(), i + 1);
            }

            mvprintw(height - 1, 0, ("1. Подтвердить | 2. Заблокировать | Страница " + 
                                     std::to_string(currentPage + 1) + " из " + std::to_string(totalPages)).c_str());
            refresh();

            int ch = getch();
            if (ch == '/') break;
            else if (ch == KEY_RIGHT && currentPage < totalPages - 1) currentPage++;
            else if (ch == KEY_LEFT && currentPage > 0) currentPage--;
            else if (ch == '1' || ch == '2') {
                std::string indexStr = inputString(height - 2, 0, "Введите номер записи: ");
                if (indexStr == "/") continue;
                int index;
                try {
                    index = std::stoi(indexStr) - 1;
                } catch (...) {
                    mvprintw(height - 1, 0, "Ошибка: введите корректный номер!");
                    refresh();
                    napms(1000);
                    continue;
                }
                if (index >= 0 && index < pending.size()) {
                    for (auto& acc : accounts) {
                        if (acc.login == pending[index].login) {
                            acc.access = (ch == '1') ? ACCESS_APPROVED : ACCESS_BLOCKED;
                            break;
                        }
                    }
                    accountService.writeAccounts(accounts);
                    pending.erase(pending.begin() + index);
                    totalPages = (pending.size() + dataRows - 1) / dataRows;
                    currentPage = std::min(currentPage, totalPages - 1);
                }
            }
        }
    }


void tripSummaryScreen() {
    int height, width;
    getmaxyx(stdscr, height, width);
    while (true) {
        clear();
        mvprintw(0, 0, "=== Сводка по командировкам ===");
        mvprintw(2, 0, "1. Общие выплаты за месяц");
        mvprintw(3, 0, "2. Наиболее часто посещаемые города за период");
        mvprintw(4, 0, "/. Назад");
        refresh();
        int ch = getch();
        if (ch == '/') break;
        int choice = ch - '0';
        if (choice == 1) {
            int year = inputInt(6, 0, "Введите год: ", 4);
            if (year == -1) continue;
            int month = inputInt(7, 0, "Введите месяц (1-12): ", 2);
            if (month == -1) continue;
            if (month < 1 || month > 12) {
                mvprintw(9, 0, "Неверный месяц!");
                refresh();
                napms(1000);
                continue;
            }
            double total = tripService.calculateTotalExpenses(year, month);
            mvprintw(9, 0, "Общие выплаты за %d-%02d: %.2f", year, month, total);
            mvprintw(11, 0, "Нажмите любую клавишу...");
            refresh();
            getch();
        } else if (choice == 2) {
            int start_year = inputInt(6, 0, "Введите начальный год: ", 4);
            if (start_year == -1) continue;
            int start_month = inputInt(7, 0, "Введите начальный месяц (1-12): ", 2);
            if (start_month == -1) continue;
            int end_year = inputInt(8, 0, "Введите конечный год: ", 4);
            if (end_year == -1) continue;
            int end_month = inputInt(9, 0, "Введите конечный месяц (1-12): ", 2);
            if (end_month == -1) continue;
            if (start_month < 1 || start_month > 12 || end_month < 1 || end_month > 12) {
                mvprintw(11, 0, "Неверный месяц!");
                refresh();
                napms(1000);
                continue;
            }
            auto cityFreq = tripService.getCityVisitFrequency(start_year, start_month, end_year, end_month);
            if (cityFreq.empty()) {
                mvprintw(11, 0, "Нет данных за указанный период.");
                mvprintw(height - 1, 0, "Нажмите любую клавишу...");
                refresh();
                getch();
                continue;
            }

            int dataRows = height - 6, currentPage = 0;
            while (true) {
                clear();
                mvprintw(0, 0, "=== Наиболее посещаемые города ===");
                int totalPages = (cityFreq.size() + dataRows - 1) / dataRows;
                int startIdx = currentPage * dataRows;
                int endIdx = std::min(startIdx + dataRows, static_cast<int>(cityFreq.size()));

                int colWidths[] = {4, 30, 12};
                int totalWidth = std::accumulate(colWidths, colWidths + 3, 0) + 2;
                if (totalWidth > width) colWidths[1] = width - (colWidths[0] + colWidths[2] + 2);

                std::string header = "№   |Город                          |Посещений";
                mvprintw(2, 0, header.substr(0, width).c_str());
                mvprintw(3, 0, std::string(totalWidth, '-').c_str());

                int y = 4;
                for (int i = startIdx; i < endIdx && y < height - 2; ++i) {
                    auto& city = cityFreq[i];
                    std::string num = std::to_string(i + 1) + std::string(colWidths[0] - std::to_string(i + 1).length(), ' ');
                    std::string name = city.first.substr(0, colWidths[1]) + std::string(colWidths[1] - std::min(city.first.length(), static_cast<size_t>(colWidths[1])), ' ');
                    std::string visits = std::to_string(city.second) + std::string(colWidths[2] - std::to_string(city.second).length(), ' ');
                    std::string line = num + "|" + name + "|" + visits;
                    mvprintw(y++, 0, line.substr(0, width).c_str());
                }

                mvprintw(height - 2, 0, ("Стрелки влево/вправо - страницы | Страница " +
                                         std::to_string(currentPage + 1) + " из " + std::to_string(totalPages)).c_str());
                mvprintw(height - 1, 0, "Нажмите '/' для выхода");
                refresh();

                int ch = getch();
                if (ch == '/') break;
                else if (ch == KEY_RIGHT && currentPage < totalPages - 1) currentPage++;
                else if (ch == KEY_LEFT && currentPage > 0) currentPage--;
            }
        }
    }
}


    void mainMenu() {
        while (true) {
            int height, width;
            getmaxyx(stdscr, height, width);
            clear();
            mvprintw(0, 0, "=== Главное меню ===");
            int y = 2;
            mvprintw(y++, 0, "1. Просмотр данных");
            mvprintw(y++, 0, "2. Поиск данных");
            mvprintw(y++, 0, "3. Сводка по командировкам");
            if (accountService.getCurrentUser()->role == ROLE_ADMIN) {
                mvprintw(y++, 0, "4. Управление учетными записями");
                mvprintw(y++, 0, "5. Добавить командировку");
                mvprintw(y++, 0, "6. Редактировать командировку");
                mvprintw(y++, 0, "7. Удалить командировку");
                mvprintw(y++, 0, "8. Регистрация пользователя");
                mvprintw(y++, 0, "9. Запросы на подтверждение");
            }
            mvprintw(y++, 0, "/. Выход");
            refresh();

            int ch = getch();
            if (ch == '/') break;
            int choice = ch - '0';
            switch (choice) {
                case 1: viewTripsScreen(); break;
                case 2: searchTripsScreen(); break;
                case 3: tripSummaryScreen(); break;
                case 4: if (accountService.getCurrentUser()->role == ROLE_ADMIN) manageAccountsScreen(); break;
                case 5: if (accountService.getCurrentUser()->role == ROLE_ADMIN) addTripScreen(); break;
                case 6: if (accountService.getCurrentUser()->role == ROLE_ADMIN) editTripScreen(); break;
                case 7: if (accountService.getCurrentUser()->role == ROLE_ADMIN) deleteTripScreen(); break;
                case 8: if (accountService.getCurrentUser()->role == ROLE_ADMIN) registerScreen(true); break;
                case 9: if (accountService.getCurrentUser()->role == ROLE_ADMIN) pendingRequestsScreen(); break;
                default: mvprintw(height - 1, 0, MSG_INVALID_INPUT.c_str()); refresh(); napms(1000);
            }
        }
    }
};

// **Main**
int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    srand(time(nullptr));

    AccountService accountService;
    TripService tripService;
    UIService uiService(accountService, tripService);

    if (!std::ifstream("accounts.txt")) {
        Account admin{DEFAULT_ADMIN_LOGIN, "", UtilityService::generateSalt(), ROLE_ADMIN, ACCESS_APPROVED};
        admin.salted_hash_password = UtilityService::hashPassword(DEFAULT_ADMIN_PASSWORD, admin.salt);
        std::vector<Account> accounts = {admin};
        accountService.writeAccounts(accounts);
    }
    if (!std::ifstream("trips.txt")) std::ofstream("trips.txt").close();

    while (true) {
        int height, width;
        getmaxyx(stdscr, height, width);
        clear();
        mvprintw(0, 0, "1. Вход | 2. Регистрация | / Выход");
        refresh();
        int ch = getch();
        if (ch == '/') break;
        int choice = ch - '0';
        if (choice == 1) {
            uiService.loginScreen();
            if (accountService.getCurrentUser()) {
                uiService.mainMenu();
                accountService.clearCurrentUser();
            }
        } else if (choice == 2) {
            uiService.registerScreen();
        } else {
            mvprintw(height - 1, 0, MSG_INVALID_INPUT.c_str());
            refresh();
            napms(1000);
        }
    }
    endwin();
    return 0;
}

