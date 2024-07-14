#pragma once

#include <array>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>

#include "options.h"
#include "options-io.h"


////////////////////////////////////////////////////////////////////////////////
namespace options_ns::app_options_io {

typedef ::options_ns::app_options S; // Struct
typedef std::string K; // Key
typedef std::variant<
  bool
> V; // Value
typedef std::map<K, std::optional<V>> Diff;

const struct keys {
  const K watch = "watch";
} keys;

/** Get a value by key.
Throws `invalid_key` exception on unknown key. */
std::optional<V> get(const S& s, const K& key);

/** Set a value by key.
Throws `invalid_key` exception on unknown key. */
void set(S& s, const K& key, const V& value);

/** Unset an optional value by key.
Throws `invalid_key` exception on unknown key.
Throws `non_optional_key` exception on non-optional key. */
void unset(S& s, const K& key);

/** Construct a `key->variant` map of differences between two instances. */
Diff diff(const S& current, const S& previous);

/** apply a diff (`key->variant` map) to an instance.
Throws `invalid_key` exception on unknown key.
Throws `non_optional_key` exception on non-optional key. */
void apply(S& s, const Diff& diff);

/** Retrieve the type name of a value by key.
Possible return values are: `"bool"`.
Throws `invalid_key` exception on unknown key. */
std::string type(const K& key);

/** Parse a value for a given key from `std::string`.
Throws `invalid_key` exception on unknown key. */
V from_string(const K& key, const std::string& value);

/** Parse a value for a given key from `json`.
Throws `invalid_key` exception on unknown key. */
V from_json(const K& key, const json& value);

/** Format a value for a given key to `std::string`.
Throws `invalid_key` exception on unknown key. */
std::string to_string(const K& key, const V& value);

class invalid_key : public std::out_of_range {
public:
  explicit invalid_key(const K& key):
    std::out_of_range("invalid key: " + key) {}
};

class non_optional_key : public std::out_of_range {
public:
  explicit non_optional_key(const K& key):
    std::out_of_range("non-optional key: " + key) {}
};


} // namespace options_ns::app_options_io


////////////////////////////////////////////////////////////////////////////////
namespace options_ns::f3d_options_io {

typedef ::options_ns::f3d_options S; // Struct
typedef std::string K; // Key
typedef std::variant<
  Colormap_t /* Colormap */,
  bool,
  double,
  int,
  std::array<double, 3> /* Color, Point3, Vector3 */,
  std::basic_string<char> /* std::string */
> V; // Value
typedef std::map<K, std::optional<V>> Diff;

const struct keys {
  const struct camera {
    const K azimuth_angle = "camera.azimuth_angle";
    const K direction = "camera.direction";
    const K elevation_angle = "camera.elevation_angle";
    const K focal_point = "camera.focal_point";
    const K position = "camera.position";
    const K view_angle = "camera.view_angle";
    const K view_up = "camera.view_up";
    const K zoom_factor = "camera.zoom_factor";
  } camera;
  const struct interactor {
    const K axis = "interactor.axis";
    const K trackball = "interactor.trackball";
  } interactor;
  const struct model {
    const struct color {
      const K opacity = "model.color.opacity";
      const K rgb = "model.color.rgb";
      const K texture = "model.color.texture";
    } color;
    const struct emissive {
      const K factor = "model.emissive.factor";
      const K texture = "model.emissive.texture";
    } emissive;
    const struct matcap {
      const K texture = "model.matcap.texture";
    } matcap;
    const struct material {
      const K metallic = "model.material.metallic";
      const K roughness = "model.material.roughness";
      const K texture = "model.material.texture";
    } material;
    const struct normal {
      const K scale = "model.normal.scale";
      const K texture = "model.normal.texture";
    } normal;
    const struct point_sprites {
      const K enable = "model.point_sprites.enable";
    } point_sprites;
    const struct scivis {
      const K cells = "model.scivis.cells";
      const K colormap = "model.scivis.colormap";
      const K component = "model.scivis.component";
    } scivis;
    const struct volume {
      const K enable = "model.volume.enable";
      const K inverse = "model.volume.inverse";
    } volume;
  } model;
  const struct render {
    const struct background {
      const struct blur {
        const K coc = "render.background.blur.coc";
        const K enable = "render.background.blur.enable";
      } blur;
      const K color = "render.background.color";
      const K hdri = "render.background.hdri";
    } background;
    const struct effect {
      const K ambient_occlusion = "render.effect.ambient_occlusion";
      const K anti_aliasing = "render.effect.anti_aliasing";
      const K tone_mapping = "render.effect.tone_mapping";
      const K translucency_support = "render.effect.translucency_support";
    } effect;
    const struct grid {
      const K absolute = "render.grid.absolute";
      const K enable = "render.grid.enable";
      const K subdivisions = "render.grid.subdivisions";
      const K unit = "render.grid.unit";
    } grid;
    const K line_width = "render.line_width";
    const K point_size = "render.point_size";
    const struct raytracing {
      const K denoise = "render.raytracing.denoise";
      const K enable = "render.raytracing.enable";
      const K samples = "render.raytracing.samples";
    } raytracing;
    const K show_edges = "render.show_edges";
  } render;
  const struct scene {
    const struct animation {
      const K frame_rate = "scene.animation.frame_rate";
      const K index = "scene.animation.index";
      const K speed_factor = "scene.animation.speed_factor";
    } animation;
    const struct camera {
      const K index = "scene.camera.index";
    } camera;
    const K up_direction = "scene.up_direction";
  } scene;
  const struct ui {
    const K bar = "ui.bar";
    const K filename = "ui.filename";
    const K font_file = "ui.font_file";
    const K fps = "ui.fps";
    const K loader_progress = "ui.loader_progress";
    const K metadata = "ui.metadata";
  } ui;
} keys;

/** Get a value by key.
Throws `invalid_key` exception on unknown key. */
std::optional<V> get(const S& s, const K& key);

/** Set a value by key.
Throws `invalid_key` exception on unknown key. */
void set(S& s, const K& key, const V& value);

/** Unset an optional value by key.
Throws `invalid_key` exception on unknown key.
Throws `non_optional_key` exception on non-optional key. */
void unset(S& s, const K& key);

/** Construct a `key->variant` map of differences between two instances. */
Diff diff(const S& current, const S& previous);

/** apply a diff (`key->variant` map) to an instance.
Throws `invalid_key` exception on unknown key.
Throws `non_optional_key` exception on non-optional key. */
void apply(S& s, const Diff& diff);

/** Retrieve the type name of a value by key.
Possible return values are: `"Color"`, `"Colormap"`, `"Point3"`, `"Vector3"`, `"bool"`, `"double"`, `"int"`, `"std::string"`.
Throws `invalid_key` exception on unknown key. */
std::string type(const K& key);

/** Parse a value for a given key from `std::string`.
Throws `invalid_key` exception on unknown key. */
V from_string(const K& key, const std::string& value);

/** Parse a value for a given key from `json`.
Throws `invalid_key` exception on unknown key. */
V from_json(const K& key, const json& value);

/** Format a value for a given key to `std::string`.
Throws `invalid_key` exception on unknown key. */
std::string to_string(const K& key, const V& value);

class invalid_key : public std::out_of_range {
public:
  explicit invalid_key(const K& key):
    std::out_of_range("invalid key: " + key) {}
};

class non_optional_key : public std::out_of_range {
public:
  explicit non_optional_key(const K& key):
    std::out_of_range("non-optional key: " + key) {}
};


} // namespace options_ns::f3d_options_io
