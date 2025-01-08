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
    // ת��д���ڼ�ƥ��
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
    // ʾ���﷨��
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

    // ��ȡ����
    std::string tableNamePart = trim(cmd.substr(posCreate + strlen("CREATE TABLE"),
        posParenL - (posCreate + strlen("CREATE TABLE"))));

    // ��ȡ�ֶβ���
    std::string fieldsPart = trim(cmd.substr(posParenL + 1, posParenR - posParenL - 1));

    // ��ȡ dbFileName (�ɲ������ã�
    std::string dbFilePart = trim(cmd.substr(posInto + strlen("INTO")));
    if (!dbFilePart.empty() && dbFilePart.back() == ';') {
        dbFilePart.pop_back();
    }
    dbFilePart = trim(dbFilePart);

    // ���� TableSchema
    TableSchema schema;
    std::memset(&schema, 0, sizeof(TableSchema));
    std::strncpy(schema.sTableName, tableNamePart.c_str(), TABLE_NAME_LENGTH - 1);

    // ����ֶζ���
    // �Զ��ŷָ�
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
        // ���� "Name char[10] KEY NO_NULL VALID"
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

        // ��������
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
            // �������ͼ򵥴���
            std::strcpy(f.sType, tokens[1].c_str());
            f.iSize = 4;
        }

        // ��������ؼ���
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

    // д�� .dbf
    m_dbManager.writeTableSchema(schema);

    std::cout << "[INFO] Table \"" << schema.sTableName
        << "\" created into " << dbFilePart << " successfully.\n";
}


//-former
//// ---------------- DROP TABLE ----------------//
//void MiniSqlEngine::dropTable(const std::string& cmd) {
//    // �﷨: DROP TABLE TableName IN dbFile;
//    // ����ֻ������ʾ�������б� -> ���˵�Ҫɾ���� -> ��д .dbf
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
//    // �����б�
//    std::vector<TableSchema> schemas;
//    if (m_dbManager.readAllTableSchemas(schemas) < 0) {
//        std::cout << "[ERROR] Cannot read dbf.\n";
//        return;
//    }
//    // �� truncate ��ʽ��д
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
//        // д��
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
//        // Ҳ���Կ����� .dat ��ɾ����Ӧ��¼��
//    }
//    else {
//        std::cout << "[INFO] Table \"" << tableName << "\" not found.\n";
//    }
//}

