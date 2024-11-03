#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <commctrl.h>
#include <time.h>

#pragma comment(lib, "comctl32.lib")

#define MAX_PROCESSES 1024
#define MAX_NAME_LEN 256
#define UPDATE_INTERVAL 1000

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateProcessList(HWND hListView);
void UpdateSystemUsage(HWND hwnd);

// Global variables for CPU and Memory Utilization labels
HWND hCpuUsage;
HWND hMemoryUsage;

// Structure to hold process information
typedef struct {
    DWORD pid;
    TCHAR name[MAX_NAME_LEN];
    SIZE_T memory_usage;
    double cpu_usage;
} ProcessInfo;

// Function to compare processes for sorting by memory usage in descending order
int compare_memory_usage(const void *a, const void *b) {
    ProcessInfo *processA = (ProcessInfo *)a;
    ProcessInfo *processB = (ProcessInfo *)b;
    return (processB->memory_usage > processA->memory_usage) ? 1 : -1;
}

// Function to retrieve information about all processes running in the system
// Returns the top processes sorted by memory usage
int get_process_info(ProcessInfo processes[], int max_processes) {
    DWORD process_ids[MAX_PROCESSES], cb_needed, count;
    if (!EnumProcesses(process_ids, sizeof(process_ids), &cb_needed)) {
        _tprintf(TEXT("EnumProcesses failed with error %lu\n"), GetLastError());
        return -1;
    }

    // Get process count and limit it to max_processes
    count = cb_needed / sizeof(DWORD);
    if (count > max_processes) count = max_processes;

    int actual_count = 0;
    for (int i = 0; i < count; i++) {
        DWORD pid = process_ids[i];
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (process) {
            HMODULE hMod;
            DWORD cb_needed;

            ProcessInfo *p = &processes[actual_count];
            p->pid = pid;

            // Retrieve the process name
            if (EnumProcessModules(process, &hMod, sizeof(hMod), &cb_needed)) {
                GetModuleBaseName(process, hMod, p->name, MAX_NAME_LEN / sizeof(TCHAR));
            } else {
                _tcscpy(p->name, TEXT("<unknown>"));
            }

            // Retrieve the memory usage
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc))) {
                p->memory_usage = pmc.WorkingSetSize / 1024;  // Convert to KB
            } else {
                p->memory_usage = 0;
            }

            // Simulate CPU usage with a random value as placeholder
            p->cpu_usage = (rand() % 1000) / 100.0;

            CloseHandle(process);
            actual_count++;
        }
    }

    // Sort processes by memory usage
    qsort(processes, actual_count, sizeof(ProcessInfo), compare_memory_usage);

    return actual_count < 10 ? actual_count : 10;  // Return the top 10 processes or less
}

