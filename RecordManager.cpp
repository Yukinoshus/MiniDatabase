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

    // �ָ���
    ofs.put(TABLE_DELIMITER);
    // ����
    ofs.write(schema.sTableName, TABLE_NAME_LENGTH);
    // ��¼��
    ofs.write(reinterpret_cast<const char*>(&recordCount), sizeof(int));
    // �ֶ���
    ofs.write(reinterpret_cast<const char*>(&schema.iFieldCount), sizeof(int));

    // дÿ����¼
    for (int i = 0; i < recordCount; ++i) {
        // ��Ч��ʶ
        char validFlag = 1;
        ofs.write(&validFlag, sizeof(char));

        for (int j = 0; j < schema.iFieldCount; ++j) {
            if (std::strcmp(schema.fieldArray[j].sType, "int") == 0) {
                int intValue = std::atoi(records[i][j]);
                ofs.write(reinterpret_cast<const char*>(&intValue), sizeof(int));
            }
            else {
                // ���� char[n]��д�̶�����
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
        // ��ȡ�ָ���
        char delimiter;
        if (!ifs.get(delimiter)) {
            break;
        }
        if (delimiter != TABLE_DELIMITER) {
            continue;
        }

        // ��ȡ����
        char tableName[TABLE_NAME_LENGTH];
        std::memset(tableName, 0, TABLE_NAME_LENGTH);
        ifs.read(tableName, TABLE_NAME_LENGTH);

        // ��ȡ��¼�����ֶ���
        int recordCountInFile = 0;
        int fieldCountInFile = 0;
        ifs.read(reinterpret_cast<char*>(&recordCountInFile), sizeof(int));
        ifs.read(reinterpret_cast<char*>(&fieldCountInFile), sizeof(int));


        // ��ƥ��Ŀ���
        if (std::strcmp(tableName, schema.sTableName) == 0) {
            for (int i = 0; i < recordCountInFile; ++i) {
                if (totalReadCount >= maxRecordNum) {
                    // ����Ѿ������ⲿ���������ޣ����ٶ�
                    // �����������ļ��еĺ����ֽ�
                    skipRecord(ifs, schema, fieldCountInFile);
                    continue;
                }

                // ��Ч��ʶ
                char validFlag = 0;
                ifs.read(&validFlag, sizeof(char));
                if (validFlag == 0) {
                    // ��Ч��¼������������¼�������ֶ�
                    skipRecord(ifs, schema, fieldCountInFile);
                    continue;
                }

                // ����ռ�
                outRecords[totalReadCount] = new char* [fieldCountInFile];
                for (int j = 0; j < fieldCountInFile; ++j) {
                    if (std::strcmp(schema.fieldArray[j].sType, "int") == 0) {
                        int intValue = 0;
                        ifs.read(reinterpret_cast<char*>(&intValue), sizeof(int));
                        // ת���ַ���
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
            // �ڱ�ʾ���У����һ�� .dat ��ֻ���һ�α�� block������ֱ�� break
            // �������ж�� block��ͬ���Σ������Բ� break
        //    break;
        }
        else {
            // �������ű�����м�¼
            for (int i = 0; i < recordCountInFile; ++i) {
                // ������Ч��ʶ
                ifs.seekg(1, std::ios::cur);
                // �����ֶ�
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
