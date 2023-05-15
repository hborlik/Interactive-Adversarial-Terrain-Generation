#pragma once
#ifndef DIRTBOX_UTILS_H_HEADER_GUARD
#define DIRTBOX_UTILS_H_HEADER_GUARD

#include <string>

#include <bgfx/bgfx.h>
#include <bimg/bimg.h>
#include <bx/pixelformat.h>


namespace dirtbox {

/// Returns true if both internal transient index and vertex buffer have
/// enough space.
///
/// @param[in] _numVertices Number of vertices.
/// @param[in] _layout Vertex layout.
/// @param[in] _numIndices Number of indices.
///
inline bool checkAvailTransientBuffers(uint32_t _numVertices, const bgfx::VertexLayout& _layout, uint32_t _numIndices)
{
	return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _layout)
		&& (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices) )
		;
}

///
inline uint32_t encodeNormalRgba8(float _x, float _y = 0.0f, float _z = 0.0f, float _w = 0.0f)
{
	const float src[] =
	{
		_x * 0.5f + 0.5f,
		_y * 0.5f + 0.5f,
		_z * 0.5f + 0.5f,
		_w * 0.5f + 0.5f,
	};
	uint32_t dst;
	bx::packRgba8(&dst, src);
	return dst;
}

struct Args
{
	Args(int _argc, const char* const* _argv);

    // render backend
	bgfx::RendererType::Enum m_type;
    // render device id
	uint16_t m_pciId;
};

void utilInit();

///
void* load(const std::string& _filePath, uint32_t* _size = nullptr);

///
void unload(void* _ptr);

///
bgfx::ShaderHandle loadShader(const std::string& _name);

///
bgfx::ProgramHandle loadProgram(const std::string& _vsName, const std::string& _fsName);

///
bgfx::TextureHandle loadTexture(const std::string& _name, uint64_t _flags = BGFX_TEXTURE_NONE|BGFX_SAMPLER_NONE, uint8_t _skip = 0, bgfx::TextureInfo* _info = NULL, bimg::Orientation::Enum* _orientation = NULL);

///
bimg::ImageContainer* imageLoad(const std::string& _filePath, bgfx::TextureFormat::Enum _dstFormat);

bool savePng(const char* _filePath, uint32_t _width, uint32_t _height, const void* _src, bool _yflip);

bx::AllocatorI* getAllocator();

}
#endif // DIRTBOX_UTILS_H_HEADER_GUARD