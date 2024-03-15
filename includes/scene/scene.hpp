#pragma once

#define NOMINMAX
#include <windows.h>

#include <dxgi.h>
#include <d3d11.h>

#include <vector>
#include <memory>

#include <types.hpp>
#include <colour.hpp>

// forward delcarations.
namespace app {
  class Application;
  class Window;
}

namespace scene {

  struct Ray {
    Vec3f origin;
    Vec3f direction;
    Vec3f hit;
  };

  struct Material {
    Vec3f colour;
    float diffuse;
    float specular;
  };

  class Sphere {
  private:
    Vec3f m_origin;
    float m_radius;

    Material m_material;

  public:
    Sphere() = default;
    Sphere( const Vec3f& origin, const float radius, const Material& material );
  
    const bool intersects( Ray& ray ) const;

  public:
    void set_origin( const Vec3f& origin ) {
      m_origin = origin;
    }

    const Vec3f& origin() const {
      return m_origin;
    }

    const float radius() const {
      return m_radius;
    }

    const Material& material() const {
      return m_material;
    }
  };

  class Scene {
  private:
    bool m_draw_debug;

    // TODO: Probably best to use smart pointers but the scope of this class wont live beyond the pointers of app or window
    app::Application* m_app;
    app::Window* m_window;

    // Our own custom texture that we write pixel data to.
    ID3D11Texture2D* m_staging;
    ID3D11Texture2D* m_texture;
    ID3D11ShaderResourceView* m_texture_resource;
    
    std::unique_ptr< uint32_t[] > m_pixel_buffer;
    size_t m_width, m_height;

    std::vector< Sphere > m_spheres;

    Vec3f m_light;

  public:
    Scene( app::Application* app, app::Window* window );

    ~Scene();

    void reset();

    void init( const Vec2< size_t >& bounds );

    void update( const double t, const double dt );

    void draw();
  
  private:
    uint32_t main_image( const Vec2f& coord, const Vec2f& uv );

    void update_texture();

    void update_pixel_buffer();
  };

}