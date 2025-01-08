
#include <iostream>
#include <string>
#include "MiniSqlEngine.h"

int main() {
    // ָ��Ĭ�ϵ� .dbf �� .dat �ļ���
    std::string dbFileName = "MyDB.dbf";
    std::string datFileName = "MyDB.dat";

    MiniSqlEngine engine(dbFileName, datFileName);

    std::cout << "=== Welcome to Mini-SQL Engine ===\n";
    std::cout << "Type SQL-like commands, or 'exit;' to quit.\n\n";



    //
    //while (true) {
    //    std::cout << "minidb> ";
    //    std::string cmd;
    //    
    //
    //    
    //    if (!std::getline(std::cin, cmd)) {
    //        // EOF �� ctrl+d
    //        break;
    //    }
    //    if (cmd.empty()) {
    //        continue;
    //    }
    //    // ���û����� exit; ���˳�
    //    std::string tmp = cmd;
    //    while (!tmp.empty() && std::isspace((unsigned char)tmp.back())) {
    //        tmp.pop_back();
    //    }
    //    if (tmp == "exit;" || tmp == "EXIT;") {
    //        break;
    //    }
    //    // ִ��
    //    engine.execCommand(cmd);
    //}

    //std::cout << "Bye.\n";

   //------------------------------------------
    std::string commandBuffer;  // �����ۼӶ���

    while (true) {
        // ��ʾ��ʾ��
        if (commandBuffer.empty())
            std::cout << "minidb> ";
        else
            std::cout << "      > "; // ����ʱ�Ķ�����ʾ��

        // ��ȡһ��
        std::string line;
        if (!std::getline(std::cin, line)) {
            // EOF �� ctrl+d
            break;
        }
        // ���û�ֻ������У�����ѡ������������ۼ�
        if (line.empty()) {
            continue;
        }

        // �ۼӵ� commandBuffer
        commandBuffer += line + " ";

        // �жϱ����Ƿ��зֺ� ';'
        // ����ҵ��� ';' �ͱ�ʾһ���������
        // Ҳ���ø����ӵ��߼�(��ֻ���ĩβ, ��ƥ������), ����ֻ����ʾ��
        if (line.find(';') != std::string::npos) {
            // ��ʱ��������һ������������, �����������
            engine.execCommand(commandBuffer);

            // ����Ա������һ������
            commandBuffer.clear();
        }
    }




    return 0;
}
