#include <terrain/generator.h>

#include <algorithm>
#include <stdexcept>

#include <third_party/httplib.h>
#include <nlohmann/json.hpp>
#include <third_party/base64.h>
#include <bimg/decode.h>
#include <util/box_utils.h>

using namespace std;
using namespace nlohmann;

namespace dirtbox::terrain {

const vec2u HttpGanGenerator::TargetSize = {512, 512};

void HttpGanGenerator::Generate(terrain::Terrain& t) const {
    if (t.getSize() != TargetSize)
        throw std::runtime_error("Invalid target texture");
    
    httplib::Client cli{hostName, hostPort};

    std::string encoded_data;
    {
        std::vector<uint8_t> buf;
        inputImage.writeImagePNG(buf);
        encoded_data = base64_encode(buf.data(), buf.size());
    }

    httplib::Result res = cli.Post("/generate", json{{"image", encoded_data}}.dump(), "application/json");
    if (!res)
        throw std::runtime_error("Failed to connect to generator server " + hostName + ":" + to_string(hostPort)); 
    if (res->status == 200) {
        std::string raw = base64_decode(json::parse(res->body).at("image").get<std::string>());
        auto img = resource::ImageData{bimg::imageParse(getAllocator(), raw.c_str(), raw.length())}.getAsFormat(bgfx::TextureFormat::RGBA32F);
        t.setTerrainData(img);
    } else {
        throw std::runtime_error("Http error code " + std::to_string(res->status));
    }
}

}