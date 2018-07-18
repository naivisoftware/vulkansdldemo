#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <set>

// Globals
const char				gAppName[] = "VulkanDemo";
const char				gEngineName[] = "VulkanDemoEngine";
int						gWindowWidth = 512;
int						gWindowHeight = 512;


/**
 *	@return the set of layers to be initialized with Vulkan
 */
const std::set<std::string>& getRequestedLayerNames()
{
	static std::set<std::string> layers;
	if (layers.empty())
	{
		layers.emplace("VK_LAYER_NV_optimus");
		layers.emplace("VK_LAYER_LUNARG_standard_validation");
	}
	return layers;
}


/**
* Initializes SDL
* @return true if SDL was initialized successfully
*/
bool initSDL()
{
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0)
		return true;
	std::cout << "Unable to initialize SDL\n";
	return false;
}


/**
 * Callback that receives a debug message from Vulkan
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData) 
{
	std::cout << "validation layer: " << layerPrefix << ": " << msg << std::endl;
	return VK_FALSE;
}


VkResult createDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) 
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else 
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}


/**
 *	Sets up the vulkan messaging callback specified above
 */
bool setupDebugCallback(VkInstance& instance, VkDebugReportCallbackEXT& callback)
{
	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (createDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) 
	{
		std::cout << "unable to create debug report callback extension\n";
		return false;
	}
	return true;
}


/**
 * Destroys the callback extension object
 */
void destroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) 
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) 
	{
		func(instance, callback, pAllocator);
	}
}


bool getAvailableVulkanLayers(std::vector<std::string>& outLayers)
{
	// Figure out the amount of available layers
	// Layers are used for debugging / validation etc / profiling..
	unsigned int instance_layer_count = 0;
	VkResult res = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
	if (res != VK_SUCCESS)
	{
		std::cout << "unable to query vulkan instance layer property count\n";
		return false;
	}

	std::vector<VkLayerProperties> instance_layer_names(instance_layer_count);
	res = vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layer_names.data());
	if (res != VK_SUCCESS)
	{
		std::cout << "unable to retrieve vulkan instance layer names\n";
		return false;
	}

	// Display layer names and find the ones we specified above
	std::cout << "found " << instance_layer_count << " instance layers:\n";
	std::vector<const char*> valid_instance_layer_names;
	const std::set<std::string>& lookup_layers = getRequestedLayerNames();
	int count(0);
	outLayers.clear();
	for (const auto& name : instance_layer_names)
	{
		std::cout << count << ": " << name.layerName << ": " << name.description << "\n";
		auto it = lookup_layers.find(std::string(name.layerName));
		if (it != lookup_layers.end())
			outLayers.emplace_back(name.layerName);
		count++;
	}

	// Print the ones we're enabling
	std::cout << "\n";
	for (const auto& layer : outLayers)
		std::cout << "found layer: " << layer.c_str() << "\n";
	return true;
}


bool getAvailableVulkanExtensions(SDL_Window* window, std::vector<std::string>& outExtensions)
{
	// Figure out the amount of extensions vulkan needs to interface with the os windowing system 
	// This is necessary because vulkan is a platform agnostic API and needs to know how to interface with the windowing system
	unsigned int ext_count = 0;
	if (!SDL_Vulkan_GetInstanceExtensions(window, &ext_count, nullptr))
	{
		std::cout << "Unable to query the number of Vulkan instance extensions\n";
		return false;
	}

	// Use the amount of extensions queried before to retrieve the names of the extensions
	std::vector<const char*> ext_names(ext_count);
	if (!SDL_Vulkan_GetInstanceExtensions(window, &ext_count, ext_names.data()))
	{
		std::cout << "Unable to query the number of Vulkan instance extension names\n";
		return false;
	}

	// Display names
	std::cout << "found " << ext_count << " Vulkan instance extensions:\n";
	for (unsigned int i = 0; i < ext_count; i++)
	{
		std::cout << i << ": " << ext_names[i] << "\n";
		outExtensions.emplace_back(ext_names[i]);
	}

	// Add debug display extension, we need this to relay debug messages
	outExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	std::cout << "\n";
	return true;
}


/**
 * Creates a vulkan instance using all the available instance extensions and layers
 * @return if the instance was created successfully
 */
