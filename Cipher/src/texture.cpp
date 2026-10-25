#include <Windows.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "texture.h"
#include <vector>

bool Tex::LoadPNG(ID3D11Device* device, const std::wstring& path, Texture& out) {
    // Wide path → UTF-8 for stbi
    int n = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string u8(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, u8.data(), n, nullptr, nullptr);
    if (!u8.empty() && u8.back() == '\0') u8.pop_back();

    int w, h, ch;
    unsigned char* data = stbi_load(u8.c_str(), &w, &h, &ch, 4);
    if (!data) return false;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub{};
    sub.pSysMem = data;
    sub.SysMemPitch = w * 4;

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &sub, &tex);
    stbi_image_free(data);
    if (FAILED(hr) || !tex) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    hr = device->CreateShaderResourceView(tex, &srvDesc, &out.srv);
    tex->Release();
    if (FAILED(hr)) return false;

    out.w = w;
    out.h = h;
    return true;
}

void Tex::Release(Texture& t) {
    if (t.srv) { t.srv->Release(); t.srv = nullptr; }
    t.w = t.h = 0;
}
