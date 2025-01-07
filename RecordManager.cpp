#include "RecordManager.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <cstdio>
#include <cstdlib>

RecordManager::RecordManager(const std::string& datFileName)
    : m_datFileName(datFileName)
{}

int RecordManager::writeRecords(const TableSchema& schema, char*** records, int recordCount) {
    std::ofstream ofs(m_datFileName.c_str(), std::ios::binary | std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "Failed to open dat file: " << m_datFileName << std::endl;
        return -1;
    }

    // 分隔符
    ofs.put(TABLE_DELIMITER);
    // 表名
    ofs.write(schema.sTableName, TABLE_NAME_LENGTH);
    // 记录数
    ofs.write(reinterpret_cast<const char*>(&recordCount), sizeof(int));
    // 字段数
    ofs.write(reinterpret_cast<const char*>(&schema.iFieldCount), sizeof(int));

    // 写每条记录
    for (int i = 0; i < recordCount; ++i) {
        // 有效标识
        char validFlag = 1;
        ofs.write(&validFlag, sizeof(char));

        for (int j = 0; j < schema.iFieldCount; ++j) {
            if (std::strcmp(schema.fieldArray[j].sType, "int") == 0) {
                int intValue = std::atoi(records[i][j]);
                ofs.write(reinterpret_cast<const char*>(&intValue), sizeof(int));
            }
            else {
                // 视作 char[n]，写固定长度
                int size = schema.fieldArray[j].iSize;
                std::vector<char> buffer(size, 0);
                std::strncpy(buffer.data(), records[i][j], size);
                ofs.write(buffer.data(), size);
            }
        }
    }

    ofs.close();
    return 0;
}



int RecordManager::readRecords(const TableSchema& schema, char*** outRecords, int maxRecordNum) {
    std::ifstream ifs(m_datFileName.c_str(), std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open dat file: " << m_datFileName << std::endl;
        return -1;
    }

    int totalReadCount = 0;
    while (!ifs.eof()) {
        // 读取分隔符
        char delimiter;
        if (!ifs.get(delimiter)) {
            break;
        }
        if (delimiter != TABLE_DELIMITER) {
            continue;
        }

        // 读取表名
        char tableName[TABLE_NAME_LENGTH];
        std::memset(tableName, 0, TABLE_NAME_LENGTH);
        ifs.read(tableName, TABLE_NAME_LENGTH);

        // 读取记录数、字段数
        int recordCountInFile = 0;
        int fieldCountInFile = 0;
        ifs.read(reinterpret_cast<char*>(&recordCountInFile), sizeof(int));
        ifs.read(reinterpret_cast<char*>(&fieldCountInFile), sizeof(int));


        // 若匹配目标表
        if (std::strcmp(tableName, schema.sTableName) == 0) {
            for (int i = 0; i < recordCountInFile; ++i) {
                if (totalReadCount >= maxRecordNum) {
                    // 如果已经到达外部可容纳上限，则不再读
                    // 但仍需跳过文件中的后续字节
                    skipRecord(ifs, schema, fieldCountInFile);
                    continue;
                }

                // 有效标识
                char validFlag = 0;
                ifs.read(&validFlag, sizeof(char));
                if (validFlag == 0) {
                    // 无效记录，跳过这条记录的所有字段
                    skipRecord(ifs, schema, fieldCountInFile);
                    continue;
                }

                // 分配空间
                outRecords[totalReadCount] = new char* [fieldCountInFile];
                for (int j = 0; j < fieldCountInFile; ++j) {
                    if (std::strcmp(schema.fieldArray[j].sType, "int") == 0) {
                        int intValue = 0;
                        ifs.read(reinterpret_cast<char*>(&intValue), sizeof(int));
                        // 转成字符串
                        char buf[64];
                        std::sprintf(buf, "%d", intValue);
                        outRecords[totalReadCount][j] = new char[std::strlen(buf) + 1];
                        std::strcpy(outRecords[totalReadCount][j], buf);
                    }
                    else {
                        int size = schema.fieldArray[j].iSize;
                        outRecords[totalReadCount][j] = new char[size + 1];
                        std::memset(outRecords[totalReadCount][j], 0, size + 1);
                        ifs.read(outRecords[totalReadCount][j], size);
                    }
                }
                totalReadCount++;
            }
            // 在本示例中，如果一个 .dat 里只存放一次表的 block，可以直接 break
            // 若可能有多个 block（同表多段），可以不 break
        //    break;
        }
        else {
            // 跳过这张表的所有记录
            for (int i = 0; i < recordCountInFile; ++i) {
                // 跳过有效标识
                ifs.seekg(1, std::ios::cur);
                // 跳过字段
                skipRecord(ifs, schema, fieldCountInFile);
            }
        }
    }

    ifs.close();
    return totalReadCount;
}


std::string RecordManager::getDatFileName() const {
    return m_datFileName;
}

void RecordManager::skipRecord(std::ifstream& ifs, const TableSchema& schema, int fieldCountInFile) {
    for (int j = 0; j < fieldCountInFile; ++j) {
        if (std::strcmp(schema.fieldArray[j].sType, "int") == 0) {
            ifs.seekg(sizeof(int), std::ios::cur);
        }
        else {
            ifs.seekg(schema.fieldArray[j].iSize, std::ios::cur);
        }
    }
}
