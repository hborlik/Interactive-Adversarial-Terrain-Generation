#include <util/box_utils.h>

#include <iostream>
#include <string>

#include <bgfx/bgfx.h>
#include <bx/commandline.h>
#include <bx/commandline.h>
#include <bx/endian.h>
#include <bx/math.h>
#include <bx/readerwriter.h>
#include <bx/string.h>
#include <bx/allocator.h>
#include <bx/file.h>

#include <bimg/decode.h>

namespace dirtbox
{

Args::Args(int _argc, const char *const *_argv)
    : m_type(bgfx::RendererType::Count), m_pciId(BGFX_PCI_ID_NONE)
{
    bx::CommandLine cmdLine(_argc, (const char **)_argv);

    if (cmdLine.hasArg("gl"))
    {
        m_type = bgfx::RendererType::OpenGL;
    }
    else if (cmdLine.hasArg("vk"))
    {
        m_type = bgfx::RendererType::Vulkan;
    }
    else if (cmdLine.hasArg("noop"))
    {
        m_type = bgfx::RendererType::Noop;
    }

    if (cmdLine.hasArg("amd"))
    {
        m_pciId = BGFX_PCI_ID_AMD;
    }
    else if (cmdLine.hasArg("nvidia"))
    {
        m_pciId = BGFX_PCI_ID_NVIDIA;
    }
    else if (cmdLine.hasArg("intel"))
    {
        m_pciId = BGFX_PCI_ID_INTEL;
    }
    else if (cmdLine.hasArg("sw"))
    {
        m_pciId = BGFX_PCI_ID_SOFTWARE_RASTERIZER;
    }
}

static bx::FileReaderI* s_fileReader = nullptr;
static bx::FileWriterI* s_fileWriter = nullptr;

static bx::DefaultAllocator s_allocator;
static bx::AllocatorI* g_allocator = &s_allocator;

void utilInit() {
    s_fileReader = BX_NEW(g_allocator, bx::FileReader);
    s_fileWriter = BX_NEW(g_allocator, bx::FileWriter);
}

bx::FileReaderI* getFileReader()
{
    return s_fileReader;
}

bx::FileWriterI* getFileWriter()
{
    return s_fileWriter;
}


bx::AllocatorI* getAllocator()
{
    if (NULL == g_allocator)
    {
        g_allocator = &s_allocator;
    }

    return g_allocator;
}

void* load(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const std::string& _filePath, uint32_t* _size)
{
    if (bx::open(_reader, _filePath.c_str()) )
    {
        uint32_t size = (uint32_t)bx::getSize(_reader);
        void* data = BX_ALLOC(_allocator, size);
        bx::read(_reader, data, size);
        bx::close(_reader);
        if (NULL != _size)
        {
            *_size = size;
        }
        return data;
    }
    else
    {
        std::clog << "Failed to open: " << std::string{_filePath} << std::endl;
    }

    if (NULL != _size)
    {
        *_size = 0;
    }

    return NULL;
}

void* load(const std::string& _filePath, uint32_t* _size)
{
    return load(getFileReader(), getAllocator(), _filePath, _size);
}

void unload(void* _ptr)
{
    BX_FREE(getAllocator(), _ptr);
}

static const bgfx::Memory* loadMem(bx::FileReaderI* _reader, const std::string& _filePath)
{
    if (bx::open(_reader, _filePath.c_str()) )
    {
        uint32_t size = (uint32_t)bx::getSize(_reader);
        const bgfx::Memory* mem = bgfx::alloc(size+1);
        bx::read(_reader, mem->data, size);
        bx::close(_reader);
        mem->data[mem->size-1] = '\0';
        return mem;
    }

    std::clog << "Failed to load " << _filePath << std::endl;
    return NULL;
}

static void* loadMem(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const std::string& _filePath, uint32_t* _size)
{
    if (bx::open(_reader, _filePath.c_str()) )
    {
        uint32_t size = (uint32_t)bx::getSize(_reader);
        void* data = BX_ALLOC(_allocator, size);
        bx::read(_reader, data, size);
        bx::close(_reader);

        if (NULL != _size)
        {
            *_size = size;
        }
        return data;
    }

    std::clog << "Failed to load " << _filePath << std::endl;
    return NULL;
}

static bgfx::ShaderHandle loadShader(bx::FileReaderI* _reader, const std::string& _name)
{
    std::string filePath;

    std::string shaderPath = "???";

    switch (bgfx::getRendererType() )
    {
    case bgfx::RendererType::Noop:
    case bgfx::RendererType::Direct3D9:  shaderPath = "shaders/dx9/";   break;
    case bgfx::RendererType::Direct3D11:
    case bgfx::RendererType::Direct3D12: shaderPath = "shaders/dx11/";  break;
    case bgfx::RendererType::Gnm:        shaderPath = "shaders/pssl/";  break;
    case bgfx::RendererType::Metal:      shaderPath = "shaders/metal/"; break;
    case bgfx::RendererType::Nvn:        shaderPath = "shaders/nvn/";   break;
    case bgfx::RendererType::OpenGL:     shaderPath = "shaders/glsl/";  break;
    case bgfx::RendererType::OpenGLES:   shaderPath = "shaders/essl/";  break;
    case bgfx::RendererType::Vulkan:     shaderPath = "shaders/spirv/"; break;
    case bgfx::RendererType::WebGPU:     shaderPath = "shaders/spirv/"; break;

    case bgfx::RendererType::Count:
        BX_ASSERT(false, "You should not be here!");
        break;
    }

    filePath = shaderPath + _name + ".bin";

    bgfx::ShaderHandle handle = bgfx::createShader(loadMem(_reader, filePath) );
    bgfx::setName(handle, _name.c_str());

    return handle;
}

bgfx::ShaderHandle loadShader(const std::string& _name)
{
    return loadShader(getFileReader(), _name);
}

bgfx::ProgramHandle loadProgram(bx::FileReaderI* _reader, const std::string& _vsName, const std::string& _fsName)
{
    bgfx::ShaderHandle vsh = loadShader(_reader, _vsName);
    bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
    if (!_fsName.empty())
    {
        fsh = loadShader(_reader, _fsName);
    }

    return bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);
}

