#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "cglm/cglm.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "main.h"
#include "time.h"

//Yes, this program does have global state, I see no problem

uint32_t vertexCount = 4;
const Vertex vertices[4] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

uint32_t indicesCount = 6;
const uint16_t indices[6] = {
    0,1,2,2,3,0
};

//do we want validation layers?
const bool enableValidationLayers = true;
const uint8_t validationLayerCount = 1;
const char * const validationLayers[50] = {"VK_LAYER_KHRONOS_validation"};

const uint8_t deviceExtensionCount = 1;
const char * const deviceExtensions[50] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const uint16_t WIDTH = 1100;
const uint16_t HEIGHT = 800;

//max number of frames we allow to be rendered at once
const uint8_t MAX_FRAMES_IN_FLIGHT = 2;
uint8_t currentFrame = 0;

//timekeeping
clock_t startClock;

//window resize
bool framebufferResized = false;

//Vulkan objects
VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice;
VkDevice logicalDevice;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkRenderPass renderPass;
VkDescriptorSet * descriptorSets;
VkDescriptorSetLayout descriptorSetLayout;
VkDescriptorPool descriptorPool;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;
VkCommandPool commandPool;
VkCommandBuffer * commandBuffers;
uint8_t commandBufferCount;

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

VkBuffer * uniformBuffers;
VkDeviceMemory * uniformBuffersMemory;
void ** uniformBuffersMapped;

VkSemaphore * imageAvailableSemaphores;
uint8_t imageAvailableSemaphoreCount;
VkSemaphore * renderFinishedSemaphores;
uint8_t renderFinishedSemaphoreCount;
VkFence * inFlightFences;
uint8_t inFlightFenceCount;

VkSwapchainKHR swapChain;
uint32_t swapChainImageCount;
VkImage * swapChainImages;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;
VkImageView * swapChainImageViews;
VkFramebuffer * swapChainFrameBuffers;

//GLFW objects
GLFWwindow * window;

void createDescriptorSets(){
    VkDescriptorSetLayout * layouts = malloc(sizeof(VkDescriptorSetLayout) * MAX_FRAMES_IN_FLIGHT);

    for(uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        layouts[i] = descriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;

    descriptorSets = malloc(sizeof(VkDescriptorSet) * MAX_FRAMES_IN_FLIGHT);
    if(vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets) != VK_SUCCESS){
        printf("could not allocate descriptor sets");
        exit(1);
    }

    for(uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, NULL);
    }
}

void createDescriptorPool(){
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT;

    if(vkCreateDescriptorPool(logicalDevice, &poolInfo, NULL, &descriptorPool) != VK_SUCCESS){
        printf("could not create descriptor pool");
        exit(1);
    }
}