bool createVulkanInstance(SDL_Window* window, const std::vector<std::string>& layerNames, const std::vector<std::string>& extensionNames, VkInstance& outInstance)
{
	// Copy layers
	std::vector<const char*> layer_names;
	for (const auto& layer : layerNames)
		layer_names.emplace_back(layer.c_str());

	// Copy extensions
	std::vector<const char*> ext_names;
	for (const auto& ext : extensionNames)
		ext_names.emplace_back(ext.c_str());

	// Get the suppoerted vulkan instance version
	unsigned int api_version;
	vkEnumerateInstanceVersion(&api_version);

	// initialize the VkApplicationInfo structure
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = gAppName;
	app_info.applicationVersion = 1;
	app_info.pEngineName = gEngineName;
	app_info.engineVersion = 1;
	app_info.apiVersion = VK_API_VERSION_1_0;

	// initialize the VkInstanceCreateInfo structure
	VkInstanceCreateInfo inst_info = {};
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext = NULL;
	inst_info.flags = 0;
	inst_info.pApplicationInfo = &app_info;
	inst_info.enabledExtensionCount = static_cast<uint32_t>(ext_names.size());
	inst_info.ppEnabledExtensionNames = ext_names.data();
	inst_info.enabledLayerCount = static_cast<uint32_t>(layer_names.size());
	inst_info.ppEnabledLayerNames = layer_names.data();

	// Create vulkan runtime instance
	std::cout << "initializing Vulkan instance\n\n";
	VkResult res = vkCreateInstance(&inst_info, NULL, &outInstance);
	switch (res)
	{
	case VK_SUCCESS:
		break;
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		std::cout << "unable to create vulkan instance, cannot find a compatible Vulkan ICD\n";
		return false;
	default:
		std::cout << "unable to create Vulkan instance: unknown error\n";
		return false;
	}
	return true;
}


/**
 * Allows the user to select a GPU (physical device)
 * @return if query, selection and assignment was successful
 * @param outDevice the selected physical device (gpu)
 * @param outQueueFamilyIndex queue command family that can handle graphics commands
 */
bool selectGPU(VkInstance& instance, VkPhysicalDevice& outDevice, unsigned int& outQueueFamilyIndex)
{
	// Get number of available physical devices, needs to be at least 1
	unsigned int physical_device_count(0);
	vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
	if (physical_device_count == 0)
	{
		std::cout << "No physical devices found\n";
		return false;
	}

	// Now get the devices
	std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
	vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());

	// Show device information
	std::cout << "found " << physical_device_count << " GPU(s):\n";
	int count(0);
	std::vector<VkPhysicalDeviceProperties> physical_device_properties(physical_devices.size());
	for (auto& physical_device : physical_devices)
	{
		vkGetPhysicalDeviceProperties(physical_device, &(physical_device_properties[count]));
		std::cout << count << ": " << physical_device_properties[count].deviceName << "\n";
		count++;
	}

	// Select one if more than 1 is available
	unsigned int selection_id = 0;
	if (physical_device_count > 1)
	{
		while (true)
		{
			std::cout << "select device: ";
			std::cin  >> selection_id;
			if (selection_id >= physical_device_count || selection_id < 0)
			{
				std::cout << "invalid selection, expected a value between 0 and " << physical_device_count - 1 << "\n";
				continue;
			}
			break;
		}
	}
	std::cout << "selected: " << physical_device_properties[selection_id].deviceName << "\n";
	VkPhysicalDevice selected_device = physical_devices[selection_id];

	// Find the number queues this device supports, we want to make sure that we have a queue that supports graphics commands
	unsigned int family_queue_count(0);
	vkGetPhysicalDeviceQueueFamilyProperties(selected_device, &family_queue_count, nullptr);
	if (family_queue_count == 0)
	{
		std::cout << "device has no family of queues associated with it\n";
		return false;
	}
	
	// Extract the properties of all the queue families
	std::vector<VkQueueFamilyProperties> queue_properties(family_queue_count);
	vkGetPhysicalDeviceQueueFamilyProperties(selected_device, &family_queue_count, queue_properties.data());

	// Make sure the family of commands contains an option to issue graphical commands
	unsigned int queue_node_index = -1;
	for (unsigned int i = 0; i < family_queue_count; i++) 
	{
		if (queue_properties[i].queueCount > 0 && queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) 
		{
			queue_node_index = i;
			break;
		}
	}

	if (queue_node_index < 0)
	{
		std::cout << "Unable to find a queue command family that accepts graphics commands\n";
		return false;
	}

	// Set the output variables
	outDevice = selected_device;
	outQueueFamilyIndex = queue_node_index;
	return true;
}


