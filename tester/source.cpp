#include <iostream>
#include "StackFrame.cpp"
#include <fstream>
#include "gentestcase.hpp"
using namespace std;

/*
Run the testcase written in `filename`
@param filename name of the file
*/
#include <sstream>
std::string getExpected(string filename) {
    std::ostringstream buf;
    auto save = std::cout.rdbuf(buf.rdbuf());
    ans::StackFrame *sf = new ans::StackFrame();
    try {
        sf->run(filename);
    }
    catch (exception& e) {
        cout << e.what();
    }
    delete sf;
    std::cout.rdbuf(save);
    return buf.str();
}

std::string getGot(string filename) {
    std::ostringstream buf;
    auto save = std::cout.rdbuf(buf.rdbuf());
    StackFrame *sf = new StackFrame();
    try {
        sf->run(filename);
    }
    catch (exception& e) {
        cout << e.what();
    }
    delete sf;
    std::cout.rdbuf(save);
    return buf.str();
}

#include <unordered_map>

extern "C" int signal_open(int fd, int reset);

extern "C" int __syscall_openat(int dirfd, intptr_t path, int flags, ...) {
 
   static const std::unordered_map<std::string, int> MapFile2FD{
       {"/dev/stdin", 0},
       {"/dev/stdout", 1},
       {"/dev/stderr", 2},
       {"testcase.txt", 3},
       {"expected.txt", 4},
       {"got.txt", 5}
   };
   int fd = MapFile2FD.at((const char*)path);
   signal_open(fd, flags != 32768);
   return fd;
}

void printJS(const char* id, const char* str) {
    std::ofstream out(id);
    out << str;
}

int main() {
    const char* filename = "testcase.txt";
    
    int const numtestcase = 10000;
    int i = 0;
    for (i = 0; i < numtestcase; ++i) {
        {
            std::ofstream out(filename);
            out << get_test_case();
        }
        std::string expected = getExpected(filename) + "\nEnd here.";
        printJS("expected.txt", expected.c_str());

        std::string got = getGot(filename) + "\nEnd here.";
        printJS("got.txt", got.c_str());
        if (expected != got) {
            break;
        }
    }
    std::string result = std::to_string(i) + "/" + std::to_string(numtestcase);
    std::cout << result << std::endl;

   
    return 0;
}