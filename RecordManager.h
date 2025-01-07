#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include <string>
#include "SharedDefs.h"

/**
 * RecordManager
 * 负责 .dat 文件中的记录管理：写入、读取等
 */
class RecordManager {
public:
    explicit RecordManager(const std::string& datFileName);

    /**
     * 在 .dat 文件中，为指定表追加多条记录
     * @param schema      目标表模式
     * @param records     记录数据: records[i][j] 表示第 i 条记录的第 j 个字段字符串
     * @param recordCount 记录条数
     */
    int writeRecords(const TableSchema& schema, char*** records, int recordCount);

    /**
     * 从 .dat 文件中读取指定表的记录
     * @param schema        表模式
     * @param outRecords    输出记录 (char***)
     * @param maxRecordNum  最多可读多少条
     * @return              实际读取到的条数
     */
    int readRecords(const TableSchema& schema, char*** outRecords, int maxRecordNum);



    // --- 新增的函数，用来获取 m_datFileName ---
    std::string getDatFileName()const;

private:
    std::string m_datFileName;

    // 帮助函数：跳过一条记录（所有字段）的字节
    void skipRecord(std::ifstream& ifs, const TableSchema& schema, int fieldCountInFile);
};

#endif
