#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

#include <fileSystem.hpp>
#include <log.hpp>

size_type curentDir = 0;

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
errorCode cat(fileSystem& fs, std::string fileName)
{
    return errorCode::OK;
}
/**
 * @brief zmeni aktualni umisteni na zadane umisteni
 *
 * @param path new current path
 */
errorCode cd(fileSystem& fs, std::string path)
{
    return errorCode::OK;
}
/**
 * @brief vypise cestu od root adresare k aktualnimu umisteni
 *
 */
std::string pwd(fileSystem& fs)
{
    return "";
}
/**
 * @brief vytvori kopii souboru z disku do VFS
 *
 * @param fileName
 * @param VFileName
 */
errorCode incp(fileSystem& fs, std::string fileName, std::string VFileName)
{
    std::fstream file(fileName, std::ios::in);
    if (!file.is_open())
    {
        return errorCode::FILE_NOT_FOUND;
    }
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(std::ios::beg);
    char* data = new char[fileSize + 1];    
    std::memset(data, 'f', fileSize);
    data[fileSize] = '\0';
    file.read(data, fileSize);
    //TODO:make path work
    auto ret = fs.touch(0, VFileName.c_str(), data);
    delete[] data;
    return ret;
}
/**
 * @brief vytvori kopii souboru z VFS na disk
 *
 * @param VFileName
 * @param fileName
 */
errorCode outcp(fileSystem& fs, std::string VFileName, std::string fileName)
{
    std::fstream file(fileName, std::ios::out | std::ios::trunc);
    if (!file.is_open())
    {
        return errorCode::PATH_NOT_FOUND;
    }
    //TODO:make path work
    auto data = fs.getData(1);
    if (data.second <= 0)
    {
        return errorCode::FILE_NOT_FOUND;
    }
    file.write(data.first.get(), data.second);
    return errorCode::OK;
}

bool procesLine(fileSystem& fs, std::string line);

errorCode load(fileSystem& fs, std::string fileName)
{
    std::fstream file(fileName, std::ios::in);
    if (!file.is_open())
    {
        return errorCode::FILE_NOT_FOUND;
    }
    std::string fline;
    for (size_t index = 0;std::getline(file, fline); index++)
    {
        std::cout << ">" << fline << std::endl;
        if (!procesLine(fs, fline))
        {
            std::cout << "Script (" << fileName << ") abnormaly ended on line " << index << ":" << fline << std::endl;
            return errorCode::OK;
        }
    }
    std::cout << "Script (" << fileName << ") ended" << std::endl;
    return errorCode::OK;
}

bool procesLine(fileSystem& fs, std::string line)
{
    std::stringstream stream;
    stream << line;
    std::string token, arg1, arg2;
    stream >> token;
    if (token.compare("exit") == 0)
    {
        return false;
    }
    if(token.compare("ls") == 0)
    {
        stream >> arg1;
        auto ret = fs.ls(0);
        auto code = std::get<errorCode>(ret);
        if(code == errorCode::OK)
        {
            std::cout << ret.second;
            return true;
        }
        std::cout << code << std::endl;
        return true;
    }
    if (token.compare("incp") == 0)
    {
        stream >> arg1;
        stream >> arg2;
        std::cout << incp(fs, arg1, arg2) << std::endl;
        return true;
    }
    if (token.compare("outcp") == 0)
    {
        stream >> arg1;
        stream >> arg2;
        std::cout << outcp(fs, arg1, arg2) << std::endl;
        return true;
    }
    if (token.compare("load") == 0)
    {
        stream >> arg1;
        std::cout << "Loading comands form:" << arg1 << std::endl;
        auto ret = load(fs, arg1);
        std::cout << ret << std::endl;
        return ret == errorCode::OK;
    }
    if (token.compare("format") == 0)
    {
        stream >> arg1;
        int diskSize = std::atoi(arg1.c_str());
        superBlock sb(diskSize);
        std::string fileName = fs.getName();
        fs = fileSystem(fileName, sb);
        auto ret = fs.format();
        std::cout << ret << std::endl;
        return ret == errorCode::OK;
    }
    std::cout << "Unknow command:" << token << std::endl;
    return true;
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
    fileSystem fs = fileSystem(fileName);
    std::string line("Put your command after you will be prompted by:>");
    std::cout << line << std::endl;
    do
    {
        std::cout << ">";
    } while (std::getline(std::cin, line) && procesLine(fs, line));
    std::cout << "exiting ..." << std::endl;
    return EXIT_SUCCESS;
}

