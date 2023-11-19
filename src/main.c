#include "vulkan.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "xcb.h"
#include "stdbool.h"
#include "vulkan_xcb.h"
#include "main.h"

const bool enableValidationLayers = true;

const uint8_t validationLayerCount = 1;
const char * const validationLayers[50] = {"VK_LAYER_KHRONOS_validation",};

//Vulkan objects/handles (typedef'd pointers to internal structure)
VkInstance instance;
VkSurfaceKHR surface;

//Xcb objects
xcb_connection_t * connection;
xcb_window_t window;
xcb_screen_t * screen;
xcb_atom_t wmProtocols;
xcb_atom_t wmDeleteWin;

void pickPhysicalDevice(){
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

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

    if(physicalDevice == VK_NULL_HANDLE){
        printf("could not find suitable GPU");
        exit(1);
    }
}

bool isDeviceSuitable(VkPhysicalDevice device){
    //queury devices info, we really only care at this point
    //if our gpu is discrete(dedicated gpu) or integrated(my laptop)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    printf("%d", deviceProperties.deviceType);

    return deviceProperties.deviceType == (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
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

    VkApplicationInfo appInfo;   
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "No engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

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

//Creates a VkSurfaceKHR
//Mutates -> (global VkSurfaceKHR) surface
void createSurface(){
    VkXcbSurfaceCreateInfoKHR surfaceCreateInfo;
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.pNext = NULL;
    surfaceCreateInfo.connection = connection;
    surfaceCreateInfo.window = window;
    surfaceCreateInfo.flags = 0;

    vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
}

//Connects to x server using xcb and creates a window.
//xcb is strange, you connect to the server and then ask for
//replies by cookies which are just contracts to be given 
//back to the x server to actually give you the response 
//data, apparently this can be used to speed up requests
//by doing a bunch of requests without needing responses
//and then getting the responses later with the cookies
//mutates -> (global xcb_connection_t*) connection
//           (global xcb_window_t)  window;
//           (global xcb_screen_t*) screen;
//           (global xcb_atom_t) wmProtocols;
//           (global xcb_atom_t) wmDeleteWin;)
void createWindow(){
    int screenp = 0;
    connection = xcb_connect(NULL, &screenp);

    if(xcb_connection_has_error(connection)){
        printf("xcb could not connect to x server");
        exit(1);
    }

    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(connection));

    for(int s = screenp; s > 0; s--){
	    xcb_screen_next(&iter);
    }

    screen = iter.data;

    window = xcb_generate_id(connection);

    uint32_t eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    uint32_t * valueList = malloc(sizeof(uint32_t) * 2);

    valueList[0] = screen->black_pixel;
    valueList[1] = 0;

    xcb_create_window(
        connection,
        XCB_COPY_FROM_PARENT,
        window,
        screen->root,
        0,
        0,
        500,
        500,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        eventMask,
        valueList
    );

    free(valueList);

    xcb_intern_atom_cookie_t wmDeleteCookie = xcb_intern_atom(
        connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW"
    );
    xcb_intern_atom_cookie_t wmProtocolsCookie = xcb_intern_atom(
        connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS"
    );
    xcb_intern_atom_reply_t * wmDeleteReply = xcb_intern_atom_reply(
        connection, wmDeleteCookie, NULL
    );
    xcb_intern_atom_reply_t * wmProtocolsReply = xcb_intern_atom_reply(
        connection, wmProtocolsCookie, NULL
    );

    wmDeleteWin = wmDeleteReply->atom;
    wmProtocols = wmProtocolsReply->atom;

    xcb_change_property(
        connection, XCB_PROP_MODE_REPLACE, window, wmProtocolsReply->atom, 4, 32, 1, &wmDeleteReply->atom
    );

    xcb_map_window(connection, window);
    xcb_flush(connection);
}

//render loop, waits for events from xserver
//if we have a client message, check if window
//close was requested, then dip out
//mutates -> (global xcb_window_t) window
void renderLoop(){
    bool running = true;
    xcb_generic_event_t * event;
    xcb_client_message_event_t * client_message;

    while (running){
        event = xcb_wait_for_event(connection);

        switch(event->response_type & ~0x80) {
            case XCB_CLIENT_MESSAGE: {
                client_message = (xcb_client_message_event_t *)event;

                if(client_message->data.data32[0] == wmDeleteWin){
                    running = false;
                }

                break;
            }
        }
        free(event);
    }

    xcb_destroy_window(connection, window);
}

//cleanup
void cleanup(){
    vkDestroyInstance(instance, NULL);
}

//driver
int main(){
    createInstance();  
    pickPhysicalDevice();
    createWindow();
    renderLoop();
    cleanup();
}
