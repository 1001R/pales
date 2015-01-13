#pragma once
#include <string>

using namespace std;

class Database {
public:
    enum State { RUNNING, FINISHED, CANCELLED, ERROR };
private:
    static const char PATHSEP = '/';

    string m_dirPath;
    string m_processId;
    int m_dirDescriptor;

    string createFileName(State state) const;
    void createEmptyFile(const string& filePath);
public:
    Database(const char* dirPath, const char* procid);
    ~Database();
    //setRunning(pid_t pid);
    //setFinished(int exitCode);
    //setCancelled();
    void update(State s, ...);
    void sync();
    void close();
};

