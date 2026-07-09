#pragma once
#include "pch.h"
#include <cstdint>
#include <cstring>

namespace CommonUtils
{
    /// <summary>
    /// SHA1哈希算法实现类
    /// </summary>
    class SHA1
    {
    public:
        SHA1();
        // 重置哈希状态
        void Reset();
        // 更新哈希数据
        void Update(const uint8_t *data, size_t length);
        // 输出最终哈希值(20字节)
        void Final(uint8_t digest[20]);

    private:
        // 核心转换函数
        void Transform(const uint8_t block[64]);

    private:
        uint32_t m_state[5];  // 哈希状态
        uint64_t m_bitCount;  // 总比特数
        uint8_t m_buffer[64]; // 数据缓冲
    };
}