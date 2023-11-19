#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "main.h"

//do we want validation layers?
const bool enableValidationLayers = true;
const uint8_t validationLayerCount = 1;
const char * const validationLayers[50] = {"VK_LAYER_KHRONOS_validation"};

const uint8_t deviceExtensionCount = 1;
const char * const deviceExtensions[50] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

//Vulkan objects/handles (typedef'd pointers to internal structure)
VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice;
VkDevice logicalDevice;
VkQueue graphicsQueue;
VkQueue presentQueue;

VkSwapchainKHR swapChain;
uint32_t swapChainImageCount;
VkImage * swapChainImages;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;
VkImageView * swapChainImageViews;

//GLFW objects
GLFWwindow * window;

//create graphics pipeline
void createGraphicsPipeline(){

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
        uint32_t width = 0;
        uint32_t height = 0;
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
    SwapChainSupportDetails details = {
        NULL,
        NULL,
        0,
        NULL,
        0
    };
    
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
            }
        }
    }

    free(availableLayers);

    return supported;
}

//Creates vulkan instance
//we need a VkApplicationInfo and VkInstanceCreateInfo filled out
//to create our vulkan instance
//Mutates -> (global VkInstance) instance
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

void initWindow(){
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(800,800, "Vulkan", NULL, NULL);
}

void mainLoop(){
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
    }
}

//Creates a VkSurfaceKHR
//Mutates -> (global VkSurfaceKHR) surface
void createSurface(){
    if(glfwCreateWindowSurface(instance,window,NULL,&surface) != VK_SUCCESS){
        printf("failed to create window surface");
        exit(1);
    }
}


//cleanup, order matters
void cleanup(){
    glfwDestroyWindow(window);
    glfwTerminate();
    for(int i = 0; i < swapChainImageCount; i++){
        vkDestroyImageView(logicalDevice, swapChainImageViews[i], NULL);
    }
    vkDestroySwapchainKHR(logicalDevice, swapChain, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyDevice(logicalDevice, NULL);
    vkDestroyInstance(instance, NULL);
}

//driver
int main(){
    //glfw
    initWindow();
    //vulkan setup
    createInstance();  
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    //loop and cleanup
    mainLoop();
    cleanup();
}
