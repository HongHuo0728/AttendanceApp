#include "reports/zip.h"
#include "storage/files.h"
#include "core/constants.h"

namespace attendance {

uint32_t Crc32(const std::string& data);
void Write16(std::ofstream& file, uint16_t value);
void Write32(std::ofstream& file, uint32_t value);

uint32_t Crc32(const std::string& data){
    static const auto table = [] {
        std::array<uint32_t, 256> values{};
        uint32_t n = 0;
        for (auto& value : values) {
            uint32_t crc = n++;
            for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
            value = crc;
        }
        return values;
    }();
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char ch : data) {
        crc = (crc >> 8) ^ table[(crc ^ ch) & 255];
    }
    return ~crc;
}

void Write16(std::ofstream& file, uint16_t value){
    file.put((char)(value & 0xFF));
    file.put((char)((value >> 8) & 0xFF));
}

void Write32(std::ofstream& file, uint32_t value){
    Write16(file, (uint16_t)(value & 0xFFFF));
    Write16(file, (uint16_t)((value >> 16) & 0xFFFF));
}

bool WriteZipStore(const std::filesystem::path& path, std::vector<ZipEntry> entries){
    if (entries.size() > UINT16_MAX) return false;
    for (const auto& entry : entries) {
        if (entry.name.size() > UINT16_MAX || entry.data.size() > UINT32_MAX) return false;
    }
    return WriteStreamFileAtomically(path, [&](std::ofstream& file) {
        for (auto& entry : entries) {
            entry.crc = Crc32(entry.data);
            std::streampos offset = file.tellp();
            if (offset < 0 || (uint64_t)offset > UINT32_MAX) return false;
            entry.offset = (uint32_t)offset;
            Write32(file, 0x04034b50);
            Write16(file, 20);
            Write16(file, 0);
            Write16(file, 0);
            Write16(file, 0);
            Write16(file, 0);
            Write32(file, entry.crc);
            Write32(file, (uint32_t)entry.data.size());
            Write32(file, (uint32_t)entry.data.size());
            Write16(file, (uint16_t)entry.name.size());
            Write16(file, 0);
            file.write(entry.name.data(), entry.name.size());
            file.write(entry.data.data(), entry.data.size());
            if (!file) return false;
        }

        std::streampos centralPosition = file.tellp();
        if (centralPosition < 0 || (uint64_t)centralPosition > UINT32_MAX) return false;
        uint32_t centralOffset = (uint32_t)centralPosition;
        for (const auto& entry : entries) {
            Write32(file, 0x02014b50);
            Write16(file, 20);
            Write16(file, 20);
            Write16(file, 0);
            Write16(file, 0);
            Write16(file, 0);
            Write16(file, 0);
            Write32(file, entry.crc);
            Write32(file, (uint32_t)entry.data.size());
            Write32(file, (uint32_t)entry.data.size());
            Write16(file, (uint16_t)entry.name.size());
            Write16(file, 0);
            Write16(file, 0);
            Write16(file, 0);
            Write16(file, 0);
            Write32(file, 0);
            Write32(file, entry.offset);
            file.write(entry.name.data(), entry.name.size());
            if (!file) return false;
        }
        std::streampos endPosition = file.tellp();
        if (endPosition < 0 || (uint64_t)endPosition > UINT32_MAX) return false;
        uint32_t centralSize = (uint32_t)endPosition - centralOffset;
        Write32(file, 0x06054b50);
        Write16(file, 0);
        Write16(file, 0);
        Write16(file, (uint16_t)entries.size());
        Write16(file, (uint16_t)entries.size());
        Write32(file, centralSize);
        Write32(file, centralOffset);
        Write16(file, 0);
        return file.good();
    });
}

} // namespace attendance
