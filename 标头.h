#ifndef MINISQLENGINE_H
#define MINISQLENGINE_H

#include <string>
#include "DatabaseManager.h"
#include "RecordManager.h"

class MiniSqlEngine {
public:
    MiniSqlEngine(const std::string& dbFileName, const std::string& datFileName);
    void execCommand(const std::string& cmd); // 解析并执行命令

private:
    DatabaseManager m_dbManager;
    RecordManager   m_recManager;

    // 各种子函数，用来执行具体的操作
    void createTable(const std::string& cmd);
    void dropTable(const std::string& cmd);
    void renameTable(const std::string& cmd);
    void insertRecord(const std::string& cmd);
    void deleteRecord(const std::string& cmd);
    void updateRecord(const std::string& cmd);
    void selectRecords(const std::string& cmd);

    // 帮助函数
    std::string trim(const std::string& str);
};

#endif
