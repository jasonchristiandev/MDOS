#pragma once
#include "printf.h"

#define EXIT                                                                                        \
	do {                                                                                            \
		UINTN exit_index;                                                                           \
		EFI_INPUT_KEY exit_key;                                                                     \
		LOG_INFO(L"Press any key to exit...\r\n");                                                  \
		system_table->BootServices->WaitForEvent(1, &system_table->ConIn->WaitForKey, &exit_index); \
		system_table->ConIn->ReadKeyStroke(system_table->ConIn, &exit_key);                         \
		LOG_INFO(L"Exiting...\r\n\r\n");                                                            \
	} while (0)

#define PRINT(format, ...) EfiPrintF(system_table, format, ##__VA_ARGS__)

#define LOG_INFO(format, ...)                                                                          \
	do {                                                                                               \
		system_table->ConOut->SetAttribute(system_table->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)); \
		PRINT(L"[INFO] " format, ##__VA_ARGS__);                                                       \
	} while (0)

#define LOG_WARNING(format, ...)                                                                        \
	do {                                                                                                \
		system_table->ConOut->SetAttribute(system_table->ConOut, EFI_TEXT_ATTR(EFI_BLACK, EFI_YELLOW)); \
		PRINT(L"[WARNING] " format, ##__VA_ARGS__);                                                     \
		system_table->ConOut->SetAttribute(system_table->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));  \
	} while (0)

#define LOG_ERROR(format, ...)                                                                         \
	do {                                                                                               \
		system_table->ConOut->SetAttribute(system_table->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_RED));   \
		PRINT(L"[ERROR] " format, ##__VA_ARGS__);                                                      \
		system_table->ConOut->SetAttribute(system_table->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)); \
	} while (0)
