#ifndef VULKAN_RAW_HPP
#define VULKAN_RAW_HPP

/**
 * I'm currently in the process of refactoring this.
 * pls don't look.
 */


/**
 * TODO remove this def I'm just using it so I can lint/check the xcb code.
 **/
#define VK_USE_PLATFORM_XCB_KHR

#include <vulkan/vulkan.h>

/**
 * TODO I'd like to replace these directory based includes
 * with more project aware includes, but
 * that would require me to setup a more complicated build system.
 * it's on my list of things to do. Once we start getting a few more moving parts
 * I'll work on it and refactor everything. I'm planning on using CMAKE since it plays
 * nice with just about any IDE out there.
 **/
#include "../../WindowManagement/window.hpp"


#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <functional>
#include <unordered_map>

/**
 * NOTE put all of the platform dependent includes down here.
 **/

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif //WIN32 check

#ifdef VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#endif //XCB_INCLUDE

/**
 * C-style macros to get a procedure address based on a vulkan instance or device.
 *
 * TODO Sadly modules haven't been added in modern C++ yet and we have no way to do
 * preprocessor macros in modern C++ safely yet, so we should remove these macros eventually
 * since C-style macros don't respect C++ namespaces. NOTE when removing these there is a TODO corresponding
 * to this in the associated vulkan cpp file where the macros are called. This shouldn't be too hard to fix.
 **/
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint) {                      \
    fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetInstanceProcAddr(inst, "vk"#entrypoint)); \
    if(fp##entrypoint == NULL) {                                        \
      exit(1);                                                          \
    }                                                                   \
  }
#define GET_DEVICE_PROC_ADDR(dev, entrypoint) {                         \
    fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetDeviceProcAddr(dev, "vk"#entrypoint)); \
    if(fp##entrypoint == NULL) {                                        \
      exit(1);                                                          \
    }                                                                   \
  }

namespace VK {

  /**
   * I've attempted to safely wrap up most of the vulkan state/api so things are automatically allocated
   * and deleted as they get constructed/destroyed. I'm hoping that this api becomes more stable as we progress.

   * TODO/NOTE I haven't filled out too much information about what each of these members are for,
   * but the vulkan specification does a really good job and
   * even  has a search engine that you can type the type names into.
   *
   * NOTE here is a link to the documentation for all of the vulkan stuff.
   * https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/
   * Usually if you are wondering why something looks the way it does this
   * should be your first stop.
   **/


  

  struct PhysicalDevice {
    VkPhysicalDevice handle;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    std::vector<VkExtensionProperties> extensionProperties;

    PhysicalDevice(const VkPhysicalDevice& inst);

    /**
     * @return True if the device is a discrete gpu.
     **/
    bool isDiscrete() const;

    /**
     * @param the flags of the queues you want to fetch.
     * @return the indicies of all the queue with flags.
     **/
    std::vector<std::size_t> getQueueFamilies(const VkQueueFlagBits& flags) const;
  };


  struct Instance {
    // There can only be one vulkan instance per application
    static std::optional<std::size_t> count;
    static VkInstance handle;

    /**
     * @param appName the name of the application requesting an instance.
     * @param eingineName the name of the engine appName is using.
     * @param extensions the instance level extension to enable.
     **/
    Instance(const std::string& appName,
             const std::string& engineName,
             const std::vector<std::string>& extensions);
    ~Instance();

    /**
     * This gets a vector of all physical devices in the system.
     * @return all the physical devices on the system.
     */
    std::vector<PhysicalDevice> getPhysicalDevices() const;

    /**
     * This creates a display surface for the provided window w.
     * @param w the window you want to create a surface for.
     * @return Vulkan surface object.
     **/
    VkSurfaceKHR createSurface(SCREEN::Window& w);
  };


  // Everything below this point is basically broken


  struct LogicalDevice {
    VkDevice handle;
    LogicalDevice(const Instance& instance,
                  const PhysicalDevice& devices) {
    }
  };


  struct SwapChainBuffer {
    VkImage image;
    VkImageView view;
  };


  struct SwapChain {
  private:
    // Function ptrs see vulkan specification for documentation
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;

  public:
    void loadSurfacePFNS(const Instance& inst);

    VkSurfaceKHR surface;
    std::size_t presentQueue;
    VkFormat colorFormat;
    VkColorSpaceKHR colorSpace;
    VkSwapchainKHR swapChain;
    std::vector<SwapChainBuffer> buffers;
  };


  /**
   * TODO maybe rename this structure.
   **/
  struct Renderer {
    Instance instance;
    std::vector<PhysicalDevice> physicalDevices;
    LogicalDevice logicalDevice;
    std::optional<SwapChain> swapChain;
    Renderer(const std::string& engineName);
    ~Renderer();
  };

  bool checkQueueFamilyIndicies(const PhysicalDevice& dev);

  std::vector<std::size_t> filterQueueFamily(std::vector<std::size_t> qf,
                                             std::vector<std::size_t> qfc);

  void classifyQueueFamilyIndicies();

  void createLogicalDevice();

  void initPhysicalDevice();

  VkSurfaceKHR createSurface(SCREEN::Window& w);

  void createSwapChain(SCREEN::Window& w);




  /**
     @param devs a vector of physical devices.
     @param flags a set of queue flags.
     @return all the devices in devs that have queues with the flags enabled.
   */
  std::vector<PhysicalDevice> filterDevicesOnQueueFamily(const std::vector<PhysicalDevice>& devs,
                                                         const std::vector<VkQueueFlagBits>& flags) {
    std::vector<PhysicalDevice> ret;
    for(auto&& dev : devs) {
      std::vector<bool> flagCheck(flags.size());
      for(auto&& queue : dev.queueFamilyProperties) {
        for(std::size_t i = 0; i < flags.size(); i++) {
          if(queue.queueFlags && flags[i]) {
            flagCheck[i] = true;
          }
        }
      }
      if(std::accumulate(flagCheck.begin(),
                         flagCheck.end(),
                         true, std::logical_and<bool>())) {
        ret.push_back(dev);
      }
    }
    return ret;
  }

  //TODO a lot of documentation here.
  LogicalDevice createVkLogicalDevice(const PhysicalDevice& physDev,
                                      const VkPhysicalDeviceFeatures& enabledFeatures,
                                      const std::vector<std::size_t>& qi,
                                      const std::vector<uint32_t>& qc,
                                      const std::vector<std::vector<float>>& qp,
                                      void* pNextChain,
                                      const std::vector<std::string>& enabledExtensions) {
    LogicalDevice ret;
    std::vector<VkDeviceQueueCreateInfo> qs(qi.size());
    for(std::size_t i = 0; i < qi.size(); i++) {
      qs[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      qs[i].queueFamilyIndex = static_cast<uint32_t>(qi[i]);
      qs[i].queueCount = qc[i];
      qs[i].pQueuePriorities = &qp[i].front();
    }
    VkDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(qs.size());;
    deviceCreateInfo.pQueueCreateInfos = qs.data();
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2;
    if(pNextChain) {
      physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
      physicalDeviceFeatures2.features = enabledFeatures;
      physicalDeviceFeatures2.pNext = pNextChain;
      deviceCreateInfo.pEnabledFeatures = nullptr;
      deviceCreateInfo.pNext = &physicalDeviceFeatures2;
    }
    std::vector<char*> cstrExtensions;
    if (enabledExtensions.size() > 0) {
      deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
      for(std::size_t i = 0; i < enabledExtensions.size(); i++) {
        cstrExtensions.push_back(const_cast<char*>(enabledExtensions[i].c_str()));
      }
      deviceCreateInfo.ppEnabledExtensionNames = &cstrExtensions.front();
    }
    vkCreateDevice(physDev.deviceHandle, &deviceCreateInfo, nullptr, &ret.deviceHandle);
    return ret;
  }

  VkCommandPool createVkCommandPool(const LogicalDevice& ld,
                                    std::size_t qi,
                                    const std::vector<VkCommandPoolCreateFlags>& cf) {
    VkCommandPoolCreateInfo cinfo;
    cinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cinfo.queueFamilyIndex = static_cast<uint32_t>(qi);
    cinfo.flags = std::accumulate(cf.begin(), cf.end(), 0, std::bit_and<VkCommandPoolCreateFlags>());
    VkCommandPool cmdPool;
    vkCreateCommandPool(ld.deviceHandle, &cinfo, nullptr, &cmdPool);
    return cmdPool;
  }

  std::vector<VkSurfaceFormatKHR> getVkSurfaceFormats(const PhysicalDevice& physdev,
                                                      const VkSurfaceKHR& srf) {
    std::vector<VkSurfaceFormatKHR> ret;
    uint32_t formatCount = 0;
    //TODO error checking
    fpGetPhysicalDeviceSurfaceFormatsKHR(physdev.deviceHandle, srf, &formatCount, nullptr);
    ret.resize(formatCount);
    fpGetPhysicalDeviceSurfaceFormatsKHR(physdev.deviceHandle, srf, &formatCount, &ret.front());
    return ret;
  }

  std::vector<VkPresentModeKHR> getVkPresentModes(const PhysicalDevice& physdev,
                                                  const VkSurfaceKHR& srf) {
    uint32_t presentModeCount;
    //NOTE NO ERROR CHECKING!!!
		fpGetPhysicalDeviceSurfacePresentModesKHR(physdev.deviceHandle, srf, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		fpGetPhysicalDeviceSurfacePresentModesKHR(physdev.deviceHandle,
                                              srf, &presentModeCount, &presentModes.front());
    return presentModes;
  }

  VkSurfaceCapabilitiesKHR getVkSurfaceCapabiliteis(const PhysicalDevice& physdev,
                                                    const VkSurfaceKHR& srf) {
    VkSurfaceCapabilitiesKHR surfCaps;
    //NOTE NO ERROR CHECKING!!!
		fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physdev.deviceHandle, srf, &surfCaps);
    return surfCaps;
  }

  void deleteVkSwapChain(const VkInstance& inst,
                         const PhysicalDevice& physDev,
                         const LogicalDevice& logicalDev,
                         const VkSurfaceKHR& surface,
                         const std::size_t imageCount,
                         VkSwapchainKHR& swapchain) {
    for(std::size_t i = 0; i < imageCount; i++) {
      //TODO what are buffers here?
      //vkDestroyImageView(physDev.deviceHandle, buffers[i].view, nullptr);
    }
    vkDestroySwapchainKHR(logicalDev.deviceHandle, swapchain, nullptr);
    vkDestroySurfaceKHR(inst, surface, nullptr);
  }

  VkExtent2D createVkSwapChainExtent(const VkSurfaceCapabilitiesKHR& surfCaps,
                                     uint32_t* width, uint32_t* height) {
    VkExtent2D swapchainExtent;
		// If width (and height) equals the special value 0xFFFFFFFF,
    // the size of the surface will be set by the swapchain
		if(surfCaps.currentExtent.width == static_cast<uint32_t>(-1)) {
      // If the surface size is undefined, the size is set to
      // the size of the images requested.
      swapchainExtent.width = *width;
      swapchainExtent.height = *height;
    }
		else {
      // If the surface size is defined, the swap chain size must match
      swapchainExtent = surfCaps.currentExtent;
      *width = surfCaps.currentExtent.width;
      *height = surfCaps.currentExtent.height;
    }
    return swapchainExtent;
  }


  VkPresentModeKHR createVkPresentMode(const std::vector<VkPresentModeKHR>& presentModes, bool vsync) {
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    if(!vsync) {
      for (size_t i = 0; i < presentModes.size(); i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
          swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
          break;
        }
        if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
            (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
          swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
      }
    }
    return swapchainPresentMode;
  }

  VkCompositeAlphaFlagBitsKHR getVkSupportedAlpha(const VkSurfaceCapabilitiesKHR& surfCaps) {
    // Find a supported composite alpha format (not all devices support alpha opaque)
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // Simply select the first composite alpha format available
    std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
                                                                    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                                                    VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                                                                    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                                                                    VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for(auto& compositeAlphaFlag : compositeAlphaFlags) {
      if(surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
        compositeAlpha = compositeAlphaFlag;
        break;
      };
    }
    return compositeAlpha;
  }

  std::size_t getVkSwapChainImagesCount(const VkSurfaceCapabilitiesKHR& surfCaps) {
    // Determine the number of images
		uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
		if((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount)) {
      desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
    }
    return static_cast<std::size_t>(desiredNumberOfSwapchainImages);
  }

  VkSurfaceTransformFlagsKHR getVkSurfaceTransform(const VkSurfaceCapabilitiesKHR& surfCaps) {
    // Find the transformation of the surface
		VkSurfaceTransformFlagsKHR preTransform;
		if(surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
      // We prefer a non-rotated transform
      preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
		else {
      preTransform = surfCaps.currentTransform;
    }
    return preTransform;
  }



  std::vector<SwapChainBuffer> createVkSwapChainBuffers(const LogicalDevice& logicalDev,
                                                        const VkSwapchainKHR& swapchain,
                                                        const VkFormat& colorFormat,
                                                        const std::size_t imageCount) {
    std::vector<VkImage> imageHandles(imageCount);
    uint32_t imc = static_cast<uint32_t>(imageCount);
    vkGetSwapchainImagesKHR(logicalDev.deviceHandle, swapchain,
                            &imc, &imageHandles.front());
    std::vector<SwapChainBuffer> buffers(imageCount);
    for(std::size_t i = 0; i < imageHandles.size(); i++) {
      VkImageViewCreateInfo colorAttachmentView;
			colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorAttachmentView.pNext = NULL;
			colorAttachmentView.format = colorFormat;
			colorAttachmentView.components = {

                                        VK_COMPONENT_SWIZZLE_R,
                                        VK_COMPONENT_SWIZZLE_G,
                                        VK_COMPONENT_SWIZZLE_B,
                                        VK_COMPONENT_SWIZZLE_A
			};
			colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			colorAttachmentView.subresourceRange.baseMipLevel = 0;
			colorAttachmentView.subresourceRange.levelCount = 1;
			colorAttachmentView.subresourceRange.baseArrayLayer = 0;
			colorAttachmentView.subresourceRange.layerCount = 1;
			colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			colorAttachmentView.flags = 0;
      buffers[i].image = imageHandles[i];
      colorAttachmentView.image = buffers[i].image;
      vkCreateImageView(logicalDev.deviceHandle, &colorAttachmentView, nullptr, &buffers[i].view);
    }
    return buffers;
  }


  VkSwapchainKHR creatVkSwapChain(const LogicalDevice& logicalDev,
                                  const VkSwapchainKHR& oldSwapchain,
                                  const VkSurfaceCapabilitiesKHR& surfCaps,
                                  const VkColorSpaceKHR& colorSpace,
                                  const VkSurfaceKHR& srf,
                                  const VkFormat& colorFormat,
                                  const VkExtent2D& swapchainExtent,
                                  const VkCompositeAlphaFlagBitsKHR& compositeAlpha,
                                  const VkPresentModeKHR& swapchainPresentMode,
                                  const VkSurfaceTransformFlagsKHR& preTransform,
                                  const std::size_t swapchainImageCount) {
    VkSwapchainKHR swapChain;
		VkSwapchainCreateInfoKHR swapchainCI;
		swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCI.pNext = NULL;
		swapchainCI.surface = srf;
		swapchainCI.minImageCount = swapchainImageCount;
		swapchainCI.imageFormat = colorFormat;
		swapchainCI.imageColorSpace = colorSpace;
		swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.pQueueFamilyIndices = NULL;
		swapchainCI.presentMode = swapchainPresentMode;
		swapchainCI.oldSwapchain = oldSwapchain;
		// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
		swapchainCI.clipped = VK_TRUE;
		swapchainCI.compositeAlpha = compositeAlpha;
		// Enable transfer source on swap chain images if supported
		if(surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		// Enable transfer destination on swap chain images if supported
		if(surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
    vkCreateSwapchainKHR(logicalDev.deviceHandle, &swapchainCI, nullptr, &swapChain);
    return swapChain;
  }

  std::vector<std::size_t> filterPresentQueues(const std::vector<std::size_t>& queueIndicies,
                                               const PhysicalDevice& physDev,
                                               const VkSurfaceKHR& surf) {
    std::vector<std::size_t> ret;
    for(auto&& i : queueIndicies) {
      VkBool32 check;
      vkGetPhysicalDeviceSurfaceSupportKHR(physDev.deviceHandle, static_cast<uint32_t>(i), surf, &check);
      if(check == VK_TRUE) {
        ret.push_back(i);
      }
    }
    return ret;
  }

  // TODO document me.
  VkFormat getVkSupportedDepthFormat(VkPhysicalDevice physicalDevice) {
    std::vector<VkFormat> depthFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT,
                                           VK_FORMAT_D32_SFLOAT,
                                           VK_FORMAT_D24_UNORM_S8_UINT,
                                           VK_FORMAT_D16_UNORM_S8_UINT,
                                           VK_FORMAT_D16_UNORM
    };
    for (auto& format : depthFormats) {
      VkFormatProperties formatProps;
      vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
      // Format must support depth stencil attachment for optimal tiling
      if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        return format;
      }
    }
    // TODO oh no mr function has no return.
  }
};


/*

  NOTE Main for testing.

int main() {
  VkInstance appinst = VKRAW::createVkInstance("testapp", "testengine", {});
  auto devices = VKRAW::filterDevicesOnQueueFamily(VKRAW::getVkPhysDevices(appinst),
                                                   {VK_QUEUE_GRAPHICS_BIT});

  //auto devices = VKRAW::getVkPhysDevices(appinst);
  for (auto&& device : devices) {

    std::cout << device.deviceProperties.deviceName << "\n";

    auto qig = VKRAW::getQueueIndiciesWithSupport(device.queueFamilyProperties, {VK_QUEUE_GRAPHICS_BIT});
    auto qic = VKRAW::getQueueIndiciesWithSupport(device.queueFamilyProperties, {VK_QUEUE_COMPUTE_BIT});
    for(auto&& i : qig) {
      std::cout << i << "\n";
    }
    std::cout << "---\n";
    for(auto&& i : qic) {
      std::cout << i << "\n";
    }
  }
}
*/

#endif // VULKAN_RAW_HPP
