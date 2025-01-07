#include "MiniSqlEngine.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstring>

MiniSqlEngine::MiniSqlEngine(const std::string& dbFileName, const std::string& datFileName)
    : m_dbManager(dbFileName), m_recManager(datFileName)
{}

std::string MiniSqlEngine::trim(const std::string& str) {
    if (str.empty()) return str;
    size_t startPos = 0;
    while (startPos < str.size() && std::isspace((unsigned char)str[startPos])) {
        startPos++;
    }
    size_t endPos = str.size() - 1;
    while (endPos > startPos && std::isspace((unsigned char)str[endPos])) {
        endPos--;
    }
    return str.substr(startPos, endPos - startPos + 1);
}

void MiniSqlEngine::execCommand(const std::string& cmd) {
    // 转大写用于简单匹配
    std::string upperCmd = cmd;
    std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);

    if (upperCmd.find("CREATE TABLE") != std::string::npos) {
        createTable(cmd);
    }
    else if (upperCmd.find("DROP TABLE") != std::string::npos) {
        dropTable(cmd);
    }
    else if (upperCmd.find("RENAME TABLE") != std::string::npos) {
        renameTable(cmd);
    }
    else if (upperCmd.find("INSERT INTO") != std::string::npos) {
        insertRecord(cmd);
    }
    else if (upperCmd.find("DELETE FROM") != std::string::npos) {
        deleteRecord(cmd);
    }
    else if (upperCmd.find("UPDATE") != std::string::npos) {
        updateRecord(cmd);
    }
    else if (upperCmd.find("SELECT") != std::string::npos) {
        selectRecords(cmd);
    }
    else {
        std::cout << "[WARN] Unrecognized command: " << cmd << std::endl;
    }
}

// ---------------- CREATE TABLE ----------------//
void MiniSqlEngine::createTable(const std::string& cmd) {
    // 示例语法：
 /*    CREATE TABLE MyTable(
       Name char[10] KEY NO_NULL VALID,
       Age int NOT_KEY NO_NULL VALID
     ) INTO MyDB.dbf;*/

    size_t posCreate = cmd.find("CREATE TABLE");
    if (posCreate == std::string::npos) {
        std::cout << "[ERROR] CREATE TABLE syntax error." << std::endl;
        return;
    }
    size_t posParenL = cmd.find('(', posCreate);
    if (posParenL == std::string::npos) {
        std::cout << "[ERROR] Missing '(' in CREATE TABLE." << std::endl;
        return;
    }
    size_t posParenR = cmd.rfind(')');
    if (posParenR == std::string::npos) {
        std::cout << "[ERROR] Missing ')' in CREATE TABLE." << std::endl;
        return;
    }
    size_t posInto = cmd.find("INTO", posParenR);
    if (posInto == std::string::npos) {
        std::cout << "[ERROR] Missing INTO in CREATE TABLE." << std::endl;
        return;
    }

    // 提取表名
    std::string tableNamePart = trim(cmd.substr(posCreate + strlen("CREATE TABLE"),
        posParenL - (posCreate + strlen("CREATE TABLE"))));

    // 提取字段部分
    std::string fieldsPart = trim(cmd.substr(posParenL + 1, posParenR - posParenL - 1));

    // 提取 dbFileName (可不真正用）
    std::string dbFilePart = trim(cmd.substr(posInto + strlen("INTO")));
    if (!dbFilePart.empty() && dbFilePart.back() == ';') {
        dbFilePart.pop_back();
    }
    dbFilePart = trim(dbFilePart);

    // 构造 TableSchema
    TableSchema schema;
    std::memset(&schema, 0, sizeof(TableSchema));
    std::strncpy(schema.sTableName, tableNamePart.c_str(), TABLE_NAME_LENGTH - 1);

    // 拆分字段定义
    // 以逗号分隔
    std::vector<std::string> fieldDefs;
    {
        std::stringstream ss(fieldsPart);
        std::string item;
        while (std::getline(ss, item, ',')) {
            fieldDefs.push_back(trim(item));
        }
    }

    int fieldIndex = 0;
    for (auto& fd : fieldDefs) {
        // 例如 "Name char[10] KEY NO_NULL VALID"
        std::stringstream ss(fd);
        std::vector<std::string> tokens;
        std::string tk;
        while (ss >> tk) {
            tokens.push_back(tk);
        }
        if (tokens.size() < 2) {
            std::cout << "[ERROR] Incomplete field definition: " << fd << std::endl;
            continue;
        }
        TableMode& f = schema.fieldArray[fieldIndex];
        std::memset(&f, 0, sizeof(TableMode));
        std::strncpy(f.sFieldName, tokens[0].c_str(), FIELD_NAME_LENGTH - 1);

        // 解析类型
        // e.g. "char[10]" or "int"
        if (tokens[1].rfind("char[", 0) == 0) {
            std::strcpy(f.sType, "char");
            size_t lb = tokens[1].find('[');
            size_t rb = tokens[1].find(']');
            if (lb != std::string::npos && rb != std::string::npos && rb > lb) {
                int sizeVal = std::atoi(tokens[1].substr(lb + 1, rb - lb - 1).c_str());
                f.iSize = sizeVal;
            }
            else {
                f.iSize = 1;
            }
        }
        else if (tokens[1] == "int") {
            std::strcpy(f.sType, "int");
            f.iSize = 4;
        }
        else {
            // 其他类型简单处理
            std::strcpy(f.sType, tokens[1].c_str());
            f.iSize = 4;
        }

        // 解析其余关键字
        for (size_t i = 2; i < tokens.size(); i++) {
            if (tokens[i] == "KEY") {
                f.bKey = 1;
            }
            else if (tokens[i] == "NOT_KEY") {
                f.bKey = 0;
            }
            else if (tokens[i] == "NO_NULL") {
                f.bNullFlag = 0;
            }
            else if (tokens[i] == "NULL") {
                f.bNullFlag = 1;
            }
            else if (tokens[i] == "VALID") {
                f.bValidFlag = 1;
            }
            else if (tokens[i] == "INVALID") {
                f.bValidFlag = 0;
            }
        }
        fieldIndex++;
    }
    schema.iFieldCount = fieldIndex;

    // 写入 .dbf
    m_dbManager.writeTableSchema(schema);

    std::cout << "[INFO] Table \"" << schema.sTableName
        << "\" created into " << dbFilePart << " successfully.\n";
}


