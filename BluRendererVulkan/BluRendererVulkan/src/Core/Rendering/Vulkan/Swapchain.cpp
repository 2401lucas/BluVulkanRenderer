#include "Swapchain.h"

#include "Tools.h"

namespace blu::core {
Swapchain::Swapchain(blu::core::Instance* vk_instance,
                     blu::core::Device* vk_device, blu::core::Window* window) {
  vk_instance_ = vk_instance;
  vk_device_ = vk_device;
  window_ = window;

  VK_CHECK_RESULT(glfwCreateWindowSurface(vk_instance->Get(), window->Get(),
                                          nullptr, &surface_));

  // Get list of supported surface formats
  uint32_t format_count;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
      vk_device_->GetPhysicalDevice(), surface_, &format_count, NULL));

  EASTL_ASSERT(format_count > 0);

  eastl::vector<VkSurfaceFormatKHR> surfaceFormats(format_count);
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
      vk_device_->GetPhysicalDevice(), surface_, &format_count,
      surfaceFormats.data()));

  // We want to get a format that best suits our needs, so we try to get one
  // from a set of preferred formats Initialize the format to the first one
  // returned by the implementation in case we can't find one of the preferred
  // formats
  VkSurfaceFormatKHR selected_format = surfaceFormats[0];
  eastl::vector<VkFormat> preferred_image_formats = {
      VK_FORMAT_R8G8B8A8_UNORM,
      VK_FORMAT_A8B8G8R8_UNORM_PACK32};

  for (eastl::vector<VkSurfaceFormatKHR>::iterator
           it = surfaceFormats.begin(),
           it_end = surfaceFormats.end();
       it != it_end; ++it) {
    if (eastl::find(preferred_image_formats.begin(),
                    preferred_image_formats.end(),
                    it->format) != preferred_image_formats.end()) {
      selected_format = *it;
      break;
    }
  }

  color_format_ = selected_format.format;
  color_space_ = selected_format.colorSpace;
}
Swapchain::~Swapchain() {
  if (swapchain_ != VK_NULL_HANDLE) {
    for (uint32_t i = 0; i < image_count_; i++) {
      vkDestroyImageView(vk_device_->GetLogicalDevice(), buffers_[i].view,
                         nullptr);
    }
  }
  if (surface_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(vk_device_->GetLogicalDevice(), swapchain_, nullptr);
    vkDestroySurfaceKHR(vk_instance_->Get(), surface_, nullptr);
  }
}

// TODO: CHECK IF FIF CHANGED WHEN WINDOW IS CREATED???
void Swapchain::Create(bool vsync, bool fullscreen) {
  VkSwapchainKHR old_swapchain = swapchain_;

  VkSurfaceCapabilitiesKHR surf_caps;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      vk_device_->GetPhysicalDevice(), surface_, &surf_caps));

  uint32_t present_mode_count;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
      vk_device_->GetPhysicalDevice(), surface_, &present_mode_count, nullptr));

  eastl::vector<VkPresentModeKHR> present_modes(present_mode_count);
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
      vk_device_->GetPhysicalDevice(), surface_, &present_mode_count,
      present_modes.data()));

  VkExtent2D swapchain_extent = {};
  // If width (and height) equals the special value 0xFFFFFFFF, the size of the
  // surface will be set by the swapchain
  if (surf_caps.currentExtent.width == (uint32_t)-1) {
    // If the surface size is undefined, the size is set to
    // the size of the images requested.
    swapchain_extent.width = window_->GetWidth();
    swapchain_extent.height = window_->GetHeight();
  } else {
    // If the surface size is defined, the swap chain size must match
    swapchain_extent = surf_caps.currentExtent;
  }

  image_width_ = swapchain_extent.width;
  image_height_ = swapchain_extent.height;

  VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

  if (!vsync) {
    for (size_t i = 0; i < present_mode_count; i++) {
      if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }
      if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        swapchain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      }
    }
  }

  uint32_t desired_number_of_swapchain_images = surf_caps.minImageCount + 1;

  if ((surf_caps.maxImageCount > 0) &&
      (desired_number_of_swapchain_images > surf_caps.maxImageCount)) {
    desired_number_of_swapchain_images = surf_caps.maxImageCount;
  }

  VkSurfaceTransformFlagsKHR pre_transform;
  if (surf_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    pre_transform = surf_caps.currentTransform;
  }
  VkCompositeAlphaFlagBitsKHR composite_alpha =
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  // TODO:
  // Simply select the first composite alpha format available
  eastl::vector<VkCompositeAlphaFlagBitsKHR> composite_alpha_flags = {
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
  };

  for (eastl::vector<VkCompositeAlphaFlagBitsKHR>::iterator
           it = composite_alpha_flags.begin(),
           it_end = composite_alpha_flags.end();
       it != it_end; ++it) {
    if (surf_caps.supportedCompositeAlpha & *it) {
      composite_alpha = *it;
      break;
    };
  }

  VkSwapchainCreateInfoKHR swapchain_ci = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface_,
      .minImageCount = desired_number_of_swapchain_images,
      .imageFormat = color_format_,
      .imageColorSpace = color_space_,
      .imageExtent = swapchain_extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .preTransform = (VkSurfaceTransformFlagBitsKHR)pre_transform,
      .compositeAlpha = composite_alpha,
      .presentMode = swapchain_present_mode,
      // Setting clipped to VK_TRUE allows the implementation to discard
      // rendering outside of the surface area
      .clipped = VK_TRUE,
      // Setting oldSwapChain to the saved handle of the previous swapchain aids
      // in resource reuse and makes sure that we can still present already
      // acquired images
      .oldSwapchain = old_swapchain,
  };

  // Enable transfer source on swap chain images if supported
  if (surf_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
    swapchain_ci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  // Enable transfer destination on swap chain images if supported
  if (surf_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    swapchain_ci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  vkCreateSwapchainKHR(vk_device_->GetLogicalDevice(), &swapchain_ci, nullptr,
                       &swapchain_);

  // If an existing swap chain is re-created, destroy the old swap chain
  // This also cleans up all the presentable images
  if (old_swapchain != VK_NULL_HANDLE) {
    for (uint32_t i = 0; i < image_count_; i++) {
      vkDestroyImageView(vk_device_->GetLogicalDevice(), buffers_[i].view,
                         nullptr);
    }
    vkDestroySwapchainKHR(vk_device_->GetLogicalDevice(), old_swapchain,
                          nullptr);
  }

  VK_CHECK_RESULT(vkGetSwapchainImagesKHR(vk_device_->GetLogicalDevice(),
                                          swapchain_, &image_count_, NULL));

  images_.resize(image_count_);
  VK_CHECK_RESULT(vkGetSwapchainImagesKHR(vk_device_->GetLogicalDevice(),
                                          swapchain_, &image_count_,
                                          images_.data()));

  buffers_.resize(image_count_);
  for (uint32_t i = 0; i < image_count_; i++) {
    VkImageViewCreateInfo color_attachment_view_ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = color_format_,
        .components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                       VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    buffers_[i].image = images_[i];
    color_attachment_view_ci.image = buffers_[i].image;

    VK_CHECK_RESULT(vkCreateImageView(vk_device_->GetLogicalDevice(),
                                      &color_attachment_view_ci, nullptr,
                                      &buffers_[i].view));
  }
}
}  // namespace blu::core