#include <cstdlib>
#include <iostream>
#include <string>

#include <fileSystem.hpp>
#include <log.hpp>

void printArgsHelp(char const argv[])
{
    std::cout << "Run program with only one parametr" << std::endl;
    std::cout << argv << " <FsFile>" << std::endl;
}

int main(int argc, char const* argv[])
{
    if (argc < 2 || argc > 2)
    {
        printArgsHelp(*argv);
        return EXIT_FAILURE;
    }
    LOGINIT("main.log");

    std::string fileName(argv[1]);
    superBlock sb(defaulSignature, defaulDescription, 56, 100, 200);
    //fileSystem fs(fileName, sb);
    fileSystem fs(fileName);
    // zarovani sekci do 16 bloku
    // fs.calcAndFormat(103*1024);
    //fs.calcAndFormat(103 * 1024);
    /*
    for (size_t i = 0; i < 30; i++)
    {
        std::string newDir("dir ");
        newDir += (char)('A' + i);
        auto ret = fs.makeDir(newDir.c_str(), 0);
        if (ret == fileSystem::errorCode::OK)
        {
            std::cout << i << ". Dir " << newDir << " created!" << std::endl;
        } else
        {
            std::cout << i << ". Dir " << newDir << " not created!   " << ret << std::endl;
        }
    }
    for (size_t i = 1; i <= 3; i++)
    {
        for (size_t j = 0; j <= 10; j++)
        {
            std::string newSubDir("sub ");
            newSubDir += (char)('A' + i-1);
            newSubDir += (char)('A' + j);
            auto ret = fs.makeDir(newSubDir.c_str(), i);
            if (ret == fileSystem::errorCode::OK)
            {
                std::cout << i << "/" << j << ". subDir " << newSubDir << " created!" << std::endl;

            } else
            {
                std::cout << i << "/" << j << ". subDir " << newSubDir << " not created!   " << ret << std::endl;
            }

        }
    }
    */
    for (size_t j = 0; j <= 3; j++)
    {
        std::vector<std::string> names;
        auto retls = fs.ls(j, names);
        if (retls == fileSystem::errorCode::OK)
        {
            for (auto name : names)
            {
                std::cout << name << std::endl;
            }
        } else {
            std::cout << retls << std::endl;
        }
        std::cout << std::endl;
    }



    return EXIT_SUCCESS;
}