bgfx::ProgramHandle loadProgram(const std::string& _vsName, const std::string& _fsName)
{
    return loadProgram(getFileReader(), _vsName, _fsName);
}

static void imageReleaseCb(void* _ptr, void* _userData)
{
    BX_UNUSED(_ptr);
    bimg::ImageContainer* imageContainer = (bimg::ImageContainer*)_userData;
    bimg::imageFree(imageContainer);
}

bgfx::TextureHandle loadTexture(bx::FileReaderI* _reader, const std::string& _filePath, uint64_t _flags, uint8_t _skip, bgfx::TextureInfo* _info, bimg::Orientation::Enum* _orientation)
{
    BX_UNUSED(_skip);
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

    uint32_t size;
    void* data = load(_reader, getAllocator(), _filePath, &size);
    if (NULL != data)
    {
        bimg::ImageContainer* imageContainer = bimg::imageParse(getAllocator(), data, size);

        if (NULL != imageContainer)
        {
            if (NULL != _orientation)
            {
                *_orientation = imageContainer->m_orientation;
            }

            const bgfx::Memory* mem = bgfx::makeRef(
                      imageContainer->m_data
                    , imageContainer->m_size
                    , imageReleaseCb
                    , imageContainer
                    );
            unload(data);

            if (imageContainer->m_cubeMap)
            {
                handle = bgfx::createTextureCube(
                      uint16_t(imageContainer->m_width)
                    , 1 < imageContainer->m_numMips
                    , imageContainer->m_numLayers
                    , bgfx::TextureFormat::Enum(imageContainer->m_format)
                    , _flags
                    , mem
                    );
            }
            else if (1 < imageContainer->m_depth)
            {
                handle = bgfx::createTexture3D(
                      uint16_t(imageContainer->m_width)
                    , uint16_t(imageContainer->m_height)
                    , uint16_t(imageContainer->m_depth)
                    , 1 < imageContainer->m_numMips
                    , bgfx::TextureFormat::Enum(imageContainer->m_format)
                    , _flags
                    , mem
                    );
            }
            else if (bgfx::isTextureValid(0, false, imageContainer->m_numLayers, bgfx::TextureFormat::Enum(imageContainer->m_format), _flags) )
            {
                handle = bgfx::createTexture2D(
                      uint16_t(imageContainer->m_width)
                    , uint16_t(imageContainer->m_height)
                    , 1 < imageContainer->m_numMips
                    , imageContainer->m_numLayers
                    , bgfx::TextureFormat::Enum(imageContainer->m_format)
                    , _flags
                    , mem
                    );
            }

            if (bgfx::isValid(handle) )
            {
                bgfx::setName(handle, _filePath.c_str());
            }

            if (NULL != _info)
            {
                bgfx::calcTextureSize(
                      *_info
                    , uint16_t(imageContainer->m_width)
                    , uint16_t(imageContainer->m_height)
                    , uint16_t(imageContainer->m_depth)
                    , imageContainer->m_cubeMap
                    , 1 < imageContainer->m_numMips
                    , imageContainer->m_numLayers
                    , bgfx::TextureFormat::Enum(imageContainer->m_format)
                    );
            }
        }
    }

    return handle;
}

bgfx::TextureHandle loadTexture(const std::string& _name, uint64_t _flags, uint8_t _skip, bgfx::TextureInfo* _info, bimg::Orientation::Enum* _orientation)
{
    return loadTexture(getFileReader(), _name, _flags, _skip, _info, _orientation);
}

bimg::ImageContainer* imageLoad(const std::string& _filePath, bgfx::TextureFormat::Enum _dstFormat)
{
    uint32_t size = 0;
    void* data = loadMem(getFileReader(), getAllocator(), _filePath, &size);

    return bimg::imageParse(getAllocator(), data, size, bimg::TextureFormat::Enum(_dstFormat) );
}

bool savePng(const char* _filePath, uint32_t _width, uint32_t _height, const void* _src, bool _yflip)
{
	bx::FileWriter writer;
	bx::Error err;
	if (bx::open(&writer, _filePath, false, &err) )
	{
		bimg::imageWritePng(&writer, _width, _height, 4 * _width, _src, bimg::TextureFormat::RGB8, _yflip, &err);
		bx::close(&writer);
        if(err.isOk())
            return true;
	}
    return false;
}

} // namespace dirtbox