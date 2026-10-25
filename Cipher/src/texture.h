#pragma once
#include <d3d11.h>
#include <string>

struct Texture {
    ID3D11ShaderResourceView* srv = nullptr;
    int w = 0, h = 0;
    bool ok() const { return srv != nullptr; }
};

namespace Tex {
    bool LoadPNG(ID3D11Device* device, const std::wstring& path, Texture& out);
    void Release(Texture& t);
}
