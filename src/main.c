#include "vulkan.h"
#include "stdlib.h"
#include "stdio.h"
#include "xcb.h"

VkInstance instance;

xcb_connection_t * connection;
xcb_window_t window;
xcb_screen_t * screen;
xcb_atom_t wmProtocols;
xcb_atom_t wmDeleteWin;

void createInstance(VkInstance instance){
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

    if(vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS){
        printf("Could not create Vulkan instance");
        exit(1);
    }
}

void cleanup(){
    vkDestroyInstance(instance, NULL);
}

void createWindow(){
    int screenp = 0;
    connection = xcb_connect(NULL, &screenp);

    if(xcb_connection_has_error(connection)){
        printf("could not connect to x server");
        exit(1);
    }
}

void renderLoop(){

}

int main(){
    createInstance(instance);   
}