//-----after
void MiniSqlEngine::dropTable(const std::string& cmd) {
    // �﷨: DROP TABLE TableName IN dbFileName;
    // ��:   DROP TABLE Student IN MyDB.dbf;

    // ---------- ��һ������������ -----------
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

    // ��ȡ����
    std::string tableName = trim(cmd.substr(
        posDrop + strlen("DROP TABLE"),
        posIn - (posDrop + strlen("DROP TABLE"))
    ));
    tableName = trim(tableName);

    // ��ȡ dbFileName
    std::string dbFilePart = trim(cmd.substr(posIn + strlen("IN")));
    if (!dbFilePart.empty() && dbFilePart.back() == ';') {
        dbFilePart.pop_back();
    }
    dbFilePart = trim(dbFilePart);

    // ---------- �ڶ������� .dbf ��ɾ���ñ��ģʽ -----------
    // ��ȡ���б�ģʽ
    std::vector<TableSchema> schemas;
    if (m_dbManager.readAllTableSchemas(schemas) < 0) {
        std::cout << "[ERROR] Cannot read dbf file.\n";
        return;
    }

    // �� truncate ��ʽ��д .dbf
    // ע�⣺m_dbManager.getDbFileName() ��Ӧ���ǵ�ǰ���ݿ��ļ������� "MyDB.dbf"
    std::ofstream ofsDBF(m_dbManager.getDbFileName().c_str(), std::ios::binary | std::ios::trunc);
    if (!ofsDBF.is_open()) {
        std::cout << "[ERROR] Cannot open dbf for rewriting.\n";
        return;
    }

    int dropCount = 0;
    for (auto& ts : schemas) {
        // ������ƥ�䣬������(����д�� => ��ͬɾ��)
        if (ts.sTableName == tableName) {
            dropCount++;
            continue;
        }
        // ����������д��
        ofsDBF.put(TABLE_DELIMITER);
        ofsDBF.write(ts.sTableName, TABLE_NAME_LENGTH);
        ofsDBF.write(reinterpret_cast<const char*>(&ts.iFieldCount), sizeof(int));
        for (int i = 0; i < ts.iFieldCount; i++) {
            ofsDBF.write(reinterpret_cast<const char*>(&ts.fieldArray[i]), sizeof(TableMode));
        }
    }
    ofsDBF.close();

    if (dropCount == 0) {
        // ���û��ƥ�䵽Ҫɾ���ı�˵������ .dbf �в�����
        std::cout << "[INFO] Table \"" << tableName
            << "\" not found in " << dbFilePart << ". (dbf cleared anyway)\n";
    }
    else {
        std::cout << "[INFO] Table \"" << tableName
            << "\" removed from " << dbFilePart << " (.dbf).\n";
    }

    // ---------- ���������� .dat ��ɾ���ñ��Ӧ�����ݿ� -----------
    // ����Ҳ��� .dat �еļ�¼ɾ������������²���
    // �������ʱ����Ҫ���� .dat����ֱ�� return

    // �� .dat �ļ� (RecordManager �������и� getDatFileName())
    std::ifstream ifsDAT(m_recManager.getDatFileName(), std::ios::binary);
    if (!ifsDAT.is_open()) {
        std::cout << "[WARN] .dat file cannot be opened or not exist. Maybe no data yet.\n";
        return;
    }

    // ����һ���ڲ��ṹ�������ݴ���� block
    struct TableBlock {
        std::string tblName;
        int recordCount;
        int fieldCount;
        // �洢���м�¼��ԭʼ���������� (validFlag + �ֶ�����)
        std::vector<std::vector<char>> recordData;
    };

    std::vector<TableBlock> allBlocks;

    while (!ifsDAT.eof()) {
        // 1�ֽڷָ���
        char delim;
        if (!ifsDAT.get(delim)) {
            break; // EOF
        }
        if (delim != TABLE_DELIMITER) {
            // �ļ���ʽ�쳣��ն���������
            continue;
        }

        // ��ȡ����
        char tmpName[TABLE_NAME_LENGTH];
        std::memset(tmpName, 0, TABLE_NAME_LENGTH);
        ifsDAT.read(tmpName, TABLE_NAME_LENGTH);

        // ��ȡ��¼��, �ֶ���
        int recCount = 0;
        int fCount = 0;
        ifsDAT.read(reinterpret_cast<char*>(&recCount), sizeof(int));
        ifsDAT.read(reinterpret_cast<char*>(&fCount), sizeof(int));

        TableBlock blk;
        blk.tblName = tmpName;
        blk.recordCount = recCount;
        blk.fieldCount = fCount;

        // ��ȡ��¼�Ķ�����
        for (int r = 0; r < recCount; r++) {
            // ��������Ҫ�� writeRecords(...) �е�д��һ��
            // => ��д 1�ֽ� validFlag, Ȼ����� fCount/Schema д�ֶ�����
            // ��: ���� int=4�ֽ�, char=���� ...
            // �������ʾ, �ȹ���
            // ���ȫ�ġ�������: 
            //   �� read 1�ֽ� validFlag,
            //   �� for fCount �ֶ� => if int => 4�ֽ�; if char => iSize
            //   ��Ҫ֪�� iSize => ��Ҫ�� .dbf ����? 
            // �����ȸ����д, assume each field 4 bytes:
            // => 1 + fCount*4
            // ע��: ������ʵ char[iSize] ��ƥ��, ����ʾ

            int recSize = 1 + (fCount * 4); // ����ʾ, ���Ͻ�
            std::vector<char> raw(recSize);
            ifsDAT.read(raw.data(), recSize);

            blk.recordData.push_back(std::move(raw));
        }
        allBlocks.push_back(std::move(blk));
    }
    ifsDAT.close();

    // ���˵� tblName == tableName �� block
    int datDropCount = 0;
    std::vector<TableBlock> newBlocks;
    for (auto& b : allBlocks) {
        if (b.tblName == tableName) {
            // ����Ҫɾ�ı�
            datDropCount++;
            continue;
        }
        newBlocks.push_back(b);
    }

    // ��д .dat
    std::ofstream ofsDAT(m_recManager.getDatFileName(), std::ios::binary | std::ios::trunc);
    if (!ofsDAT.is_open()) {
        std::cout << "[ERROR] Cannot open .dat for rewriting.\n";
        return;
    }

    for (auto& blk : newBlocks) {
        // д�ָ���
        ofsDAT.put(TABLE_DELIMITER);

        // д���� (���� TABLE_NAME_LENGTH)
        char tName[TABLE_NAME_LENGTH];
        std::memset(tName, 0, TABLE_NAME_LENGTH);
        strncpy(tName, blk.tblName.c_str(), TABLE_NAME_LENGTH - 1);
        ofsDAT.write(tName, TABLE_NAME_LENGTH);

        // д recordCount, fieldCount
        ofsDAT.write(reinterpret_cast<const char*>(&blk.recordCount), sizeof(int));
        ofsDAT.write(reinterpret_cast<const char*>(&blk.fieldCount), sizeof(int));

        // д�ؼ�¼����
        for (auto& rd : blk.recordData) {
            ofsDAT.write(rd.data(), rd.size());
        }
    }
    ofsDAT.close();

    // ��ӡ��ʾ
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
    // �﷨��RENAME TABLE OldName NewName IN dbFile;
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

    // ��ȫ����
    std::vector<TableSchema> schemas;
    if (m_dbManager.readAllTableSchemas(schemas) < 0) {
        std::cout << "[ERROR] Cannot read dbf.\n";
        return;
    }
    // ��д
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
    // �﷨��INSERT INTO TableName VALUES(a, b, c, ...) IN dbFile;
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

    // �� VALUES(...)
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

    // ����ṹ
    TableSchema schema;
    if (!m_dbManager.getTableSchemaByName(tableName, schema)) {
        std::cout << "[ERROR] Table not found: " << tableName << std::endl;
        return;
    }

    // ��ֶ���
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

    // ����һ����¼
    char*** records = new char** [1];
    records[0] = new char* [schema.iFieldCount];
    for (int i = 0; i < schema.iFieldCount; i++) {
        records[0][i] = new char[128];
        std::memset(records[0][i], 0, 128);
        std::strncpy(records[0][i], vals[i].c_str(), 127);
    }

    m_recManager.writeRecords(schema, records, 1);

    // �ͷ�
    for (int i = 0; i < schema.iFieldCount; i++) {
        delete[] records[0][i];
    }
    delete[] records[0];
    delete[] records;

    std::cout << "[INFO] 1 record inserted into " << tableName << ".\n";
}