void updateUniformBuffer(uint8_t currentImage){
    UniformBufferObject ubo = {};

    clock_t diff;

    vec3 rotAxis = {0.0f,0.0f,1.0f};

    diff = clock() - startClock;
    float time = (float)((float)(diff) / (float)CLOCKS_PER_SEC);

    glm_mat4_identity(ubo.model);
    glm_rotate(ubo.model, time * glm_rad(90.0f), rotAxis);

    vec3 eye = {2.0f, 2.0f, 2.0f};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 0.0f, 1.0f};
    glm_lookat(eye, center, up, ubo.view);

    glm_perspective(glm_rad(60.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f, ubo.proj);
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void createUniformBuffers(){
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers = malloc(sizeof(VkBuffer) * MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory = malloc(sizeof(VkDeviceMemory) * MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped = malloc(sizeof(void*) * MAX_FRAMES_IN_FLIGHT);

    for(uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffers[i], &uniformBuffersMemory[i]);
        vkMapMemory(logicalDevice, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void createDescriptorSetLayout(){
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if(vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, NULL, &descriptorSetLayout) != VK_SUCCESS){
        printf("could not create descriptor set layout");
        exit(1);
    }

}

void createIndexBuffer(){
    VkDeviceSize bufferSize = sizeof(uint16_t) * indicesCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory); 

    void * data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices, (size_t)bufferSize);
    vkUnmapMemory(logicalDevice, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(logicalDevice, stagingBufferMemory, NULL);
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties){
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for(int i = 0; i < memoryProperties.memoryTypeCount; i++){
        if((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties){
            return i;
        }
    }

    printf("could not find suitable memory type");
    exit(1);
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer * buffer, VkDeviceMemory * bufferMemory){
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size; 
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(logicalDevice, &bufferInfo, NULL, buffer) != VK_SUCCESS){
        printf("could not create vertex buffer");
        exit(1);
    }

    VkMemoryRequirements memoryRequirements = {};
    vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memoryRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, propertyFlags);

    if(vkAllocateMemory(logicalDevice, &allocInfo, NULL, bufferMemory) != VK_SUCCESS){
        printf("could not allocate memory for vertex buffer");
        exit(1);
    }

    vkBindBufferMemory(logicalDevice, *buffer, *bufferMemory, 0);
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

void createVertexBuffer(){
    VkDeviceSize bufferSize = sizeof(Vertex) * vertexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
    
    void * data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(logicalDevice, stagingBufferMemory);
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(logicalDevice, stagingBufferMemory, NULL);
}

//Gets binding description for our vertex data
VkVertexInputBindingDescription getBindingDescription_Vertex(){
    VkVertexInputBindingDescription bindingDescription;
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

//gets a struct that represents the attributes of the binding
VkVertexInputAttributeDescription * getAttributeDescriptions_Vertex(){
    //we have two attribute descriptions, (position and color)
    VkVertexInputAttributeDescription * attributeDescriptions = malloc(sizeof(VkVertexInputAttributeDescription) * 2);
    memset(attributeDescriptions, 0, sizeof(VkVertexInputAttributeDescription) * 2);

    //attribute description for position attribute
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos); 

    //attribute description for color attribute
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color); 

    return attributeDescriptions;
}

void recreateSwapChain(){
    printf("recreating swap chain");
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(logicalDevice);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createFramebuffers();
}

//wait for previous frame to finish
//acquire image from swap chain
//record a command buffer which draws to the image
//submit recorded command buffer
//present the swap chain image
void drawFrame(){
    vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || framebufferResized){
        framebufferResized = false;
        recreateSwapChain();
        return;
    } else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        printf("could not get next image from swap chain");
        exit(1);
    }

    updateUniformBuffer(currentFrame);

    vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSemaphore waitSemaphores[1] = {imageAvailableSemaphores[currentFrame]};
    VkSemaphore signalSemaphores[1] = {renderFinishedSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame]; 
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS){
        printf("could not submit draw command buffer");
        exit(1);
    }

    VkSwapchainKHR swapChains[1] = {swapChain};

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized){
        framebufferResized = false;
        recreateSwapChain();
    } else if(result != VK_SUCCESS){
        printf("could not present swap chain image");
        exit(1);
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; 
}

//creates needed semaphores and fences
void createSyncObjects(){
    imageAvailableSemaphoreCount = MAX_FRAMES_IN_FLIGHT;
    renderFinishedSemaphoreCount = MAX_FRAMES_IN_FLIGHT;
    inFlightFenceCount = MAX_FRAMES_IN_FLIGHT;

    imageAvailableSemaphores = malloc(sizeof(VkSemaphore) * imageAvailableSemaphoreCount);
    renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * renderFinishedSemaphoreCount);
    inFlightFences = malloc(sizeof(VkFence) * inFlightFenceCount);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        if(vkCreateSemaphore(logicalDevice, &semaphoreInfo, NULL, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(logicalDevice, &semaphoreInfo, NULL, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(logicalDevice, &fenceInfo, NULL, &inFlightFences[i]) != VK_SUCCESS){
            printf("could not create synchronization objects");
            exit(1);
        }
    }
}

