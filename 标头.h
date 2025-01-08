#ifndef MINISQLENGINE_H
#define MINISQLENGINE_H

#include <string>
#include "DatabaseManager.h"
#include "RecordManager.h"

class MiniSqlEngine {
public:
    MiniSqlEngine(const std::string& dbFileName, const std::string& datFileName);
    void execCommand(const std::string& cmd); // ������ִ������

private:
    DatabaseManager m_dbManager;
    RecordManager   m_recManager;

    // �����Ӻ���������ִ�о���Ĳ���
    void createTable(const std::string& cmd);
    void dropTable(const std::string& cmd);
    void renameTable(const std::string& cmd);
    void insertRecord(const std::string& cmd);
    void deleteRecord(const std::string& cmd);
    void updateRecord(const std::string& cmd);
    void selectRecords(const std::string& cmd);

    // ��������
    std::string trim(const std::string& str);
};

#endif