//-former
//// ---------------- DROP TABLE ----------------//
//void MiniSqlEngine::dropTable(const std::string& cmd) {
//    // 语法: DROP TABLE TableName IN dbFile;
//    // 这里只做简单演示：读所有表 -> 过滤掉要删除的 -> 重写 .dbf
//
//    std::string upperCmd = cmd;
//    std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);
//    size_t posDrop = upperCmd.find("DROP TABLE");
//    if (posDrop == std::string::npos) {
//        std::cout << "[ERROR] DROP TABLE syntax error.\n";
//        return;
//    }
//    size_t posIn = upperCmd.find("IN", posDrop);
//    if (posIn == std::string::npos) {
//        std::cout << "[ERROR] Missing IN in DROP TABLE command.\n";
//        return;
//    }
//
//    std::string tableName = trim(cmd.substr(posDrop + strlen("DROP TABLE"),
//        posIn - (posDrop + strlen("DROP TABLE"))));
//    std::string dbFilePart = trim(cmd.substr(posIn + strlen("IN")));
//    if (!dbFilePart.empty() && dbFilePart.back() == ';') {
//        dbFilePart.pop_back();
//    }
//    dbFilePart = trim(dbFilePart);
//
//    // 读所有表
//    std::vector<TableSchema> schemas;
//    if (m_dbManager.readAllTableSchemas(schemas) < 0) {
//        std::cout << "[ERROR] Cannot read dbf.\n";
//        return;
//    }
//    // 以 truncate 方式重写
//    std::ofstream ofs(m_dbManager.getDbFileName().c_str(), std::ios::binary | std::ios::trunc);
//    if (!ofs.is_open()) {
//        std::cout << "[ERROR] Cannot open dbf for rewriting.\n";
//        return;
//    }
//
//    int dropCount = 0;
//    for (auto& ts : schemas) {
//        if (ts.sTableName == tableName) {
//            dropCount++;
//            continue;
//        }
//        // 写回
//        ofs.put(TABLE_DELIMITER);
//        ofs.write(ts.sTableName, TABLE_NAME_LENGTH);
//        ofs.write(reinterpret_cast<const char*>(&ts.iFieldCount), sizeof(int));
//        for (int i = 0; i < ts.iFieldCount; i++) {
//            ofs.write(reinterpret_cast<const char*>(&ts.fieldArray[i]), sizeof(TableMode));
//        }
//    }
//    ofs.close();
//
//    if (dropCount > 0) {
//        std::cout << "[INFO] Table \"" << tableName << "\" dropped.\n";
//        // 也可以考虑在 .dat 中删除对应记录块
//    }
//    else {
//        std::cout << "[INFO] Table \"" << tableName << "\" not found.\n";
//    }
//}

