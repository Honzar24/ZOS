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
    fs.calcAndFormat(103 * 1024);
    for (size_t i = 0; i < 2; i++)
    {
        std::string newDir("dir ");
        newDir.append(std::to_string(i));
        auto ret = fs.makeDir(newDir.c_str(), 0);
        if (ret == fileSystem::errorCode::OK)
        {
            std::cout << i << ". Dir " << newDir << " created!" << std::endl;
        } else
        {
            std::cout << i << ". Dir " << newDir << " not created!   " << ret << std::endl;
        }
    }
    for (size_t i = 1; i <= 2; i++)
    {
       for (size_t j = 0; j < 5; j++)
            {
                std::string newSubDir("sub ");
                newSubDir.append(std::to_string(j));
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
    
    return EXIT_SUCCESS;
}

