#include "Window.hpp"

#include <VulkanUtils.h>

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
#include <imgui_impl_win32.h>
#include <windowsx.h> // GET_X_LPARAM(), GET_Y_LPARAM()

#pragma comment(lib,"user32.lib")


// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HANDLE handle_shader_directory_changes = INVALID_HANDLE_VALUE;

// win32 specific data
namespace {

    std::vector<const char*> platform_extensions = {
        "VK_KHR_win32_surface"
    };

    // this is the main message handler for the program
    LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);

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

            switch (msg.message)
            {
            /*case WM_SYSKEYDOWN:
                CC_LOG(CC_INFO, "WM_SYSKEYDOWN: 0x%x\n", msg.wParam);
                break;

            case WM_SYSCHAR:
                CC_LOG(CC_INFO, "WM_SYSCHAR: %c\n", (wchar_t)msg.wParam);
                break;

            case WM_SYSKEYUP:
                CC_LOG(CC_INFO, "WM_SYSKEYUP: 0x%x\n", msg.wParam);
                break;

            case WM_KEYDOWN:
                CC_LOG(CC_INFO, "WM_KEYDOWN: 0x%x\n", msg.wParam);
                break;

            case WM_KEYUP:
                CC_LOG(CC_INFO, "WM_KEYUP: 0x%x\n", msg.wParam);
                break;

            case WM_CHAR:
                CC_LOG(CC_INFO, "WM_CHAR: %c\n", (wchar_t)msg.wParam);
                break;*/

            //case WM_NCMOUSEMOVE:
            //    POINT mouse_pos = { (LONG)GET_X_LPARAM(msg.lParam), (LONG)GET_Y_LPARAM(msg.lParam) };
            //    if (ScreenToClient(msg.hwnd, &mouse_pos) == FALSE) // WM_NCMOUSEMOVE are provided in absolute coordinates.
            //        continue;

            //    break;
            case WM_QUIT:
                this->m_should_quit = true;
                break;
            }
        }
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
            .width  = (uint32_t)(windowRect.right - windowRect.left),
            .height = (uint32_t)(windowRect.bottom - windowRect.top)
        };
    }

    // checks event queue for changes to shader files.
    // TODO make this async
    bool Window::TMP_reload_shaders() {
        auto ret = WaitForSingleObject(handle_shader_directory_changes, 0);

        if(ret == WAIT_OBJECT_0) {
            // remove notification from the queue
            FindNextChangeNotification(
                handle_shader_directory_changes
            );
            CC_LOG(CC_WARNING, "dir monitor: %d", ret);
            return true;
        }
        return false;
    }

    // registers a file watcher at the OS level
    // Current implementation is very simple and ad-hoc for shaders
    void Window::register_file_watcher() {
        const char SHADER_BIN_PATH_FMT[] = "%s\\res\\shaders";

        if(handle_shader_directory_changes != INVALID_HANDLE_VALUE)
        {
            CC_LOG(CC_WARNING, "Only monitoring shaders for now. Future implementations will allow to monitor for specific directories with optional callbacks");
            return;
        }

        // all of this is VERY loose, add check and proper error handling
        // (not worth doing it yet since even the shader source monitoring
        // script is bad and will need to be rewritten
        char buf[MAX_PATH + sizeof(SHADER_BIN_PATH_FMT)];
        GetCurrentDirectory(MAX_PATH, buf);

        sprintf_s(buf, SHADER_BIN_PATH_FMT, buf);

        // register file watcher for shader binaries
        handle_shader_directory_changes = FindFirstChangeNotificationA(
            buf,
            FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE
        );
    }

    void* Window::get_native_window_handle() {
        return ::hWnd;
    }
}