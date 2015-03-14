
#pragma once

#include <cstring>
#include <string>
#include <algorithm>
#include <random>

#include <QtCore>

/*
 * TEA impl
 */


struct TEA
{
private:
    std::vector<uint8_t> j(const std::vector<uint8_t>& A);

	void o();

	std::vector<uint8_t> h(const std::vector<uint8_t>& A);
public:
	TEA(std::string bin_key);

	std::string enAsBase64(const std::string& data);


	std::string strToBytes(const std::string& str);

private:

	std::vector<uint8_t> r;
	int a=0;

	std::vector<uint8_t> g;
	std::vector<uint8_t> w;

	int x=0;
	int t=0;

	std::vector<uint8_t> l;
	bool m = true;
};
