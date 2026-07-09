#include "pch.h"
#include "SHA1.h"

namespace CommonUtils
{
    SHA1::SHA1()
    {
        Reset();
    }

    void SHA1::Reset()
    {
        // SHA1初始向量
        m_state[0] = 0x67452301;
        m_state[1] = 0xEFCDAB89;
        m_state[2] = 0x98BADCFE;
        m_state[3] = 0x10325476;
        m_state[4] = 0xC3D2E1F0;

        m_bitCount = 0;
        std::memset(m_buffer, 0, sizeof(m_buffer));
    }

    void SHA1::Transform(const uint8_t block[64])
    {
        uint32_t w[80] = {0};
        // 初始化前16个字
        for (int i = 0; i < 16; ++i)
        {
            w[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) |
                   (block[i * 4 + 2] << 8) | block[i * 4 + 3];
        }

        // 扩展到80个字
        for (int i = 16; i < 80; ++i)
        {
            const uint32_t val = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (val << 1) | (val >> 31);
        }

        // 初始化寄存器
        uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3], e = m_state[4];
        uint32_t f, k;

        // 主循环
        for (int i = 0; i < 80; ++i)
        {
            if (i < 20)
            {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            }
            else if (i < 40)
            {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if (i < 60)
            {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            }
            else
            {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            const uint32_t temp = (a << 5 | a >> 27) + f + e + k + w[i];
            e = d;
            d = c;
            c = b << 30 | b >> 2;
            b = a;
            a = temp;
        }

        // 更新哈希状态
        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;
        m_state[4] += e;
    }

    void SHA1::Update(const uint8_t *data, size_t length)
    {
        size_t index = (m_bitCount >> 3) & 63;
        m_bitCount += length << 3;

        // 填充缓冲区
        if (index > 0)
        {
            const size_t fillSize = 64 - index;
            if (length >= fillSize)
            {
                std::memcpy(m_buffer + index, data, fillSize);
                Transform(m_buffer);
                data += fillSize;
                length -= fillSize;
                index = 0;
            }
        }

        // 处理完整块
        while (length >= 64)
        {
            Transform(data);
            data += 64;
            length -= 64;
        }

        // 保存剩余数据
        if (length > 0)
        {
            std::memcpy(m_buffer + index, data, length);
        }
    }

    void SHA1::Final(uint8_t digest[20])
    {
        uint8_t bitCountBytes[8] = {0};
        // 存储总比特数（大端序）
        for (int i = 0; i < 8; ++i)
        {
            bitCountBytes[i] = (m_bitCount >> ((7 - i) * 8)) & 0xFF;
        }

        // 填充结束标志
        const uint8_t pad = 0x80;
        Update(&pad, 1);

        // 填充0直到缓冲区剩余56字节
        const size_t index = (m_bitCount >> 3) & 63;
        const size_t padLength = (index < 56) ? (56 - index) : (120 - index);
        uint8_t zeroPad[64] = {0};
        Update(zeroPad, padLength);

        // 追加长度
        Update(bitCountBytes, 8);

        // 输出哈希结果（大端序）
        for (int i = 0; i < 20; ++i)
        {
            digest[i] = (m_state[i / 4] >> ((3 - (i % 4)) * 8)) & 0xFF;
        }
    }
}
