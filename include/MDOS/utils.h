#pragma once
#include "printf.h" // IWYU pragma: keep

#define PRINT(format, ...) printf(system_table, format, ##__VA_ARGS__)

#define LOG_INFO(name, format, ...)                                                                    \
	do {                                                                                               \
		system_table->ConOut->SetAttribute(system_table->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)); \
		PRINT(L"[INFO]    <%s> " format, name, ##__VA_ARGS__);                                            \
		system_table->ConOut->SetAttribute(system_table->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)); \
	} while (0)

#define LOG_WARNING(name, format, ...)                                                                  \
	do {                                                                                                \
		system_table->ConOut->SetAttribute(system_table->ConOut, EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLACK)); \
		PRINT(L"[WARNING] <%s> " format, name, ##__VA_ARGS__);                                          \
		system_table->ConOut->SetAttribute(system_table->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));  \
	} while (0)

#define LOG_ERROR(name, format, ...)                                                                   \
	do {                                                                                               \
		system_table->ConOut->SetAttribute(system_table->ConOut, EFI_TEXT_ATTR(EFI_LIGHTRED, EFI_BLACK));   \
		PRINT(L"[ERROR]   <%s> " format, name, ##__VA_ARGS__);                                           \
		system_table->ConOut->SetAttribute(system_table->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)); \
	} while (0)

#define WAIT_FOR_KEY()                                                                                      \
	do {                                                                                                    \
		UINTN wait_for_key_index;                                                                           \
		EFI_INPUT_KEY wait_for_key_input_key;                                                               \
		system_table->BootServices->WaitForEvent(1, &system_table->ConIn->WaitForKey, &wait_for_key_index); \
		system_table->ConIn->ReadKeyStroke(system_table->ConIn, &wait_for_key_input_key);                   \
	} while (0)

#define EXIT(name)                                       \
	do {                                                 \
		LOG_INFO(name, L"Press any key to exit...\r\n"); \
		WAIT_FOR_KEY();                                  \
		LOG_INFO(name, L"Exiting...\r\n");               \
	} while (0)
