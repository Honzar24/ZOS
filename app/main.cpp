#include <cstdlib>
#include <cassert>

#include <iostream>
#include <string>
#include <fstream>

#include <fileSystem.hpp>
#include <log.hpp>

constexpr char DIRSEPARATOR = '/';

size_type curentDir = 0;

void printArgsHelp(char const argv[])
{
    std::cout << "Run program with only one parametr" << std::endl;
    std::cout << argv << " <FsFile>" << std::endl;
}

std::pair<std::string, std::string> stripName(std::string& name)
{
    std::string path, fileName;
    size_t pos = name.rfind(DIRSEPARATOR);
    if (pos == name.size() - 1)
    {
        name = name.substr(0, pos);
        pos = name.rfind(DIRSEPARATOR);
    }
    if (pos != std::string::npos)
    {
        path = name.substr(0, pos);
        fileName = name.substr(pos + 1);
    } else
    {
        path = "";
        fileName = std::move(name);
    }
    return std::make_pair(path, fileName);
}

std::pair<errorCode, size_type> pathToInode(fileSystem& fs, size_type dirID, std::stringstream& tokens);

std::pair<errorCode, size_type> pathToInode(fileSystem& fs, size_type dirID, std::string& path)
{
    std::stringstream tokens;
    tokens << path;
    return pathToInode(fs, dirID, tokens);
}

std::pair<errorCode, size_type> pathToInode(fileSystem& fs, size_type dirID, std::stringstream& tokens)
{
    std::string token;
    std::getline(tokens, token, DIRSEPARATOR);
    if (!tokens)
    {
        return std::make_pair(errorCode::OK, dirID);
    }
    if (token.compare(".") == 0)
    {
        return pathToInode(fs, curentDir, tokens);
    }
    if (token.compare("") == 0)
    {
        //FIXME:neni zaruceno ze 0 je root ale neni moc dulezite
        return pathToInode(fs, 0, tokens);
    }
    for (auto direntI : fs.readDir(dirID))
    {
        if (std::strcmp(direntI.name, token.c_str()) == 0)
        {
            return pathToInode(fs, direntI.id, tokens);
        }
    }
    return std::make_pair(errorCode::PATH_NOT_FOUND, 0);
}

/**
 * @brief vypise cestu od root adresare k aktualnimu umisteni
 *
 */
std::string pwd(fileSystem& fs, size_type inode)
{
    Dirent self, parent;
    bool foundParent = false, foundSelf = false;
    auto dirents = fs.readDir(inode);
    for (auto it = dirents.begin();it != dirents.end();it++)
    {
        if (std::strcmp((*it).name, ".") == 0)
        {
            self = (*it);
            foundSelf = true;
            if (foundParent)
                break;
            continue;
        }
        if (std::strcmp((*it).name, "..") == 0)
        {
            parent = (*it);
            foundParent = true;
            if (foundSelf)
                break;
            continue;
        }
    }
    assert(foundParent == true && foundSelf == true);
    if (self.id == parent.id)
    {
        return std::string(1, DIRSEPARATOR);
    }
    std::string ret = pwd(fs, parent.id);
    for (auto direntI : fs.readDir(parent.id))
    {
        if (direntI.id == self.id)
        {
            ret += direntI.name;
            break;
        }
    }
    ret += DIRSEPARATOR;
    return  ret;
}

/**
 * @brief vypise obsah souboru jako text na vystupni stream
 *
 * @param fs
 * @param out
 * @param fileName
 * @return errorCode
 */
errorCode cat(fileSystem& fs, std::ostream& out, std::string fileName)
{
    auto fileQ = pathToInode(fs, curentDir, fileName);
    if (fileQ.first != errorCode::OK)
    {
        return errorCode::PATH_NOT_FOUND;
    }
    auto data = fs.getData(std::get<size_type>(fileQ));
    out.write(data.first.get(), data.second) << std::endl;
    return errorCode::OK;
}

/**
 * @brief vytvori kopii souboru z disku do VFS
 *
 * @param fileName
 * @param VFile
 */
errorCode incp(fileSystem& fs, std::string& fileName, std::string& VFile)
{
    std::fstream file(fileName, std::ios::in);
    if (!file.is_open())
    {
        return errorCode::FILE_NOT_FOUND;
    }
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(std::ios::beg);
    std::unique_ptr<char[]> data(new char[fileSize + 1]);
    std::memset(data.get(), 'f', fileSize);
    data[fileSize] = '\0';
    file.read(data.get(), fileSize);
    size_type dirID;
    auto pathAndName = stripName(VFile);
    auto pathQ = pathToInode(fs, curentDir, pathAndName.first);
    if (pathQ.first != errorCode::OK)
    {
        return errorCode::PATH_NOT_FOUND;
    }
    dirID = std::get<size_type>(pathQ);
    auto ret = fs.touch(dirID, pathAndName.second.c_str(), data.get(), fileSize);
    return ret.first;
}
/**
 * @brief vytvori kopii souboru z VFS na disk
 *
 * @param VFile
 * @param fileName
 */