//// ---------------- DELETE ----------------//
//void MiniSqlEngine::deleteRecord(const std::string& cmd) {
//    // �﷨��DELETE FROM TableName WHERE Field=Value IN dbFile;
//    // ʾ�����ݲ�����ʵ�֣�ֻ����ʾ
//    std::cout << "[INFO] Delete command not fully implemented in this demo.\n";
//}
//

// ---------------- DELETE ----------------//
void MiniSqlEngine::deleteRecord(const std::string& cmd) {
    // �ڴ��﷨��DELETE FROM ���� WHERE �ֶ�=ֵ IN dbFile;
    // ���磺DELETE FROM Student WHERE StuID=123 IN MyDB.dbf;

    // 1) ��������
    std::string upperCmd = cmd;
    std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);

    size_t posDelete = upperCmd.find("DELETE FROM");
    if (posDelete == std::string::npos) {
        std::cout << "[ERROR] DELETE syntax error.\n";
        return;
    }

    // �� WHERE
    size_t posWhere = upperCmd.find("WHERE", posDelete);
    if (posWhere == std::string::npos) {
        std::cout << "[ERROR] DELETE missing WHERE clause.\n";
        return;
    }

    // �� IN
    size_t posIn = upperCmd.find(" IN ", posWhere);
    if (posIn == std::string::npos) {
        std::cout << "[ERROR] DELETE missing IN.\n";
        return;
    }

    // ��ȡ����
    std::string tableName = trim(cmd.substr(posDelete + strlen("DELETE FROM"),
        posWhere - (posDelete + strlen("DELETE FROM"))));

    // ��ȡ WHERE "Field=Value"
    // ���������Ȱ�"WHERE"��"IN"֮������ݵ����ó�
    std::string wherePart = trim(cmd.substr(posWhere + strlen("WHERE"),
        posIn - (posWhere + strlen("WHERE"))));
    // ����: "StuID=123"

    // ��ȡ dbFilePart
    std::string dbFilePart = trim(cmd.substr(posIn + strlen("IN")));
    if (!dbFilePart.empty() && dbFilePart.back() == ';') {
        dbFilePart.pop_back();
    }
    dbFilePart = trim(dbFilePart);

    // 2) ���� wherePart => fieldName, fieldValue
    // �򵥼���ֻ�� "Field=Value" ���ָ�ʽ
    std::string fieldName, fieldValue;
    {
        size_t eqPos = wherePart.find('=');
        if (eqPos == std::string::npos) {
            std::cout << "[ERROR] WHERE clause must be Field=Value.\n";
            return;
        }
        fieldName = trim(wherePart.substr(0, eqPos));
        fieldValue = trim(wherePart.substr(eqPos + 1));
        // ��� value ������(���� 'Alice')������ȥ������
        if (!fieldValue.empty() && fieldValue.front() == '\'') {
            fieldValue.erase(0, 1);
        }
        if (!fieldValue.empty() && fieldValue.back() == '\'') {
            fieldValue.pop_back();
        }
    }

    // 3) ��ȡ��ṹ
    TableSchema schema;
    if (!m_dbManager.getTableSchemaByName(tableName, schema)) {
        std::cout << "[ERROR] Table " << tableName << " not found.\n";
        return;
    }

    // 4) ��ȡ .dat �����м�¼���ҵ� "TableName" ��
    //    ���������Լ���"��ȡ-�޸�-д��"�Ĺ���
    std::ifstream ifs(m_recManager.getDatFileName(), std::ios::binary);
    if (!ifs.is_open()) {
        std::cout << "[ERROR] Cannot open dat file: " << m_recManager.getDatFileName() << "\n";
        return;
    }

    // �ռ����б����ڴ���Ϣ���Ա����һ������д
    struct TableBlock {
        std::string tableName;
        int recordCount;
        int fieldCount;
        // recordData ���: (validFlag + fields raw data) �Ķ������ڴ�
        std::vector<std::vector<char>> recordData;
    };
    std::vector<TableBlock> allBlocks;

    while (!ifs.eof()) {
        char delim;
        if (!ifs.get(delim)) break; // EOF
        if (delim != TABLE_DELIMITER) {
            continue;
        }

        // ������
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

        // ��ȡ���м�¼(���� validFlag + ���ֶ�raw data)
        for (int r = 0; r < recCountInFile; r++) {
            // ���㵥����¼���ܴ�С(1�ֽ�validFlag + sum of each field size)
            // ע�⣺int�ֶ�4�ֽڣ�char�ֶ� iSize �ֽ�
            int oneRecordSize = 1; // validFlag
            // ������Ҫ�� schema ��ȡ����Ӧ�ֶ���Ϣ
            // ����� block.tableName != tableName ������ֻ������
            // Ϊ�򻯣���ȫ������ buffer���ٺ����ٴ���
            if (block.tableName == schema.sTableName) {
                // ͬһ���� => �� schema(fieldArray) ����
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
                // ��������Ҫ�����ı� => �� block.fieldCountInFile �ķ�ʽ����
                // �����������ı����ݿ�
                // �����ȼ򵥴��� => int assumed 4 / char assumed iSize from fileCountInFile?
                // �� block.fieldCountInFile ��һ������ schema.iFieldCount
                // => ��Ҫ�Լ��ٶ� TableSchema? 
                // ������: ��һ�� validFlag, �ٶ�ÿ���ֶζ� 4 or iSize?
                // ������û����� block �� schema => ֻ�����¶�ȡ .dbf ? 
                // ����Ϊ�˼�, ���� fieldCountInFile == schema.iFieldCount for all blocks
                // (��ʵ���Ҫ��ϸ�Ĺ���)
                oneRecordSize += 1; // validFlag(����), ֻ��ʾ��
                for (int ff = 0; ff < fieldCountInFile; ff++) {
                    // �ݶ����� int? => 4 �ֽ�? ��������д��ʱ��ƥ��
                    // ��Ϊ��ʾ...
                    oneRecordSize += 4;
                }
            }

            std::vector<char> recBuf(oneRecordSize);
            // �Ѿ����� 1 �ֽ� delim; ����Ҫ�� delim ���� validFlag?
            // ���ԣ����� if (!ifs.get(delim)) break; ��ʵ�Ѿ����ĵ��� 1�ֽ�
            // ��û�� validFlag ...
            // => ���ǻ�������ʵʵһ���Զ�ȡ oneRecordSize
            recBuf[0] = 0; // ������
            ifs.read(recBuf.data(), oneRecordSize);

            block.recordData.push_back(std::move(recBuf));
        }

        allBlocks.push_back(block);
    }
    ifs.close();

    // 5) ���ڴ����޸�"Ŀ���"�ļ�¼(������ WHERE �� validFlag = 0)
    //    ���ҵ� block.tableName == tableName
    for (auto& blk : allBlocks) {
        if (blk.tableName == schema.sTableName) {
            // �ҵ�ƥ��� block
            // ����ÿ����¼ => parse => if match => set valid=0
            int fieldOffset = 1; // offset after validFlag
            for (int r = 0; r < (int)blk.recordData.size(); r++) {
                char* recPtr = blk.recordData[r].data();
                char validFlag = recPtr[0];
                if (validFlag == 0) {
                    continue; // already invalid
                }

                // ����ÿ���ֶ�, �� target fieldName
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

                // ���� targetFieldIndex �� recordData ���λ��
                // ��Ҫ����ֶμ��� offset
                // or �� parse all fields
                // ����д�� parseAllFields ����:
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
                // parsedFields[targetFieldIndex] �����ǹ��ĵ��ֶ�
                // ������ֵ == fieldValue, ���� validFlag=0
                if (parsedFields[targetFieldIndex].value == fieldValue) {
                    // set valid=0
                    blk.recordData[r][0] = 0;
                }
            }
        }
    }

    // 6) ��д .dat �ļ�
    std::ofstream ofs(m_recManager.getDatFileName(), std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        std::cout << "[ERROR] Cannot open dat file for rewriting.\n";
        return;
    }

    for (auto& blk : allBlocks) {
        ofs.put(TABLE_DELIMITER);
        // ����(����)
        char tname[TABLE_NAME_LENGTH];
        memset(tname, 0, TABLE_NAME_LENGTH);
        strncpy(tname, blk.tableName.c_str(), TABLE_NAME_LENGTH - 1);
        ofs.write(tname, TABLE_NAME_LENGTH);

        // ��¼��
        int recCount = (int)blk.recordData.size();
        ofs.write(reinterpret_cast<const char*>(&recCount), sizeof(int));
        // �ֶ���
        ofs.write(reinterpret_cast<const char*>(&blk.fieldCount), sizeof(int));

        // дÿ����¼
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
//    // �﷨��UPDATE TableName (SET Field1=Value1 WHERE Field2=Value2) IN dbFile;
//    std::cout << "[INFO] Update command not fully implemented in this demo.\n";
//}



// ---------------- UPDATE ----------------//
void MiniSqlEngine::updateRecord(const std::string& cmd) {
    // �﷨ʾ����
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

    // ��ȡ����
    std::string tableNamePart = trim(cmd.substr(posUpdate + strlen("UPDATE"),
        posParenL - (posUpdate + strlen("UPDATE"))));

    // ��ȡ ������(...) �ڲ����� => "SET Field1=NewValue WHERE Field2=ConditionValue"
    std::string insidePart = trim(cmd.substr(posParenL + 1, posParenR - posParenL - 1));

    // ��ȡ dbFileName
    std::string dbFilePart = trim(cmd.substr(posIn + strlen("IN")));
    if (!dbFilePart.empty() && dbFilePart.back() == ';') {
        dbFilePart.pop_back();
    }
    dbFilePart = trim(dbFilePart);

    // insidePart ������ "SET " �� " WHERE "
    // e.g. "SET Field1=NewValue WHERE Field2=ConditionValue"
    size_t posSet = insidePart.find("SET");
    size_t posWhere = insidePart.find("WHERE");
    if (posSet == std::string::npos || posWhere == std::string::npos) {
        std::cout << "[ERROR] UPDATE missing SET or WHERE.\n";
        return;
    }

    std::string setPart = trim(insidePart.substr(posSet + strlen("SET"), posWhere - (posSet + strlen("SET"))));
    std::string wherePart = trim(insidePart.substr(posWhere + strlen("WHERE")));

    // ���� setPart => "Field1=NewValue"
    std::string setFieldName, setFieldValue;
    {
        size_t eqPos = setPart.find('=');
        if (eqPos == std::string::npos) {
            std::cout << "[ERROR] SET clause must be Field=Value\n";
            return;
        }
        setFieldName = trim(setPart.substr(0, eqPos));
        setFieldValue = trim(setPart.substr(eqPos + 1));
        // ȥ����
        if (!setFieldValue.empty() && setFieldValue.front() == '\'') {
            setFieldValue.erase(0, 1);
        }
        if (!setFieldValue.empty() && setFieldValue.back() == '\'') {
            setFieldValue.pop_back();
        }
    }

    // ���� wherePart => "Field2=ConditionValue"
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

    // ��ȡ��ṹ
    TableSchema schema;
    if (!m_dbManager.getTableSchemaByName(tableNamePart, schema)) {
        std::cout << "[ERROR] Table " << tableNamePart << " not found.\n";
        return;
    }

    // ��ȡ���޸ġ�д�� => ԭ���� deleteRecord ����
    // ��ȥ������ deleteRecord �ظ���ע��
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
            // �����¼��С
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
                // �򻯼���
                oneRecordSize += fieldCountInFile * 4;
            }

            std::vector<char> recBuf(oneRecordSize);
            ifs.read(recBuf.data(), oneRecordSize);
            blk.recordData.push_back(std::move(recBuf));
        }

        allBlocks.push_back(blk);
    }
    ifs.close();

    // �޸� block
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

                // �ҵ� condFieldName, compare => if match => update setFieldName
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
                    // �����ھ�����
                    continue;
                }

                if (parsed[condIndex].value == condFieldValue) {
                    // update setIndex
                    if (parsed[setIndex].isInt) {
                        // ת int
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

    // ��д
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
//     �﷨��SELECT * FROM TableName WHERE ...
//    std::cout << "[INFO] Select command not fully implemented in this demo.\n";
//}
// 
// 
// ---------------- SELECT ----------------//
void MiniSqlEngine::selectRecords(const std::string& cmd) {
    // ��򵥣�SELECT * FROM TableName WHERE Field=Value
    // �������JOIN��ͶӰ�б��

    // 1) �� "SELECT * FROM"
    // 2) �� "WHERE"
    // 3) �������� & Field=Value
    // 4) ��ȡ����ӡƥ���¼

    std::string upperCmd = cmd;
    std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);

    size_t posSelect = upperCmd.find("SELECT");
    if (posSelect == std::string::npos) {
        std::cout << "[ERROR] SELECT syntax error.\n";
        return;
    }
    // �ڴ� "SELECT * FROM"
    size_t posFrom = upperCmd.find("FROM", posSelect);
    if (posFrom == std::string::npos) {
        std::cout << "[ERROR] SELECT missing FROM.\n";
        return;
    }
    // �� WHERE(��ѡ)
    size_t posWhere = upperCmd.find("WHERE", posFrom);

    // ��ȡ����
    std::string tableName;
    if (posWhere != std::string::npos) {
        tableName = trim(cmd.substr(posFrom + strlen("FROM"), posWhere - (posFrom + strlen("FROM"))));
    }
    else {
        // û�� where
        // ���� "SELECT * FROM TableName;"
        // �ҽ�β
        size_t semiPos = upperCmd.find(';', posFrom);
        if (semiPos == std::string::npos) {
            semiPos = cmd.size();
        }
        tableName = trim(cmd.substr(posFrom + strlen("FROM"), semiPos - (posFrom + strlen("FROM"))));
    }

    // ��ȡ WHERE => "Field=Value"
    std::string condField, condValue;
    bool hasWhere = (posWhere != std::string::npos);
    if (hasWhere) {
        // e.g. "WHERE Field=Value"
        // �ҷֺ�(����ĩ)
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

    // 2) ��ȡ��ṹ
    TableSchema schema;
    if (!m_dbManager.getTableSchemaByName(tableName, schema)) {
        std::cout << "[ERROR] Table " << tableName << " not found.\n";
        return;
    }

    // 3) ���� RecordManager ԭ�е� readRecords(...) �������м�¼
    //    Ȼ�������ֶ��� WHERE ����
    const int MAX_RECORD_NUM = 100; // ��������
    char*** outRecords = new char** [MAX_RECORD_NUM];
    memset(outRecords, 0, sizeof(char**) * MAX_RECORD_NUM);

    int readCount = m_recManager.readRecords(schema, outRecords, MAX_RECORD_NUM);
    if (readCount < 0) {
        std::cout << "[ERROR] readRecords fail.\n";
        return;
    }

    // 4) ��ӡƥ����
    //    ���û�� WHERE����ȫ��ӡ
    //    ����� WHERE����ֻ��ӡ���� condField=condValue ��
    //    ������ condFieldIndex
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
            // �ͷż�¼
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

    // �򵥴�ӡ��ͷ
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

    // �ͷ�
    for (int i = 0; i < readCount; i++) {
        for (int j = 0; j < schema.iFieldCount; j++) {
            delete[] outRecords[i][j];
        }
        delete[] outRecords[i];
    }
    delete[] outRecords;

    std::cout << "[INFO] SELECT done from " << tableName << ".\n";
}