/**
 *	Creates a logical device
 */
bool createLogicalDevice(VkPhysicalDevice& physicalDevice, 
	unsigned int queueFamilyIndex, 
	const std::vector<std::string>& layerNames,
	const std::vector<std::string>& extNames,
	VkDevice& outDevice)
{
	// Copy layer names
	std::vector<const char*> layer_names;
	for (const auto& layer : layerNames)
		layer_names.emplace_back(layer.c_str());

	// Copy extensions
	std::vector<const char*> ext_names;
	for (const auto& ext : extNames)
		ext_names.emplace_back(ext.c_str());

	// Create queue information structure used by device based on the previously fetched queue information from the physical device
	// We create one command processing queue for graphics
	VkDeviceQueueCreateInfo queue_create_info;
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = queueFamilyIndex;
	queue_create_info.queueCount = 1;
	float queue_prio = 1.0f;
	queue_create_info.pQueuePriorities = &queue_prio;
	queue_create_info.pNext = NULL;
	queue_create_info.flags = NULL;

	// Device creation information
	VkDeviceCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = 1;
	create_info.pQueueCreateInfos = &queue_create_info;
	create_info.ppEnabledLayerNames = layer_names.data();
	create_info.enabledLayerCount = static_cast<uint32_t>(layer_names.size());
	create_info.ppEnabledExtensionNames = NULL;
	create_info.enabledExtensionCount = 0;
	create_info.pNext = NULL;
	create_info.pEnabledFeatures = NULL;
	create_info.flags = NULL;

	// Finally we're ready to create a new device
	VkResult res = vkCreateDevice(physicalDevice, &create_info, nullptr, &outDevice);
	if (res != VK_SUCCESS)
	{
		std::cout << "failed to create logical device!\n";
		return false;
	}
	return true;
}


/**
 * Create a vulkan window
 */
SDL_Window* createWindow()
{
	return SDL_CreateWindow(gAppName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, gWindowWidth, gWindowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
}


/**
 *	Destroys the vulkan instance
 */
void quit(VkInstance& instance, VkDevice& device, VkDebugReportCallbackEXT& callback)
{
	SDL_Quit();
	vkDestroyDevice(device, nullptr);
	destroyDebugReportCallbackEXT(instance, callback, NULL);
	vkDestroyInstance(instance, nullptr);
}


int main(int argc, char *argv[])
{
	// Initialize SDL
	if (!initSDL())
		return -1;

	// Create vulkan compatible window
	SDL_Window* window = createWindow();

	// Get available vulkan extensions, necessary for interfacting with native window
	std::vector<std::string> found_extensions;
	if (!getAvailableVulkanExtensions(window, found_extensions))
		return -2;

	// Get available vulkan layer extensions, notify when not all could be found
	std::vector<std::string> found_layers;
	if (!getAvailableVulkanLayers(found_layers))
		return -3;

	// Warn when not all requested layers could be found
	if (found_layers.size() != getRequestedLayerNames().size())
		std::cout << "warning! not all requested layers could be found!\n";

	// Create Vulkan Instance
	VkInstance instance;
	if (!createVulkanInstance(window, found_layers, found_extensions, instance))
		return -4;

	// Vulkan messaging callback
	VkDebugReportCallbackEXT callback;
	setupDebugCallback(instance, callback);

	// Select GPU after succsessful creation of a vulkan instance (jeeeej no global states anymore)
	VkPhysicalDevice gpu;
	unsigned int graphics_queue_index(-1);
	if (!selectGPU(instance, gpu, graphics_queue_index))
		return -5;

	// Create a logical device that interfaces with the physical device
	VkDevice device;
	if (!createLogicalDevice(gpu, graphics_queue_index, found_layers, found_extensions, device))
		return -6;

	// WOOP, finally ready to render some stuff!
	bool run = true;
	while (run)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				run = false;
			}
		}
	}

	// Destroy Vulkan Instance
	quit(instance, device, callback);
	
	return 1;
}