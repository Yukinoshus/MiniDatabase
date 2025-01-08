#include "DatabaseManager.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>

DatabaseManager::DatabaseManager(const std::string& dbFileName)
    : m_dbFileName(dbFileName)
{}

int DatabaseManager::writeTableSchema(const TableSchema& schema) {
    // �Զ�����׷�ӷ�ʽд�� .dbf
    std::ofstream ofs(m_dbFileName.c_str(), std::ios::binary | std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "Failed to open dbf file: " << m_dbFileName << std::endl;
        return -1;
    }

    // д�ָ���
    ofs.put(TABLE_DELIMITER);

    // д����
    ofs.write(schema.sTableName, TABLE_NAME_LENGTH);

    // д�ֶ���
    ofs.write(reinterpret_cast<const char*>(&schema.iFieldCount), sizeof(int));

    // д�ֶνṹ����
    for (int i = 0; i < schema.iFieldCount; ++i) {
        ofs.write(reinterpret_cast<const char*>(&schema.fieldArray[i]), sizeof(TableMode));
    }

    ofs.close();
    return 0;
}

int DatabaseManager::readAllTableSchemas(std::vector<TableSchema>& outSchemas) {
    std::ifstream ifs(m_dbFileName.c_str(), std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open dbf file: " << m_dbFileName << std::endl;
        return -1;
    }

    outSchemas.clear();
    while (!ifs.eof()) {
        char delimiter;
        if (!ifs.get(delimiter)) {
            // �������������ݣ�����
            break;
        }
        if (delimiter != TABLE_DELIMITER) {
            // �����ǿ��ļ����ʽ����
            continue;
        }

        TableSchema ts;
        std::memset(&ts, 0, sizeof(TableSchema));
        // ������
        ifs.read(ts.sTableName, TABLE_NAME_LENGTH);
        // ���ֶ���
        ifs.read(reinterpret_cast<char*>(&ts.iFieldCount), sizeof(int));
        // ���ֶ�����
        for (int i = 0; i < ts.iFieldCount; ++i) {
            ifs.read(reinterpret_cast<char*>(&ts.fieldArray[i]), sizeof(TableMode));
        }

        outSchemas.push_back(ts);
    }

    ifs.close();
    return static_cast<int>(outSchemas.size());
}

bool DatabaseManager::getTableSchemaByName(const std::string& tableName, TableSchema& outSchema) {
    std::vector<TableSchema> schemas;
    if (readAllTableSchemas(schemas) < 0) {
        // ��ʧ��
        return false;
    }
    for (auto& ts : schemas) {
        if (tableName == ts.sTableName) {
            outSchema = ts;
            return true;
        }
    }
    return false;
}

void DatabaseManager::printAllTableSchemas(const std::vector<TableSchema>& schemas) {
    std::cout << "=== All Tables in " << m_dbFileName << " ===\n";
    for (size_t i = 0; i < schemas.size(); ++i) {
        std::cout << (i + 1) << ") Table Name: " << schemas[i].sTableName
            << ", Field Count: " << schemas[i].iFieldCount << "\n";
    }
    std::cout << "==========================================\n";
}