errorCode outcp(fileSystem& fs, std::string& VFileName, std::string& fileName)
{
    auto path = pathToInode(fs, curentDir, VFileName);
    errorCode code = std::get<errorCode>(path);
    if (code != errorCode::OK)
    {
        return code;
    }
    size_type inodeID = std::get<size_type>(path);
    auto data = fs.getData(inodeID);
    if (data.second <= 0)
    {
        return errorCode::FILE_NOT_FOUND;
    }
    std::fstream file(fileName, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open())
    {
        return errorCode::PATH_NOT_FOUND;
    }
    file.write(data.first.get(), data.second);
    file.flush();
    file.close();
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
    out << "Loading comands form:" << fileName << std::endl;
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

    if (token.compare("format") == 0)
    {
        stream >> arg1;
        int diskSize, pos = arg1.rfind("MB");
        if (pos != std::string::npos)
        {
            diskSize = std::atoi(arg1.substr(0, pos).c_str());
            diskSize *= 1024 * 1024;
        } else {
            diskSize = std::atoi(arg1.c_str());
        }
        superBlock sb(diskSize);
        std::string fileName = fs.getName();
        fs = fileSystem(fileName, sb);
        auto ret = fs.format();
        out << ret << std::endl;
        if (ret == errorCode::OK)
        {
            curentDir = 0;
        }
        return true;
    }

    if (token.compare("load") == 0)
    {
        stream >> arg1;
        auto ret = load(fs, out, arg1);
        out << ret << std::endl;
        return true;
    }

    if (token.compare("exit") == 0)
    {
        return false;
    }

    if (!fs.isFormated())
    {
        out << "File system is no formated please format it before running any commands!" << std::endl;
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
    if (token.compare("cd") == 0)
    {
        stream >> arg1;
        auto cdc = pathToInode(fs, curentDir, arg1);
        errorCode code = std::get<errorCode>(cdc);
        if (code == errorCode::OK)
        {
            curentDir = std::get<size_type>(cdc);
        }
        out << code << std::endl;
        return true;
    }
    if (token.compare("cp") == 0)
    {
        stream >> arg1;
        stream >> arg2;
        auto pathAndName1 = stripName(arg1);
        auto path1 = pathToInode(fs, curentDir, pathAndName1.first);
        auto pathAndName2 = stripName(arg2);
        auto path2 = pathToInode(fs, curentDir, pathAndName2.first);
        errorCode code1 = std::get<errorCode>(path1);
        errorCode code2 = std::get<errorCode>(path2);

        if (code1 == errorCode::OK && code2 == errorCode::OK)
        {
            code1 = fs.cp(std::get<size_type>(path1), pathAndName1.second.c_str(), std::get<size_type>(path2), pathAndName2.second.c_str());
        }
        out << code1 << std::endl;
        return true;
    }
    if (token.compare("ln") == 0)
    {
        stream >> arg1;
        stream >> arg2;
        auto path1 = pathToInode(fs, curentDir, arg1);
        auto pathAndName2 = stripName(arg2);
        auto path2 = pathToInode(fs, curentDir, pathAndName2.first);
        errorCode code1 = std::get<errorCode>(path1);
        errorCode code2 = std::get<errorCode>(path2);

        if (code1 == errorCode::OK && code2 == errorCode::OK)
        {
            code1 = fs.ln(std::get<size_type>(path1), std::get<size_type>(path2), pathAndName2.second.c_str());
        }
        out << code1 << std::endl;
        return true;
    }

    if (token.compare("mv") == 0)
    {
        stream >> arg1;
        stream >> arg2;
        auto pathAndName1 = stripName(arg1);
        auto path1 = pathToInode(fs, curentDir, pathAndName1.first);
        auto pathAndName2 = stripName(arg2);
        auto path2 = pathToInode(fs, curentDir, pathAndName2.first);
        errorCode code1 = std::get<errorCode>(path1);
        errorCode code2 = std::get<errorCode>(path2);

        if (code1 == errorCode::OK && code2 == errorCode::OK)
        {
            code1 = fs.mv(std::get<size_type>(path1), pathAndName1.second.c_str(), std::get<size_type>(path2), pathAndName2.second.c_str());
        }
        out << code1 << std::endl;
        return true;
    }

    if (token.compare("pwd") == 0)
    {
        out << pwd(fs, curentDir) << std::endl;
        return true;
    }

    if (token.compare("rm") == 0)
    {
        stream >> arg1;
        auto pathAndName = stripName(arg1);
        auto cdc = pathToInode(fs, curentDir, pathAndName.first);
        errorCode code = std::get<errorCode>(cdc);
        if (code == errorCode::OK)
        {
            code = fs.rm(std::get<size_type>(cdc), pathAndName.second.c_str());
        }
        out << code << std::endl;
        return true;
    }

    if (token.compare("cat") == 0)
    {
        stream >> arg1;
        auto ret = cat(fs, out, arg1);
        if (ret != errorCode::OK)
        {
            out << ret << std::endl;
        }
        return true;
    }

    if (token.compare("mkdir") == 0)
    {
        stream >> arg1;
        auto pathAndName = stripName(arg1);
        auto node = pathToInode(fs, curentDir, pathAndName.first);
        errorCode code = std::get<errorCode>(node);
        if (code == errorCode::OK)
        {
            code = fs.mkdir(pathAndName.second.c_str(), std::get<size_type>(node));
        }
        out << code << std::endl;
        return true;
    }
    if (token.compare("rmdir") == 0)
    {
        stream >> arg1;
        auto pathAndName = stripName(arg1);
        auto node = pathToInode(fs, curentDir, arg1);
        errorCode code = std::get<errorCode>(node);
        if (code == errorCode::OK)
        {
            code = fs.rmdir(std::get<size_type>(node), pathAndName.second.c_str());
        }
        out << code << std::endl;
        return true;

    }
    if (token.compare("info") == 0)
    {
        stream >> arg1;
        auto pathAndName = stripName(arg1);
        auto node = pathToInode(fs, curentDir, pathAndName.first);
        errorCode code = std::get<errorCode>(node);
        if (code == errorCode::OK)
        {
            auto ret = fs.info(std::get<size_type>(node), pathAndName.second.c_str());
            code = std::get<errorCode>(ret);
            if (code == errorCode::OK)
            {
                out << ret.second << std::endl;
                return true;
            }
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

