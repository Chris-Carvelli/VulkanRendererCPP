#include <cc_logger.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

// TMP headers
#include <map>

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

#define FOURCC_DXT1 0x31545844  // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844  // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844  // Equivalent to "DXT5" in ASCII

// DDS Pixel Format
typedef struct {
    uint32_t size;
    uint32_t flags;
    uint32_t fourcc;
    uint32_t rgb_bit_count;
    uint32_t r_bit_mask;
    uint32_t g_bit_mask;
    uint32_t b_bit_mask;
    uint32_t a_bit_mask;
} dds_pixel_format;

// DDS Header (124 bytes)
typedef struct {
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitch_or_linear_size;
    uint32_t depth;
    uint32_t mipmap_count;
    uint32_t reserved1[11];
    dds_pixel_format ddspf;
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
} dds_header;

std::map<unsigned int, unsigned int> TMP_fourcc_counts;
std::map<unsigned int, VkFormat> TMP_fourcc_names {
    { MAKEFOURCC('D', 'X', 'T', '1'), VK_FORMAT_BC1_RGBA_UNORM_BLOCK },
    { MAKEFOURCC('D', 'X', 'T', '3'), VK_FORMAT_BC2_UNORM_BLOCK },
    { MAKEFOURCC('D', 'X', 'T', '5'), VK_FORMAT_BC3_UNORM_BLOCK },
    { MAKEFOURCC('D', 'X', 'T', '2'), VK_FORMAT_BC2_UNORM_BLOCK },
    { MAKEFOURCC('D', 'X', 'T', '4'), VK_FORMAT_BC3_UNORM_BLOCK },
    { MAKEFOURCC('A', 'T', 'I', '1'), VK_FORMAT_BC4_UNORM_BLOCK },
    { MAKEFOURCC('B', 'C', '4', 'U'), VK_FORMAT_BC4_UNORM_BLOCK },
    { MAKEFOURCC('B', 'C', '4', 'S'), VK_FORMAT_BC4_SNORM_BLOCK },
    { MAKEFOURCC('A', 'T', 'I', '2'), VK_FORMAT_BC5_UNORM_BLOCK },
    { MAKEFOURCC('B', 'C', '5', 'U'), VK_FORMAT_BC5_UNORM_BLOCK },
    { MAKEFOURCC('B', 'C', '5', 'S'), VK_FORMAT_BC5_SNORM_BLOCK },
    { MAKEFOURCC('R', 'G', 'B', 'G'), VK_FORMAT_G8B8G8R8_422_UNORM },
    { MAKEFOURCC('G', 'R', 'G', 'B'), VK_FORMAT_B8G8R8G8_422_UNORM },
    { MAKEFOURCC('U', 'Y', 'V', 'Y'), VK_FORMAT_G8B8G8R8_422_UNORM },
    { MAKEFOURCC('Y', 'U', 'Y', '2'), VK_FORMAT_B8G8R8G8_422_UNORM },
    { MAKEFOURCC('U', 'Y', 'V', 'Y'), VK_FORMAT_G8B8G8R8_422_UNORM_KHR },
    { MAKEFOURCC('Y', 'U', 'Y', '2'), VK_FORMAT_G8B8G8R8_422_UNORM_KHR }
};

void print_fourcc_count() {
    /*CC_LOG(IMPORTANT, "FourCC counts");

    for(auto& kp : TMP_fourcc_counts) {
        unsigned int fourcc_id = kp.first;
        unsigned int fourcc_count = kp.second;

        if(TMP_fourcc_names.contains(fourcc_id))
        {
            VkFormat format = TMP_fourcc_names[fourcc_id];
            CC_LOG(LOG, "%-32s %3d", string_VkFormat(format), fourcc_count);
        }
        else
            CC_LOG(LOG, "<UNKNOWN>%d\t\t%3d", fourcc_id, fourcc_count);
    }*/
}