//record to our command buffer
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex){
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //both optional
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = NULL;

    if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS){
        printf("could not record to command buffer");
        exit(1);
    }

    VkOffset2D offset = {};
    offset.x = 0;
    offset.y = 0;

    VkClearValue clearColor = {};
    clearColor.color.float32[0] = 0.0f; 
    clearColor.color.float32[1] = 0.0f; 
    clearColor.color.float32[2] = 0.0f; 
    clearColor.color.float32[3] = 1.0f; 

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFrameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = offset;
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, NULL);
    vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
        printf("could not record command buffer");
        exit(1);
    }
}

//commandBuffers are what get sent to the gpu for an operation
void createCommandBuffers(){
    commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    commandBuffers = malloc(sizeof(VkCommandBuffer) * commandBufferCount);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBufferCount;

    if(vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers) != VK_SUCCESS){
        printf("could not allocate command buffer");
        exit(1);
    }
}

//command pools take care of the memory used to store command buffers
void createCommandPool(){
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = *(queueFamilyIndices.graphicsFamily);

    if(vkCreateCommandPool(logicalDevice, &poolInfo, NULL, &commandPool) != VK_SUCCESS){
        printf("could not create command pool");
        exit(1);
    }
}

//creates framebuffers for each image in swapchain
void createFramebuffers(){
    swapChainFrameBuffers = malloc(sizeof(VkFramebuffer) * swapChainImageCount);

    for(int i = 0; i < swapChainImageCount; i++){
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if(vkCreateFramebuffer(logicalDevice, &framebufferInfo, NULL, &swapChainFrameBuffers[i]) != VK_SUCCESS){
            printf("failed to create framebuffer");
            exit(1);
        }
    }
}

//creates the render pass object
void createRenderPass(){
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if(vkCreateRenderPass(logicalDevice, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS){
        printf("could not create render pass");
        exit(1);
    }
}

//creates a shader module, basically just a thin wrapper
//over the raw buffer
VkShaderModule createShaderModule(shaderCode code){
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = code.size;
    shaderModuleCreateInfo.pCode = code.buffer; 

    VkShaderModule shaderModule;
    if(vkCreateShaderModule(logicalDevice, &shaderModuleCreateInfo, NULL, &shaderModule) != VK_SUCCESS){
        printf("could not create shader module");
        exit(1);
    }

    return shaderModule;
}

//helper to read file into shaderCode buffer
static shaderCode readFile(const char * filename){
    shaderCode code = {};


    FILE * fptr;

    fptr = fopen(filename, "rb");

    if(fptr == NULL){
        printf("could not open shader code");
        exit(1);
    }

    fseek(fptr, 0L, SEEK_END);
    code.size = ftell(fptr) / 4;

    code.buffer = malloc(sizeof(uint32_t) * code.size);

    fseek(fptr, 0L, SEEK_SET);

    fread(code.buffer, sizeof(uint32_t), code.size, fptr);

    printf("bytecode size: %d\n", code.size * 4);

    code.size *= 4;
    fclose(fptr);

    return code;
}

//create graphics pipeline
void createGraphicsPipeline(){
    shaderCode vertShaderCode = readFile("../shaders/vert.spv");
    shaderCode fragShaderCode = readFile("../shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    /*
    uint8_t dynamicStateCount = 2;
    VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = dynamicStateCount;
    dynamicState.pDynamicStates = &dynamicStates;
    */

    VkVertexInputBindingDescription vertexBindingDescription = getBindingDescription_Vertex();
    VkVertexInputAttributeDescription * vertexAttributeDescription = getAttributeDescriptions_Vertex();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkOffset2D offset = {};
    offset.x = 0;
    offset.y = 0;

    VkRect2D scissor = {};
    scissor.offset = offset;
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    //i want to use wire frame at some point, apparently you have to enable a 
    //gpu feature for this, but this setting controls that
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = NULL;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    //all optional, bc blending is not enabled but all these
    //settings will create a new color by mixing the new color from fragment shader
    //with old color in frame buffer based on the operation and values defined here
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    //optional
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = NULL; 

    if(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS){
        printf("failed to create pipeline layout");
        exit(1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = NULL;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = NULL;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS){
        printf("could not create graphics pipeline");
        exit(1);
    }

    vkDestroyShaderModule(logicalDevice, fragShaderModule, NULL);
    vkDestroyShaderModule(logicalDevice, vertShaderModule, NULL);

    free(vertShaderCode.buffer);
    free(fragShaderCode.buffer);
}

//creates imageviews. To use a VkImage in the render pipeline,
//we must create a VkImageView object. We will be creating an image view
//for each image in the swap chain to eventually use as color targets in the
//rendering pipeline
void createImageViews(){
    swapChainImageViews = malloc(sizeof(VkImageView) * swapChainImageCount);

    for(int i = 0; i < swapChainImageCount; i++){
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapChainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = swapChainImageFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        if(vkCreateImageView(logicalDevice, &imageViewCreateInfo, NULL, &swapChainImageViews[i]) != VK_SUCCESS){
            printf("could not create image views");
            exit(1);
        }
    }
}

//creates swapchain
void createSwapChain(){
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats, swapChainSupport.formatCount);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, swapChainSupport.presentModeCount);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    swapChainImageCount = swapChainSupport.capabilities.minImageCount + 1;

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = surface;
    swapChainCreateInfo.minImageCount = swapChainImageCount;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {*(indices.graphicsFamily), *(indices.presentFamily)};

    if(*(indices.graphicsFamily) != *(indices.presentFamily)){
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else{
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = NULL;
    }

    swapChainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(logicalDevice, &swapChainCreateInfo, NULL, &swapChain) != VK_SUCCESS){
        printf("could not create swap chain");
        exit(1);
    }

    vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, NULL);
    swapChainImages = malloc(sizeof(VkImage) * swapChainImageCount);
    vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, swapChainImages); 

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

