#pragma once
#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <string>
#include <vector>
#include "SharedDefs.h"

/**
 * DatabaseManager
 * ���� .dbf �ļ��еı�ṹ����д�롢��ȡ��ɾ������д��
 */
class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& dbFileName);

    // д��һ�ű�Ľṹ��׷��д��
    int writeTableSchema(const TableSchema& schema);

    // ��ȡ���б�ṹ�� outSchemas
    int readAllTableSchemas(std::vector<TableSchema>& outSchemas);

    // ��ȡָ�������Ľṹ���������������ص�һ��ƥ�䣩
    bool getTableSchemaByName(const std::string& tableName, TableSchema& outSchema);

    // ����ѡ����ȡ��ǰ .dbf �ļ���
    const std::string& getDbFileName() const { return m_dbFileName; }

    // ���Դ�ӡ
    void printAllTableSchemas(const std::vector<TableSchema>& schemas);

private:
    std::string m_dbFileName;
};

#endif