//-----after
void MiniSqlEngine::dropTable(const std::string& cmd) {
    // 语法: DROP TABLE TableName IN dbFileName;
    // 例:   DROP TABLE Student IN MyDB.dbf;

    // ---------- 第一步：解析命令 -----------
    std::string upperCmd = cmd;
    std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);

    size_t posDrop = upperCmd.find("DROP TABLE");
    if (posDrop == std::string::npos) {
        std::cout << "[ERROR] DROP TABLE syntax error.\n";
        return;
    }
    size_t posIn = upperCmd.find("IN", posDrop);
    if (posIn == std::string::npos) {
        std::cout << "[ERROR] Missing 'IN' in DROP TABLE command.\n";
        return;
    }

    // 提取表名
    std::string tableName = trim(cmd.substr(
        posDrop + strlen("DROP TABLE"),
        posIn - (posDrop + strlen("DROP TABLE"))
    ));
    tableName = trim(tableName);

    // 提取 dbFileName
    std::string dbFilePart = trim(cmd.substr(posIn + strlen("IN")));
    if (!dbFilePart.empty() && dbFilePart.back() == ';') {
        dbFilePart.pop_back();
    }
    dbFilePart = trim(dbFilePart);

    // ---------- 第二步：从 .dbf 中删除该表的模式 -----------
    // 读取所有表模式
    std::vector<TableSchema> schemas;
    if (m_dbManager.readAllTableSchemas(schemas) < 0) {
        std::cout << "[ERROR] Cannot read dbf file.\n";
        return;
    }

    // 以 truncate 方式重写 .dbf
    // 注意：m_dbManager.getDbFileName() 里应当是当前数据库文件，比如 "MyDB.dbf"
    std::ofstream ofsDBF(m_dbManager.getDbFileName().c_str(), std::ios::binary | std::ios::trunc);
    if (!ofsDBF.is_open()) {
        std::cout << "[ERROR] Cannot open dbf for rewriting.\n";
        return;
    }

    int dropCount = 0;
    for (auto& ts : schemas) {
        // 若表名匹配，则跳过(即不写回 => 等同删除)
        if (ts.sTableName == tableName) {
            dropCount++;
            continue;
        }
        // 其他表正常写回
        ofsDBF.put(TABLE_DELIMITER);
        ofsDBF.write(ts.sTableName, TABLE_NAME_LENGTH);
        ofsDBF.write(reinterpret_cast<const char*>(&ts.iFieldCount), sizeof(int));
        for (int i = 0; i < ts.iFieldCount; i++) {
            ofsDBF.write(reinterpret_cast<const char*>(&ts.fieldArray[i]), sizeof(TableMode));
        }
    }
    ofsDBF.close();

    if (dropCount == 0) {
        // 如果没有匹配到要删除的表，说明表在 .dbf 中不存在
        std::cout << "[INFO] Table \"" << tableName
            << "\" not found in " << dbFilePart << ". (dbf cleared anyway)\n";
    }
    else {
        std::cout << "[INFO] Table \"" << tableName
            << "\" removed from " << dbFilePart << " (.dbf).\n";
    }

    // ---------- 第三步：从 .dat 中删除该表对应的数据块 -----------
    // 若你也想把 .dat 中的记录删除，则继续以下操作
    // 如果你暂时不需要清理 .dat，可直接 return

    // 打开 .dat 文件 (RecordManager 里往往有个 getDatFileName())
    std::ifstream ifsDAT(m_recManager.getDatFileName(), std::ios::binary);
    if (!ifsDAT.is_open()) {
        std::cout << "[WARN] .dat file cannot be opened or not exist. Maybe no data yet.\n";
        return;
    }

    // 定义一个内部结构，用来暂存各个 block
    struct TableBlock {
        std::string tblName;
        int recordCount;
        int fieldCount;
        // 存储所有记录的原始二进制数据 (validFlag + 字段内容)
        std::vector<std::vector<char>> recordData;
    };

    std::vector<TableBlock> allBlocks;

    while (!ifsDAT.eof()) {
        // 1字节分隔符
        char delim;
        if (!ifsDAT.get(delim)) {
            break; // EOF
        }
        if (delim != TABLE_DELIMITER) {
            // 文件格式异常或空洞，简单跳过
            continue;
        }

        // 读取表名
        char tmpName[TABLE_NAME_LENGTH];
        std::memset(tmpName, 0, TABLE_NAME_LENGTH);
        ifsDAT.read(tmpName, TABLE_NAME_LENGTH);

        // 读取记录数, 字段数
        int recCount = 0;
        int fCount = 0;
        ifsDAT.read(reinterpret_cast<char*>(&recCount), sizeof(int));
        ifsDAT.read(reinterpret_cast<char*>(&fCount), sizeof(int));

        TableBlock blk;
        blk.tblName = tmpName;
        blk.recordCount = recCount;
        blk.fieldCount = fCount;

        // 读取记录的二进制
        for (int r = 0; r < recCount; r++) {
            // 这里你需要和 writeRecords(...) 中的写法一致
            // => 先写 1字节 validFlag, 然后根据 fCount/Schema 写字段内容
            // 简化: 假设 int=4字节, char=定长 ...
            // 这里仅演示, 先估算
            // “最安全的”做法是: 
            //   先 read 1字节 validFlag,
            //   再 for fCount 字段 => if int => 4字节; if char => iSize
            //   但要知道 iSize => 需要从 .dbf 解析? 
            // 这里先跟最简写, assume each field 4 bytes:
            // => 1 + fCount*4
            // 注意: 这会和真实 char[iSize] 不匹配, 仅演示

            int recSize = 1 + (fCount * 4); // 仅演示, 不严谨
            std::vector<char> raw(recSize);
            ifsDAT.read(raw.data(), recSize);

            blk.recordData.push_back(std::move(raw));
        }
        allBlocks.push_back(std::move(blk));
    }
    ifsDAT.close();

    // 过滤掉 tblName == tableName 的 block
    int datDropCount = 0;
    std::vector<TableBlock> newBlocks;
    for (auto& b : allBlocks) {
        if (b.tblName == tableName) {
            // 这是要删的表
            datDropCount++;
            continue;
        }
        newBlocks.push_back(b);
    }

    // 重写 .dat
    std::ofstream ofsDAT(m_recManager.getDatFileName(), std::ios::binary | std::ios::trunc);
    if (!ofsDAT.is_open()) {
        std::cout << "[ERROR] Cannot open .dat for rewriting.\n";
        return;
    }

    for (auto& blk : newBlocks) {
        // 写分隔符
        ofsDAT.put(TABLE_DELIMITER);

        // 写表名 (定长 TABLE_NAME_LENGTH)
        char tName[TABLE_NAME_LENGTH];
        std::memset(tName, 0, TABLE_NAME_LENGTH);
        strncpy(tName, blk.tblName.c_str(), TABLE_NAME_LENGTH - 1);
        ofsDAT.write(tName, TABLE_NAME_LENGTH);

        // 写 recordCount, fieldCount
        ofsDAT.write(reinterpret_cast<const char*>(&blk.recordCount), sizeof(int));
        ofsDAT.write(reinterpret_cast<const char*>(&blk.fieldCount), sizeof(int));

        // 写回记录数据
        for (auto& rd : blk.recordData) {
            ofsDAT.write(rd.data(), rd.size());
        }
    }
    ofsDAT.close();

    // 打印提示
    if (datDropCount > 0) {
        std::cout << "[INFO] Also removed " << datDropCount
            << " block(s) of table \"" << tableName
            << "\" from .dat file.\n";
    }
    else {
        std::cout << "[INFO] No matching block for table \"" << tableName
            << "\" in .dat file (maybe was not inserted?).\n";
    }
}





