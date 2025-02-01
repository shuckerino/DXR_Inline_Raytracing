#pragma once
#include <cstdint>
#pragma warning(push)
#pragma warning(disable : 4310)
#pragma warning(disable : 4701)
#pragma warning(disable : 4201)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_precision.hpp>
#pragma warning(pop)

namespace gims
{
// Short Types
typedef uint8_t  ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef char c8;

typedef float  f32;
typedef double f64;

typedef glm::vec1 f32v1;
typedef glm::vec2 f32v2;
typedef glm::vec3 f32v3;
typedef glm::vec4 f32v4;

typedef glm::dvec1 f64v1;
typedef glm::dvec2 f64v2;
typedef glm::dvec3 f64v3;
typedef glm::dvec4 f64v4;

typedef glm::u64vec1 ui64v1;
typedef glm::u64vec2 ui64v2;
typedef glm::u64vec3 ui64v3;
typedef glm::u64vec4 ui64v4;

typedef glm::i64vec1 i64v1;
typedef glm::i64vec2 i64v2;
typedef glm::i64vec3 i64v3;
typedef glm::i64vec4 i64v4;

typedef glm::uvec1 ui32v1;
typedef glm::uvec2 ui32v2;
typedef glm::uvec3 ui32v3;
typedef glm::uvec4 ui32v4;

typedef glm::ivec1 i32v1;
typedef glm::ivec2 i32v2;
typedef glm::ivec3 i32v3;
typedef glm::ivec4 i32v4;

typedef glm::u16vec1 ui16v1;
typedef glm::u16vec2 ui16v2;
typedef glm::u16vec3 ui16v3;
typedef glm::u16vec4 ui16v4;

typedef glm::i16vec1 i16v1;
typedef glm::i16vec2 i16v2;
typedef glm::i16vec3 i16v3;
typedef glm::i16vec4 i16v4;

typedef glm::u8vec1 ui8v1;
typedef glm::u8vec2 ui8v2;
typedef glm::u8vec3 ui8v3;
typedef glm::u8vec4 ui8v4;

typedef glm::i8vec1 i8v1;
typedef glm::i8vec2 i8v2;
typedef glm::i8vec3 i8v3;
typedef glm::i8vec4 i8v4;

typedef glm::bvec2 bv2;
typedef glm::bvec3 bv3;
typedef glm::bvec4 bv4;

typedef glm::mat2 f32m2;
typedef glm::mat3 f32m3;
typedef glm::mat4 f32m4;

typedef glm::dmat2 f64m2;
typedef glm::dmat3 f64m3;
typedef glm::dmat4 f64m4;

typedef glm::quat    f32q;
typedef glm::f64quat f64q;
} // namespace gims
