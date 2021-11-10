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

/**
 * @brief vypise obsah souboru jako text
 *
 * @param fileName
 */
void cat(std::string fileName) {}
/**
 * @brief zmeni aktualni umisteni na zadane umisteni
 *
 * @param path new current path
 */
void cd(std::string path) {}
/**
 * @brief vypise cestu od root adresare k aktualnimu umisteni
 *
 */
void pwd() {}
/**
 * @brief vytvori kopii souboru z disku do VFS
 *
 * @param fileName
 * @param VFileName
 */
void incp(std::string fileName, std::string VFileName) {}
/**
 * @brief vytvori kopii souboru z VFS na disk
 *
 * @param VFileName
 * @param fileName
 */
void outcp(std::string VFileName, std::string fileName) {}
/**
 * @brief nacita prikazy ze souboru
 *
 * @param fileName
 */
void load(std::string fileName) {}

int main(int argc, char const* argv[])
{
    if (argc < 2 || argc > 2)
    {
        printArgsHelp(*argv);
        return EXIT_FAILURE;
    }
    LOGINIT("main.log");

    std::string fileName(argv[1]);
    //superBlock sb(defaulSignature, defaulDescription, 56, 100, 200);
    superBlock sb(103 * 1024,64);
    fileSystem fs(fileName, sb);
    std::vector<std::string> files;

/*
    for (size_t i = 0; i < 30; i++)
    {
        std::string newDir("dir ");
        newDir += (char)('A' + i);
        auto ret = fs.mkdir(newDir.c_str(), 0);
        if (ret == fileSystem::errorCode::OK)
        {
            std::cout << i << ". Dir " << newDir << " created!" << std::endl;
        } else
        {
            std::cout << i << ". Dir " << newDir << " not created!   " << ret << std::endl;
        }
    }
    fs.ls(0, files);
    for (auto file : files)
    {
        std::cout << file << std::endl;
    }
    std::cout << fs.rmdir(20) << std::endl;
    files.clear();
    fs.ls(0, files);
    for (auto file : files)
    {
        std::cout << file << std::endl;
    }
    std::cout << fs.rmdir(10) << std::endl;
    files.clear();
    fs.ls(0, files);
    for (auto file : files)
    {
        std::cout << file << std::endl;
    }
    */

    std::cout << fs.touch(0,"test","00000000111111112222222333333334444444555555555555555544444443333333322222221111111100000000") << std::endl;
    std::cout << fs.touch(0,"testcp") << std::endl;
    std::cout << fs.cp(1,2) << std::endl;
    files.clear();
    fs.ls(0, files);
    for (auto file : files)
    {
        std::cout << file << std::endl;
    }

    return EXIT_SUCCESS;
}