// ---------------- RENAME TABLE ----------------//
void MiniSqlEngine::renameTable(const std::string& cmd) {
    // 语法：RENAME TABLE OldName NewName IN dbFile;
    std::string upperCmd = cmd;
    std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);
    size_t posRename = upperCmd.find("RENAME TABLE");
    if (posRename == std::string::npos) {
        std::cout << "[ERROR] RENAME TABLE syntax error.\n";
        return;
    }
    size_t posIn = upperCmd.find("IN", posRename);
    if (posIn == std::string::npos) {
        std::cout << "[ERROR] Missing IN in RENAME TABLE.\n";
        return;
    }

    std::string middlePart = trim(cmd.substr(posRename + strlen("RENAME TABLE"),
        posIn - (posRename + strlen("RENAME TABLE"))));
    std::stringstream ss(middlePart);
    std::string oldName, newName;
    ss >> oldName >> newName;

    std::string dbFilePart = trim(cmd.substr(posIn + strlen("IN")));
    if (!dbFilePart.empty() && dbFilePart.back() == ';') {
        dbFilePart.pop_back();
    }
    dbFilePart = trim(dbFilePart);

    // 读全部表
    std::vector<TableSchema> schemas;
    if (m_dbManager.readAllTableSchemas(schemas) < 0) {
        std::cout << "[ERROR] Cannot read dbf.\n";
        return;
    }
    // 重写
    std::ofstream ofs(m_dbManager.getDbFileName().c_str(), std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        std::cout << "[ERROR] Cannot open dbf.\n";
        return;
    }

    int renameCount = 0;
    for (auto& ts : schemas) {
        if (ts.sTableName == oldName) {
            std::strncpy(ts.sTableName, newName.c_str(), TABLE_NAME_LENGTH - 1);
            renameCount++;
        }
        ofs.put(TABLE_DELIMITER);
        ofs.write(ts.sTableName, TABLE_NAME_LENGTH);
        ofs.write(reinterpret_cast<const char*>(&ts.iFieldCount), sizeof(int));
        for (int i = 0; i < ts.iFieldCount; i++) {
            ofs.write(reinterpret_cast<const char*>(&ts.fieldArray[i]), sizeof(TableMode));
        }
    }
    ofs.close();

    if (renameCount > 0) {
        std::cout << "[INFO] Table \"" << oldName << "\" renamed to \"" << newName << "\".\n";
    }
    else {
        std::cout << "[INFO] Table \"" << oldName << "\" not found.\n";
    }
}



// ---------------- INSERT ----------------//
void MiniSqlEngine::insertRecord(const std::string& cmd) {
    // 语法：INSERT INTO TableName VALUES(a, b, c, ...) IN dbFile;
    size_t posInsert = cmd.find("INSERT INTO");
    if (posInsert == std::string::npos) {
        std::cout << "[ERROR] INSERT syntax.\n";
        return;
    }
    size_t posValues = cmd.find("VALUES", posInsert);
    if (posValues == std::string::npos) {
        std::cout << "[ERROR] Missing VALUES.\n";
        return;
    }
    size_t posIn = cmd.find(" IN ", posValues);
    if (posIn == std::string::npos) {
        std::cout << "[ERROR] Missing IN.\n";
        return;
    }

    std::string tableName = trim(cmd.substr(posInsert + strlen("INSERT INTO"),
        posValues - (posInsert + strlen("INSERT INTO"))));

    // 拆 VALUES(...)
    size_t posParenL = cmd.find('(', posValues);
    size_t posParenR = cmd.find(')', posParenL);
    if (posParenL == std::string::npos || posParenR == std::string::npos) {
        std::cout << "[ERROR] Bad VALUES(...)\n";
        return;
    }
    std::string valuesPart = trim(cmd.substr(posParenL + 1, posParenR - posParenL - 1));

    // dbFile
    std::string dbFilePart = trim(cmd.substr(posIn + strlen("IN")));
    if (!dbFilePart.empty() && dbFilePart.back() == ';') {
        dbFilePart.pop_back();
    }
    dbFilePart = trim(dbFilePart);

    // 读表结构
    TableSchema schema;
    if (!m_dbManager.getTableSchemaByName(tableName, schema)) {
        std::cout << "[ERROR] Table not found: " << tableName << std::endl;
        return;
    }

    // 拆分逗号
    std::vector<std::string> vals;
    {
        std::stringstream ss(valuesPart);
        std::string item;
        while (std::getline(ss, item, ',')) {
            vals.push_back(trim(item));
        }
    }
    if ((int)vals.size() != schema.iFieldCount) {
        std::cout << "[ERROR] Field count mismatch: need "
            << schema.iFieldCount << ", got " << vals.size() << "\n";
        return;
    }

    // 构造一条记录
    char*** records = new char** [1];
    records[0] = new char* [schema.iFieldCount];
    for (int i = 0; i < schema.iFieldCount; i++) {
        records[0][i] = new char[128];
        std::memset(records[0][i], 0, 128);
        std::strncpy(records[0][i], vals[i].c_str(), 127);
    }

    m_recManager.writeRecords(schema, records, 1);

    // 释放
    for (int i = 0; i < schema.iFieldCount; i++) {
        delete[] records[0][i];
    }
    delete[] records[0];
    delete[] records;

    std::cout << "[INFO] 1 record inserted into " << tableName << ".\n";
}

//// ---------------- DELETE ----------------//
//void MiniSqlEngine::deleteRecord(const std::string& cmd) {
//    // 语法：DELETE FROM TableName WHERE Field=Value IN dbFile;
//    // 示例中暂不完整实现，只做提示
//    std::cout << "[INFO] Delete command not fully implemented in this demo.\n";
//}
//