//choose swap chain surface format, we want 8 bit bgra in srgb color space
VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR * availableFormats, uint32_t formatCount){
    for(int i = 0; i < formatCount; i++){
        if(availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            return availableFormats[i];
        }
    }
    return availableFormats[0];
}

//choose swapchain present mode, we want mailbox
VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR * availablePresentModes, uint32_t presentModeCount){
    for(int i = 0; i < presentModeCount; i++){
        if(availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
            return availablePresentModes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

//Clamp implementation for 32 bit unsigned integers
uint32_t uint32CLAMP(uint32_t val, uint32_t min, uint32_t max){
    if(val < min) val = min;
    if(val > max) val = max;
    return val;
}

//choose size of swap chain images
VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities){
    if(capabilities.currentExtent.width != UINT32_MAX){
        return capabilities.currentExtent;
    }else{
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            (uint32_t)width,
            (uint32_t)height
        };

        actualExtent.width = uint32CLAMP(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = uint32CLAMP(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

//queuries details from available swapchain
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device){
    SwapChainSupportDetails details = {};
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    //queurying available surface formats
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, NULL);

    if(details.formatCount != 0){
        details.formats = malloc(sizeof(VkSurfaceFormatKHR) * details.formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, details.formats);
    }
    //queuerying available presentation mode()
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, NULL);

    if(details.presentModeCount != 0){
        details.presentModes = malloc(sizeof(VkPresentModeKHR) * details.presentModeCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.presentModeCount, details.presentModes);
    }

    return details;
}

//create the logical device
void createLogicalDevice(){
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {};
    graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueCreateInfo.queueFamilyIndex = *(indices.graphicsFamily);
    graphicsQueueCreateInfo.queueCount = 1;

    VkDeviceQueueCreateInfo presentQueueCreateInfo = {};
    presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    presentQueueCreateInfo.queueFamilyIndex = *(indices.presentFamily);
    presentQueueCreateInfo.queueCount = 1;

    float queuePriority = 1.0f;
    graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;
    presentQueueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceQueueCreateInfo queueCreateInfos[2] = {graphicsQueueCreateInfo,  presentQueueCreateInfo};

    //we will come back to this once we need more from vulkan
    VkPhysicalDeviceFeatures deviceFeatures;

    VkDeviceCreateInfo logicalDeviceCreateInfo = {};
    logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logicalDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    logicalDeviceCreateInfo.queueCreateInfoCount = 2;
    logicalDeviceCreateInfo.pEnabledFeatures = NULL;
    logicalDeviceCreateInfo.enabledExtensionCount = deviceExtensionCount;
    logicalDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    logicalDeviceCreateInfo.flags = 0;

    //not needed because already done in instance creation, but needed
    //for older vulkan implementations
    if(enableValidationLayers){
        logicalDeviceCreateInfo.enabledLayerCount = validationLayerCount;
        logicalDeviceCreateInfo.ppEnabledLayerNames = validationLayers;
    } else{
        logicalDeviceCreateInfo.enabledLayerCount = 0;
    }

    if(vkCreateDevice(physicalDevice, &logicalDeviceCreateInfo, NULL, &logicalDevice) != VK_SUCCESS){
        printf("could not create logical device");
        exit(1);
    }

    vkGetDeviceQueue(logicalDevice, *(indices.graphicsFamily), 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, *(indices.presentFamily), 0, &presentQueue);
}

//every card has a number of queues that belong to a queue family,
//queues are necessary to submit operations to GPU. We want to find a 
//queue family that supports graphics operations.
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device){
    QueueFamilyIndices indices = {
        NULL, 
        NULL
    };

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties * queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    VkBool32 presentSupport = false;

    for(int i = 0; i < queueFamilyCount; i++){
        vkGetPhysicalDeviceSurfaceSupportKHR(device,i,surface,&presentSupport);
        //check queue for present support
        if(presentSupport){
            indices.presentFamily = malloc(sizeof(uint32_t));
            *(indices.presentFamily) = i;
        }
        //check queue for graphics support
        if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            indices.graphicsFamily = malloc(sizeof(uint32_t));
            *(indices.graphicsFamily) = i;
            break;
        }
    }


    return indices;
}

