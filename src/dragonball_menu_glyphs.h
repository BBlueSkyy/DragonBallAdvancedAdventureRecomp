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
inline constexpr std::array<uint64_t,22> Press={
0x0001e38180000000ull,0x3c3c7e3e3e008000ull,0x66667e3e3c7e3cf2ull,0x66661866f9666606ull,0x66f8666606666618ull,0x0606666618666618ull,0x661866661806bc06ull,0x66980cfc0c0c8666ull,0xf81818be6666db66ull,0x863e3edb66661918ull,0xdb3e7e1830833030ull,0x1860ff6060863606ull,0x6666866606db3666ull,0x6686db66661b66ffull,0x6666db66ff6666f6ull,0x66ff66660666f6dbull,0x3c7e66f6db6666dbull,0xf0db6666db3cff3cull,0x00c300ff00000000ull,0xff00000000f0c300ull,0x0000f0c30000c300ull,0xc30000c381ff8181ull};
inline constexpr std::array<uint64_t,14> Option={
0x080811f83c040000ull,0x1bfc7e1f00000001ull,0xc63b800000030c3eull,0xc00000031c771860ull,0x00033cc198618660ull,0x3cc198618660c000ull,0x9860c64040000003ull,0x7e60c00000036c80ull,0xc0000003ccc19860ull,0x0003ccc198603e60ull,0x8c771860063b8000ull,0x1860061f00000003ull,0x0204000000030c3eull,0x0000000108081000ull};
inline constexpr std::array<uint64_t,20> Collection={
0x0000000000000000ull,0x2000000000000000ull,0x0840008fc003f020ull,0x9f0fc0c0c1f07c00ull,0x0ee3b8006187c37full,0x8e3b8c30ee030303ull,0x0c0c0c0c60c03001ull,0x8300c006798330c0ull,0xe60cc30031f03031ull,0x47c0c0c404010019ull,0x300c006d90130c00ull,0x60cc300303030318ull,0x0c0c0c60c03001e6ull,0x3b8007998330c00cull,0xb8c30ee0303030eeull,0xcfcfc1f07c001c63ull,0x00006187c30c1f0full,0x0800003f3e3e0000ull,0x0000000000008400ull,0x0000000000000000ull};
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
