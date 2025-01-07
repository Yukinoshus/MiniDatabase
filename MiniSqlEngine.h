#pragma once
#ifndef MINISQLENGINE_H
#define MINISQLENGINE_H

#include <string>
#include "DatabaseManager.h"
#include "RecordManager.h"

/**
 * MiniSqlEngine
 * 负责解析非常简单的“SQL-like”命令并调用 DatabaseManager / RecordManager 执行
 */
class MiniSqlEngine {
public:
    MiniSqlEngine(const std::string& dbFileName, const std::string& datFileName);

    // 解析并执行单条命令
    void execCommand(const std::string& cmd);

private:
    DatabaseManager m_dbManager;
    RecordManager   m_recManager;

    // 各种子函数
    void createTable(const std::string& cmd);
    void dropTable(const std::string& cmd);
    void renameTable(const std::string& cmd);
    void insertRecord(const std::string& cmd);
    void deleteRecord(const std::string& cmd);
    void updateRecord(const std::string& cmd);
    void selectRecords(const std::string& cmd);

    // 帮助函数：去除首尾空格
    std::string trim(const std::string& str);
};

#endif
