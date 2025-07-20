#include "Graphics/pch.h"

#include "Graphics/RHI/Queue.h"

#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Fence.h"
#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Semaphore.h"
#include "Graphics/RHI/SwapChain.h"

namespace cgs::graphics::rhi
{
    Queue::Queue(const CreateInfo& createInfo) noexcept
        : mDevice(createInfo.RhiDevice)
        , mQueueFamily(createInfo.RhiQueueFamily)
        , mQueue(createInfo.Queue)
    {
        assert(mQueue != VK_NULL_HANDLE);
    }
    
    void Queue::Submit(const CommandBuffer& commandBuffer) const noexcept
    {
		const Semaphore& presentSemaphore = commandBuffer.GetPresentCompletionSemaphore();
		const Semaphore& renderSemaphore = commandBuffer.GetRenderCompletionSemaphore();

		const VkSemaphoreSubmitInfo waitSemaphoreInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.pNext = nullptr,
			.semaphore = presentSemaphore.GetVkSemaphore(), // Use the present completion semaphore
			.value = 0, // Initial value for the semaphore
			.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Specify the pipeline stage for the wait
		};

		const VkCommandBufferSubmitInfo commandBufferInfo = 
		{
		    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		    .pNext = nullptr,
		    .commandBuffer = commandBuffer.GetVkCommandBuffer(), // Use the command buffer to submit
            .deviceMask = 0,
		};

		const VkSemaphoreSubmitInfo signalSemaphoreInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.pNext = nullptr,
			.semaphore = renderSemaphore.GetVkSemaphore(), // Use the render completion semaphore
			.value = 0, // Initial value for the semaphore
			.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Specify the pipeline stage for the signal
		};

        const VkSubmitInfo2 submitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext = nullptr,
			.flags = 0, // No special flags
            .waitSemaphoreInfoCount = 1, // No wait semaphores
            .pWaitSemaphoreInfos = &waitSemaphoreInfo, // No wait semaphores
            .commandBufferInfoCount = 1, // No command buffers
            .pCommandBufferInfos = &commandBufferInfo, // No command buffers
            .signalSemaphoreInfoCount = 1, // No signal semaphores
			.pSignalSemaphoreInfos = &signalSemaphoreInfo, // Signal the render completion semaphore
        };
        const Fence& fence = commandBuffer.GetFence();
        VkResult vr = vkQueueSubmit2(mQueue, 1, &submitInfo, fence.GetVkFence());
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to submit queue: %s", VkResultToString(vr));
        }
        else
        {
            CGS_LOG_INFO("Queue submitted successfully.");
        }
    }

    Queue::~Queue() noexcept
    {
        mQueue = VK_NULL_HANDLE;
    }

    void Queue::Present(const CommandBuffer& commandBuffer) const noexcept
    {
		const SwapChain& swapChain = mDevice.GetSwapChain();

        const Semaphore& presentSemaphore = commandBuffer.GetPresentCompletionSemaphore();

		VkSemaphore presentSemaphores[] = { presentSemaphore.GetVkSemaphore() };
		VkSwapchainKHR swapChains[] = { swapChain.GetVkSwapChain() };
		const uint32_t backBufferIndex = commandBuffer.GetBackBufferIndex();

        const VkPresentInfoKHR presentInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
			.waitSemaphoreCount = 1, // Wait for the present semaphore
			.pWaitSemaphores = presentSemaphores, // Wait for the present semaphore
			.swapchainCount = 1, // Present to one swap chain
            .pSwapchains = swapChains, // No swap chains
            .pImageIndices = &backBufferIndex, // No image indices
            .pResults = nullptr // No results
        };
        VkResult vr = vkQueuePresentKHR(mQueue, &presentInfo);
        if (vr != VK_SUCCESS)
        {
            CGS_LOG_ERROR("Failed to present queue: %s", VkResultToString(vr));
        }
    }
} // namespace cgs::graphics::rhi
