/**
 * @file Scene.cpp
 * @author Kalinka (KalinkaGit) (remi.grimault@gmail.com)
 * @brief Implementation of the Scene class
 * @version 1.0.0
 * @date 2024-04-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <iostream>

#include "Core/Manager/Scene.hpp"

namespace GLTFLoader
{
    SceneManager::SceneManager(BufferManager &bufferManager)
        : m_bufferManager(bufferManager)
    {}

    int SceneManager::buildSceneFromJSON(const std::string &jsonContent)
    {
        m_json = nlohmann::json::parse(jsonContent);

        if (m_json.contains("nodes"))
        {
            processNodes(m_json["nodes"]);
        }

        if (m_json.contains("meshes"))
        {
            processMeshes(m_json["meshes"]);
        }

        if (m_json.contains("materials"))
        {
            processMaterials(m_json["materials"]);
        }

        return (0);
    }

    void SceneManager::processNodes(const nlohmann::json &nodesJson)
    {
        for (const auto &nodeJson : nodesJson)
        {
            auto node = std::make_shared<Node>();

            node->name = nodeJson.value("name", "");
            node->meshIndex = nodeJson.value("mesh", -1);

            if (nodeJson.contains("matrix"))
            {
                std::vector<float> matrixEl = nodeJson["matrix"].get<std::vector<float>>();
                std::copy(matrixEl.begin(), matrixEl.end(), node->transform.data);
            }

            if (nodeJson.contains("children"))
            {
                for (auto childIndex : nodeJson["children"])
                {
                    node->children.push_back(m_nodeMap.at(childIndex.get<int>()));
                }
            }

            m_nodeMap[nodeJson.value("index", static_cast<int>(m_nodeMap.size()))] = node;
            if (node->children.empty())
            {
                m_scene.nodes.push_back(node);
            }
        }
    }

    void SceneManager::processMeshes(const nlohmann::json &meshesJson)
    {
        for (const auto &meshJson : meshesJson)
        {
            Mesh newMesh;

            if (meshJson.contains("primitives"))
            {
                for (const auto &primitive : meshJson["primitives"])
                {
                    if (primitive.contains("attributes"))
                    {
                        auto attributes = primitive["attributes"];

                        if (attributes.contains("POSITION"))
                        {
                            int accessorIndex = attributes["POSITION"];

                            newMesh.vertices = loadVertexPositions(accessorIndex);
                        }

                        if (attributes.contains("indices"))
                        {
                            int indicesAccessor = attributes["indices"];

                            newMesh.indices = loadIndices(indicesAccessor);
                        }
                    }

                    if (primitive.contains("material"))
                    {
                        newMesh.materialIndex = primitive["material"];
                    }
                    
                    m_scene.meshes.push_back(newMesh);
                }
            }
        }
    }

    void SceneManager::processMaterials(const nlohmann::json &materialsJson)
    {
        for (const auto &materialJson : materialsJson)
        {
            Material newMaterial;

            if (materialJson.contains("pbrMetallicRoughness"))
            {
                auto &pbr = materialJson["pbrMetallicRoughness"];

                if (pbr.contains("baseColorFactor"))
                {
                    std::array<float, 4> baseColorFactor = pbr["baseColorFactor"].get<std::array<float, 4>>();
                    newMaterial.baseColorFactor = {baseColorFactor[0], baseColorFactor[1], baseColorFactor[2], baseColorFactor[3]};
                }

                newMaterial.metallicFactor = pbr.value("metallicFactor", 1.0f);
                newMaterial.roughnessFactor = pbr.value("roughnessFactor", 1.0f);

                if (pbr.contains("baseColorTexture"))
                {
                    newMaterial.albedoTexture = loadTexture(pbr["baseColorTexture"]["index"]);
                }
            }

            m_scene.materials.push_back(newMaterial);
        }
    }
    
    const Scene &SceneManager::getScene(void) const
    {
        return (m_scene);
    }

    std::vector<Vertex> SceneManager::loadVertexPositions(int accessorIndex)
    {
        const auto &accessor = m_json["accessors"][accessorIndex];
        const auto &bufferView = m_json["bufferViews"][accessor["bufferView"]];
        const auto &buffer = m_bufferManager.getBufferData(bufferView["buffer"]);

        int byteOffset = bufferView.value("byteOffset", 0) + accessor.value("byteOffset", 0);
        int count = accessor["count"];
        std::vector<Vertex> positions(count);

        for (int i = 0; i < count; i++)
        {
            int offset = byteOffset + i * sizeof(v3f);
            memcpy(&positions[i], &buffer[offset], sizeof(v3f));
        }

        return (positions);
    }

    std::vector<unsigned int> SceneManager::loadIndices(int accessorIndex)
    {
        const auto &accessor = m_json["accessors"][accessorIndex];
        const auto &bufferView = m_json["bufferViews"][accessor["bufferView"]];
        const auto &buffer = m_bufferManager.getBufferData(bufferView["buffer"]);

        int byteOffset = bufferView.value("byteOffset", 0) + accessor.value("byteOffset", 0);
        int count = accessor["count"];
        std::vector<unsigned int> indices(count);

        std::string type = accessor["componentType"];
        if (type == "UNSIGNED_INT")
        {
            memcpy(indices.data(), &buffer[byteOffset], count * sizeof(unsigned int));
        }
        else if (type == "UNSIGNED_SHORT")
        {
            for (int i = 0; i < count; i++)
            {
                unsigned short index;
                memcpy(&index, &buffer[byteOffset + i * sizeof(unsigned short)], sizeof(unsigned short));
                indices[i] = index;
            }
        }
        else if (type == "UNSIGNED_BYTE")
        {
            for (int i = 0; i < count; i++)
            {
                indices[i] = buffer[byteOffset + i];
            }
        }

        return (indices);
    }

    std::string SceneManager::loadTexture(int textureIndex)
    {
        const auto &texture = m_json["textures"][textureIndex];
        const auto &sampler = m_json["samplers"][texture["sampler"]];
        const auto &image = m_json["images"][texture["source"]];
        const auto &bufferView = m_json["bufferViews"][image["bufferView"]];
        const auto &buffer = m_bufferManager.getBufferData(bufferView["buffer"]);

        return (image["uri"]);
    }
}
