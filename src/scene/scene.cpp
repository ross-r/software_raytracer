#include <scene/scene.hpp>

#include <application.hpp>
#include <window.hpp>

#include <ext/imgui/imgui.h>

#include <colour.hpp>

#include <memory>
#include <random>
#include <algorithm>
#include <functional>

template< typename T >
T clamp( const T value, const T min, const T max ) {
  return std::max( std::min( value, max ), min );
}

//
// SPHERE
//

scene::Sphere::Sphere( const Vec3f& origin, const float radius, const Material& material ) :
  m_origin{ origin }, 
  m_radius{ radius },
  m_material{ material } {}

const bool scene::Sphere::intersects( Ray& ray ) const {
  const Vec3f origin = m_origin - ray.origin;

  const float a = ray.direction.dot( ray.direction );
  const float b = 2.F * origin.dot( ray.direction );
  const float c = origin.dot( origin ) - m_radius * m_radius;

  // https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection.html
  // This blog details a possibly better way to solve quadratic formula
  float discriminant = b * b - 4.F * a * c;
  if( discriminant < 0.F ) {
    ray.hit = Vec3f();
    ray.normal = Vec3f();
    return false;
  }

  // t0 will always be < t1 (i.e., closest point)
  float t0 = ( -b - sqrt( discriminant ) ) / ( 2.F * a );
  //float t1 = ( -b + sqrt( discriminant ) ) / ( 2.F * a );

  ray.length = t0;
  ray.hit = origin + ray.direction * ray.length;
  ray.normal = ray.hit.normalized();

  return true;
}

//
// LIGHT
//
scene::Light::Light( const Vec3f& origin, const Vec3f& colour ) :
  m_origin{ origin }, 
  m_colour{ colour } {}

//
// SCENE
// 

scene::Scene::Scene( app::Application* app, app::Window* window ) :
  m_app( app ),
  m_window( window ),
  m_staging{},
  m_texture{},
  m_texture_resource{},
  m_width{},
  m_height{},
  m_light{ Vec3f( 2.F, 2.F, -2.F ), Vec3f( 1.F, 1.F, 1.F ) }
{
  m_draw_debug = true;

  m_spheres.emplace_back( Sphere( Vec3f( -0.55F, 0.F, 0.F ), 0.5F, Material{ Vec3f( 1.F, 0.F, 1.F ), 1.F, 0.001F } ) );
  m_spheres.emplace_back( Sphere( Vec3f( 0.55F, 0.F, 0.F ), 0.25F, Material{ Vec3f( 1.F, 0.F, 0.F ), 1.F, 0.001F } ) );
}

scene::Scene::~Scene() {
  reset();
}

void scene::Scene::reset() {
  if( m_staging ) {
    m_staging->Release();
    m_staging = nullptr;
  }

  if( m_texture ) {
    m_texture->Release();
    m_texture = nullptr;
  }

  if( m_texture_resource ) {
    m_texture_resource->Release();
    m_texture_resource = nullptr;
  }

  if( m_pixel_buffer ) {
    m_pixel_buffer.release();
    m_pixel_buffer = nullptr;
  }
}

