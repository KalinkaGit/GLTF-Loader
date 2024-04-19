#include "GLTFLoader.hpp"

void print_scene(GLTFLoader::Scene scene)
{
    printf("Scene:\n");
    printf("  Nodes:\n");
    for (auto node : scene.nodes)
    {
        printf("    Node:\n");
        printf("      Name: %s\n", node->name.c_str());
        printf("      Mesh Index: %d\n", node->meshIndex);
        printf("      Translation: %f %f %f\n", node->translation.x, node->translation.y, node->translation.z);
        printf("      Rotation: %f %f %f %f\n", node->rotation.x, node->rotation.y, node->rotation.z, node->rotation.w);
        printf("      Scale: %f %f %f\n", node->scale.x, node->scale.y, node->scale.z);
        printf("      Transform:\n");
        for (int i = 0; i < 16; i++)
        {
            printf("        %f\n", node->transform.data[i]);
        }
        printf("      Children:\n");
        for (auto child : node->children)
        {
            printf("        %s\n", child->name.c_str());
        }
    }
    printf("  Meshes:\n");
    for (auto mesh : scene.meshes)
    {
        printf("    Mesh:\n");
        printf("      Material Index: %d\n", mesh.materialIndex);
        printf("      Vertices:\n");
        for (auto vertex : mesh.vertices)
        {
            printf("        Position: %f %f %f\n", vertex.position.x, vertex.position.y, vertex.position.z);
            printf("        Normal: %f %f %f\n", vertex.normal.x, vertex.normal.y, vertex.normal.z);
            printf("        TexCoords: %f %f\n", vertex.texCoords.x, vertex.texCoords.y);
        }
        printf("      Indices:\n");
        for (auto index : mesh.indices)
        {
            printf("        %d\n", index);
        }
    }
    printf("  Animation:\n");
    for (auto animation : scene.animations)
    {
        printf("    Animation:\n");
        printf("      Name: %s\n", animation.name.c_str());
        for (auto channel : animation.channels)
        {
            printf("      Channel:\n");
            printf("        Target Node: %s\n", channel.targetNode.c_str());
            for (auto key : channel.keys)
            {
                printf("        Key:\n");
                printf("          Time: %f\n", key.time);
                printf("          Translation: %f %f %f\n", key.translation.x, key.translation.y, key.translation.z);
                printf("          Rotation: %f %f %f %f\n", key.rotation.x, key.rotation.y, key.rotation.z, key.rotation.w);
                printf("          Scale: %f %f %f\n", key.scale.x, key.scale.y, key.scale.z);
            }
        }
    }
}

int main(void)
{
    GLTFLoader::Loader Loader("scene.gltf");
    if (Loader.load() == -1)
    {
        return (-1);
    }

    GLTFLoader::Scene scene = Loader.getScene();

    print_scene(scene);

    return (0);
}