// Main function to create and display the main window
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const TCHAR CLASS_NAME[] = TEXT("ProcessManagerClass");
    WNDCLASS wc = { 0 };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        TEXT("Process Manager"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 500,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// Window procedure function that handles window messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hListView;

    switch (uMsg) {
    case WM_CREATE: {
        // Initialize common controls for ListView
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);

        // Create ListView control to display process information
        hListView = CreateWindowEx(
            0, WC_LISTVIEW, TEXT(""),
            WS_CHILD | WS_VISIBLE | LVS_REPORT,
            10, 10, 560, 300,
            hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );

        // Add columns to ListView for PID, Name, Memory Usage, and CPU Usage
        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        lvc.pszText = TEXT("PID");
        lvc.cx = 50;
        ListView_InsertColumn(hListView, 0, &lvc);

        lvc.pszText = TEXT("Name");
        lvc.cx = 150;
        ListView_InsertColumn(hListView, 1, &lvc);

        lvc.pszText = TEXT("Memory Usage (KB)");
        lvc.cx = 150;
        ListView_InsertColumn(hListView, 2, &lvc);

        lvc.pszText = TEXT("CPU Usage (%)");
        lvc.cx = 100;
        ListView_InsertColumn(hListView, 3, &lvc);

        // Create a button to refresh process information
        CreateWindow(
            TEXT("BUTTON"), TEXT("Refresh"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            240, 320, 100, 30,
            hwnd, (HMENU)1, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );

        // Create labels for CPU and Memory utilization
        hCpuUsage = CreateWindow(
            TEXT("STATIC"), TEXT("CPU Usage: 0%"),
            WS_VISIBLE | WS_CHILD,
            10, 360, 200, 20,
            hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );

        hMemoryUsage = CreateWindow(
            TEXT("STATIC"), TEXT("Memory Usage: 0%"),
            WS_VISIBLE | WS_CHILD,
            300, 360, 200, 20,
            hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );

        UpdateProcessList(hListView);
        UpdateSystemUsage(hwnd);
    }
    break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {  // Refresh button clicked
            UpdateProcessList(hListView);
            UpdateSystemUsage(hwnd);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Function to update the ListView with current process information
void UpdateProcessList(HWND hListView) {
    ProcessInfo processes[MAX_PROCESSES];
    int process_count = get_process_info(processes, MAX_PROCESSES);

    // Clear existing ListView items
    ListView_DeleteAllItems(hListView);

    // Add new items to ListView for each process
    for (int i = 0; i < process_count; i++) {
        LVITEM lvi;
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        TCHAR buffer[256];

        // Set PID
        _stprintf_s(buffer, 256, TEXT("%u"), processes[i].pid);
        lvi.pszText = buffer;
        ListView_InsertItem(hListView, &lvi);

        // Set Name
        ListView_SetItemText(hListView, i, 1, processes[i].name);

        // Set Memory Usage
        _stprintf_s(buffer, 256, TEXT("%zu"), processes[i].memory_usage);
        ListView_SetItemText(hListView, i, 2, buffer);

        // Set CPU Usage
        _stprintf_s(buffer, 256, TEXT("%.2f"), processes[i].cpu_usage);
        ListView_SetItemText(hListView, i, 3, buffer);
    }
}

// Function to update the CPU and Memory usage labels with current system usage
void UpdateSystemUsage(HWND hwnd) {
    static ULARGE_INTEGER last_idle_time = { 0 }, last_kernel_time = { 0 }, last_user_time = { 0 };
    FILETIME idle_time, kernel_time, user_time;
    TCHAR cpuUsageText[256];

    if (GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        ULARGE_INTEGER curr_idle_time, curr_kernel_time, curr_user_time;
        curr_idle_time.LowPart = idle_time.dwLowDateTime;
        curr_idle_time.HighPart = idle_time.dwHighDateTime;
        curr_kernel_time.LowPart = kernel_time.dwLowDateTime;
        curr_kernel_time.HighPart = kernel_time.dwHighDateTime;
        curr_user_time.LowPart = user_time.dwLowDateTime;
        curr_user_time.HighPart = user_time.dwHighDateTime;

        ULONGLONG system_time_diff = (curr_kernel_time.QuadPart - last_kernel_time.QuadPart) +
                                     (curr_user_time.QuadPart - last_user_time.QuadPart);
        ULONGLONG idle_time_diff = curr_idle_time.QuadPart - last_idle_time.QuadPart;
        double cpu_usage = 100.0 * (1.0 - (double)idle_time_diff / system_time_diff);

        _stprintf_s(cpuUsageText, 256, TEXT("CPU Usage: %.2f%%"), cpu_usage);
        SetWindowText(hCpuUsage, cpuUsageText);

        last_idle_time = curr_idle_time;
        last_kernel_time = curr_kernel_time;
        last_user_time = curr_user_time;
    }

    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    if (GlobalMemoryStatusEx(&mem_status)) {
        TCHAR memoryUsageText[256];
        _stprintf_s(memoryUsageText, 256, TEXT("Memory Usage: %lu%%"), mem_status.dwMemoryLoad);
        SetWindowText(hMemoryUsage, memoryUsageText);
    }
}
