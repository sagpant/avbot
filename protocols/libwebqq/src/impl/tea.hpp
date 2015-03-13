
#pragma once

#include <cstring>
#include <string>
#include <algorithm>
#include <random>

#include <boost/base64.hpp>

/*
 * TEA impl
 */


struct TEA
{
	unsigned i(const std::string& B, int C, int y)
	{
        if (!y || y > 4) {
            y = 4;
        }
        auto z = 0ul;
        for (auto A = C; A < C + y; A++) {
            z <<= 8;
            z |= (unsigned)(B[A]);
        }
        return z;
    }

    void b(std::vector<uint8_t>&z, int A, uint8_t y)
	{
        z[A + 3] = (y >> 0);
        z[A + 2] = (y >> 8);
        z[A + 1] = (y >> 16);
        z[A + 0] = (y >> 24);
    }

    template<class T>
    std::vector<uint8_t> j(const T& A)
	{
        auto B = 16;
        auto G = i(A, 0, 4);
        auto F = i(A, 4, 4);
        auto I = i(r, 0, 4);
        auto H = i(r, 4, 4);
        auto E = i(r, 8, 4);
        auto D = i(r, 12, 4);
        auto C = 0u;
        auto J = 265443576u;

		while (B-- > 0)
		{
            C += J;
			G += ((F << 4) + I) ^ (F + C) ^ ((F >> 5) + H);
            F += ((G << 4) + E) ^ (G + C) ^ ((G >> 5) + D);
        }
        std::vector<uint8_t> K(8);
        b(K, 0, G);
        b(K, 4, F);
        return K;
    }

	void o()
	{
        for (auto y = 0; y < 8; y++)
		{
            if (m) {
                g[y] ^= w[y];
            } else {
                g[y] ^= l[t + y];
            }
        }
        auto z = j(g);
        for (auto y = 0; y < 8; y++) {
            l[x + y] = z[y] ^ w[y];
            w[y] = g[y];
        }
        t = x;
        x += 8;
        a = 0;
        m = false;
    }

	std::string h(const std::string& A)
	{
		std::uniform_int_distribution<uint8_t> _e;
		std::mt19937 rand_generator;

		auto e = [&_e,&rand_generator](){
			return _e(rand_generator);
		};

		x = t = 0;
        m = true;
        a = 0;
        auto y = A.length();
        auto B = 0;

		a = (y + 10) % 8;
        if (a != 0) {
            a = 8 - a;
        }

        l.resize(y + a + 10);

		g[0] = ((e() & 248) | a) & 255;

		for (auto z = 1; z <= a; z++) {
            g[z] = e();
        }

        a++;

        B = 1;
        while (B <= 2) {
            if (a < 8) {
                g[a++] = e() & 255;
                B++;
            }
            if (a == 8) {
                o();
            }
        }
        auto z = 0;
        while (y > 0) {
            if (a < 8) {
                g[a++] = A[z++];
                y--;
            }
            if (a == 8) {
                o();
            }
        }
        B = 1;
        while (B <= 7) {
            if (a < 8) {
                g[a++] = 0;
                B++;
            }
            if (a == 8) {
                o();
            }
        }
        return l;
    }

	TEA(std::string bin_key)
	{
		r = std::move(bin_key);
	}

	std::string enAsBase64(const std::string& data)
	{
		auto A = h(data);

		std::string A_(reinterpret_cast<const char*>(A.data()), A.size());

		return boost::base64_encode(A_);
	}


	std::string strToBytes(const std::string& str)
	{
        std::string y;

		for (auto z = 0; z < str.length(); z++)
		{
			char A[4]={0};
			std::snprintf(A,sizeof A, "%02X", (unsigned)str[z]);
            y += A;
        }
        return y;
	}
// 	    var r = "",
//         a = 0,
//         g = [],
//         w = [],
//         x = 0,
//         t = 0,
//         l = [],
//         s = [],
//         m = true;

	std::string r;
	int a=0;

	char g[8]={0};
	char w[8]={0};

	int x=0;
	int t=0;

	std::string l;
	bool m = true;
};
