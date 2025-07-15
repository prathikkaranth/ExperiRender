#include <filesystem>
#include <iostream>
#include <variant>
#include <vk_loader.h>

// header for std::visit

#include <glm/gtx/quaternion.hpp>
#include <vk_buffers.h>
#include <vk_images.h>
#include "stb_image.h"
#include "vk_engine.h"
#include "vk_types.h"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <spdlog/spdlog.h>

//> loadimg
std::optional<AllocatedImage> load_image(VulkanEngine *engine, fastgltf::Asset &asset, fastgltf::Image &image,
                                         std::string baseDir) {
    AllocatedImage newImage{};

    int width, height, nrChannels;

    std::visit(
        fastgltf::visitor{
            [](auto &arg) {},
            [&](fastgltf::sources::URI &filePath) {
                assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                assert(filePath.uri.isLocalPath()); // We're only capable of loading
                                                    // local files.

                const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());

                // Resolve the full path using filesystem utilities
                std::filesystem::path fullPath = std::filesystem::path(baseDir) / filePath.uri.path();
                fullPath = fullPath.lexically_normal(); // Normalize path separators

                unsigned char *data = stbi_load(fullPath.string().c_str(), &width, &height, &nrChannels, 4);
                if (data) {
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newImage = vkutil::create_image(engine, data, imagesize, VK_FORMAT_R8G8B8A8_UNORM,
                                                    VK_IMAGE_USAGE_SAMPLED_BIT, true, path.c_str());

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::Vector &vector) {
                unsigned char *data = stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()),
                                                            &width, &height, &nrChannels, 4);
                if (data) {
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newImage = vkutil::create_image(engine, data, imagesize, VK_FORMAT_R8G8B8A8_UNORM,
                                                    VK_IMAGE_USAGE_SAMPLED_BIT, true, "Loader Img alloc for Vector");

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::BufferView &view) {
                auto &bufferView = asset.bufferViews[view.bufferViewIndex];
                auto &buffer = asset.buffers[bufferView.bufferIndex];

                std::visit(fastgltf::visitor{// We only care about VectorWithMime here, because we
                                             // specify LoadExternalBuffers, meaning all buffers
                                             // are already loaded into a vector.
                                             [](auto &arg) {},
                                             [&](fastgltf::sources::Array &array) {
                                                 unsigned char *data =
                                                     stbi_load_from_memory(array.bytes.data() + bufferView.byteOffset,
                                                                           static_cast<int>(bufferView.byteLength),
                                                                           &width, &height, &nrChannels, 4);
                                                 if (data) {
                                                     VkExtent3D imagesize;
                                                     imagesize.width = width;
                                                     imagesize.height = height;
                                                     imagesize.depth = 1;

                                                     newImage = vkutil::create_image(
                                                         engine, data, imagesize, VK_FORMAT_R8G8B8A8_UNORM,
                                                         VK_IMAGE_USAGE_SAMPLED_BIT, true,
                                                         "Loader Image Allocation from Buffer view");

                                                     stbi_image_free(data);
                                                 }
                                             }},
                           buffer.data);
            },
        },
        image.data);

    // if any of the attempts to load the data failed, we haven't written the image
    // so handle is null
    if (newImage.image == VK_NULL_HANDLE) {
        return {};
    } else {
        return newImage;
    }
}


VkFilter extract_filter(fastgltf::Filter filter) {
    switch (filter) {
            // nearest samplers
        case fastgltf::Filter::Nearest:
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::NearestMipMapLinear:
            return VK_FILTER_NEAREST;

            // linear samplers
        case fastgltf::Filter::Linear:
        case fastgltf::Filter::LinearMipMapNearest:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_FILTER_LINEAR;
    }
}

VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter) {
    switch (filter) {
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::LinearMipMapNearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine *engine, std::string_view filePath) {
    spdlog::info("Loading GLTF: {}", filePath);


    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
    scene->creator = engine;
    LoadedGLTF &file = *scene;

    // Enable required extensions
    fastgltf::Parser parser{fastgltf::Extensions::KHR_materials_transmission |
                            fastgltf::Extensions::KHR_lights_punctual | fastgltf::Extensions::KHR_materials_ior};

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                 fastgltf::Options::LoadExternalBuffers;
    // fastgltf::Options::LoadExternalImages;

    auto gltfFile = fastgltf::MappedGltfFile::FromPath(filePath);
    if (!bool(gltfFile)) {
        std::cerr << "Failed to open glTF file: " << fastgltf::getErrorMessage(gltfFile.error()) << '\n';
        throw std::runtime_error("Failed to open glTF file");
    }

    fastgltf::Asset gltf;

    std::filesystem::path path = filePath;

    auto type = fastgltf::determineGltfFileType(gltfFile.get());
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        } else {
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    } else if (type == fastgltf::GltfType::GLB) {
        auto load = parser.loadGltfBinary(gltfFile.get(), path.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        } else {
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            throw std::runtime_error("Failed to load glTF");
        }
    } else {
        std::cerr << "Failed to determine glTF container" << std::endl;
        throw std::runtime_error("Failed to load glTF");
    }

    // we can stimate the descriptors we will need accurately
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
                                                                     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                                                                     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

    file.descriptorPool.init(engine->_device, gltf.materials.size(), sizes);

    // load samplers
    for (fastgltf::Sampler &sampler: gltf.samplers) {

        VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
        sampl.maxLod = VK_LOD_CLAMP_NONE;
        sampl.minLod = 0;
        sampl.mipLodBias = 0.0f;

        sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Linear));
        sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::LinearMipMapLinear));

        sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::LinearMipMapLinear));

        // Enable anisotropic filtering for better texture quality
        sampl.anisotropyEnable = VK_TRUE;
        sampl.maxAnisotropy = 16.0f;

        VkSampler newSampler;
        vkCreateSampler(engine->_device, &sampl, nullptr, &newSampler);

        file.samplers.push_back(newSampler);
    }

    // temporal arrays for all the objects to use while creating the GLTF data
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<AllocatedImage> images;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;

    // load all textures
    for (fastgltf::Image &image: gltf.images) {
        std::string baseDir = (path.parent_path() / "").string();
        std::optional<AllocatedImage> img = load_image(engine, gltf, image, baseDir);

        if (img.has_value()) {
            images.push_back(*img);
            image.name = std::to_string(images.size() - 1);
            file.images[image.name.c_str()] = *img;

        } else {
            // we failed to load, so lets give the slot a default white texture to not
            // completely break loading
            images.push_back(engine->_resourceManager.getErrorCheckerboardImage());
            std::cout << "gltf failed to load texture " << image.name << std::endl;
        }
    }


    // create buffer to hold the material data
    file.materialDataBuffer = vkutil::create_buffer(
        engine, sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, "GLTF Material Data Buffer");
    int data_index = 0;
    auto *sceneMaterialConstants =
        static_cast<GLTFMetallic_Roughness::MaterialConstants *>(file.materialDataBuffer.info.pMappedData);

    for (fastgltf::Material &mat: gltf.materials) {
        std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
        materials.push_back(newMat);
        file.materials[mat.name.c_str()] = newMat;

        GLTFMetallic_Roughness::MaterialConstants constants{};
        constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
        constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
        constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
        constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

        constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
        constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;

        constants.hasMetalRoughTex = mat.pbrData.metallicRoughnessTexture.has_value();

        // Handle transmission properties
        constants.transmissionFactor = mat.transmission ? mat.transmission->transmissionFactor : 0.0f;
        constants.hasTransmissionTex = mat.transmission && mat.transmission->transmissionTexture.has_value();

        // Handle IOR (Index of Refraction)
        constants.ior = mat.ior;

        // Handle emissive properties
        constants.emissiveFactor = glm::vec4(mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2], 1.0f);
        constants.hasEmissiveTex = false; // Will be set to true only if texture is successfully loaded

        // Material constants will be written after texture loading

        MaterialPass passType = MaterialPass::MainColor;
        if (mat.alphaMode == fastgltf::AlphaMode::Blend || constants.transmissionFactor > 0.0f) {
            passType = MaterialPass::Transparent;
        }

        GLTFMetallic_Roughness::MaterialResources materialResources{};
        // default the material textures
        materialResources.colorImage = engine->_resourceManager.getWhiteImage();
        materialResources.colorSampler = engine->_resourceManager.getLinearSampler();
        materialResources.metalRoughImage = engine->_resourceManager.getWhiteImage();
        materialResources.metalRoughSampler = engine->_resourceManager.getLinearSampler();
        materialResources.normalImage = engine->_resourceManager.getGreyImage();
        materialResources.normalSampler = engine->_resourceManager.getLinearSampler();
        materialResources.transmissionImage = engine->_resourceManager.getWhiteImage();
        materialResources.transmissionSampler = engine->_resourceManager.getLinearSampler();
        materialResources.emissiveImage = engine->_resourceManager.getBlackImage();
        materialResources.emissiveSampler = engine->_resourceManager.getLinearSampler();

        // For RT
        materialResources.albedo = glm::vec4(mat.pbrData.baseColorFactor[0], mat.pbrData.baseColorFactor[1],
                                             mat.pbrData.baseColorFactor[2], mat.pbrData.baseColorFactor[3]);
        materialResources.albedoTexIndex = data_index;
        materialResources.metalRoughFactors = constants.metal_rough_factors;
        materialResources.transmissionFactor = constants.transmissionFactor;
        materialResources.ior = constants.ior;
        materialResources.emissiveFactor = constants.emissiveFactor;
        materialResources.emissiveTexIndex = data_index;

        // set the uniform buffer for the material data
        materialResources.dataBuffer = file.materialDataBuffer.buffer;
        materialResources.dataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);
        // grab textures from gltf file
        // albedo
        if (mat.pbrData.baseColorTexture.has_value()) {
            const auto &baseColorTexture = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex];
            if (baseColorTexture.imageIndex.has_value()) {
                size_t img = baseColorTexture.imageIndex.value();
                materialResources.colorImage = images[img];
                materialResources.colorTexIndex = static_cast<uint32_t>(img);

                if (baseColorTexture.samplerIndex.has_value()) {
                    size_t sampler = baseColorTexture.samplerIndex.value();
                    materialResources.colorSampler = file.samplers[sampler];
                } else {
                    materialResources.colorSampler = engine->_resourceManager.getLinearSampler();
                }
            }
        }
        // metallic roughness
        if (mat.pbrData.metallicRoughnessTexture.has_value()) {
            const auto &metallicRoughnessTexture =
                gltf.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex];
            if (metallicRoughnessTexture.imageIndex.has_value()) {
                size_t img = metallicRoughnessTexture.imageIndex.value();
                materialResources.metalRoughImage = images[img];

                if (metallicRoughnessTexture.samplerIndex.has_value()) {
                    size_t sampler = metallicRoughnessTexture.samplerIndex.value();
                    materialResources.metalRoughSampler = file.samplers[sampler];
                } else {
                    materialResources.metalRoughSampler = engine->_resourceManager.getLinearSampler();
                }
            }
        }
        // normal
        if (mat.normalTexture.has_value()) {
            const auto &normalTexture = gltf.textures[mat.normalTexture.value().textureIndex];
            if (normalTexture.imageIndex.has_value()) {
                size_t img = normalTexture.imageIndex.value();
                materialResources.normalImage = images[img];

                if (normalTexture.samplerIndex.has_value()) {
                    size_t sampler = normalTexture.samplerIndex.value();
                    materialResources.normalSampler = file.samplers[sampler];
                } else {
                    materialResources.normalSampler = engine->_resourceManager.getLinearSampler();
                }
            }
        }
        // transmission
        if (mat.transmission && mat.transmission->transmissionTexture.has_value()) {
            const auto &transmissionTexture = gltf.textures[mat.transmission->transmissionTexture.value().textureIndex];
            if (transmissionTexture.imageIndex.has_value()) {
                size_t img = transmissionTexture.imageIndex.value();
                materialResources.transmissionImage = images[img];

                if (transmissionTexture.samplerIndex.has_value()) {
                    size_t sampler = transmissionTexture.samplerIndex.value();
                    materialResources.transmissionSampler = file.samplers[sampler];
                } else {
                    materialResources.transmissionSampler = engine->_resourceManager.getLinearSampler();
                }
            }
        }
        // emissive
        if (mat.emissiveTexture.has_value()) {
            const auto &emissiveTexture = gltf.textures[mat.emissiveTexture.value().textureIndex];
            if (emissiveTexture.imageIndex.has_value()) {
                size_t img = emissiveTexture.imageIndex.value();
                materialResources.emissiveImage = images[img];
                constants.hasEmissiveTex = true; // Set flag only when texture is successfully loaded

                if (emissiveTexture.samplerIndex.has_value()) {
                    size_t sampler = emissiveTexture.samplerIndex.value();
                    materialResources.emissiveSampler = file.samplers[sampler];
                } else {
                    // Use default linear sampler if no sampler specified
                    materialResources.emissiveSampler = engine->_resourceManager.getLinearSampler();
                }
            }
        }

        // Update the constants after texture loading
        sceneMaterialConstants[data_index] = constants;

        // build material
        newMat->data = engine->metalRoughMaterial.write_material(engine, engine->_device, passType, materialResources,
                                                                 file.descriptorPool);

        data_index++;
    }

    // use the same vectors for all meshes so that the memory doesnt reallocate as
    // often
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (fastgltf::Mesh &mesh: gltf.meshes) {
        std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
        meshes.push_back(newmesh);
        file.meshes[mesh.name.c_str()] = newmesh;
        newmesh->name = mesh.name;

        // clear the mesh arrays each mesh, we dont want to merge them by error
        indices.clear();
        vertices.clear();

        for (auto &&p: mesh.primitives) {
            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t) indices.size();
            newSurface.count = (uint32_t) gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vtx = vertices.size();

            // load indexes
            {
                fastgltf::Accessor &indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(
                    gltf, indexaccessor, [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
            }

            // load vertex positions
            {
                fastgltf::Accessor &posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor, [&](glm::vec3 v, size_t index) {
                    Vertex newvtx{};
                    newvtx.position = v;
                    newvtx.normal = {1, 0, 0};
                    newvtx.color = glm::vec4{1.f};
                    newvtx.uv_x = 0;
                    newvtx.uv_y = 0;
                    vertices[initial_vtx + index] = newvtx;
                });
            }

            // load tangents
            auto tangents = p.findAttribute("TANGENT");
            if (tangents != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                    gltf, gltf.accessors[(*tangents).second],
                    [&](glm::vec4 v, size_t index) { vertices[initial_vtx + index].tangent = v; });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 v, size_t index) { vertices[initial_vtx + index].normal = v; });
            }

            // calculate bi-tangents from normals and tangents
            for (auto &v: vertices) {
                v.bitangent = glm::cross(v.normal, v.tangent);
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                                                              [&](glm::vec2 v, size_t index) {
                                                                  vertices[initial_vtx + index].uv_x = v.x;
                                                                  vertices[initial_vtx + index].uv_y = v.y;
                                                              });
            }

            // load vertex colors
            if (auto colors = p.findAttribute("COLOR_0"); colors != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                    gltf, gltf.accessors[(*colors).second],
                    [&](glm::vec4 v, size_t index) { vertices[initial_vtx + index].color = v; });
            }

            if (p.materialIndex.has_value()) {
                newSurface.material = materials[p.materialIndex.value()];
            } else {
                newSurface.material = materials[0];
            }

            // loop the vertices of this surface, find min/max bounds
            glm::vec3 minpos = vertices[initial_vtx].position;
            glm::vec3 maxpos = vertices[initial_vtx].position;
            for (int i = static_cast<int>(initial_vtx); i < vertices.size(); i++) {
                minpos = glm::min(minpos, vertices[i].position);
                maxpos = glm::max(maxpos, vertices[i].position);
            }
            // calculate origin and extents from the min/max, use extent length for radius
            newSurface.bounds.origin = (maxpos + minpos) / 2.f;
            newSurface.bounds.extents = (maxpos - minpos) / 2.f;
            newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);

            newmesh->surfaces.push_back(newSurface);
        }
        newmesh->nbIndices = static_cast<uint32_t>(indices.size());
        newmesh->nbVertices = static_cast<uint32_t>(vertices.size());
        newmesh->meshIndex = static_cast<uint32_t>(meshes.size()) - 1;

        newmesh->meshBuffers = engine->uploadMesh(indices, vertices);
    }

    // load all nodes and their meshes
    for (fastgltf::Node &node: gltf.nodes) {
        std::shared_ptr<Node> newNode;

        // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode
        // class
        if (node.meshIndex.has_value()) {
            newNode = std::make_shared<MeshNode>();
            dynamic_cast<MeshNode *>(newNode.get())->mesh = meshes[*node.meshIndex];
        } else {
            newNode = std::make_shared<Node>();
        }

        nodes.push_back(newNode);
        file.nodes[node.name.c_str()];

        std::visit(fastgltf::visitor{[&](fastgltf::math::fmat4x4 matrix) {
                                         memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
                                     },
                                     [&](fastgltf::TRS transform) {
                                         glm::vec3 tl(transform.translation[0], transform.translation[1],
                                                      transform.translation[2]);
                                         glm::quat rot(transform.rotation[3], transform.rotation[0],
                                                       transform.rotation[1], transform.rotation[2]);
                                         glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                                         glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                         glm::mat4 rm = glm::toMat4(rot);
                                         glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                         newNode->localTransform = tm * rm * sm;
                                     }},
                   node.transform);
    }

    // run loop again to setup transform hierarchy
    for (int i = 0; i < gltf.nodes.size(); i++) {
        fastgltf::Node &node = gltf.nodes[i];
        std::shared_ptr<Node> &sceneNode = nodes[i];

        for (auto &c: node.children) {
            sceneNode->children.push_back(nodes[c]);
            nodes[c]->parent = sceneNode;
        }
    }

    // find the top nodes, with no parents
    for (auto &node: nodes) {
        if (node->parent.lock() == nullptr) {
            file.topNodes.push_back(node);
            node->refreshTransform(glm::mat4{1.f});
        }
    }
    return scene;
}

void LoadedGLTF::Draw(const glm::mat4 &topMatrix, DrawContext &ctx) {
    // create renderables from the scenenodes
    for (const auto &n: topNodes) {
        n->Draw(topMatrix, ctx);
    }
}

void LoadedGLTF::clearAll() {
    VkDevice dv = creator->_device;

    descriptorPool.destroy_pools(dv);
    vkutil::destroy_buffer(creator, materialDataBuffer);

    for (auto &[k, v]: meshes) {

        vkutil::destroy_buffer(creator, v->meshBuffers.indexBuffer);
        vkutil::destroy_buffer(creator, v->meshBuffers.vertexBuffer);
    }

    for (auto &[k, v]: images) {
        VmaAllocationInfo info;
        vmaGetAllocationInfo(creator->_allocator, v.allocation, &info);
        if (v.image == creator->_resourceManager.getErrorCheckerboardImage().image) {
            // dont destroy the default images
            continue;
        }
        vkutil::destroy_image(creator, v);
    }

    for (const auto &sampler: samplers) {
        vkDestroySampler(dv, sampler, nullptr);
    }

    samplers.clear();

    // clear all the nodes
    topNodes.clear();
    nodes.clear();
}
