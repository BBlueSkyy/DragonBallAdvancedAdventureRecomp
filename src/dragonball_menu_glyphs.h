// Title glyph shapes traced from the supplied menu captures, not ROM offsets.
#pragma once
#include <array>
#include <bit>
#include <algorithm>
#include <cstdint>
namespace dragonball::menu_touch::glyph {
inline constexpr std::array<uint64_t,14> Data={
0x47e808020fc10070ull,0x18071fe381f81088ull,0x8307c3f830d867ecull,0xc71830d8e06e380full,0x30d9e06e380d8306ull,0xe3eff818c30c6618ull,0xd818c30c661830d9ull,0xe31fe61830db63edull,0xf71830de606c983full,0x30de606c183fe31full,0x606c1870733833f8ull,0x1860333019f839dcull,0x303018001f9867ecull,0x00000f0847e80860ull};
inline constexpr std::array<uint64_t,14> Game={
0x47e8083f404100c0ull,0x183f60c383f01088ull,0x71c3873830d867ecull,0xc61830d8e06e3803ull,0x30d9e06e380371c6ull,0xe3eff81f7fc4400cull,0xd81f6ec4430c30d9ull,0x64cfe70c30db63edull,0xe61830de606c9803ull,0x30de606c180360cfull,0x606c180360d83738ull,0x183f60d837f039dcull,0x405016001f9867ecull,0x00000f0847e8083full};
inline constexpr std::array<uint64_t,4> Pause={
0xdbdf6df7c0000000ull,0xd86f7db7df186db6ull,0x00003df7db0c6c36ull,0x0000000000000000ull};
template<size_t N> bool matches(const uint8_t* p,unsigned stride,unsigned x0,unsigned y0,
 unsigned w,unsigned h,bool red,const std::array<uint64_t,N>& expected) {
 unsigned lo=255,hi=0;
 auto level=[&](unsigned x,unsigned y){const auto* q=p+3*(y*stride+x);return red?unsigned(q[0]):unsigned(std::min({q[0],q[1],q[2]}));};
 for(unsigned y=y0;y<y0+h;++y)for(unsigned x=x0;x<x0+w;++x){auto v=level(x,y);lo=std::min(lo,v);hi=std::max(hi,v);}
 if(hi-lo<32)return false; // a flat/dark fade cannot establish a title
 const auto cut=lo+(hi-lo)*3/4;
 std::array<uint64_t,N> actual{};unsigned bit=0;
 for(unsigned y=y0;y<y0+h;++y)for(unsigned x=x0;x<x0+w;++x,++bit)
  if(level(x,y)>cut)actual[bit/64]|=uint64_t(1)<<(bit%64);
 unsigned errors=0;for(size_t i=0;i<N;++i)errors+=std::popcount(actual[i]^expected[i]);
 // At least 97.5% of the raster must agree, independent of title/background hue.
 return errors<=w*h/40;
}
}
