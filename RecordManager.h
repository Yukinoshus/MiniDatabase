#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include <string>
#include "SharedDefs.h"

/**
 * RecordManager
 * ���� .dat �ļ��еļ�¼����д�롢��ȡ��
 */
class RecordManager {
public:
    explicit RecordManager(const std::string& datFileName);

    /**
     * �� .dat �ļ��У�Ϊָ����׷�Ӷ�����¼
     * @param schema      Ŀ���ģʽ
     * @param records     ��¼����: records[i][j] ��ʾ�� i ����¼�ĵ� j ���ֶ��ַ���
     * @param recordCount ��¼����
     */
    int writeRecords(const TableSchema& schema, char*** records, int recordCount);

    /**
     * �� .dat �ļ��ж�ȡָ����ļ�¼
     * @param schema        ��ģʽ
     * @param outRecords    �����¼ (char***)
     * @param maxRecordNum  ���ɶ�������
     * @return              ʵ�ʶ�ȡ��������
     */
    int readRecords(const TableSchema& schema, char*** outRecords, int maxRecordNum);



    // --- �����ĺ�����������ȡ m_datFileName ---
    std::string getDatFileName()const;

private:
    std::string m_datFileName;

    // ��������������һ����¼�������ֶΣ����ֽ�
    void skipRecord(std::ifstream& ifs, const TableSchema& schema, int fieldCountInFile);
};

#endif