//Picks a physical device, currently just chooses any discrete or integrated
//card
void pickPhysicalDevice(){

    //get number of devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

    if(deviceCount == 0){
        printf("no devices with vulkan support");
        exit(1);
    }

    //allocate space for multiple devices
    VkPhysicalDevice * devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    //check each device for suitability, if its suitable(discrete gpu)
    //then set it as our physical device
    for(int i = 0; i < deviceCount; i++){
        if(isDeviceSuitable(devices[i])){
            physicalDevice = devices[i];
            break;
        }
    }

    free(devices);

    if(physicalDevice == VK_NULL_HANDLE){
        printf("could not find suitable GPU");
        exit(1);
    }
}

//checks if our physical device suppports the extensions we want
bool checkDeviceExtensionSupport(VkPhysicalDevice device){
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    VkExtensionProperties * availableExtensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);

    bool supported = false;
    for(int i = 0; i < deviceExtensionCount; i++){
        for(int j = 0; j < extensionCount; j++){
            if(strcmp(deviceExtensions[i], availableExtensions[j].extensionName) == 0){
                supported = true;
                break;
            }
        }
    }

    return supported;
}

//checks if a device is suitable for our needs
//all we need as of now is a discrete or integrated gpu,
//and a queue family that supports graphics operations
bool isDeviceSuitable(VkPhysicalDevice device){
    //queury devices info, we really only care at this point
    //if our gpu is discrete(dedicated gpu) or integrated(my laptop)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if(extensionsSupported){
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = (swapChainSupport.formats != NULL) && (swapChainSupport.presentModes != NULL);
    }
    printf("DEVICE TYPE OK: %d\n", (deviceProperties.deviceType == (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)));
    printf("QUEUE OK: %d\n", (indices.graphicsFamily != NULL));
    printf("EXTENSIONS SUPPORTED: %d\n", extensionsSupported);
    printf("SWAP CHAIN OK: %d\n", swapChainAdequate);
    return 
        (deviceProperties.deviceType == (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU))
        && (indices.graphicsFamily != NULL)
        && (extensionsSupported)
        && swapChainAdequate;
}

