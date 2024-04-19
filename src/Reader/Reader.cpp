/**
 * @file Reader.cpp
 * @author Kalinka (KalinkaGit) (remi.grimault@gmail.com)
 * @brief Implementation of the Reader class
 * @version 1.0.0
 * @date 2024-04-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "Core/Reader/Reader.hpp"

namespace GLTFLoader
{
    Reader::Reader(const std::string &path)
        : m_path(path)
    {}

    void Reader::setFilePath(const std::string &path)
    {
        this->m_path = path;
    }

    std::string Reader::readTextFile() const
    {
        std::ifstream file(m_path);

        if (!file.is_open())
        {
            std::cerr << "Error: could not open file " << m_path << std::endl;
            return ("");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        return buffer.str();
    }

    std::vector<unsigned char> Reader::readBinaryFile() const
    {
        std::ifstream file(m_path, std::ios::binary);

        if (!file.is_open())
        {
            std::cerr << "Error: could not open file " << m_path << std::endl;
            return (std::vector<unsigned char>());
        }

        return std::vector<unsigned char>(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );
    }
}