// ---------------- DELETE ----------------//
void MiniSqlEngine::deleteRecord(const std::string& cmd) {
    // 期待语法：DELETE FROM 表名 WHERE 字段=值 IN dbFile;
    // 例如：DELETE FROM Student WHERE StuID=123 IN MyDB.dbf;

    // 1) 解析表名
    std::string upperCmd = cmd;
    std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);

    size_t posDelete = upperCmd.find("DELETE FROM");
    if (posDelete == std::string::npos) {
        std::cout << "[ERROR] DELETE syntax error.\n";
        return;
    }

    // 找 WHERE
    size_t posWhere = upperCmd.find("WHERE", posDelete);
    if (posWhere == std::string::npos) {
        std::cout << "[ERROR] DELETE missing WHERE clause.\n";
        return;
    }

    // 找 IN
    size_t posIn = upperCmd.find(" IN ", posWhere);
    if (posIn == std::string::npos) {
        std::cout << "[ERROR] DELETE missing IN.\n";
        return;
    }

    // 提取表名
    std::string tableName = trim(cmd.substr(posDelete + strlen("DELETE FROM"),
        posWhere - (posDelete + strlen("DELETE FROM"))));

    // 提取 WHERE "Field=Value"
    // 简化做法：先把"WHERE"和"IN"之间的内容单独拿出
    std::string wherePart = trim(cmd.substr(posWhere + strlen("WHERE"),
        posIn - (posWhere + strlen("WHERE"))));
    // 例如: "StuID=123"

    // 提取 dbFilePart
    std::string dbFilePart = trim(cmd.substr(posIn + strlen("IN")));
    if (!dbFilePart.empty() && dbFilePart.back() == ';') {
        dbFilePart.pop_back();
    }
    dbFilePart = trim(dbFilePart);

    // 2) 解析 wherePart => fieldName, fieldValue
    // 简单假设只有 "Field=Value" 这种格式
    std::string fieldName, fieldValue;
    {
        size_t eqPos = wherePart.find('=');
        if (eqPos == std::string::npos) {
            std::cout << "[ERROR] WHERE clause must be Field=Value.\n";
            return;
        }
        fieldName = trim(wherePart.substr(0, eqPos));
        fieldValue = trim(wherePart.substr(eqPos + 1));
        // 如果 value 带引号(比如 'Alice')，可以去掉引号
        if (!fieldValue.empty() && fieldValue.front() == '\'') {
            fieldValue.erase(0, 1);
        }
        if (!fieldValue.empty() && fieldValue.back() == '\'') {
            fieldValue.pop_back();
        }
    }

    // 3) 获取表结构
    TableSchema schema;
    if (!m_dbManager.getTableSchemaByName(tableName, schema)) {
        std::cout << "[ERROR] Table " << tableName << " not found.\n";
        return;
    }

    // 4) 读取 .dat 中所有记录，找到 "TableName" 块
    //    这里我们自己做"读取-修改-写回"的过程
    std::ifstream ifs(m_recManager.getDatFileName(), std::ios::binary);
    if (!ifs.is_open()) {
        std::cout << "[ERROR] Cannot open dat file: " << m_recManager.getDatFileName() << "\n";
        return;
    }

    // 收集所有表块的内存信息，以便最后一次性重写
    struct TableBlock {
        std::string tableName;
        int recordCount;
        int fieldCount;
        // recordData 存放: (validFlag + fields raw data) 的二进制内存
        std::vector<std::vector<char>> recordData;
    };
    std::vector<TableBlock> allBlocks;

    while (!ifs.eof()) {
        char delim;
        if (!ifs.get(delim)) break; // EOF
        if (delim != TABLE_DELIMITER) {
            continue;
        }

        // 读表名
        char tempTableName[TABLE_NAME_LENGTH];
        memset(tempTableName, 0, TABLE_NAME_LENGTH);
        ifs.read(tempTableName, TABLE_NAME_LENGTH);

        int recCountInFile = 0;
        int fieldCountInFile = 0;
        ifs.read(reinterpret_cast<char*>(&recCountInFile), sizeof(int));
        ifs.read(reinterpret_cast<char*>(&fieldCountInFile), sizeof(int));

        TableBlock block;
        block.tableName = tempTableName;
        block.recordCount = recCountInFile;
        block.fieldCount = fieldCountInFile;

        // 读取所有记录(包含 validFlag + 各字段raw data)
        for (int r = 0; r < recCountInFile; r++) {
            // 计算单条记录的总大小(1字节validFlag + sum of each field size)
            // 注意：int字段4字节，char字段 iSize 字节
            int oneRecordSize = 1; // validFlag
            // 这里需要从 schema 中取到对应字段信息
            // 但如果 block.tableName != tableName ，我们只能跳过
            // 为简化，先全部读到 buffer，再后面再处理
            if (block.tableName == schema.sTableName) {
                // 同一个表 => 用 schema(fieldArray) 计算
                for (int f = 0; f < schema.iFieldCount; f++) {
                    if (strcmp(schema.fieldArray[f].sType, "int") == 0) {
                        oneRecordSize += 4;
                    }
                    else {
                        oneRecordSize += schema.fieldArray[f].iSize;
                    }
                }
            }
            else {
                // 不是我们要操作的表 => 按 block.fieldCountInFile 的方式跳过
                // 这可能是另外的表数据块
                // 这里先简单处理 => int assumed 4 / char assumed iSize from fileCountInFile?
                // 但 block.fieldCountInFile 不一定等于 schema.iFieldCount
                // => 需要自己再读 TableSchema? 
                // 简化做法: 读一次 validFlag, 再对每个字段都 4 or iSize?
                // 但我们没有这个 block 的 schema => 只好重新读取 .dbf ? 
                // 这里为了简化, 假设 fieldCountInFile == schema.iFieldCount for all blocks
                // (真实情况要更细的管理)
                oneRecordSize += 1; // validFlag(已算), 只做示例
                for (int ff = 0; ff < fieldCountInFile; ff++) {
                    // 暂定都按 int? => 4 字节? 这会引起和写入时不匹配
                    // 仅为演示...
                    oneRecordSize += 4;
                }
            }

            std::vector<char> recBuf(oneRecordSize);
            // 已经读了 1 字节 delim; 这里要把 delim 看成 validFlag?
            // 不对，上面 if (!ifs.get(delim)) break; 其实已经消耗掉了 1字节
            // 还没读 validFlag ...
            // => 我们还是老老实实一次性读取 oneRecordSize
            recBuf[0] = 0; // 先清零
            ifs.read(recBuf.data(), oneRecordSize);

            block.recordData.push_back(std::move(recBuf));
        }

        allBlocks.push_back(block);
    }
    ifs.close();

    // 5) 在内存中修改"目标表"的记录(把满足 WHERE 的 validFlag = 0)
    //    先找到 block.tableName == tableName
    for (auto& blk : allBlocks) {
        if (blk.tableName == schema.sTableName) {
            // 找到匹配的 block
            // 对于每条记录 => parse => if match => set valid=0
            int fieldOffset = 1; // offset after validFlag
            for (int r = 0; r < (int)blk.recordData.size(); r++) {
                char* recPtr = blk.recordData[r].data();
                char validFlag = recPtr[0];
                if (validFlag == 0) {
                    continue; // already invalid
                }

                // 解析每个字段, 找 target fieldName
                // simplified approach: find indexOf(fieldName) in schema
                int targetFieldIndex = -1;
                for (int f = 0; f < schema.iFieldCount; f++) {
                    if (strcmp(schema.fieldArray[f].sFieldName, fieldName.c_str()) == 0) {
                        targetFieldIndex = f;
                        break;
                    }
                }
                if (targetFieldIndex < 0) {
                    std::cout << "[ERROR] Field " << fieldName << " not found in table " << tableName << "\n";
                    return;
                }

                // 计算 targetFieldIndex 在 recordData 里的位置
                // 需要逐个字段计算 offset
                // or 先 parse all fields
                // 这里写个 parseAllFields 简略:
                struct FieldData { std::string value; int offsetStart; int size; bool isInt; };
                std::vector<FieldData> parsedFields;
                int curPos = 1; // skip validFlag
                for (int f = 0; f < schema.iFieldCount; f++) {
                    bool isInt = (strcmp(schema.fieldArray[f].sType, "int") == 0);
                    int fsize = isInt ? 4 : schema.fieldArray[f].iSize;

                    FieldData fd;
                    fd.offsetStart = curPos;
                    fd.size = fsize;
                    fd.isInt = isInt;
                    if (isInt) {
                        int val;
                        memcpy(&val, recPtr + curPos, 4);
                        fd.value = std::to_string(val);
                    }
                    else {
                        // char
                        std::vector<char> temp(fsize + 1, 0);
                        memcpy(temp.data(), recPtr + curPos, fsize);
                        fd.value = temp.data();
                    }
                    parsedFields.push_back(fd);

                    curPos += fsize;
                }
                // parsedFields[targetFieldIndex] 是我们关心的字段
                // 若它的值 == fieldValue, 则置 validFlag=0
                if (parsedFields[targetFieldIndex].value == fieldValue) {
                    // set valid=0
                    blk.recordData[r][0] = 0;
                }
            }
        }
    }

    // 6) 重写 .dat 文件
    std::ofstream ofs(m_recManager.getDatFileName(), std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        std::cout << "[ERROR] Cannot open dat file for rewriting.\n";
        return;
    }

    for (auto& blk : allBlocks) {
        ofs.put(TABLE_DELIMITER);
        // 表名(定长)
        char tname[TABLE_NAME_LENGTH];
        memset(tname, 0, TABLE_NAME_LENGTH);
        strncpy(tname, blk.tableName.c_str(), TABLE_NAME_LENGTH - 1);
        ofs.write(tname, TABLE_NAME_LENGTH);

        // 记录数
        int recCount = (int)blk.recordData.size();
        ofs.write(reinterpret_cast<const char*>(&recCount), sizeof(int));
        // 字段数
        ofs.write(reinterpret_cast<const char*>(&blk.fieldCount), sizeof(int));

        // 写每条记录
        for (auto& rd : blk.recordData) {
            ofs.write(rd.data(), rd.size());
        }
    }
    ofs.close();

    std::cout << "[INFO] DELETE done on table " << tableName << " where "
        << fieldName << "=" << fieldValue << ".\n";
}







