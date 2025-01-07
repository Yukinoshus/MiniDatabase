#pragma once
#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <string>
#include <vector>
#include "SharedDefs.h"

/**
 * DatabaseManager
 * 负责 .dbf 文件中的表结构管理：写入、读取、删除、重写等
 */
class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& dbFileName);

    // 写入一张表的结构（追加写）
    int writeTableSchema(const TableSchema& schema);

    // 读取所有表结构到 outSchemas
    int readAllTableSchemas(std::vector<TableSchema>& outSchemas);

    // 获取指定表名的结构（若有重名，返回第一个匹配）
    bool getTableSchemaByName(const std::string& tableName, TableSchema& outSchema);

    // （可选）获取当前 .dbf 文件名
    const std::string& getDbFileName() const { return m_dbFileName; }

    // 调试打印
    void printAllTableSchemas(const std::vector<TableSchema>& schemas);

private:
    std::string m_dbFileName;
};

#endif
