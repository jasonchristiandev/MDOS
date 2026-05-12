#pragma once
#include <efi.h>
#include <efistdarg.h>

static inline void itoa(unsigned long long n, CHAR16 *str, int base, BOOLEAN signed_val) {
	CHAR16 *p = str;
	CHAR16 *pa, *pb;
	unsigned long long decimal = n;

	if (signed_val && base == 10 && (long long) n < 0) {
		*p++ = L'-';
		decimal = -(long long) n;
	}

	CHAR16 *first_digit = p;
	do {
		int digit = decimal % base;
		*p++ = (digit < 10) ? (L'0' + digit) : (L'A' + digit - 10);
	} while (decimal /= base);
	*p = L'\0';

	pa = first_digit;
	pb = p - 1;
	while (pa < pb) {
		CHAR16 tmp = *pa;
		*pa = *pb;
		*pb = tmp;
		pa++;
		pb--;
	}
}

static inline void EfiPrintF(EFI_SYSTEM_TABLE *system_table, const CHAR16 *format, ...) {
	va_list args;
	va_start(args, format);

	CHAR16 buf[128];
	for (int i = 0; format[i] != L'\0'; i++) {
		if (format[i] == L'%' && format[i + 1] != L'\0') {
			i++;

			BOOLEAN is64 = FALSE;
			if (format[i] == L'l' && format[i + 1] == L'l') {
				is64 = TRUE;
				i += 2;
			}

			switch (format[i]) {
				case L's': {
					CHAR16 *s = va_arg(args, CHAR16 *);
					system_table->ConOut->OutputString(system_table->ConOut, s);
					break;
				}
				case L'd': {
					if (is64) {
						itoa(va_arg(args, long long), buf, 10, TRUE);
					}
					else {
						itoa(va_arg(args, int), buf, 10, TRUE);
					}

					system_table->ConOut->OutputString(system_table->ConOut, buf);
					break;
				}
				case L'u': {
					if (is64) {
						itoa(va_arg(args, unsigned long long), buf, 10, FALSE);
					} else {
						itoa(va_arg(args, unsigned int), buf, 10, FALSE);
					}

					system_table->ConOut->OutputString(system_table->ConOut, buf);
					break;
				}
				case L'x': {
					if (is64) {
						itoa(va_arg(args, unsigned long long), buf, 16, FALSE);
					} else {
						itoa(va_arg(args, unsigned int), buf, 16, FALSE);
					}

					system_table->ConOut->OutputString(system_table->ConOut, buf);
					break;
				}
			}
		} else {
			CHAR16 single[2] = {format[i], L'\0'};
			system_table->ConOut->OutputString(system_table->ConOut, single);
		}
	}
	va_end(args);
}