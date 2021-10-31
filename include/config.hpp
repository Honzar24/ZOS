#pragma once

#include <string>
#include <cstdlib>
#include <cassert>

// types
/// typ vyuzivany cisla
using size_type = u_int64_t;
/// typ vyuzivany ukazateli
using pointer_type = u_int64_t;
/// typ pro mala cisla
using counter_type = u_int8_t;

// structures
constexpr const size_t directPointersCount = 2;
constexpr const size_t indirectPointersCount = 3;

// superBlock
constexpr const size_t maxSignatureLenght = 10;
constexpr const size_t maxDescriptionLenght = 20;

constexpr const char defaulSignature[] = "honzar";
static_assert(std::size(defaulSignature) <= maxSignatureLenght);
constexpr const char defaulDescription[] = "Default descripcion";
static_assert(std::size(defaulDescription) <= maxDescriptionLenght);

/// Velikost bloku v Bytech
constexpr const size_type defaultBlockSize = 1024;
static_assert(defaultBlockSize % 8 == 0);
// základní pocet bloku na disku
constexpr const size_type defaultBlockCount = 1024;
static_assert(defaultBlockCount % 8 == 0);
// základní pocet inodu na disku
constexpr const size_type defaultInodeCount = 128;
static_assert(defaultInodeCount % 8 == 0);

constexpr const float pomerDataInode = 25 / 100.0;
constexpr const size_t minInodeCount = 16;
constexpr const size_t minDataBlockCount = 64;

// files a dirs
constexpr const size_t maxFileNameLenght = 8;
constexpr const size_t maxSufixLenght = 3;
constexpr const size_t fileLiteralLenght = maxFileNameLenght + maxSufixLenght + 1;