// from https://github.com/Sixshaman/DDSTextureLoaderVk/blob/master/DDSTextureLoaderVk.cpp#L2196
void fourcc_check(dds_header* header)
{
    if (!TMP_fourcc_counts.contains(header->ddspf.fourcc))
        TMP_fourcc_counts[header->ddspf.fourcc] = 0;
    TMP_fourcc_counts[header->ddspf.fourcc]++;

    //if      (MAKEFOURCC('D', 'X', 'T', '1') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_BC1_RGBA_UNORM_BLOCK");
    //else if (MAKEFOURCC('D', 'X', 'T', '3') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_BC2_UNORM_BLOCK");
    //else if (MAKEFOURCC('D', 'X', 'T', '5') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_BC3_UNORM_BLOCK");
    //else if (MAKEFOURCC('D', 'X', 'T', '2') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_BC2_UNORM_BLOCK");     // While pre-multiplied alpha isn't directly supported by the VK formats,
    //else if (MAKEFOURCC('D', 'X', 'T', '4') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_BC3_UNORM_BLOCK");     // they are basically the same as these BC formats so they can be mapped
    //else if (MAKEFOURCC('A', 'T', 'I', '1') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_BC4_UNORM_BLOCK");
    //else if (MAKEFOURCC('B', 'C', '4', 'U') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_BC4_UNORM_BLOCK");
    //else if (MAKEFOURCC('B', 'C', '4', 'S') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_BC4_SNORM_BLOCK");
    //else if (MAKEFOURCC('A', 'T', 'I', '2') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_BC5_UNORM_BLOCK");
    //else if (MAKEFOURCC('B', 'C', '5', 'U') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_BC5_UNORM_BLOCK");
    //else if (MAKEFOURCC('B', 'C', '5', 'S') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_BC5_SNORM_BLOCK");
    //else if (MAKEFOURCC('R', 'G', 'B', 'G') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_G8B8G8R8_422_UNORM"); // BC6H and BC7 are written using the "DX10"extended header
    //else if (MAKEFOURCC('G', 'R', 'G', 'B') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_B8G8R8G8_422_UNORM");
    //else if (MAKEFOURCC('U', 'Y', 'V', 'Y') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_G8B8G8R8_422_UNORM"); //#if defined(VK_VERSION_1_1) && VK_VERSION_1_1
    //else if (MAKEFOURCC('Y', 'U', 'Y', '2') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_B8G8R8G8_422_UNORM");
    //else if (MAKEFOURCC('U', 'Y', 'V', 'Y') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_G8B8G8R8_422_UNORM_KHR"); //#elif defined(VK_KHR_sampler_ycbcr_conversion)
    //else if (MAKEFOURCC('Y', 'U', 'Y', '2') == header->ddspf.fourcc) CC_LOG(LOG, "VK_FORMAT_G8B8G8R8_422_UNORM_KHR"); //#endif // #if defined(VK_VERSION_1_1) && VK_VERSION_1_1
    //else                                                             CC_LOG(WARNING, "UNRECOGNIZED FOURCC %d", header->ddspf.fourcc);

    /*CC_LOG(LOG, "%s", fourcc_name);*/
}

VkFormat get_format(dds_header* header) {
    if      (MAKEFOURCC('D', 'X', 'T', '1') == header->ddspf.fourcc) {
        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    }
    else if (MAKEFOURCC('D', 'X', 'T', '3') == header->ddspf.fourcc) {
        return VK_FORMAT_BC2_UNORM_BLOCK;
    }
    else if (MAKEFOURCC('D', 'X', 'T', '5') == header->ddspf.fourcc) {
        return VK_FORMAT_BC3_UNORM_BLOCK;
    }
    else if (MAKEFOURCC('D', 'X', 'T', '2') == header->ddspf.fourcc) {
        return VK_FORMAT_BC2_UNORM_BLOCK;     // While pre-multiplied alpha isn't directly supported by the VK formats,
    }
    else if (MAKEFOURCC('D', 'X', 'T', '4') == header->ddspf.fourcc) {
        return VK_FORMAT_BC3_UNORM_BLOCK;     // they are basically the same as these BC formats so they can be mapped
    }
    else if (MAKEFOURCC('A', 'T', 'I', '1') == header->ddspf.fourcc) {
        return VK_FORMAT_BC4_UNORM_BLOCK;
    }
    else if (MAKEFOURCC('B', 'C', '4', 'U') == header->ddspf.fourcc) {
        return VK_FORMAT_BC4_UNORM_BLOCK;
    }
    else if (MAKEFOURCC('B', 'C', '4', 'S') == header->ddspf.fourcc) {
        return VK_FORMAT_BC4_SNORM_BLOCK;
    }
    else if (MAKEFOURCC('A', 'T', 'I', '2') == header->ddspf.fourcc) {
        return VK_FORMAT_BC5_UNORM_BLOCK;
    }
    else if (MAKEFOURCC('B', 'C', '5', 'U') == header->ddspf.fourcc) {
        return VK_FORMAT_BC5_UNORM_BLOCK;
    }
    else if (MAKEFOURCC('B', 'C', '5', 'S') == header->ddspf.fourcc) {
        return VK_FORMAT_BC5_SNORM_BLOCK;
    }
    else if (MAKEFOURCC('R', 'G', 'B', 'G') == header->ddspf.fourcc) {
        return VK_FORMAT_G8B8G8R8_422_UNORM; // BC6H and BC7 are written using the "DX10"extended header
    }
    else if (MAKEFOURCC('G', 'R', 'G', 'B') == header->ddspf.fourcc) {
        return VK_FORMAT_B8G8R8G8_422_UNORM;
    }
    else if (MAKEFOURCC('U', 'Y', 'V', 'Y') == header->ddspf.fourcc) {
        return VK_FORMAT_G8B8G8R8_422_UNORM; //#if defined(VK_VERSION_1_1) && VK_VERSION_1_1
    }
    else if (MAKEFOURCC('Y', 'U', 'Y', '2') == header->ddspf.fourcc) {
        return VK_FORMAT_B8G8R8G8_422_UNORM;
    }
    else if (MAKEFOURCC('U', 'Y', 'V', 'Y') == header->ddspf.fourcc) {
        return VK_FORMAT_G8B8G8R8_422_UNORM_KHR; //#elif defined(VK_KHR_sampler_ycbcr_conversion)
    }
    else if (MAKEFOURCC('Y', 'U', 'Y', '2') == header->ddspf.fourcc) {
        return VK_FORMAT_G8B8G8R8_422_UNORM_KHR; //#endif // #if defined(VK_VERSION_1_1) && VK_VERSION_1_1
    }
    else CC_LOG(WARNING, "UNRECOGNIZED FOURCC %d", header->ddspf.fourcc);
}