void scene::Scene::init( const Vec2< size_t >& bounds ) {
  auto& renderer = m_window->renderer();
  auto device = renderer.device();
  auto context = renderer.context();

  m_width = bounds.x;
  m_height = bounds.y;

  m_pixel_buffer = std::make_unique< uint32_t[] >( m_width * m_height );
  
  HRESULT hr = S_OK;

  {
    D3D11_TEXTURE2D_DESC textureDescription{};
    memset( &textureDescription, 0, sizeof( textureDescription ) );

    textureDescription.Width = m_width;
    textureDescription.Height = m_height;
    textureDescription.ArraySize = 1;
    textureDescription.SampleDesc.Count = 1;
    textureDescription.SampleDesc.Quality = 0;
    textureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDescription.Usage = D3D11_USAGE_STAGING;
    textureDescription.BindFlags = 0;
    textureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    textureDescription.MiscFlags = 0;
    textureDescription.MipLevels = 1;

    if( FAILED( hr = device->CreateTexture2D( &textureDescription, nullptr, &m_staging ) ) ) {
      return;
    }
  }

  {
    D3D11_TEXTURE2D_DESC textureDescription{};
    memset( &textureDescription, 0, sizeof( textureDescription ) );

    textureDescription.Width = m_width;
    textureDescription.Height = m_height;
    textureDescription.ArraySize = 1;
    textureDescription.SampleDesc.Count = 1;
    textureDescription.SampleDesc.Quality = 0;
    textureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDescription.Usage = D3D11_USAGE_DEFAULT;
    textureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDescription.CPUAccessFlags = 0;
    textureDescription.MiscFlags = 0;
    textureDescription.MipLevels = 1;

    if( FAILED( hr = device->CreateTexture2D( &textureDescription, nullptr, &m_texture ) ) ) {
      return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory( &srvDesc, sizeof( srvDesc ) );
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = textureDescription.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    if( FAILED( hr = device->CreateShaderResourceView( m_texture, &srvDesc, &m_texture_resource ) ) ) {
      return;
    }
  }
}

void scene::Scene::update( const double t, const double dt ) {
  m_app->set_time_scale( 1.0 );

  m_spheres[ 0 ].set_origin( { -0.55F, 0.25F * sinf( t ), 0.F } );
}

void scene::Scene::draw() {
  ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

  update_pixel_buffer();
  update_texture();
  
  RECT rect{};
  GetClientRect( m_window->handle(), &rect );

  ImGui::Begin( "Scene", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize ); 
  {
    ImGui::SetWindowPos( { 0.F, 0.F } );
    ImGui::SetWindowSize( { ( float ) ( rect.right - rect.left ), ( float ) ( rect.bottom - rect.top ) } );

    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 0.F, 0.F } );

    ImGui::BeginChild( "__scene", { ( float ) m_width, ( float ) m_height }, true );
    {
      ImGui::Image( m_texture_resource, { ( float ) m_width, ( float ) m_height } );
      ImGui::EndChild();
    }

    ImGui::PopStyleVar();

    ImGui::SameLine();
    
    ImGui::BeginChild( "__configuration", ImVec2( 0, ImGui::GetContentRegionAvail().y ), true);
    {
      ImGui::Text( "FPS: %.2f (%.8f)", m_app->frames_per_second(), m_app->delta_time() );

      //ImGui::SliderFloat( "Sphere X", &m_sphere.x, -1.F, 1.F );
      //ImGui::SliderFloat( "Sphere Y", &m_sphere.y, -1.F, 1.F );
      //ImGui::SliderFloat( "Sphere Z", &m_sphere.z, -1.F, 1.F );
      //ImGui::SliderFloat( "Sphere Radius", &m_sphere_radius, 0.F, 1.F );

      //ImGui::SliderFloat( "Light X", &m_light.x, -10.F, 10.F );
      //ImGui::SliderFloat( "Light Y", &m_light.y, -10.F, 10.F );
      //ImGui::SliderFloat( "Light Z", &m_light.z, -10.F, 10.F );

      ImGui::EndChild();
    }

    ImGui::End();
  }
}

bool scene::Scene::trace( Ray& ray, Intersection* intersection ) {
  *intersection = Intersection{};
  intersection->hit = false;

  for( const auto& sphere : m_spheres ) {
    if( !sphere.intersects( ray ) ) {
      continue;
    }

    intersection->origin = sphere.origin();
    intersection->material = sphere.material();
    intersection->hit = true;

    break;
  }

  return intersection->hit;
}