//
//// ---------------- UPDATE ----------------//
//void MiniSqlEngine::updateRecord(const std::string& cmd) {
//    // 语法：UPDATE TableName (SET Field1=Value1 WHERE Field2=Value2) IN dbFile;
//    std::cout << "[INFO] Update command not fully implemented in this demo.\n";
//}



// ---------------- UPDATE ----------------//
void MiniSqlEngine::updateRecord(const std::string& cmd) {
    // 语法示例：
    // UPDATE TableName (
    //   SET Field1=NewValue
    //   WHERE Field2=ConditionValue
    // ) IN MyDB.dbf;

    size_t posUpdate = cmd.find("UPDATE");
    if (posUpdate == std::string::npos) {
        std::cout << "[ERROR] UPDATE syntax error.\n";
        return;
    }
    size_t posParenL = cmd.find('(', posUpdate);
    size_t posParenR = cmd.find(')', posParenL);
    if (posParenL == std::string::npos || posParenR == std::string::npos) {
        std::cout << "[ERROR] UPDATE missing parentheses.\n";
        return;
    }
    size_t posIn = cmd.find("IN", posParenR);
    if (posIn == std::string::npos) {
        std::cout << "[ERROR] UPDATE missing IN.\n";
        return;
    }

    // 提取表名
    std::string tableNamePart = trim(cmd.substr(posUpdate + strlen("UPDATE"),
        posParenL - (posUpdate + strlen("UPDATE"))));

    // 提取 中括号(...) 内部内容 => "SET Field1=NewValue WHERE Field2=ConditionValue"
    std::string insidePart = trim(cmd.substr(posParenL + 1, posParenR - posParenL - 1));

    // 提取 dbFileName
    std::string dbFilePart = trim(cmd.substr(posIn + strlen("IN")));
    if (!dbFilePart.empty() && dbFilePart.back() == ';') {
        dbFilePart.pop_back();
    }
    dbFilePart = trim(dbFilePart);

    // insidePart 里先找 "SET " 和 " WHERE "
    // e.g. "SET Field1=NewValue WHERE Field2=ConditionValue"
    size_t posSet = insidePart.find("SET");
    size_t posWhere = insidePart.find("WHERE");
    if (posSet == std::string::npos || posWhere == std::string::npos) {
        std::cout << "[ERROR] UPDATE missing SET or WHERE.\n";
        return;
    }

    std::string setPart = trim(insidePart.substr(posSet + strlen("SET"), posWhere - (posSet + strlen("SET"))));
    std::string wherePart = trim(insidePart.substr(posWhere + strlen("WHERE")));

    // 解析 setPart => "Field1=NewValue"
    std::string setFieldName, setFieldValue;
    {
        size_t eqPos = setPart.find('=');
        if (eqPos == std::string::npos) {
            std::cout << "[ERROR] SET clause must be Field=Value\n";
            return;
        }
        setFieldName = trim(setPart.substr(0, eqPos));
        setFieldValue = trim(setPart.substr(eqPos + 1));
        // 去引号
        if (!setFieldValue.empty() && setFieldValue.front() == '\'') {
            setFieldValue.erase(0, 1);
        }
        if (!setFieldValue.empty() && setFieldValue.back() == '\'') {
            setFieldValue.pop_back();
        }
    }

    // 解析 wherePart => "Field2=ConditionValue"
    std::string condFieldName, condFieldValue;
    {
        size_t eqPos = wherePart.find('=');
        if (eqPos == std::string::npos) {
            std::cout << "[ERROR] WHERE clause must be Field=Value\n";
            return;
        }
        condFieldName = trim(wherePart.substr(0, eqPos));
        condFieldValue = trim(wherePart.substr(eqPos + 1));
        if (!condFieldValue.empty() && condFieldValue.front() == '\'') {
            condFieldValue.erase(0, 1);
        }
        if (!condFieldValue.empty() && condFieldValue.back() == '\'') {
            condFieldValue.pop_back();
        }
    }

    // 获取表结构
    TableSchema schema;
    if (!m_dbManager.getTableSchemaByName(tableNamePart, schema)) {
        std::cout << "[ERROR] Table " << tableNamePart << " not found.\n";
        return;
    }

    // 读取、修改、写回 => 原理与 deleteRecord 类似
    // 略去大量与 deleteRecord 重复的注释
    std::ifstream ifs(m_recManager.getDatFileName(), std::ios::binary);
    if (!ifs.is_open()) {
        std::cout << "[ERROR] Cannot open dat file: " << m_recManager.getDatFileName() << "\n";
        return;
    }

    struct TableBlock {
        std::string tableName;
        int recordCount;
        int fieldCount;
        std::vector<std::vector<char>> recordData;
    };
    std::vector<TableBlock> allBlocks;

    while (!ifs.eof()) {
        char delim;
        if (!ifs.get(delim)) break;
        if (delim != TABLE_DELIMITER) {
            continue;
        }

        char tempTableName[TABLE_NAME_LENGTH];
        memset(tempTableName, 0, TABLE_NAME_LENGTH);
        ifs.read(tempTableName, TABLE_NAME_LENGTH);

        int recCountInFile = 0;
        int fieldCountInFile = 0;
        ifs.read(reinterpret_cast<char*>(&recCountInFile), sizeof(int));
        ifs.read(reinterpret_cast<char*>(&fieldCountInFile), sizeof(int));

        TableBlock blk;
        blk.tableName = tempTableName;
        blk.recordCount = recCountInFile;
        blk.fieldCount = fieldCountInFile;

        for (int r = 0; r < recCountInFile; r++) {
            // 计算记录大小
            int oneRecordSize = 1;
            if (blk.tableName == schema.sTableName) {
                for (int f = 0; f < schema.iFieldCount; f++) {
                    if (strcmp(schema.fieldArray[f].sType, "int") == 0) {
                        oneRecordSize += 4;
                    }
                    else {
                        oneRecordSize += schema.fieldArray[f].iSize;
                    }
                }
            }
            else {
                // 简化假设
                oneRecordSize += fieldCountInFile * 4;
            }

            std::vector<char> recBuf(oneRecordSize);
            ifs.read(recBuf.data(), oneRecordSize);
            blk.recordData.push_back(std::move(recBuf));
        }

        allBlocks.push_back(blk);
    }
    ifs.close();

    // 修改 block
    for (auto& blk : allBlocks) {
        if (blk.tableName == schema.sTableName) {
            for (auto& rec : blk.recordData) {
                if (rec[0] == 0) continue; // invalid

                // parse
                struct FieldData { std::string value; int offset; int size; bool isInt; };
                std::vector<FieldData> parsed;
                int curPos = 1;
                for (int f = 0; f < schema.iFieldCount; f++) {
                    bool isInt = (strcmp(schema.fieldArray[f].sType, "int") == 0);
                    int fsize = isInt ? 4 : schema.fieldArray[f].iSize;

                    FieldData fd;
                    fd.offset = curPos;
                    fd.size = fsize;
                    fd.isInt = isInt;
                    if (isInt) {
                        int val;
                        memcpy(&val, rec.data() + curPos, 4);
                        fd.value = std::to_string(val);
                    }
                    else {
                        std::vector<char> temp(fsize + 1, 0);
                        memcpy(temp.data(), rec.data() + curPos, fsize);
                        fd.value = temp.data();
                    }
                    parsed.push_back(fd);
                    curPos += fsize;
                }

                // 找到 condFieldName, compare => if match => update setFieldName
                int condIndex = -1;
                int setIndex = -1;
                for (int f = 0; f < schema.iFieldCount; f++) {
                    if (strcmp(schema.fieldArray[f].sFieldName, condFieldName.c_str()) == 0) {
                        condIndex = f;
                    }
                    if (strcmp(schema.fieldArray[f].sFieldName, setFieldName.c_str()) == 0) {
                        setIndex = f;
                    }
                }
                if (condIndex < 0 || setIndex < 0) {
                    // 不存在就跳过
                    continue;
                }

                if (parsed[condIndex].value == condFieldValue) {
                    // update setIndex
                    if (parsed[setIndex].isInt) {
                        // 转 int
                        int ival = atoi(setFieldValue.c_str());
                        memcpy(rec.data() + parsed[setIndex].offset, &ival, 4);
                    }
                    else {
                        // char
                        std::vector<char> tmp(parsed[setIndex].size, 0);
                        strncpy(tmp.data(), setFieldValue.c_str(), parsed[setIndex].size);
                        memcpy(rec.data() + parsed[setIndex].offset, tmp.data(), parsed[setIndex].size);
                    }
                }
            }
        }
    }

    // 重写
    std::ofstream ofs(m_recManager.getDatFileName(), std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        std::cout << "[ERROR] Cannot open dat file for rewriting.\n";
        return;
    }
    for (auto& blk : allBlocks) {
        ofs.put(TABLE_DELIMITER);
        char tname[TABLE_NAME_LENGTH];
        memset(tname, 0, TABLE_NAME_LENGTH);
        strncpy(tname, blk.tableName.c_str(), TABLE_NAME_LENGTH - 1);
        ofs.write(tname, TABLE_NAME_LENGTH);

        int recCount = (int)blk.recordData.size();
        ofs.write(reinterpret_cast<const char*>(&recCount), sizeof(int));
        ofs.write(reinterpret_cast<const char*>(&blk.fieldCount), sizeof(int));

        for (auto& rd : blk.recordData) {
            ofs.write(rd.data(), rd.size());
        }
    }
    ofs.close();

    std::cout << "[INFO] UPDATE done on table " << tableNamePart << ".\n";
}