inline void *rl_load_dds_from_memory(const unsigned char *file_data, unsigned int file_size, int *width, int *height, int *format, int *mips, uint32_t *image_data_size)
{
    *image_data_size = 0;

    void *image_data = nullptr;        // Image data pointer
    int image_pixel_size = 0;       // Image pixel size

    unsigned char *file_data_ptr = (unsigned char *)file_data;

    int data_size = 0;

    if (file_data_ptr == nullptr)
        return nullptr;

    // Verify the type of file
    unsigned char *dds_header_id = file_data_ptr;
    file_data_ptr += 4;

    if ((dds_header_id[0] != 'D') || (dds_header_id[1] != 'D') || (dds_header_id[2] != 'S') || (dds_header_id[3] != ' '))
    {
        CC_LOG(WARNING, "IMAGE: DDS file data not valid");
        return nullptr;
    }

    dds_header *header = (dds_header *)file_data_ptr;

    //fourcc_check(header);

    auto header_size = sizeof(dds_header);
    file_data_ptr += sizeof(dds_header);        // Skip header

    *width = header->width;
    *height = header->height;

    image_pixel_size = header->width * header->height;

    if (header->mipmap_count == 0)
        *mips = 1;   // Parameter not used
    else
        *mips = header->mipmap_count;

    *format = get_format(header);

    //CC_LOG(LOG, "%s", string_VkFormat((VkFormat)*format));

    if (header->ddspf.rgb_bit_count == 16 && header->ddspf.flags == 0x40)      // 16bit mode, no compressed
        data_size = image_pixel_size*sizeof(unsigned short);
    else if ((header->ddspf.flags == 0x40) && (header->ddspf.rgb_bit_count == 24))   // DDS_RGB, no compressed
        data_size = image_pixel_size * 3 * sizeof(unsigned char);
    else if (((header->ddspf.flags == 0x04) || (header->ddspf.flags == 0x05)) && (header->ddspf.fourcc > 0)) // Compressed
        data_size = header->pitch_or_linear_size;
    else if(header->ddspf.fourcc == MAKEFOURCC('A', 'T', 'I', '2')) // special case for 1x1 compressed normal maps from Bistro_5_2
        data_size = header->pitch_or_linear_size;
    else
        CC_ASSERT(false, "DDS format not supported");

    if (header->mipmap_count > 1)
        data_size = data_size + data_size / 3;

    // TMP
    data_size = file_size - 4 - header_size;

    // TMP manually count mipmap?
    image_data = malloc(data_size);
    memcpy(image_data, file_data_ptr, data_size);
    *image_data_size = data_size;
    return image_data;
}