//
// Main image routine, this is the same as a pixel shader on the GPU.
//
// https://www.shadertoy.com/view/4ljGRd
uint32_t scene::Scene::main_image( const Vec2f& coord, const Vec2f& uv ) {
  // TODO: [DEBUG BUILD]
  //          Constructing this on the stack seems to drop performance, possibly due to copy operators, no inlining, or whatever else it may be.
  // Colour(...).argb()

  Vec3f colour( 0.33F, 0.33F, 0.33F );
  Vec3f mask = Vec3f( 1.F, 1.F, 1.F );

  Ray ray{
    Vec3f( 0.F, 0.F, -2.F ),
    Vec3f( uv.x, uv.y, -1.F )
  };

  Intersection intersection;
  if( trace( ray, &intersection ) ) {
    const Material& material = intersection.material;

    // http://en.wikipedia.org/wiki/Schlick's_approximation
    const Vec3f r0 = material.colour * material.specular;
    const float hv = clamp( ray.normal.dot( ray.direction * -1.F ), 0.F, 1.F );
    const Vec3f fresnel = r0 + ( r0 * -1.F ) * powf( 1.F - hv, 5.F );
    mask = mask * fresnel;
    
    const float intensity = std::max( ray.normal.dot( m_light.origin() * -1.F ), 0.F );

    colour.x =
      clamp( colour.x * intensity, 0.F, 1.F )
      * material.colour.x
      * m_light.colour().x
      * material.diffuse
      * ( ( 1.F - fresnel.x ) * mask.x / fresnel.x );

    colour.y =
      clamp( colour.y * intensity, 0.F, 1.F )
      * material.colour.y
      * m_light.colour().y
      * material.diffuse
      * ( ( 1.F - fresnel.y ) * mask.y / fresnel.y );

    colour.z =
      clamp( colour.z * intensity, 0.F, 1.F )
      * material.colour.z
      * m_light.colour().z
      * material.diffuse
      * ( ( 1.F - fresnel.z ) * mask.z / fresnel.z );

    // TODO: reflect( ray, material )
  }
 
  Colour fragColour{
    ( uint8_t ) ( clamp( colour.x, 0.F, 1.F ) * 255.F ),
    ( uint8_t ) ( clamp( colour.y, 0.F, 1.F ) * 255.F ),
    ( uint8_t ) ( clamp( colour.z, 0.F, 1.F ) * 255.F ),
    255
  };

  return fragColour.argb();
}

void scene::Scene::update_texture() {
  //
  // Copies the pixel buffer to the staging textures buffer.
  //

  auto& renderer = m_window->renderer();
  auto device = renderer.device();
  auto context = renderer.context();

  if( m_staging == nullptr || m_texture == nullptr ) {
    return;
  }

  D3D11_MAPPED_SUBRESOURCE subresource;
  if( FAILED( context->Map(
    m_staging,
    0,
    D3D11_MAP_WRITE,
    0,
    &subresource
  ) ) ) {
    return;
  }

  if( subresource.pData == nullptr ) {
    return;
  }

  memcpy(
    subresource.pData,
    m_pixel_buffer.get(),
    sizeof( uint32_t ) * m_width * m_height
  );

  context->Unmap( m_staging, 0 );

  //
  // Copy the staging texture to the texture that has a shader resource bound to it.
  //
  context->CopyResource( m_texture, m_staging );
}

void scene::Scene::update_pixel_buffer() {
  const float aspect_ratio = ( float ) m_width / m_height;
  
  for( size_t y{}; y < m_height; ++y ) {
    for( size_t x{}; x < m_width; ++x ) {

      // fragCoord
      const Vec2f coord( ( float ) x / m_width, ( float ) y / m_height );

      // uv
      const Vec2f uv{
        ( coord.x - 0.5F ) * aspect_ratio,  // Fixes any distortion
        coord.y - 0.5F
      };

      // fragColor
      const uint32_t colour = main_image( coord, uv );

      m_pixel_buffer[ x + m_width * y ] = colour;
    }
  }
}