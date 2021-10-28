#include <cstdlib>
#include <iostream>

#include <fileSystem.hpp>

void printArgsHelp(char const argv[])
{
    std::cout << "Run program with only one parametr" << std::endl;
    std::cout << argv << " <FsFile>" << std::endl;
}

int main(int argc, char const *argv[])
{
    if (argc < 2 || argc > 2)
    {
        printArgsHelp(*argv);
        return EXIT_FAILURE;
    }
    std::string fileName(argv[1]);
    fileSystem fs(fileName);
    fs.format(104*1024);
    return EXIT_SUCCESS;
}