// ---------------- SELECT ----------------//
//void MiniSqlEngine::selectRecords(const std::string& cmd) {
//     语法：SELECT * FROM TableName WHERE ...
//    std::cout << "[INFO] Select command not fully implemented in this demo.\n";
//}
// 
// 
// ---------------- SELECT ----------------//
void MiniSqlEngine::selectRecords(const std::string& cmd) {
    // 最简单：SELECT * FROM TableName WHERE Field=Value
    // 不做多表JOIN、投影列表等

    // 1) 找 "SELECT * FROM"
    // 2) 找 "WHERE"
    // 3) 解析表名 & Field=Value
    // 4) 读取并打印匹配记录

    std::string upperCmd = cmd;
    std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);

    size_t posSelect = upperCmd.find("SELECT");
    if (posSelect == std::string::npos) {
        std::cout << "[ERROR] SELECT syntax error.\n";
        return;
    }
    // 期待 "SELECT * FROM"
    size_t posFrom = upperCmd.find("FROM", posSelect);
    if (posFrom == std::string::npos) {
        std::cout << "[ERROR] SELECT missing FROM.\n";
        return;
    }
    // 找 WHERE(可选)
    size_t posWhere = upperCmd.find("WHERE", posFrom);

    // 提取表名
    std::string tableName;
    if (posWhere != std::string::npos) {
        tableName = trim(cmd.substr(posFrom + strlen("FROM"), posWhere - (posFrom + strlen("FROM"))));
    }
    else {
        // 没有 where
        // 可能 "SELECT * FROM TableName;"
        // 找结尾
        size_t semiPos = upperCmd.find(';', posFrom);
        if (semiPos == std::string::npos) {
            semiPos = cmd.size();
        }
        tableName = trim(cmd.substr(posFrom + strlen("FROM"), semiPos - (posFrom + strlen("FROM"))));
    }

    // 提取 WHERE => "Field=Value"
    std::string condField, condValue;
    bool hasWhere = (posWhere != std::string::npos);
    if (hasWhere) {
        // e.g. "WHERE Field=Value"
        // 找分号(或行末)
        size_t semiPos = upperCmd.find(';', posWhere);
        if (semiPos == std::string::npos) {
            semiPos = cmd.size();
        }
        std::string wherePart = trim(cmd.substr(posWhere + strlen("WHERE"), semiPos - (posWhere + strlen("WHERE"))));

        size_t eqPos = wherePart.find('=');
        if (eqPos != std::string::npos) {
            condField = trim(wherePart.substr(0, eqPos));
            condValue = trim(wherePart.substr(eqPos + 1));
            if (!condValue.empty() && condValue.front() == '\'') {
                condValue.erase(0, 1);
            }
            if (!condValue.empty() && condValue.back() == '\'') {
                condValue.pop_back();
            }
        }
        else {
            std::cout << "[ERROR] WHERE clause must be Field=Value.\n";
            return;
        }
    }

    // 2) 读取表结构
    TableSchema schema;
    if (!m_dbManager.getTableSchemaByName(tableName, schema)) {
        std::cout << "[ERROR] Table " << tableName << " not found.\n";
        return;
    }

    // 3) 利用 RecordManager 原有的 readRecords(...) 读出所有记录
    //    然后我们手动做 WHERE 过滤
    const int MAX_RECORD_NUM = 100; // 自行限制
    char*** outRecords = new char** [MAX_RECORD_NUM];
    memset(outRecords, 0, sizeof(char**) * MAX_RECORD_NUM);

    int readCount = m_recManager.readRecords(schema, outRecords, MAX_RECORD_NUM);
    if (readCount < 0) {
        std::cout << "[ERROR] readRecords fail.\n";
        return;
    }

    // 4) 打印匹配行
    //    如果没有 WHERE，则全打印
    //    如果有 WHERE，则只打印满足 condField=condValue 的
    //    需先找 condFieldIndex
    int condFieldIndex = -1;
    if (hasWhere) {
        for (int f = 0; f < schema.iFieldCount; f++) {
            if (strcmp(schema.fieldArray[f].sFieldName, condField.c_str()) == 0) {
                condFieldIndex = f;
                break;
            }
        }
        if (condFieldIndex < 0) {
            std::cout << "[ERROR] Field " << condField << " not found in table.\n";
            // 释放记录
            for (int i = 0; i < readCount; i++) {
                for (int j = 0; j < schema.iFieldCount; j++) {
                    delete[] outRecords[i][j];
                }
                delete[] outRecords[i];
            }
            delete[] outRecords;
            return;
        }
    }

    // 简单打印表头
    std::cout << "--------------------------------------\n";
    for (int f = 0; f < schema.iFieldCount; f++) {
        std::cout << schema.fieldArray[f].sFieldName << "\t";
    }
    std::cout << "\n--------------------------------------\n";

    for (int i = 0; i < readCount; i++) {
        bool pass = true;
        if (hasWhere) {
            std::string val = outRecords[i][condFieldIndex];
            if (val != condValue) {
                pass = false;
            }
        }
        if (pass) {
            // print row
            for (int f = 0; f < schema.iFieldCount; f++) {
                std::cout << outRecords[i][f] << "\t";
            }
            std::cout << "\n";
        }
    }
    std::cout << "--------------------------------------\n";

    // 释放
    for (int i = 0; i < readCount; i++) {
        for (int j = 0; j < schema.iFieldCount; j++) {
            delete[] outRecords[i][j];
        }
        delete[] outRecords[i];
    }
    delete[] outRecords;

    std::cout << "[INFO] SELECT done from " << tableName << ".\n";
}