#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

#include <fileSystem.hpp>
#include <log.hpp>

constexpr auto DIRSEPARATOR = "/";

size_type curentDir = 0;

void printArgsHelp(char const argv[])
{
    std::cout << "Run program with only one parametr" << std::endl;
    std::cout << argv << " <FsFile>" << std::endl;
}
size_type pathToInode(std::string path)
{

    return path.at(0);
}

/**
 * @brief vypise cestu od root adresare k aktualnimu umisteni
 *
 */
std::string pwd(fileSystem& fs, size_type inode)
{
    Dirent self, parent;
    for (auto direntI : fs.readDir(inode))
    {
        if (std::strcmp(direntI.name, ".") == 0)
        {
            self = direntI;
            continue;
        }
        if (std::strcmp(direntI.name, "..") == 0)
        {
            parent = direntI;
            continue;
        }
        //TODO: zastavit pokud oba odkazy nalezeny        
    }
    if (self.id == parent.id)
    {
        return DIRSEPARATOR;
    }
    std::string ret = pwd(fs, parent.id);
    for (auto direntI : fs.readDir(parent.id))
    {
        if(direntI.id == self.id)
        {
            ret += direntI.name;
            break;
        }
    }
    ret += DIRSEPARATOR;
    return  ret;
}

/**
 * @brief vypise obsah souboru jako text
 *
 * @param fileName
 */
errorCode cat(fileSystem& fs, std::string fileName)
{
    fs.ls(0);
    fileName.at(0);
    return errorCode::OK;
}
/**
 * @brief zmeni aktualni umisteni na zadane umisteni
 *
 * @param path new current path
 */
errorCode cd(fileSystem& fs, std::string path)
{
    fs.ls(0);
    curentDir = atoi(path.c_str());
    return errorCode::OK;
}
/**
 * @brief vypise cestu od root adresare k aktualnimu umisteni
 *
 */
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
    auto inode = pathToInode(VFileName);
    auto data = fs.getData(inode);
    if (data.second <= 0)
    {
        return errorCode::FILE_NOT_FOUND;
    }
    file.write(data.first.get(), data.second);
    return errorCode::OK;
}

bool procesLine(fileSystem& fs, std::ostream& out, std::string line);

errorCode load(fileSystem& fs, std::ostream& out, std::string fileName)
{
    std::fstream file(fileName, std::ios::in);
    if (!file.is_open())
    {
        return errorCode::FILE_NOT_FOUND;
    }
    std::string fline;
    for (size_t index = 0;std::getline(file, fline); index++)
    {
        out << ">" << fline << std::endl;
        if (!procesLine(fs, out, fline))
        {
            out << "Script (" << fileName << ") abnormaly ended on line " << index << ":" << fline << std::endl;
            return errorCode::OK;
        }
    }
    out << "Script (" << fileName << ") ended" << std::endl;
    return errorCode::OK;
}

bool procesLine(fileSystem& fs, std::ostream& out, std::string line)
{
    std::stringstream stream;
    stream << line;
    std::string token, arg1, arg2;
    stream >> token;
    if (token.compare("exit") == 0)
    {
        return false;
    }
    if (token.compare("mkdir") == 0)
    {
        stream >> arg1;
        //TODO: relative path
        out << fs.mkdir(arg1.c_str(),curentDir) << std::endl;
        return true;
    }
    
    if (token.compare("cd") == 0)
    {
        stream >> arg1;
        out << cd(fs, arg1) << std::endl;
        return true;
    }
    if (token.compare("pwd") == 0)
    {
        out << pwd(fs, curentDir) << std::endl;
        return true;
    }
    if (token.compare("ls") == 0)
    {
        stream >> arg1;
        auto ret = fs.ls(curentDir);
        auto code = std::get<errorCode>(ret);
        if (code == errorCode::OK)
        {
            std::cout << ret.second;
            return true;
        }
        out << code << std::endl;
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
        out << outcp(fs, arg1, arg2) << std::endl;
        return true;
    }
    if (token.compare("load") == 0)
    {
        stream >> arg1;
        out << "Loading comands form:" << arg1 << std::endl;
        auto ret = load(fs, out, arg1);
        out << ret << std::endl;
        return true;
    }
    if (token.compare("format") == 0)
    {
        stream >> arg1;
        int diskSize = std::atoi(arg1.c_str());
        superBlock sb(diskSize);
        std::string fileName = fs.getName();
        fs = fileSystem(fileName, sb);
        auto ret = fs.format();
        out << ret << std::endl;
        return true;
    }
    out << "Unknow command:" << token << std::endl;
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
    } while (std::getline(std::cin, line) && procesLine(fs, std::cout, line));
    std::cout << "exiting ..." << std::endl;
    return EXIT_SUCCESS;
}

