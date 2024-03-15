#pragma once

// forward delcarations.
namespace app {
  class Application;
  class Window;
}

template< typename T >
struct Vec2 {
  T x;
  T y;

  const bool operator==( const Vec2< T >& other ) const {
    return x == other.x && y == other.y;
  }

  const Vec2 operator*( const float f ) const {
    return {
      x * f,
      y * f
    };
  }
  
  const Vec2 operator-( const float f ) const {
    return {
      x - f,
      y - f
    };
  }
};

template< typename T >
struct Vec3 {
  T x;
  T y;
  T z;

  Vec3 operator-( const Vec3& other ) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  const Vec3 operator-( const Vec3& other ) const {
    return {
      x - other.x,
      y - other.y,
      z - other.z
    };
  }

  Vec3 operator+( const Vec3& other ) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  const Vec3 operator+( const Vec3& other ) const {
    return {
      x + other.x,
      y + other.y,
      z + other.z
    };
  }

  const Vec3 operator*( const float f ) const {
    return {
      x * f,
      y * f,
      z * f
    };
  }

  const Vec3 operator*( const Vec3& other ) const {
    return {
      x * other.x,
      y * other.y,
      z * other.z
    };
  }

  const Vec3 normalized() const {
    const float len = sqrt( x * x + y * y + z * z );
    return {
      x / len,
      y / len,
      z / len
    };
  }

  const float dot( const Vec3& other ) const {
    return x * other.x + y * other.y + z * other.z;
  }
};

using Vec2i = Vec2< int >;
using Vec2f = Vec2< float >;

// For better code readability.
using Tile = Vec2i;
using Point = Vec2f;

using SizeI = Vec2i;
using SizeF = Vec2f;

using Vec3f = Vec3< float >;