//check if the validation layers we want are supported
bool checkValidationLayerSupport(){
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties * availableLayers = malloc(sizeof(VkLayerProperties) * layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    //for(int i = 0; i < layerCount; i++){
    //    printf("%s\n",(availableLayers + i)->layerName);
    //}

    //check wanted layers against queried layers
    bool supported = false;
    for(int i = 0; i < validationLayerCount; i++){
        const char * neededLayer = validationLayers[i];
        for(int j = 0; j < layerCount; j++){
            if(strcmp(neededLayer, (availableLayers + j)->layerName) == 0){
                supported = true;
                break;
            }
        }
    }

    free(availableLayers);

    return supported;
}

//Creates vulkan instance
void createInstance(){
    if(enableValidationLayers && !checkValidationLayerSupport()){
        printf("validation layers requested but not available");
        exit(1);
    }

    VkApplicationInfo appInfo = {};   
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "No engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    if(enableValidationLayers){
        createInfo.enabledLayerCount = validationLayerCount;
        createInfo.ppEnabledLayerNames = validationLayers;
    } else{
        createInfo.enabledLayerCount = 0;
    }

    if(vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS){
        printf("Could not create Vulkan instance");
        exit(1);
    }
}

//sets up glfw and creates a window
void initWindow(){
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

static void framebufferResizeCallback(GLFWwindow * window, int width, int height){
    framebufferResized = true;
}

//main 'engine' loop
void mainLoop(){
    startClock = clock();
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(logicalDevice);
}

//Creates a VkSurfaceKHR
void createSurface(){
    if(glfwCreateWindowSurface(instance,window,NULL,&surface) != VK_SUCCESS){
        printf("failed to create window surface");
        exit(1);
    }
}

//cleanup swapchain, seperated bc we need to also do this
//when recreating the swapchain
void cleanupSwapChain(){
    for(int i = 0; i < swapChainImageCount; i++){
        vkDestroyFramebuffer(logicalDevice, swapChainFrameBuffers[i], NULL);
    }

    for(int i = 0; i < swapChainImageCount; i++){
        vkDestroyImageView(logicalDevice, swapChainImageViews[i], NULL);
    }

    vkDestroySwapchainKHR(logicalDevice, swapChain, NULL);
}

//cleanup, order matters
void cleanup(){
    cleanupSwapChain();
    for(uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        vkDestroyBuffer(logicalDevice, uniformBuffers[i], NULL);
        vkFreeMemory(logicalDevice, uniformBuffersMemory[i], NULL);
    }
    vkDestroyDescriptorPool(logicalDevice, descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, NULL);
    vkDestroyBuffer(logicalDevice, vertexBuffer, NULL);
    vkFreeMemory(logicalDevice, vertexBufferMemory, NULL);
    vkDestroyBuffer(logicalDevice, indexBuffer, NULL);
    vkFreeMemory(logicalDevice, indexBufferMemory, NULL);
    vkDestroyPipeline(logicalDevice, graphicsPipeline, NULL);
    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, NULL);
    vkDestroyRenderPass(logicalDevice, renderPass, NULL);
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], NULL);
        vkDestroyFence(logicalDevice, inFlightFences[i], NULL);
    }
    vkDestroyCommandPool(logicalDevice, commandPool, NULL);
    vkDestroyDevice(logicalDevice, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
}

//driver
int main(){
    //glfw setup
    initWindow();
    //vulkan setup
    createInstance();  
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
    //loop and cleanup
    mainLoop();
    cleanup();
}
