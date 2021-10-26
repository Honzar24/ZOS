#pragma once

#include <string>
#include <cstdlib>
#include <cassert>

//types
/// typ vyuzivany cisla
typedef u_int64_t size_type;
/// typ vyuzivany ukazateli   
typedef u_int64_t pointer_type;
/// typ pro mala cisla
typedef u_int8_t counter_type;

//structures

//superBlock
constexpr const size_t maxSignatureLenght = 10;
constexpr const size_t maxDescriptionLenght = 20;

constexpr const char defaulSignature[] = "honzar";
static_assert(std::size(defaulSignature) <= maxSignatureLenght);
constexpr const char defaulDescription[] = "Default descripcion";
static_assert(std::size(defaulDescription) <= maxDescriptionLenght);

constexpr const float pomerDataInode = 90/100.0f;

//sekce delitelnosti osmi
///Velikost bloku v Bytech
constexpr const size_type defaultBlockSize = 1024;
static_assert(defaultBlockSize % 8 == 0);


// základní pocet bloku na disku
constexpr const size_type defaultBlockCount = 1024;
static_assert(defaultBlockCount % 8 == 0);
// základní pocet inodu na disku
constexpr const size_type defaultInodeCount = 128;
static_assert(defaultInodeCount % 8 == 0);


//files a dirs
constexpr const size_t maxFileNameLenght = 8;
constexpr const size_t maxSufixLenght = 3;
constexpr const size_t fileLiteralLenght = maxFileNameLenght + maxSufixLenght + 1 ;


