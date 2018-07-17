#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <vulkan/vulkan_core.h>
#include <vector>

// Globals
const char	gAppName[] = "VulkanDemo";
const char	gEngineName[] = "VulkanDemoEngine";
int			gWindowWidth = 512;
int			gWindowHeight = 512;

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
 * Creates a vulkan instance using all the available instance extensions and layers
 * @return if the instance was created successfully
 */
bool createVulkanInstance(SDL_Window* window, VkInstance& outInstance)
{
	// Figure out the amount of extensions vulkan needs to interface with the os windowing system 
	// This is necessary because vulkan is a platform agnostic API and needs to know how to interface with the windowing system
	unsigned int ext_count = 0;
	if(!SDL_Vulkan_GetInstanceExtensions(window, &ext_count, nullptr))
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
	}
	std::cout << "\n";

	// Figure out the amount of available layers
	// Layers are used for debugging / validation etc / profiling..
	unsigned int instance_layer_count = 0;
	VkResult res = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
	std::vector<VkLayerProperties> instance_layer_names(instance_layer_count);
	vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layer_names.data());

	// Display layer names
	std::cout << "found " << instance_layer_count << " Vulkan instance layers:\n";
	std::vector<const char*> valid_instance_layer_names;
	int count(0);
	for (const auto& name : instance_layer_names)
	{
		std::cout << count << ": " << name.layerName << ": " << name.description << "\n";
		valid_instance_layer_names.emplace_back(name.layerName);
		count++;
	}
	std::cout << "\n";

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
	inst_info.enabledExtensionCount = ext_count;
	inst_info.ppEnabledExtensionNames = ext_names.data();
	inst_info.enabledLayerCount = instance_layer_count;
	inst_info.ppEnabledLayerNames = valid_instance_layer_names.data();

	// Create vulkan runtime instance
	std::cout << "initializing Vulkan instance\n\n";
	res = vkCreateInstance(&inst_info, NULL, &outInstance);
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
		if ((queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) 
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
 * Create a vulkan window
 */
SDL_Window* createWindow()
{
	return SDL_CreateWindow(gAppName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, gWindowWidth, gWindowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
}


/**
 *	Destroys the vulkan instance
 */
void quit(VkInstance instance)
{
	SDL_Quit();
	vkDestroyInstance(instance, nullptr);
}


int main(int argc, char *argv[])
{
	// Initialize SDL
	if (!initSDL())
		return -1;

	// Create vulkan compatible window
	SDL_Window* window = createWindow();

	// Create Vulkan Instance
	VkInstance instance;
	if (!createVulkanInstance(window, instance))
		return -1;

	// Select GPU after succsessful creation of a vulkan instance (jeeeej no global states anymore)
	VkPhysicalDevice gpu;
	unsigned int graphics_queue_node_index(-1);
	if (!selectGPU(instance, gpu, graphics_queue_node_index))
		return -1;

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
	quit(instance);
	
	return 1;
}