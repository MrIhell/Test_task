#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <iomanip>
#include <sstream>
#include <vector>

using namespace std;



// Класс, представляющий простую задачу
class Task {
public:
    string name;    
    int delay;      
    int priority;   

    Task(string n, int d, int p) : name(n), delay(d), priority(p) {}
};

// Функция сравнения приоритетов задач
bool taskCompare(const Task& t1, const Task& t2) {
    return t1.priority > t2.priority; // приоритет меньше - выше приоритет в очереди
}

// Глобальные переменные для синхронизации и управления очередями задач
mutex MTX;
condition_variable CV;
priority_queue<Task, vector<Task>, decltype(&taskCompare)> simpleTasks(taskCompare);

// Объявление функций
string currentTime();
void processSimpleTask(const Task& task);
void processDeferredTask(const string& deferredName, const string& simpleName, int deferredDelay, int simpleDelay, int priority);
void taskManager();

// Функция для получения текущего времени в строковом формате
string currentTime() {
    auto now = chrono::system_clock::now();
    auto now_c = chrono::system_clock::to_time_t(now);
    struct tm buf;
    gmtime_s(&buf, &now_c);
    ostringstream oss;
    oss << put_time(&buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Функция для обработки простой задачи
void processSimpleTask(const Task& task) {
    {
        lock_guard<mutex> lock(MTX);
        cout << currentTime() << ": queueS1 : " << task.name << " - " << task.delay << " running...\n";
    }
    this_thread::sleep_for(chrono::seconds(task.delay)); // Имитируем выполнение задачи

    {
        lock_guard<mutex> lock(MTX);
        cout << currentTime() << ": queueS1 : " << task.name << " - " << task.delay << " complited.\n";
    }

    // Создаем отложенную задачу после завершения простой задачи
    if (task.name == "taskS1") {
        thread(processDeferredTask, "taskD1", "taskS1", 10, 2, 1).detach();
    }
    else if (task.name == "taskS2") {
        thread(processDeferredTask, "taskD2", "taskS2", 10, 2, 2).detach();
    }
    else if (task.name == "taskS3") {
        thread(processDeferredTask, "taskD3", "taskS3", 10, 2, 1).detach();
    }
}

// Функция для обработки отложенной задачи
void processDeferredTask(const string& deferredName, const string& simpleName, int deferredDelay, int simpleDelay, int priority) {
    while (true) {

        {
            lock_guard<mutex> lock(MTX);
            cout << currentTime() << ": " << deferredName << " - " << deferredDelay << " : created\n";
        }

        this_thread::sleep_for(chrono::seconds(deferredDelay)); // Имитируем задержку

        Task simpleTask(simpleName, simpleDelay, priority);

        {
            lock_guard<mutex> lock(MTX);
            simpleTasks.push(simpleTask);
            cout << currentTime() << ": " << deferredName << " - " << deferredDelay << " : (" << simpleName << " : queueS1) pushed.\n";
        }

        CV.notify_one(); // Уведомляем менеджер задач о новой задаче
    }
}

// Функция-менеджер для управления выполнением простых задач
void taskManager() {
    while (true) {
        unique_lock<mutex> lock(MTX);
        CV.wait(lock, [] { return !simpleTasks.empty(); }); // Ждём, пока в очереди не появится задача

        Task task = simpleTasks.top();
        simpleTasks.pop();
        lock.unlock();

        processSimpleTask(task);
    }
}

int main() {
    // Создаём потоки для отложенных задач
    thread deferredTask1(processDeferredTask, "taskD1", "taskS1", 10, 2, 1);
    thread deferredTask2(processDeferredTask, "taskD2", "taskS2", 10, 2, 2);
    thread deferredTask3(processDeferredTask, "taskD3", "taskS3", 10, 2, 1);
    // Создаём поток для менеджера задач
    thread manager(taskManager);

    // Без этого компилятор ругается
    deferredTask1.join();
    deferredTask2.join();
    deferredTask3.join();
    manager.join();

    return 0;
}