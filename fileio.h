#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <windows.h>

namespace FileIO {
    // Memory-mapped file structure
    struct MappedFile {
        HANDLE hFile;
        HANDLE hMap;
        void* view;
        size_t bytes;
        const uint32_t* data;
        size_t count;
        
        MappedFile() 
            : hFile(INVALID_HANDLE_VALUE)
            , hMap(NULL)
            , view(nullptr)
            , bytes(0)
            , data(nullptr)
            , count(0)
        {}
    };
    
    // Memory-mapped file functions
    bool MapBinaryUInt32File(const std::wstring& path, MappedFile& out, std::wstring& err);
    void UnmapFile(MappedFile& mf);
    
    // Legacy functions
    std::vector<uint64_t> ReadNumbersFromFile(const std::wstring& filepath);
    bool WriteResultsToFile(const std::wstring& filepath, const std::wstring& content);
    std::wstring GetResultsFolder();
}