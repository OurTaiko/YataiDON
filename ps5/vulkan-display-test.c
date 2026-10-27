/* Host contract test: exercise the actual surface bridge without a GPU. */
#include <assert.h>
#include "native-vulkan.c"
static int no_devices, no_display, missing_proc, destroyed;
static VkDisplaySurfaceCreateInfoKHR last_surface;
static VkResult create_instance(const VkInstanceCreateInfo* info, const VkAllocationCallbacks* a, VkInstance* out) {
    assert(info->enabledExtensionCount == 2);
    *out = (VkInstance)(uintptr_t)1; return VK_SUCCESS;
}
static void destroy_instance(VkInstance i, const VkAllocationCallbacks* a) { ++destroyed; }
static VkResult devices(VkInstance i, uint32_t* n, VkPhysicalDevice* out) {
    *n = no_devices ? 0 : 1; if(out) out[0] = (VkPhysicalDevice)(uintptr_t)2; return VK_SUCCESS;
}
static VkResult displays(VkPhysicalDevice p, uint32_t* n, VkDisplayPropertiesKHR* out) {
    *n = no_display ? 0 : 1; if(out) out[0].display = (VkDisplayKHR)(uintptr_t)3; return VK_SUCCESS;
}
static VkResult modes(VkPhysicalDevice p, VkDisplayKHR d, uint32_t* n, VkDisplayModePropertiesKHR* out) {
    *n = 3;
    if(out) {
        out[0] = (VkDisplayModePropertiesKHR){(VkDisplayModeKHR)(uintptr_t)4, {{1280,720},60000}};
        out[1] = (VkDisplayModePropertiesKHR){(VkDisplayModeKHR)(uintptr_t)5, {{3840,2160},60000}};
        out[2] = (VkDisplayModePropertiesKHR){(VkDisplayModeKHR)(uintptr_t)6, {{7680,4320},60000}};
    }
    return VK_SUCCESS;
}
static VkResult planes(VkPhysicalDevice p, uint32_t* n, VkDisplayPlanePropertiesKHR* out) {
    *n = 1; if(out) out[0].currentStackIndex = 0; return VK_SUCCESS;
}
static VkResult supported(VkPhysicalDevice p, uint32_t plane, uint32_t* n, VkDisplayKHR* out) {
    *n = 1; if(out) out[0] = (VkDisplayKHR)(uintptr_t)3; return VK_SUCCESS;
}
static VkResult capabilities(VkPhysicalDevice p, VkDisplayModeKHR mode, uint32_t plane, VkDisplayPlaneCapabilitiesKHR* out) {
    memset(out,0,sizeof(*out)); out->supportedAlpha = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR; return VK_SUCCESS;
}
static VkResult surface(VkInstance i, const VkDisplaySurfaceCreateInfoKHR* info, const VkAllocationCallbacks* a, VkSurfaceKHR* out) {
    last_surface = *info; *out = (VkSurfaceKHR)(uintptr_t)7; return VK_SUCCESS;
}
PFN_vkVoidFunction VKAPI_CALL vk_common_GetDeviceProcAddr(VkDevice d, const char* name) { return NULL; }
PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance i, const char* name) {
    if (missing_proc) return NULL;
#define MAP(n, f) if(!strcmp(name, #n)) return (PFN_vkVoidFunction)f
    MAP(vkCreateInstance,create_instance); MAP(vkDestroyInstance,destroy_instance);
    MAP(vkEnumeratePhysicalDevices,devices); MAP(vkGetPhysicalDeviceDisplayPropertiesKHR,displays);
    MAP(vkGetDisplayModePropertiesKHR,modes); MAP(vkGetPhysicalDeviceDisplayPlanePropertiesKHR,planes);
    MAP(vkGetDisplayPlaneSupportedDisplaysKHR,supported); MAP(vkGetDisplayPlaneCapabilitiesKHR,capabilities);
    MAP(vkCreateDisplayPlaneSurfaceKHR,surface);
    return NULL;
}
int main(void) {
    int w=0,h=0;
    assert(yataidon_ps5_display_size(NULL,&h) == VK_ERROR_INITIALIZATION_FAILED);
    assert(yataidon_ps5_display_size(&w,&h) == VK_SUCCESS && w==3840 && h==2160 && destroyed==1);
    VkSurfaceKHR s=VK_NULL_HANDLE;
    assert(yataidon_ps5_vk_surface(NULL,(VkInstance)(uintptr_t)1,NULL,&s) == VK_SUCCESS);
    assert(last_surface.imageExtent.width==3840 && last_surface.imageExtent.height==2160);
    assert(last_surface.displayMode==(VkDisplayModeKHR)(uintptr_t)5);
    no_devices=1; assert(yataidon_ps5_display_size(&w,&h) == VK_ERROR_INITIALIZATION_FAILED && w==0 && h==0);
    no_devices=0; no_display=1; assert(yataidon_ps5_display_size(&w,&h) == VK_ERROR_INITIALIZATION_FAILED);
    no_display=0; missing_proc=1; assert(yataidon_ps5_display_size(&w,&h) == VK_ERROR_INITIALIZATION_FAILED);
    return 0;
}
