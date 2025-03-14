#include "Window.hpp"

// windows only for now
// CANNOT INCLUDE CC_LOGGER, VERBOSE IS TAKEN...
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define TMP_CC_EXIT(x, msg, ...) { printf(msg, __VA_ARGS__); exit(x); }

inline void TMP_CC_LOG_SYS_ERROR() {
	char buffer[512];
	strerror_s(buffer, 512, errno);
	printf("[%d] %s\n", errno, buffer);
}

#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#include <winuser.h>
#include <windef.h>
#include <vulkan/vulkan_win32.h>
#pragma comment(lib,"user32.lib")

// win32 specific data
namespace {
    std::vector<const char*> platform_extensions = {
        "VK_KHR_win32_surface"
    };

    // this is the main message handler for the program
    LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_DESTROY)
        {
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    static HINSTANCE hInstance;
    HWND hWnd;
}

namespace vkc {
	Window::Window(const char* title, int width, int height) {
        WNDCLASSEX wc;

        ZeroMemory(&wc, sizeof(WNDCLASSEX));

        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wc.lpszClassName = "WindowClass";

        RegisterClassEx(&wc);

        hWnd = CreateWindowA(
            "WindowClass", title,
            0, 0, 0, width, height,
            NULL, NULL, hInstance, NULL
        );
        SetWindowLong(
            hWnd, GWL_STYLE, WS_SIZEBOX | WS_MAXIMIZEBOX |
            WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION
        );

        ShowWindow(hWnd, SW_SHOW);
	}

    void Window::collect_input() {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT)
            this->m_should_quit = true;
    }

    std::vector<const char*> Window::get_platform_extensions() {
        return platform_extensions;
    }

    VkSurfaceKHR Window::create_surfaceKHR(VkInstance instance) {
        VkSurfaceKHR surface;
        VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        createInfo.hwnd = hWnd;
        createInfo.hinstance = GetModuleHandle(NULL);

        VkResult res = vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, &surface);
        if (res != VK_SUCCESS) {
            TMP_CC_EXIT(res, "failed to create window surface!");
        }

        return surface;
    }

    VkExtent2D Window::get_current_extent() {
        RECT windowRect;
        GetWindowRect(hWnd, &windowRect);

        return (VkExtent2D) {
            .width = (uint32_t)(windowRect.right - windowRect.left),
            .height = (uint32_t)(windowRect.top - windowRect.bottom)
        };
    }
}