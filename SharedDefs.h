#ifndef SHAREDDEFS_H

#define SHAREDDEFS_H

#include <cstring>

//------------------ �������� ------------------//
static const int FIELD_NAME_LENGTH = 32;   // �ֶ�����󳤶�
static const int TABLE_NAME_LENGTH = 32;   // ������󳤶�
static const char TABLE_DELIMITER = '~'; // ����¼�ָ���

//------------------ ���ݽṹ���� ------------------//

/**
 * �����ֶνṹ
 */
struct TableMode {
    char sFieldName[FIELD_NAME_LENGTH];  // �ֶ���
    char sType[8];                       // �ֶ����ͣ��� "int", "char", ...
    int  iSize;                          // �ֶγ��ȣ��� char[10] �� iSize = 10
    char bKey;                           // �Ƿ�Ϊ KEY ����1��ʾ��KEY��0��ʾ����KEY
    char bNullFlag;                      // �Ƿ�����ΪNULL��1��ʾ��Ϊ�գ�0��ʾ����Ϊ��
    char bValidFlag;                     // �Ƿ���Ч��1��ʾ��Ч��0��ʾ��Ч���߼�ɾ����
};

/**
 * һ�ű��ģʽ�ṹ
 */
struct TableSchema {
    char        sTableName[TABLE_NAME_LENGTH]; // ����
    int         iFieldCount;                   // �ֶ�����
    TableMode   fieldArray[32];               // �ٶ�ÿ�ű���� 32 ���ֶ�
};

#endif
