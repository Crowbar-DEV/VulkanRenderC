//struct to hold the queue family we want from the device
//we will have an index for the graphics family we want and the
//NOTE: We do not use an optional like the tutorial, we use a pointer,
//if we find a queue that supports graphics we point the graphicsFamily
//to the heap where we store the index, then we can check if its NULL like
//an optional
typedef struct QueueFamilyIndices{
    uint32_t * graphicsFamily;
    uint32_t * presentFamily;
}QueueFamilyIndices;

//struct used to get bytecode from compiled shaders
typedef struct shaderCode{
    uint32_t * buffer;
    uint32_t size;
}shaderCode;

//struct used to define vertex data that will be passed to vertex shader
typedef struct Vertex{
    vec2 pos;
    vec3 color;
}Vertex;


//struct to hold properties of available swapchain
typedef struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR * formats;
    uint32_t formatCount;
    VkPresentModeKHR * presentModes;
    uint32_t presentModeCount;
}SwapChainSupportDetails;

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer * buffer, VkDeviceMemory * bufferMemory);
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
void cleanupSwapChain();
void createSwapChain();
void createImageViews();
void createFramebuffers();
static void framebufferResizeCallback(GLFWwindow * window, int width, int height);
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
bool isDeviceSuitable(VkPhysicalDevice device);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR * availableFormats, uint32_t formatCount);
VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR * availablePresentModes, uint32_t presentModeCount);
uint32_t uint32CLAMP(uint32_t val, uint32_t min, uint32_t max);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities);

