#include "../inc/Vulkan.hpp"

/**
 * Currently in the process of refactoring.
 * This contains some good and some bad.
 **/

namespace VK {

  /**
   * Instance struct member implementation
   **/
  Instance::Instance(const std::string& appName,
                     const std::string& engineName,
                     const std::vector<std::string>& extensions) {
    if(!this->count.has_value()) {
      this->count = 1;
      VkInstance instance;
      VkApplicationInfo appInfo;
      appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      appInfo.pApplicationName = appName.c_str();
      appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.pEngineName = engineName.c_str();
      appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.apiVersion = VK_API_VERSION_1_0;
      VkInstanceCreateInfo createInfo;
      createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      createInfo.pApplicationInfo = &appInfo;
      createInfo.enabledExtensionCount = extensions.size();
      std::vector<char*> cstrExtensions;
      for(std::size_t i = 0; i < extensions.size(); i++) {
        cstrExtensions.push_back(const_cast<char*>(extensions[i].c_str()));
      }
      createInfo.ppEnabledExtensionNames = &cstrExtensions.front();
      createInfo.enabledLayerCount = 0;
      vkCreateInstance(&createInfo, nullptr, &instance); // !! NOTE No error checking here.
      this->handle = instance;
    } else {
      this->count = this->count.value()++;
    }
  }

  Instance::~Instance() {
    this->count = this->count.value()--;
    if(this->count == 0) {
      this->count.reset();
      //NOTE the nullptr here is for memory allocation callbacks.
      vkDestroyInstance(this->handle, nullptr);
    }
  }

  std::vector<PhysicalDevice> Instance::getPhysicalDevices() const {
    std::vector<PhysicalDevice> devices;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(this->handle, &deviceCount, nullptr);
    if(deviceCount == 0) {
      std::cerr << "VK::getPhysicalDevices::NO_DEVICES\n";
    }
    std::vector<VkPhysicalDevice> vkdevices(deviceCount);
    vkEnumeratePhysicalDevices(this->handle, &deviceCount, &vkdevices.front());
    for(auto&& i : vkdevices) {
      devices.push_back(PhysicalDevice{i});
    }
    return devices;
  }

  VkSurfaceKHR Instance::createSurface(SCREEN::Window& w) {
    VkSurfaceKHR surface;
    auto bindings = w.getBindings();
#ifdef VK_USE_PLATFORM_WIN32_KHR
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hinstance = (HINSTANCE)bindings.platformHandle;
		surfaceCreateInfo.hwnd = (HWND)bindings.platformWindow;
		vkCreateWin32SurfaceKHR(this->handle, &surfaceCreateInfo, nullptr, &surface);
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
		VkXcbSurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.connection = bindings.connection;
		surfaceCreateInfo.window = bindings.window;
		vkCreateXcbSurfaceKHR(this->handle, &surfaceCreateInfo, nullptr, &surface);
#endif
    return surface;
  }


  /**
   * PhysicalDevice struct member implementation
   **/
  PhysicalDevice::PhysicalDevice(const VkPhysicalDevice& handle) : handle{handle} {
    vkGetPhysicalDeviceProperties(handle, &(this->properties));
    vkGetPhysicalDeviceFeatures(handle, &(this->features));
    vkGetPhysicalDeviceMemoryProperties(handle, &(this->memoryProperties));
    uint32_t count = 0;
    //TODO figure out what the nullptr is for.
    vkGetPhysicalDeviceQueueFamilyProperties(handle, &count, nullptr);
    this->queueFamilyProperties.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(handle, &count, &(this->queueFamilyProperties.front()));
    count = 0;
    vkEnumerateDeviceExtensionProperties(handle, nullptr, &count, nullptr);
    this->extensionProperties.resize(count);
    vkEnumerateDeviceExtensionProperties(handle, nullptr, &count, &(this->extensionProperties.front()));
  }

  bool PhysicalDevice::isDiscrete() const {
    return this->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
  }

  std::vector<std::size_t> PhysicalDevice::getQueueFamilies(const VkQueueFlagBits& flags) const {
    std::vector<std::size_t> indicies;
    for(std::size_t i = 0; i < this->queueFamilyProperties.size(); i++) {
      if(this->queueFamilyProperties[i].queueFlags && flags) {
        indicies.push_back(i);
      }
    }
    return indicies;
  }

  std::size_t selectPhysicalDevice(const std::vector<PhysicalDevice>& devs) {
    if(devs.size() == 0) {
      std::cerr << "VK::selectPhysicalDevice::NO_DEVICES\n";
    }
    //TODO maybe fix this up a bit currently we just return the first device satisfying our constraints.
    for(std::size_t i = 0; i < devs.size(); i++) {
      if(devs[i].isDiscrete() && checkQueueFamilyIndicies(devs[i])) {
        return i;
      }
    }
    std::cerr << "VK::selectPhysicalDevice::NO_DEVICES_MEETING_CRITERIA\n";
    return -1;
  }

