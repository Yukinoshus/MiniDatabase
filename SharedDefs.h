#ifndef SHAREDDEFS_H

#define SHAREDDEFS_H

#include <cstring>

//------------------ 常量定义 ------------------//
static const int FIELD_NAME_LENGTH = 32;   // 字段名最大长度
static const int TABLE_NAME_LENGTH = 32;   // 表名最大长度
static const char TABLE_DELIMITER = '~'; // 表或记录分隔符

//------------------ 数据结构定义 ------------------//

/**
 * 单个字段结构
 */
struct TableMode {
    char sFieldName[FIELD_NAME_LENGTH];  // 字段名
    char sType[8];                       // 字段类型，如 "int", "char", ...
    int  iSize;                          // 字段长度，如 char[10] 则 iSize = 10
    char bKey;                           // 是否为 KEY 键，1表示是KEY，0表示不是KEY
    char bNullFlag;                      // 是否允许为NULL，1表示可为空，0表示不可为空
    char bValidFlag;                     // 是否有效，1表示有效，0表示无效（逻辑删除）
};

/**
 * 一张表的模式结构
 */
struct TableSchema {
    char        sTableName[TABLE_NAME_LENGTH]; // 表名
    int         iFieldCount;                   // 字段数量
    TableMode   fieldArray[32];               // 假定每张表最多 32 个字段
};

#endif
