//
// pch.h
//

#pragma once

#include <chrono>
#include <d3d11_4.h>
#include <exception>
#include <memory>
#include <string>
#include <windows.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

// Helper class for COM exceptions
class com_exception : public std::exception
{
public:
    com_exception(HRESULT hr) : result(hr) {}

    virtual const char* what() const override
    {
        static char s_str[64] = {};
        sprintf_s(s_str, "Failure with HRESULT of %08X", result);
        return s_str;
    }

private:
    HRESULT result;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw com_exception(hr);
    }
}