  void SwapChain::loadSurfacePFNS(const Instance& inst) {
    //TODO rewrite these after removing the macros in the header.
    GET_INSTANCE_PROC_ADDR(inst.handle, GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(inst.handle, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(inst.handle, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(inst.handle, GetPhysicalDeviceSurfacePresentModesKHR);
  }

  Renderer::Renderer(const std::string& engineName) {
    if(this->rendererCount == 0) {
      this->instance = Instance(engineName,
                                engineName,
                                //These are platform specific extensions that
                                //get enabled or disabled at compile time.
                                {VK_KHR_SURFACE_EXTENSION_NAME,
                          #ifdef VK_USE_PLATFORM_WIN32_KHR
                                 VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
                          #endif
                          #ifdef VK_USE_PLATFORM_XCB_KHR
                                 VK_KHR_XCB_SURFACE_EXTENSION_NAME,
                          #endif
                          #ifdef VK_USE_PLATFORM_WAYLAND_KHR
                                 VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
                          #endif
                                });
      auto devs = instance.getPhysicalDevices();
      this->physicalDevice = devs[selectPhysicalDevice(devs)];
    }

  }


  bool checkQueueFamilyIndicies(const PhysicalDevice& dev) {
    return getQueueIndiciesWithSupport(dev.queueFamilyProperties,
                                              {VK_QUEUE_GRAPHICS_BIT,
                                               VK_QUEUE_COMPUTE_BIT,
                                               VK_QUEUE_TRANSFER_BIT}).size() > 0;
  }


  std::vector<std::size_t> filterQueueFamily(std::vector<std::size_t> qf,
                                             std::vector<std::size_t> qfc) {
    std::vector<std::size_t> ret;
    for(auto&& qi : qf) {
      bool ninqigc = true;
      for(auto&& qj : qfc) {
        if(qi == qj) {
          ninqigc = false;
          break;
        }
      }
      if(ninqigc) {
        ret.push_back(qi);
      }
    }
    return ret;
  }

  void classifyQueueFamilyIndicies() {
    allQueueIndicies =
      getQueueIndiciesWithSupport(physicalDevice.queueFamilyProperties,
                                         {VK_QUEUE_GRAPHICS_BIT,
                                          VK_QUEUE_COMPUTE_BIT,
                                          VK_QUEUE_TRANSFER_BIT});

    graphicsExclusiveQueueIndicies =
      filterQueueFamily(getQueueIndiciesWithSupport(physicalDevice.queueFamilyProperties,
                                                           {VK_QUEUE_GRAPHICS_BIT}),
                        allQueueIndicies);

    computeExclusiveQueueIndicies =
      filterQueueFamily(getQueueIndiciesWithSupport(physicalDevice.queueFamilyProperties,
                                                           {VK_QUEUE_COMPUTE_BIT}),
                        allQueueIndicies);

    transferExclusiveQueueIndicies = filterQueueFamily(getQueueIndiciesWithSupport(physicalDevice.queueFamilyProperties,
                                                                                          {VK_QUEUE_TRANSFER_BIT}),
                                                       allQueueIndicies);
  }

  void createLogicalDevice() {
    std::vector<uint32_t> qcount;
    for(auto q : allQueueIndicies) {
      qcount.push_back(physicalDevice.queueFamilyProperties[q].queueCount);
    }
    //TODO this may not be optimal and may need some more configuration.
    std::vector<std::vector<float>> qp;
    for(auto qc : qcount) {
      qp.push_back(std::vector<float>(static_cast<size_t>(qc), 1.0f));
    }
    VkPhysicalDeviceFeatures ef;
    classifyQueueFamilyIndicies();
    //NOTE maybe take a look at the extensions and features.
    logicalDevice = createVkLogicalDevice(physicalDevice,
                                          ef,
                                          allQueueIndicies,
                                          qcount,
                                          qp,
                                          nullptr,
                                          {});
  }

  void initPhysicalDevice() {
    auto devices{getVkPhysDevices(instanceHandle)};
    //TODO add support for sli/crossfire
    physicalDevice = devices[selectPhysicalDevice(devices)];
  }

  VkSurfaceKHR createSurface(SCREEN::Window& w) {
    auto windowBindings{w.getBindings()};
#ifdef VK_USE_PLATFORM_XCB_KHR
    return initSurface(instanceHandle, windowBindings.connection, windowBindings.window);
#endif
  }

  void createSwapChain(SCREEN::Window& w) {
    SwapChain setup;
    setup.surface = createSurface(w);
    //TODO I just choose the first queue that fits the bill here.
    setup.presentQueue = filterPresentQueues(allQueueIndicies, physicalDevice, setup.surface)[0];
  }



};
