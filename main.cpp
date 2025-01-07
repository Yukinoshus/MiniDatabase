
#include <iostream>
#include <string>
#include "MiniSqlEngine.h"

int main() {
    // 指定默认的 .dbf 与 .dat 文件名
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
    //        // EOF 或 ctrl+d
    //        break;
    //    }
    //    if (cmd.empty()) {
    //        continue;
    //    }
    //    // 若用户输入 exit; 则退出
    //    std::string tmp = cmd;
    //    while (!tmp.empty() && std::isspace((unsigned char)tmp.back())) {
    //        tmp.pop_back();
    //    }
    //    if (tmp == "exit;" || tmp == "EXIT;") {
    //        break;
    //    }
    //    // 执行
    //    engine.execCommand(cmd);
    //}

    //std::cout << "Bye.\n";

   //------------------------------------------
    std::string commandBuffer;  // 用于累加多行

    while (true) {
        // 显示提示符
        if (commandBuffer.empty())
            std::cout << "minidb> ";
        else
            std::cout << "      > "; // 多行时的二级提示符

        // 读取一行
        std::string line;
        if (!std::getline(std::cin, line)) {
            // EOF 或 ctrl+d
            break;
        }
        // 若用户只输入空行，可以选择跳过或继续累加
        if (line.empty()) {
            continue;
        }

        // 累加到 commandBuffer
        commandBuffer += line + " ";

        // 判断本行是否含有分号 ';'
        // 如果找到了 ';' 就表示一条命令结束
        // 也可用更复杂的逻辑(如只检查末尾, 或匹配括号), 这里只做简单示范
        if (line.find(';') != std::string::npos) {
            // 此时我们有了一个完整的命令, 交给引擎解析
            engine.execCommand(commandBuffer);

            // 清空以便解析下一条命令
            commandBuffer.clear();
        }
    }




    return 0;
}
