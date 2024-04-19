/**
 * @file Loader.cpp
 * @author Kalinka (KalinkaGit) (remi.grimault@gmail.com)
 * @brief Loader class implementation
 * @version 1.0.0
 * @date 2024-04-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "Core/Loader/Loader.hpp"

namespace GLTFLoader
{
    Loader::Loader(const std::string &path) : m_path(path)
    {
    }

    int Loader::load(void)
    {
        std::ifstream file(m_path);
        if (!file.is_open())
        {
            return (-1);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string contents = buffer.str();

        m_json = nlohmann::json::parse(contents);
        if (parseJSON(m_json) < 0)
        {
            std::cerr << "Error while parsing JSON." << std::endl;
            return (-1);
        }

        if (loadBuffers() < 0)
        {
            std::cerr << "Error while loading buffers." << std::endl;
            return (-1);
        }

        if (loadMaterials() < 0)
        {
            std::cerr << "Error while loading materials." << std::endl;
            return (-1);
        }

        if (loadMeshes() < 0)
        {
            std::cerr << "Error while loading meshes." << std::endl;
            return (-1);
        }

        if (loadNodes() < 0)
        {
            std::cerr << "Error while loading nodes." << std::endl;
            return (-1);
        }

        if (loadAnimations() < 0)
        {
            std::cerr << "Error while loading animations." << std::endl;
            return (-1);
        }

        return (0);
    }

    Scene Loader::getScene(void) const
    {
        return (m_scene);
    }

    std::vector<unsigned char> Loader::loadBinaryData(const std::string &uri)
    {
        std::ifstream file(uri, std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Error: could not open file " << uri << std::endl;
            return (std::vector<unsigned char>());
        }

        std::vector<unsigned char> data(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        file.close();

        return (data);
    }

    int Loader::parseJSON(const nlohmann::json &json)
    {
        try
        {
            if (json.contains("nodes") && json["nodes"].is_array())
            {
                for (const auto &node : json["nodes"])
                {
                    if (!node.is_object()) {
                        continue;
                    }

                    std::shared_ptr<Node> newNode = std::make_shared<Node>();
                    newNode->name = node.value("name", "");

                    if (node.contains("rotation") && node["rotation"].is_array())
                    {
                        std::vector<float> rotation = node["rotation"].get<std::vector<float>>();
                        if (rotation.size() == 4) {
                            newNode->rotation = v4f{rotation[0], rotation[1], rotation[2], rotation[3]};
                        }
                    }

                    if (node.contains("scale") && node["scale"].is_array())
                    {
                        std::vector<float> scale = node["scale"].get<std::vector<float>>();
                        if (scale.size() == 3) {
                            newNode->scale = v3f{scale[0], scale[1], scale[2]};
                        }
                    }

                    if (node.contains("translation") && node["translation"].is_array())
                    {
                        std::vector<float> translation = node["translation"].get<std::vector<float>>();
                        if (translation.size() == 3) {
                            newNode->translation = v3f{translation[0], translation[1], translation[2]};
                        }
                    }

                    if (node.contains("mesh") && node["mesh"].is_number_integer())
                    {
                        newNode->meshIndex = node["mesh"].get<int>();
                    }

                    if (node.contains("matrix") && node["matrix"].is_array())
                    {
                        std::vector<float> matrixElements = node["matrix"].get<std::vector<float>>();
                        if (matrixElements.size() == 16) {
                            std::copy(matrixElements.begin(), matrixElements.end(), newNode->transform.data);
                        }
                    }

                    m_scene.nodes.push_back(newNode);
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error while parsing JSON: " << e.what() << std::endl;
            return -1;
        }

        return 0;
    }

    std::vector<GLTFLoader::Vertex> Loader::extractVertices(int accessorIndex)
    {
        if (!m_json["accessors"].is_array() || accessorIndex >= m_json["accessors"].size())
        {
            throw std::runtime_error("Accessors array index out of bounds or not an array.");
        }
        const auto &accessor = m_json["accessors"][accessorIndex];

        if (!accessor.contains("bufferView") || !accessor["bufferView"].is_number_unsigned())
        {
            throw std::runtime_error("Accessor does not contain a valid 'bufferView' index.");
        }

        int bufferViewIndex = accessor["bufferView"].get<int>();
        if (!m_json["bufferViews"].is_array() || bufferViewIndex >= m_json["bufferViews"].size())
        {
            throw std::runtime_error("BufferViews array index out of bounds or not an array.");
        }
        const auto &bufferView = m_json["bufferViews"][bufferViewIndex];

        if (!bufferView.contains("buffer") || !bufferView["buffer"].is_number_unsigned())
        {
            throw std::runtime_error("BufferView does not contain a valid 'buffer' index.");
        }

        const auto &buffer = m_buffers[bufferView["buffer"].get<int>()];

        int byteStride = bufferView.value("byteStride", sizeof(GLTFLoader::v3f));
        int byteOffset = bufferView.value("byteOffset", 0) + accessor.value("byteOffset", 0);
        int count = accessor["count"];

        std::vector<GLTFLoader::Vertex> vertices(count);

        for (int i = 0; i < count; ++i)
        {
            GLTFLoader::v3f position;
            if (byteOffset + i * byteStride + sizeof(GLTFLoader::v3f) > buffer.size())
            {
                throw std::runtime_error("Buffer access out of bounds.");
            }
            memcpy(&position, &buffer[byteOffset + i * byteStride], sizeof(GLTFLoader::v3f));
            vertices[i].position = {position.x, position.y, position.z};
        }

        return vertices;
    }

    std::vector<unsigned int> Loader::extractIndices(int accessorIndex)
    {
        if (!m_json["accessors"].is_array() || accessorIndex >= m_json["accessors"].size())
        {
            throw std::runtime_error("Accessors array index out of bounds or not an array.");
        }
        const auto &accessor = m_json["accessors"][accessorIndex];

        if (!accessor.contains("bufferView") || !accessor["bufferView"].is_number_unsigned())
        {
            throw std::runtime_error("Accessor does not contain a valid 'bufferView' index.");
        }

        int bufferViewIndex = accessor["bufferView"].get<int>();
        if (!m_json["bufferViews"].is_array() || bufferViewIndex >= m_json["bufferViews"].size())
        {
            throw std::runtime_error("BufferViews array index out of bounds or not an array.");
        }
        const auto &bufferView = m_json["bufferViews"][bufferViewIndex];

        if (!bufferView.contains("buffer") || !bufferView["buffer"].is_number_unsigned())
        {
            throw std::runtime_error("BufferView does not contain a valid 'buffer' index.");
        }

        const auto &buffer = m_buffers[bufferView["buffer"].get<int>()];
        int byteOffset = bufferView.value("byteOffset", 0) + accessor.value("byteOffset", 0);
        int count = accessor["count"];

        if (!accessor.contains("componentType"))
        {
            throw std::runtime_error("Accessor does not contain 'componentType'.");
        }
        int componentType = accessor["componentType"];

        std::vector<unsigned int> indices(count);

        switch (componentType)
        {
            case 5123: // UNSIGNED_SHORT
                for (int i = 0; i < count; ++i)
                {
                    unsigned short index;
                    memcpy(&index, &buffer[byteOffset + i * sizeof(unsigned short)], sizeof(unsigned short));
                    indices[i] = index;
                }
                break;
            case 5125: // UNSIGNED_INT
                memcpy(indices.data(), &buffer[byteOffset], count * sizeof(unsigned int));
                break;
            case 5121: // UNSIGNED_BYTE
                for (int i = 0; i < count; ++i)
                {
                    indices[i] = buffer[byteOffset + i];
                }
                break;
            default:
                std::cerr << "Error: unsupported component type " << componentType << std::endl;
                break;
        }

        return (indices);
    }

    int Loader::loadBuffers()
    {
        if (!m_json.contains("buffers") || !m_json["buffers"].is_array())
        {
            std::cerr << "No 'buffers' array found in the JSON data." << std::endl;
            return (-1);
        }

        m_buffers.reserve(m_json["buffers"].size());
        for (const auto& bufferJson : m_json["buffers"])
        {
            std::string uri = bufferJson["uri"].get<std::string>();

            std::vector<unsigned char> bufferData = loadBinaryData(uri);
            if (bufferData.empty())
            {
                std::cerr << "Failed to load buffer from URI: " << uri << std::endl;
                return false;
            }

            m_buffers.push_back(bufferData);
        }

        return (0);
    }

    int Loader::loadMeshes()
    {
        if (!m_json.contains("meshes") || !m_json["meshes"].is_array())
        {
            std::cerr << "No 'meshes' array found in JSON data." << std::endl;
            return (-1);
        }

        for (const auto& meshJson : m_json["meshes"])
        {
            Mesh newMesh;

            for (const auto& primitive : meshJson["primitives"])
            {
                if (!primitive.contains("attributes")) continue;

                auto attributes = primitive["attributes"];

                if (attributes.contains("POSITION"))
                {
                    int accessorIndex = attributes["POSITION"];
                    newMesh.vertices = extractVertices(accessorIndex);
                }

                if (primitive.contains("indices"))
                {
                    int indicesAccessor = primitive["indices"];
                    newMesh.indices = extractIndices(indicesAccessor);
                }

                if (primitive.contains("material"))
                {
                    newMesh.materialIndex = primitive["material"];
                }
            }

            m_scene.meshes.push_back(newMesh);
        }

        return (0);
    }

    int Loader::loadMaterials()
    {
        if (!m_json.contains("materials") || !m_json["materials"].is_array())
        {
            std::cerr << "No 'materials' array found in JSON data." << std::endl;
            return (-1);
        }

        for (const auto& materialJson : m_json["materials"])
        {
            if (!materialJson.is_object()) {
                std::cerr << "Material is not a JSON object." << std::endl;
                continue;
            }

            Material newMaterial;

            if (!materialJson.contains("pbrMetallicRoughness") || !materialJson["pbrMetallicRoughness"].is_object()) {
                std::cerr << "pbrMetallicRoughness not found or is not an object." << std::endl;
                continue;
            }

            auto& pbr = materialJson["pbrMetallicRoughness"];

            if (pbr.contains("baseColorFactor") && pbr["baseColorFactor"].is_array())
            {
                std::vector<float> color = pbr["baseColorFactor"].get<std::vector<float>>();
                if (color.size() == 4) {
                    newMaterial.baseColorFactor = v4f{color[0], color[1], color[2], color[3]};
                } else {
                    std::cerr << "baseColorFactor does not contain 4 elements." << std::endl;
                }
            }

            newMaterial.metallicFactor = pbr.value("metallicFactor", 1.0f);
            newMaterial.roughnessFactor = pbr.value("roughnessFactor", 1.0f);

            if (pbr.contains("baseColorTexture") && pbr["baseColorTexture"].is_object() && pbr["baseColorTexture"].contains("index"))
            {
                newMaterial.albedoTexture = loadTexture(pbr["baseColorTexture"]["index"]);
            }

            m_scene.materials.push_back(newMaterial);
        }

        return (0);
    }

    std::string Loader::loadTexture(int textureIndex)
    {
        if (!m_json.contains("textures") || !m_json["textures"].is_array())
        {
            std::cerr << "No 'textures' array found in JSON data." << std::endl;
            return ("");
        }

        if (textureIndex < 0 || textureIndex >= m_json["textures"].size())
        {
            std::cerr << "Texture index out of range." << std::endl;
            return ("");
        }

        const auto& texture = m_json["textures"][textureIndex];
        if (!texture.contains("source") || !texture["source"].is_number_integer())
        {
            std::cerr << "Texture source not found or not an integer." << std::endl;
            return ("");
        }

        int imageIndex = texture["source"].get<int>();

        if (imageIndex < 0 || imageIndex >= m_json["images"].size())
        {
            std::cerr << "Image index out of range." << std::endl;
            return ("");
        }

        const auto& image = m_json["images"][imageIndex];

        if (!image.contains("uri") || !image["uri"].is_string())
        {
            std::cerr << "Image URI not found or not a string." << std::endl;
            return ("");
        }

        std::string uri = image["uri"].get<std::string>();

        return (uri);
    }

    int Loader::loadNodes()
    {
        if (!m_json.contains("nodes") || !m_json["nodes"].is_array())
        {
            std::cerr << "No 'nodes' array found in the JSON data." << std::endl;
            return (-1);
        }

        std::vector<std::shared_ptr<Node>> tempNodes(m_json["nodes"].size());

        for (size_t i = 0; i < m_json["nodes"].size(); i++)
        {
            const auto& nodeJson = m_json["nodes"][i];
            auto newNode = std::make_shared<Node>();
            newNode->name = nodeJson.value("name", std::string("Unnamed_Node_") + std::to_string(i));

            if (nodeJson.contains("mesh") && nodeJson["mesh"].is_number_integer())
            {
                newNode->meshIndex = nodeJson["mesh"].get<int>();
            }

            if (nodeJson.contains("matrix") && nodeJson["matrix"].is_array())
            {
                std::vector<float> matrixElements = nodeJson["matrix"].get<std::vector<float>>();
                std::copy(matrixElements.begin(), matrixElements.end(), newNode->transform.data);
            }
            else
            {
                newNode->transform = m4f{1.0f};
            }

            tempNodes[i] = newNode;
        }

        for (size_t i = 0; i < m_json["nodes"].size(); i++)
        {
            const auto& nodeJson = m_json["nodes"][i];
            if (nodeJson.contains("children") && nodeJson["children"].is_array())
            {
                for (auto& childIndex : nodeJson["children"])
                {
                    if (childIndex.is_number_integer())
                    {
                        int index = childIndex.get<int>();
                        if (index >= 0 && index < tempNodes.size())
                        {
                            tempNodes[i]->children.push_back(tempNodes[index]);
                        }
                    }
                }
            }
        }

        for (const auto& node : tempNodes)
        {
            if (node->children.empty())
            {
                m_scene.nodes.push_back(node);
            }
        }

        return (0);
    }

    std::vector<unsigned char> Loader::resolveAccessor(int accessorIndex)
    {
        if (!m_json.contains("accessors") || !m_json["accessors"].is_array()
            || accessorIndex >= m_json["accessors"].size() || accessorIndex < 0)
        {
            std::cerr << "Invalid accessor index." << std::endl;
            throw std::runtime_error("Invalid accessor index in resolveAccessor.");
        }

        const auto &accessor = m_json["accessors"][accessorIndex];
        int bufferViewIndex = accessor.value("bufferView", -1);
        if (bufferViewIndex == -1 || !m_json.contains("bufferViews")
            || !m_json["bufferViews"].is_array() || bufferViewIndex >= m_json["bufferViews"].size())
        {
            std::cerr << "Invalid buffer view index." << std::endl;
            throw std::runtime_error("Invalid buffer view index in resolveAccessor.");
        }

        const auto &bufferView = m_json["bufferViews"][bufferViewIndex];
        int bufferIndex = bufferView.value("buffer", -1);
        if (bufferIndex == -1 || bufferIndex >= m_buffers.size())
        {
            std::cerr << "Invalid buffer index." << std::endl;
            throw std::runtime_error("Invalid buffer index in resolveAccessor.");
        }

        size_t byteOffset = bufferView.value("byteOffset", 0);
        size_t byteLength = bufferView.value("byteLength", 0);
        size_t byteStride = bufferView.value("byteStride", 0);

        const auto &buffer = m_buffers[bufferIndex];

        std::vector<unsigned char> data;
        if (byteStride > 0)
        {
            size_t offset = byteOffset;
            while (offset + byteStride <= byteOffset + byteLength
                    && offset < buffer.size())
            {
                for (size_t i = 0; i < byteStride; ++i)
                {
                    if (offset + i < buffer.size())
                    {
                        data.push_back(buffer[offset + i]);
                    }
                }

                offset += byteStride;
            }
        }
        else
        {
            data.insert(data.end(), buffer.begin() + byteOffset, buffer.begin() + byteOffset + byteLength);
        }

        return (data);
    }

    std::vector<float> Loader::extractAnimationTimes(int accessorIndex)
    {
        auto buffer = resolveAccessor(accessorIndex);
        std::vector<float> times;
        size_t elementsCount = buffer.size() / sizeof(float);

        for (size_t i = 0; i < elementsCount; ++i)
        {
            float time;
            memcpy(&time, &buffer[i * sizeof(float)], sizeof(float));
            times.push_back(time);
        }

        return (times);
    }

    std::vector<v3f> Loader::extractTranslationKeys(int accessorIndex)
    {
        auto buffer = resolveAccessor(accessorIndex);
        std::vector<v3f> translations;
        size_t elementsCount = buffer.size() / sizeof(v3f);

        for (size_t i = 0; i < elementsCount; ++i)
        {
            v3f translation;
            memcpy(&translation, &buffer[i * sizeof(v3f)], sizeof(v3f));
            translations.push_back(translation);
        }

        return (translations);
    }

    std::vector<v4f> Loader::extractRotationKeys(int accessorIndex)
    {
        auto buffer = resolveAccessor(accessorIndex);
        std::vector<v4f> rotations;
        size_t elementsCount = buffer.size() / sizeof(v4f);

        for (size_t i = 0; i < elementsCount; ++i)
        {
            v4f rotation;
            memcpy(&rotation, &buffer[i * sizeof(v4f)], sizeof(v4f));
            rotations.push_back(rotation);
        }

        return (rotations);
    }

    std::vector<v3f> Loader::extractScaleKeys(int accessorIndex)
    {
        auto buffer = resolveAccessor(accessorIndex);
        std::vector<v3f> scales;
        size_t elementsCount = buffer.size() / sizeof(v3f);

        for (size_t i = 0; i < elementsCount; ++i)
        {
            v3f scale;
            memcpy(&scale, &buffer[i * sizeof(v3f)], sizeof(v3f));
            scales.push_back(scale);
        }

        return (scales);
    }

    int Loader::loadAnimations()
    {
        if (!m_json.contains("animations") || !m_json["animations"].is_array())
        {
            std::cerr << "No animations found in the file." << std::endl;
            return -1;
        }

        const auto& animations = m_json["animations"];
        for (const auto& anim : animations)
        {
            Animation animation;
            if (anim.contains("name") && anim["name"].is_string())
            {
                animation.name = anim["name"];
            }

            if (!anim.contains("channels") || !anim["channels"].is_array())
            {
                std::cerr << "Animation missing channels." << std::endl;
                continue;
            }

            for (const auto& channel : anim["channels"])
            {
                AnimationChannel animChannel;

                if (channel.contains("target") && channel["target"].is_object() &&
                    channel["target"].contains("node") && channel["target"]["node"].is_number_integer())
                {
                    animChannel.targetNode = std::to_string(channel["target"]["node"].get<int>());
                }

                if (!channel.contains("sampler") || !channel["sampler"].is_number_integer())
                {
                    std::cerr << "Animation channel missing sampler index." << std::endl;
                    continue;
                }

                int samplerIndex = channel["sampler"];
                if (!anim.contains("samplers") || !anim["samplers"].is_array() || 
                    samplerIndex >= anim["samplers"].size() || !anim["samplers"][samplerIndex].is_object())
                {
                    std::cerr << "Invalid sampler index." << std::endl;
                    continue;
                }

                const auto& sampler = anim["samplers"][samplerIndex];
                if (!sampler.contains("input") || !sampler.contains("output") ||
                    !sampler["input"].is_number_integer() || !sampler["output"].is_number_integer())
                {
                    std::cerr << "Sampler missing required fields." << std::endl;
                    continue;
                }

                int inputIndex = sampler["input"];
                int outputIndex = sampler["output"];

/*
                std::vector<float> times = extractAnimationTimes(inputIndex);
                std::vector<v3f> translations = extractTranslationKeys(outputIndex);
                std::vector<v4f> rotations = extractRotationKeys(outputIndex);
                std::vector<v3f> scales = extractScaleKeys(outputIndex);
*/

                std::vector<float> times = extractAnimationTimes(inputIndex);
                // create empty vectors for translations, rotations, and scales
                std::vector<v3f> translations = {};
                std::vector<v4f> rotations = {};
                std::vector<v3f> scales = {};
        
                size_t numKeys = times.size();
                for (size_t i = 0; i < numKeys; ++i)
                {
                    AnimationKey key;
                    key.time = times[i];
                    if (i < translations.size()) key.translation = translations[i];
                    if (i < rotations.size()) key.rotation = rotations[i];
                    if (i < scales.size()) key.scale = scales[i];

                    animChannel.keys.push_back(key);
                }

                animation.channels.push_back(animChannel);
            }

            m_scene.animations.push_back(animation);
        }

        return (0);
    }
}
