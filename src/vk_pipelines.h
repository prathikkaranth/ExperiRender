#pragma once 
#include <vk_initializers.h>
#include <vector>

namespace vkutil {
    bool load_shader_module(const char* filePath,
        VkDevice device,
        VkShaderModule* outShaderModule);
}

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _renderInfo;

    PipelineBuilder() { clear(); }

    void clear();

    VkPipeline build_pipeline(VkDevice device);


    enum BlendMode {
        ALPHA_BLEND,
        ADDITIVE_BLEND,
        NO_BLEND,
    };

    void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void set_input_topology(VkPrimitiveTopology topology);
    void set_polygon_mode(VkPolygonMode mode);
    void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void set_multisampling_none();
    void add_color_attachment(VkFormat format, BlendMode blendMode);
    void set_depth_format(VkFormat format);
    void disable_depthtest();
    void enable_depthtest(bool depthWriteEnable, VkCompareOp op);
    void clear_attachments();

private:
    std::vector<VkFormat> _colorAttachmentFormats;
    std::vector<VkPipelineColorBlendAttachmentState> _colorBlendAttachments;


};
