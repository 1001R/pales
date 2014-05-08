#pragma once
#include <Windows.h>
#include <stdbool.h>

wchar_t *wcpcpy(wchar_t *to, const wchar_t *from);
bool ArgumentMustBeQuoted(const wchar_t *s, size_t *len);
wchar_t *QuoteArgument(wchar_t *s, const wchar_t *arg);
wchar_t *BuildCommandLine(size_t argc, const wchar_t **argv);