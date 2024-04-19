/**
 * @file Buffer.cpp
 * @author Kalinka (KalinkaGit) (remi.grimault@gmail.com)
 * @brief Implementation of the Buffer class
 * @version 1.0.0
 * @date 2024-04-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <iostream>

#include "Core/Manager/Buffer.hpp"
#include "nlohmann/json.hpp"

namespace GLTFLoader
{
    BufferManager::BufferManager()
    {}

    int BufferManager::loadBuffersFromGLTF(const std::string &jsonContent)
    {
        auto json = nlohmann::json::parse(jsonContent);

        if (!json.contains("buffers") || !json["buffers"].is_array())
        {
            std::cerr << "Error: no buffers found in the GLTF file" << std::endl;
            return (-1);
        }

        for (const auto &bufferEntry : json["buffers"])
        {
            if (!bufferEntry.contains("uri"))
            {
                std::cerr << "Error: no URI found for a buffer" << std::endl;
                return (-1);
            }

            std::string uri = bufferEntry["uri"].get<std::string>();

            if (!loadBuffer(uri))
            {
                std::cerr << "Error: could not load buffer " << uri << std::endl;
                return (-1);
            }
        }

        return (0);
    }

    int BufferManager::loadBuffer(const std::string &uri)
    {
        Reader Reader(uri);

        std::vector<unsigned char> data = Reader.readBinaryFile();
        if (data.empty())
        {
            std::cerr << "Error: could not read buffer " << uri << std::endl;
            return (-1);
        }

        m_buffers.push_back(std::move(data));

        return (0);
    }

    const std::vector<unsigned char> &BufferManager::getBufferData(int index) const
    {
        if (index < 0 || index >= m_buffers.size())
        {
            static const std::vector<unsigned char> emptyBuffer;

            std::cerr << "Error: buffer index out of range" << index << std::endl;
            
            return (emptyBuffer);
        }

        return (m_buffers[index]);
    }
}