#include "options-structio.h"



////////////////////////////////////////////////////////////////////////////////
namespace options_ns::app_options_io {

const std::array<K, 1> sorted_keys = {
  keys.watch, // 0 "watch"
};

inline size_t key_index(const K& key) {
  const auto lower = std::lower_bound(sorted_keys.begin(), sorted_keys.end(), key);
  if (lower == sorted_keys.end() || *lower != key)
    throw invalid_key(key);
  else
    return std::distance(sorted_keys.begin(), lower);
}

std::optional<V> get(const S& s, const K& key) {
  switch(key_index(key)){
    case 0: // "watch"
      return s.watch;
    default: throw std::invalid_argument(key); // unreachable
  }
}

void set(S& s, const K& key, const V& value) {
  switch(key_index(key)){
    case 0: // "watch"
      s.watch = std::get<bool>(value); break;
    default: throw std::invalid_argument(key); // unreachable
  }
}

void unset(S& s, const K& key) {
  switch(key_index(key)){
    default: throw non_optional_key(key);
  }
}

Diff diff(const S& current, const S& previous) {
  Diff d;
  if(current.watch != previous.watch) d[keys.watch] = current.watch;
  return d;
}

void apply(S& s, const Diff& diff) {
  for (const auto item : diff)
    if (item.second.has_value())
      set(s, item.first, item.second.value());
    else
      unset(s, item.first);
}

std::string type(const K& key) {
  switch(key_index(key)){
    case 0: // "watch"
      return "bool";
    default: throw std::invalid_argument(key); // unreachable
  }
}

V from_string(const K& key, const std::string& value) {
  switch(key_index(key)){
    case 0: // "watch"
      return options_ns::parse_bool(value);
    default: throw std::invalid_argument(key); // unreachable
  }
}

V from_json(const K& key, const json& value) {
  switch(key_index(key)){
    case 0: // "watch"
      return options_ns::json_to_bool(value);
    default: throw std::invalid_argument(key); // unreachable
  }
}

std::string to_string(const K& key, const V& value) {
  switch(key_index(key)){
    case 0: // "watch"
      return options_ns::format_bool(std::get<bool>(value));
    default: throw std::invalid_argument(key); // unreachable
  }
}

} // namespace options_ns::app_options_io


////////////////////////////////////////////////////////////////////////////////
namespace options_ns::f3d_options_io {

const std::array<K, 56> sorted_keys = {
  keys.camera.azimuth_angle, // 0 "camera.azimuth_angle"
  keys.camera.direction, // 1 "camera.direction"
  keys.camera.elevation_angle, // 2 "camera.elevation_angle"
  keys.camera.focal_point, // 3 "camera.focal_point"
  keys.camera.position, // 4 "camera.position"
  keys.camera.view_angle, // 5 "camera.view_angle"
  keys.camera.view_up, // 6 "camera.view_up"
  keys.camera.zoom_factor, // 7 "camera.zoom_factor"
  keys.interactor.axis, // 8 "interactor.axis"
  keys.interactor.trackball, // 9 "interactor.trackball"
  keys.model.color.opacity, // 10 "model.color.opacity"
  keys.model.color.rgb, // 11 "model.color.rgb"
  keys.model.color.texture, // 12 "model.color.texture"
  keys.model.emissive.factor, // 13 "model.emissive.factor"
  keys.model.emissive.texture, // 14 "model.emissive.texture"
  keys.model.matcap.texture, // 15 "model.matcap.texture"
  keys.model.material.metallic, // 16 "model.material.metallic"
  keys.model.material.roughness, // 17 "model.material.roughness"
  keys.model.material.texture, // 18 "model.material.texture"
  keys.model.normal.scale, // 19 "model.normal.scale"
  keys.model.normal.texture, // 20 "model.normal.texture"
  keys.model.point_sprites.enable, // 21 "model.point_sprites.enable"
  keys.model.scivis.cells, // 22 "model.scivis.cells"
  keys.model.scivis.colormap, // 23 "model.scivis.colormap"
  keys.model.scivis.component, // 24 "model.scivis.component"
  keys.model.volume.enable, // 25 "model.volume.enable"
  keys.model.volume.inverse, // 26 "model.volume.inverse"
  keys.render.background.blur.coc, // 27 "render.background.blur.coc"
  keys.render.background.blur.enable, // 28 "render.background.blur.enable"
  keys.render.background.color, // 29 "render.background.color"
  keys.render.background.hdri, // 30 "render.background.hdri"
  keys.render.effect.ambient_occlusion, // 31 "render.effect.ambient_occlusion"
  keys.render.effect.anti_aliasing, // 32 "render.effect.anti_aliasing"
  keys.render.effect.tone_mapping, // 33 "render.effect.tone_mapping"
  keys.render.effect.translucency_support, // 34 "render.effect.translucency_support"
  keys.render.grid.absolute, // 35 "render.grid.absolute"
  keys.render.grid.enable, // 36 "render.grid.enable"
  keys.render.grid.subdivisions, // 37 "render.grid.subdivisions"
  keys.render.grid.unit, // 38 "render.grid.unit"
  keys.render.line_width, // 39 "render.line_width"
  keys.render.point_size, // 40 "render.point_size"
  keys.render.raytracing.denoise, // 41 "render.raytracing.denoise"
  keys.render.raytracing.enable, // 42 "render.raytracing.enable"
  keys.render.raytracing.samples, // 43 "render.raytracing.samples"
  keys.render.show_edges, // 44 "render.show_edges"
  keys.scene.animation.frame_rate, // 45 "scene.animation.frame_rate"
  keys.scene.animation.index, // 46 "scene.animation.index"
  keys.scene.animation.speed_factor, // 47 "scene.animation.speed_factor"
  keys.scene.camera.index, // 48 "scene.camera.index"
  keys.scene.up_direction, // 49 "scene.up_direction"
  keys.ui.bar, // 50 "ui.bar"
  keys.ui.filename, // 51 "ui.filename"
  keys.ui.font_file, // 52 "ui.font_file"
  keys.ui.fps, // 53 "ui.fps"
  keys.ui.loader_progress, // 54 "ui.loader_progress"
  keys.ui.metadata, // 55 "ui.metadata"
};

inline size_t key_index(const K& key) {
  const auto lower = std::lower_bound(sorted_keys.begin(), sorted_keys.end(), key);
  if (lower == sorted_keys.end() || *lower != key)
    throw invalid_key(key);
  else
    return std::distance(sorted_keys.begin(), lower);
}

std::optional<V> get(const S& s, const K& key) {
  switch(key_index(key)){
    case 0: // "camera.azimuth_angle"
      return s.camera.azimuth_angle;
    case 1: // "camera.direction"
      return s.camera.direction;
    case 2: // "camera.elevation_angle"
      return s.camera.elevation_angle;
    case 3: // "camera.focal_point"
      return s.camera.focal_point;
    case 4: // "camera.position"
      return s.camera.position;
    case 5: // "camera.view_angle"
      return s.camera.view_angle;
    case 6: // "camera.view_up"
      return s.camera.view_up;
    case 7: // "camera.zoom_factor"
      return s.camera.zoom_factor;
    case 8: // "interactor.axis"
      return s.interactor.axis;
    case 9: // "interactor.trackball"
      return s.interactor.trackball;
    case 10: // "model.color.opacity"
      return s.model.color.opacity;
    case 11: // "model.color.rgb"
      return s.model.color.rgb;
    case 12: // "model.color.texture"
      return s.model.color.texture;
    case 13: // "model.emissive.factor"
      return s.model.emissive.factor;
    case 14: // "model.emissive.texture"
      return s.model.emissive.texture;
    case 15: // "model.matcap.texture"
      return s.model.matcap.texture;
    case 16: // "model.material.metallic"
      return s.model.material.metallic;
    case 17: // "model.material.roughness"
      return s.model.material.roughness;
    case 18: // "model.material.texture"
      return s.model.material.texture;
    case 19: // "model.normal.scale"
      return s.model.normal.scale;
    case 20: // "model.normal.texture"
      return s.model.normal.texture;
    case 21: // "model.point_sprites.enable"
      return s.model.point_sprites.enable;
    case 22: // "model.scivis.cells"
      return s.model.scivis.cells;
    case 23: // "model.scivis.colormap"
      return s.model.scivis.colormap;
    case 24: // "model.scivis.component"
      return s.model.scivis.component;
    case 25: // "model.volume.enable"
      return s.model.volume.enable;
    case 26: // "model.volume.inverse"
      return s.model.volume.inverse;
    case 27: // "render.background.blur.coc"
      return s.render.background.blur.coc;
    case 28: // "render.background.blur.enable"
      return s.render.background.blur.enable;
    case 29: // "render.background.color"
      return s.render.background.color;
    case 30: // "render.background.hdri"
      return s.render.background.hdri;
    case 31: // "render.effect.ambient_occlusion"
      return s.render.effect.ambient_occlusion;
    case 32: // "render.effect.anti_aliasing"
      return s.render.effect.anti_aliasing;
    case 33: // "render.effect.tone_mapping"
      return s.render.effect.tone_mapping;
    case 34: // "render.effect.translucency_support"
      return s.render.effect.translucency_support;
    case 35: // "render.grid.absolute"
      return s.render.grid.absolute;
    case 36: // "render.grid.enable"
      return s.render.grid.enable;
    case 37: // "render.grid.subdivisions"
      return s.render.grid.subdivisions;
    case 38: // "render.grid.unit"
      return s.render.grid.unit;
    case 39: // "render.line_width"
      return s.render.line_width;
    case 40: // "render.point_size"
      return s.render.point_size;
    case 41: // "render.raytracing.denoise"
      return s.render.raytracing.denoise;
    case 42: // "render.raytracing.enable"
      return s.render.raytracing.enable;
    case 43: // "render.raytracing.samples"
      return s.render.raytracing.samples;
    case 44: // "render.show_edges"
      return s.render.show_edges;
    case 45: // "scene.animation.frame_rate"
      return s.scene.animation.frame_rate;
    case 46: // "scene.animation.index"
      return s.scene.animation.index;
    case 47: // "scene.animation.speed_factor"
      return s.scene.animation.speed_factor;
    case 48: // "scene.camera.index"
      return s.scene.camera.index;
    case 49: // "scene.up_direction"
      return s.scene.up_direction;
    case 50: // "ui.bar"
      return s.ui.bar;
    case 51: // "ui.filename"
      return s.ui.filename;
    case 52: // "ui.font_file"
      return s.ui.font_file;
    case 53: // "ui.fps"
      return s.ui.fps;
    case 54: // "ui.loader_progress"
      return s.ui.loader_progress;
    case 55: // "ui.metadata"
      return s.ui.metadata;
    default: throw std::invalid_argument(key); // unreachable
  }
}

void set(S& s, const K& key, const V& value) {
  switch(key_index(key)){
    case 0: // "camera.azimuth_angle"
      s.camera.azimuth_angle = std::get<double>(value); break;
    case 1: // "camera.direction"
      s.camera.direction = std::get<std::array<double, 3>>(value); break;
    case 2: // "camera.elevation_angle"
      s.camera.elevation_angle = std::get<double>(value); break;
    case 3: // "camera.focal_point"
      s.camera.focal_point = std::get<std::array<double, 3>>(value); break;
    case 4: // "camera.position"
      s.camera.position = std::get<std::array<double, 3>>(value); break;
    case 5: // "camera.view_angle"
      s.camera.view_angle = std::get<double>(value); break;
    case 6: // "camera.view_up"
      s.camera.view_up = std::get<std::array<double, 3>>(value); break;
    case 7: // "camera.zoom_factor"
      s.camera.zoom_factor = std::get<double>(value); break;
    case 8: // "interactor.axis"
      s.interactor.axis = std::get<bool>(value); break;
    case 9: // "interactor.trackball"
      s.interactor.trackball = std::get<bool>(value); break;
    case 10: // "model.color.opacity"
      s.model.color.opacity = std::get<double>(value); break;
    case 11: // "model.color.rgb"
      s.model.color.rgb = std::get<std::array<double, 3>>(value); break;
    case 12: // "model.color.texture"
      s.model.color.texture = std::get<std::basic_string<char>>(value); break;
    case 13: // "model.emissive.factor"
      s.model.emissive.factor = std::get<std::array<double, 3>>(value); break;
    case 14: // "model.emissive.texture"
      s.model.emissive.texture = std::get<std::basic_string<char>>(value); break;
    case 15: // "model.matcap.texture"
      s.model.matcap.texture = std::get<std::basic_string<char>>(value); break;
    case 16: // "model.material.metallic"
      s.model.material.metallic = std::get<double>(value); break;
    case 17: // "model.material.roughness"
      s.model.material.roughness = std::get<double>(value); break;
    case 18: // "model.material.texture"
      s.model.material.texture = std::get<std::basic_string<char>>(value); break;
    case 19: // "model.normal.scale"
      s.model.normal.scale = std::get<double>(value); break;
    case 20: // "model.normal.texture"
      s.model.normal.texture = std::get<std::basic_string<char>>(value); break;
    case 21: // "model.point_sprites.enable"
      s.model.point_sprites.enable = std::get<bool>(value); break;
    case 22: // "model.scivis.cells"
      s.model.scivis.cells = std::get<bool>(value); break;
    case 23: // "model.scivis.colormap"
      s.model.scivis.colormap = std::get<Colormap_t>(value); break;
    case 24: // "model.scivis.component"
      s.model.scivis.component = std::get<int>(value); break;
    case 25: // "model.volume.enable"
      s.model.volume.enable = std::get<bool>(value); break;
    case 26: // "model.volume.inverse"
      s.model.volume.inverse = std::get<bool>(value); break;
    case 27: // "render.background.blur.coc"
      s.render.background.blur.coc = std::get<double>(value); break;
    case 28: // "render.background.blur.enable"
      s.render.background.blur.enable = std::get<bool>(value); break;
    case 29: // "render.background.color"
      s.render.background.color = std::get<std::array<double, 3>>(value); break;
    case 30: // "render.background.hdri"
      s.render.background.hdri = std::get<std::basic_string<char>>(value); break;
    case 31: // "render.effect.ambient_occlusion"
      s.render.effect.ambient_occlusion = std::get<bool>(value); break;
    case 32: // "render.effect.anti_aliasing"
      s.render.effect.anti_aliasing = std::get<bool>(value); break;
    case 33: // "render.effect.tone_mapping"
      s.render.effect.tone_mapping = std::get<bool>(value); break;
    case 34: // "render.effect.translucency_support"
      s.render.effect.translucency_support = std::get<bool>(value); break;
    case 35: // "render.grid.absolute"
      s.render.grid.absolute = std::get<bool>(value); break;
    case 36: // "render.grid.enable"
      s.render.grid.enable = std::get<bool>(value); break;
    case 37: // "render.grid.subdivisions"
      s.render.grid.subdivisions = std::get<int>(value); break;
    case 38: // "render.grid.unit"
      s.render.grid.unit = std::get<double>(value); break;
    case 39: // "render.line_width"
      s.render.line_width = std::get<double>(value); break;
    case 40: // "render.point_size"
      s.render.point_size = std::get<double>(value); break;
    case 41: // "render.raytracing.denoise"
      s.render.raytracing.denoise = std::get<bool>(value); break;
    case 42: // "render.raytracing.enable"
      s.render.raytracing.enable = std::get<bool>(value); break;
    case 43: // "render.raytracing.samples"
      s.render.raytracing.samples = std::get<int>(value); break;
    case 44: // "render.show_edges"
      s.render.show_edges = std::get<bool>(value); break;
    case 45: // "scene.animation.frame_rate"
      s.scene.animation.frame_rate = std::get<double>(value); break;
    case 46: // "scene.animation.index"
      s.scene.animation.index = std::get<int>(value); break;
    case 47: // "scene.animation.speed_factor"
      s.scene.animation.speed_factor = std::get<double>(value); break;
    case 48: // "scene.camera.index"
      s.scene.camera.index = std::get<int>(value); break;
    case 49: // "scene.up_direction"
      s.scene.up_direction = std::get<std::array<double, 3>>(value); break;
    case 50: // "ui.bar"
      s.ui.bar = std::get<bool>(value); break;
    case 51: // "ui.filename"
      s.ui.filename = std::get<bool>(value); break;
    case 52: // "ui.font_file"
      s.ui.font_file = std::get<std::basic_string<char>>(value); break;
    case 53: // "ui.fps"
      s.ui.fps = std::get<bool>(value); break;
    case 54: // "ui.loader_progress"
      s.ui.loader_progress = std::get<bool>(value); break;
    case 55: // "ui.metadata"
      s.ui.metadata = std::get<bool>(value); break;
    default: throw std::invalid_argument(key); // unreachable
  }
}

void unset(S& s, const K& key) {
  switch(key_index(key)){
    case 0: // "camera.azimuth_angle"
      s.camera.azimuth_angle = std::nullopt; break;
    case 2: // "camera.elevation_angle"
      s.camera.elevation_angle = std::nullopt; break;
    case 3: // "camera.focal_point"
      s.camera.focal_point = std::nullopt; break;
    case 4: // "camera.position"
      s.camera.position = std::nullopt; break;
    case 6: // "camera.view_up"
      s.camera.view_up = std::nullopt; break;
    case 38: // "render.grid.unit"
      s.render.grid.unit = std::nullopt; break;
    case 52: // "ui.font_file"
      s.ui.font_file = std::nullopt; break;
    default: throw non_optional_key(key);
  }
}

Diff diff(const S& current, const S& previous) {
  Diff d;
  if(current.camera.azimuth_angle != previous.camera.azimuth_angle) d[keys.camera.azimuth_angle] = current.camera.azimuth_angle;
  if(current.camera.direction != previous.camera.direction) d[keys.camera.direction] = current.camera.direction;
  if(current.camera.elevation_angle != previous.camera.elevation_angle) d[keys.camera.elevation_angle] = current.camera.elevation_angle;
  if(current.camera.focal_point != previous.camera.focal_point) d[keys.camera.focal_point] = current.camera.focal_point;
  if(current.camera.position != previous.camera.position) d[keys.camera.position] = current.camera.position;
  if(current.camera.view_angle != previous.camera.view_angle) d[keys.camera.view_angle] = current.camera.view_angle;
  if(current.camera.view_up != previous.camera.view_up) d[keys.camera.view_up] = current.camera.view_up;
  if(current.camera.zoom_factor != previous.camera.zoom_factor) d[keys.camera.zoom_factor] = current.camera.zoom_factor;
  if(current.interactor.axis != previous.interactor.axis) d[keys.interactor.axis] = current.interactor.axis;
  if(current.interactor.trackball != previous.interactor.trackball) d[keys.interactor.trackball] = current.interactor.trackball;
  if(current.model.color.opacity != previous.model.color.opacity) d[keys.model.color.opacity] = current.model.color.opacity;
  if(current.model.color.rgb != previous.model.color.rgb) d[keys.model.color.rgb] = current.model.color.rgb;
  if(current.model.color.texture != previous.model.color.texture) d[keys.model.color.texture] = current.model.color.texture;
  if(current.model.emissive.factor != previous.model.emissive.factor) d[keys.model.emissive.factor] = current.model.emissive.factor;
  if(current.model.emissive.texture != previous.model.emissive.texture) d[keys.model.emissive.texture] = current.model.emissive.texture;
  if(current.model.matcap.texture != previous.model.matcap.texture) d[keys.model.matcap.texture] = current.model.matcap.texture;
  if(current.model.material.metallic != previous.model.material.metallic) d[keys.model.material.metallic] = current.model.material.metallic;
  if(current.model.material.roughness != previous.model.material.roughness) d[keys.model.material.roughness] = current.model.material.roughness;
  if(current.model.material.texture != previous.model.material.texture) d[keys.model.material.texture] = current.model.material.texture;
  if(current.model.normal.scale != previous.model.normal.scale) d[keys.model.normal.scale] = current.model.normal.scale;
  if(current.model.normal.texture != previous.model.normal.texture) d[keys.model.normal.texture] = current.model.normal.texture;
  if(current.model.point_sprites.enable != previous.model.point_sprites.enable) d[keys.model.point_sprites.enable] = current.model.point_sprites.enable;
  if(current.model.scivis.cells != previous.model.scivis.cells) d[keys.model.scivis.cells] = current.model.scivis.cells;
  if(current.model.scivis.colormap != previous.model.scivis.colormap) d[keys.model.scivis.colormap] = current.model.scivis.colormap;
  if(current.model.scivis.component != previous.model.scivis.component) d[keys.model.scivis.component] = current.model.scivis.component;
  if(current.model.volume.enable != previous.model.volume.enable) d[keys.model.volume.enable] = current.model.volume.enable;
  if(current.model.volume.inverse != previous.model.volume.inverse) d[keys.model.volume.inverse] = current.model.volume.inverse;
  if(current.render.background.blur.coc != previous.render.background.blur.coc) d[keys.render.background.blur.coc] = current.render.background.blur.coc;
  if(current.render.background.blur.enable != previous.render.background.blur.enable) d[keys.render.background.blur.enable] = current.render.background.blur.enable;
  if(current.render.background.color != previous.render.background.color) d[keys.render.background.color] = current.render.background.color;
  if(current.render.background.hdri != previous.render.background.hdri) d[keys.render.background.hdri] = current.render.background.hdri;
  if(current.render.effect.ambient_occlusion != previous.render.effect.ambient_occlusion) d[keys.render.effect.ambient_occlusion] = current.render.effect.ambient_occlusion;
  if(current.render.effect.anti_aliasing != previous.render.effect.anti_aliasing) d[keys.render.effect.anti_aliasing] = current.render.effect.anti_aliasing;
  if(current.render.effect.tone_mapping != previous.render.effect.tone_mapping) d[keys.render.effect.tone_mapping] = current.render.effect.tone_mapping;
  if(current.render.effect.translucency_support != previous.render.effect.translucency_support) d[keys.render.effect.translucency_support] = current.render.effect.translucency_support;
  if(current.render.grid.absolute != previous.render.grid.absolute) d[keys.render.grid.absolute] = current.render.grid.absolute;
  if(current.render.grid.enable != previous.render.grid.enable) d[keys.render.grid.enable] = current.render.grid.enable;
  if(current.render.grid.subdivisions != previous.render.grid.subdivisions) d[keys.render.grid.subdivisions] = current.render.grid.subdivisions;
  if(current.render.grid.unit != previous.render.grid.unit) d[keys.render.grid.unit] = current.render.grid.unit;
  if(current.render.line_width != previous.render.line_width) d[keys.render.line_width] = current.render.line_width;
  if(current.render.point_size != previous.render.point_size) d[keys.render.point_size] = current.render.point_size;
  if(current.render.raytracing.denoise != previous.render.raytracing.denoise) d[keys.render.raytracing.denoise] = current.render.raytracing.denoise;
  if(current.render.raytracing.enable != previous.render.raytracing.enable) d[keys.render.raytracing.enable] = current.render.raytracing.enable;
  if(current.render.raytracing.samples != previous.render.raytracing.samples) d[keys.render.raytracing.samples] = current.render.raytracing.samples;
  if(current.render.show_edges != previous.render.show_edges) d[keys.render.show_edges] = current.render.show_edges;
  if(current.scene.animation.frame_rate != previous.scene.animation.frame_rate) d[keys.scene.animation.frame_rate] = current.scene.animation.frame_rate;
  if(current.scene.animation.index != previous.scene.animation.index) d[keys.scene.animation.index] = current.scene.animation.index;
  if(current.scene.animation.speed_factor != previous.scene.animation.speed_factor) d[keys.scene.animation.speed_factor] = current.scene.animation.speed_factor;
  if(current.scene.camera.index != previous.scene.camera.index) d[keys.scene.camera.index] = current.scene.camera.index;
  if(current.scene.up_direction != previous.scene.up_direction) d[keys.scene.up_direction] = current.scene.up_direction;
  if(current.ui.bar != previous.ui.bar) d[keys.ui.bar] = current.ui.bar;
  if(current.ui.filename != previous.ui.filename) d[keys.ui.filename] = current.ui.filename;
  if(current.ui.font_file != previous.ui.font_file) d[keys.ui.font_file] = current.ui.font_file;
  if(current.ui.fps != previous.ui.fps) d[keys.ui.fps] = current.ui.fps;
  if(current.ui.loader_progress != previous.ui.loader_progress) d[keys.ui.loader_progress] = current.ui.loader_progress;
  if(current.ui.metadata != previous.ui.metadata) d[keys.ui.metadata] = current.ui.metadata;
  return d;
}

void apply(S& s, const Diff& diff) {
  for (const auto item : diff)
    if (item.second.has_value())
      set(s, item.first, item.second.value());
    else
      unset(s, item.first);
}

std::string type(const K& key) {
  switch(key_index(key)){
    case 0: // "camera.azimuth_angle"
    case 2: // "camera.elevation_angle"
    case 5: // "camera.view_angle"
    case 7: // "camera.zoom_factor"
    case 10: // "model.color.opacity"
    case 16: // "model.material.metallic"
    case 17: // "model.material.roughness"
    case 19: // "model.normal.scale"
    case 27: // "render.background.blur.coc"
    case 38: // "render.grid.unit"
    case 39: // "render.line_width"
    case 40: // "render.point_size"
    case 45: // "scene.animation.frame_rate"
    case 47: // "scene.animation.speed_factor"
      return "double";
    case 1: // "camera.direction"
    case 6: // "camera.view_up"
    case 13: // "model.emissive.factor"
    case 49: // "scene.up_direction"
      return "Vector3";
    case 3: // "camera.focal_point"
    case 4: // "camera.position"
      return "Point3";
    case 8: // "interactor.axis"
    case 9: // "interactor.trackball"
    case 21: // "model.point_sprites.enable"
    case 22: // "model.scivis.cells"
    case 25: // "model.volume.enable"
    case 26: // "model.volume.inverse"
    case 28: // "render.background.blur.enable"
    case 31: // "render.effect.ambient_occlusion"
    case 32: // "render.effect.anti_aliasing"
    case 33: // "render.effect.tone_mapping"
    case 34: // "render.effect.translucency_support"
    case 35: // "render.grid.absolute"
    case 36: // "render.grid.enable"
    case 41: // "render.raytracing.denoise"
    case 42: // "render.raytracing.enable"
    case 44: // "render.show_edges"
    case 50: // "ui.bar"
    case 51: // "ui.filename"
    case 53: // "ui.fps"
    case 54: // "ui.loader_progress"
    case 55: // "ui.metadata"
      return "bool";
    case 11: // "model.color.rgb"
    case 29: // "render.background.color"
      return "Color";
    case 12: // "model.color.texture"
    case 14: // "model.emissive.texture"
    case 15: // "model.matcap.texture"
    case 18: // "model.material.texture"
    case 20: // "model.normal.texture"
    case 30: // "render.background.hdri"
    case 52: // "ui.font_file"
      return "std::string";
    case 23: // "model.scivis.colormap"
      return "Colormap";
    case 24: // "model.scivis.component"
    case 37: // "render.grid.subdivisions"
    case 43: // "render.raytracing.samples"
    case 46: // "scene.animation.index"
    case 48: // "scene.camera.index"
      return "int";
    default: throw std::invalid_argument(key); // unreachable
  }
}

V from_string(const K& key, const std::string& value) {
  switch(key_index(key)){
    case 0: // "camera.azimuth_angle"
    case 2: // "camera.elevation_angle"
    case 5: // "camera.view_angle"
    case 7: // "camera.zoom_factor"
    case 10: // "model.color.opacity"
    case 16: // "model.material.metallic"
    case 17: // "model.material.roughness"
    case 19: // "model.normal.scale"
    case 27: // "render.background.blur.coc"
    case 38: // "render.grid.unit"
    case 39: // "render.line_width"
    case 40: // "render.point_size"
    case 45: // "scene.animation.frame_rate"
    case 47: // "scene.animation.speed_factor"
      return options_ns::parse_double(value);
    case 1: // "camera.direction"
    case 6: // "camera.view_up"
    case 13: // "model.emissive.factor"
    case 49: // "scene.up_direction"
      return options_ns::parse_Vector3(value);
    case 3: // "camera.focal_point"
    case 4: // "camera.position"
      return options_ns::parse_Point3(value);
    case 8: // "interactor.axis"
    case 9: // "interactor.trackball"
    case 21: // "model.point_sprites.enable"
    case 22: // "model.scivis.cells"
    case 25: // "model.volume.enable"
    case 26: // "model.volume.inverse"
    case 28: // "render.background.blur.enable"
    case 31: // "render.effect.ambient_occlusion"
    case 32: // "render.effect.anti_aliasing"
    case 33: // "render.effect.tone_mapping"
    case 34: // "render.effect.translucency_support"
    case 35: // "render.grid.absolute"
    case 36: // "render.grid.enable"
    case 41: // "render.raytracing.denoise"
    case 42: // "render.raytracing.enable"
    case 44: // "render.show_edges"
    case 50: // "ui.bar"
    case 51: // "ui.filename"
    case 53: // "ui.fps"
    case 54: // "ui.loader_progress"
    case 55: // "ui.metadata"
      return options_ns::parse_bool(value);
    case 11: // "model.color.rgb"
    case 29: // "render.background.color"
      return options_ns::parse_Color(value);
    case 12: // "model.color.texture"
    case 14: // "model.emissive.texture"
    case 15: // "model.matcap.texture"
    case 18: // "model.material.texture"
    case 20: // "model.normal.texture"
    case 30: // "render.background.hdri"
    case 52: // "ui.font_file"
      return options_ns::parse_std_string(value);
    case 23: // "model.scivis.colormap"
      return options_ns::parse_Colormap(value);
    case 24: // "model.scivis.component"
    case 37: // "render.grid.subdivisions"
    case 43: // "render.raytracing.samples"
    case 46: // "scene.animation.index"
    case 48: // "scene.camera.index"
      return options_ns::parse_int(value);
    default: throw std::invalid_argument(key); // unreachable
  }
}

V from_json(const K& key, const json& value) {
  switch(key_index(key)){
    case 0: // "camera.azimuth_angle"
    case 2: // "camera.elevation_angle"
    case 5: // "camera.view_angle"
    case 7: // "camera.zoom_factor"
    case 10: // "model.color.opacity"
    case 16: // "model.material.metallic"
    case 17: // "model.material.roughness"
    case 19: // "model.normal.scale"
    case 27: // "render.background.blur.coc"
    case 38: // "render.grid.unit"
    case 39: // "render.line_width"
    case 40: // "render.point_size"
    case 45: // "scene.animation.frame_rate"
    case 47: // "scene.animation.speed_factor"
      return options_ns::json_to_double(value);
    case 1: // "camera.direction"
    case 6: // "camera.view_up"
    case 13: // "model.emissive.factor"
    case 49: // "scene.up_direction"
      return options_ns::json_to_Vector3(value);
    case 3: // "camera.focal_point"
    case 4: // "camera.position"
      return options_ns::json_to_Point3(value);
    case 8: // "interactor.axis"
    case 9: // "interactor.trackball"
    case 21: // "model.point_sprites.enable"
    case 22: // "model.scivis.cells"
    case 25: // "model.volume.enable"
    case 26: // "model.volume.inverse"
    case 28: // "render.background.blur.enable"
    case 31: // "render.effect.ambient_occlusion"
    case 32: // "render.effect.anti_aliasing"
    case 33: // "render.effect.tone_mapping"
    case 34: // "render.effect.translucency_support"
    case 35: // "render.grid.absolute"
    case 36: // "render.grid.enable"
    case 41: // "render.raytracing.denoise"
    case 42: // "render.raytracing.enable"
    case 44: // "render.show_edges"
    case 50: // "ui.bar"
    case 51: // "ui.filename"
    case 53: // "ui.fps"
    case 54: // "ui.loader_progress"
    case 55: // "ui.metadata"
      return options_ns::json_to_bool(value);
    case 11: // "model.color.rgb"
    case 29: // "render.background.color"
      return options_ns::json_to_Color(value);
    case 12: // "model.color.texture"
    case 14: // "model.emissive.texture"
    case 15: // "model.matcap.texture"
    case 18: // "model.material.texture"
    case 20: // "model.normal.texture"
    case 30: // "render.background.hdri"
    case 52: // "ui.font_file"
      return options_ns::json_to_std_string(value);
    case 23: // "model.scivis.colormap"
      return options_ns::json_to_Colormap(value);
    case 24: // "model.scivis.component"
    case 37: // "render.grid.subdivisions"
    case 43: // "render.raytracing.samples"
    case 46: // "scene.animation.index"
    case 48: // "scene.camera.index"
      return options_ns::json_to_int(value);
    default: throw std::invalid_argument(key); // unreachable
  }
}

std::string to_string(const K& key, const V& value) {
  switch(key_index(key)){
    case 0: // "camera.azimuth_angle"
    case 2: // "camera.elevation_angle"
    case 5: // "camera.view_angle"
    case 7: // "camera.zoom_factor"
    case 10: // "model.color.opacity"
    case 16: // "model.material.metallic"
    case 17: // "model.material.roughness"
    case 19: // "model.normal.scale"
    case 27: // "render.background.blur.coc"
    case 38: // "render.grid.unit"
    case 39: // "render.line_width"
    case 40: // "render.point_size"
    case 45: // "scene.animation.frame_rate"
    case 47: // "scene.animation.speed_factor"
      return options_ns::format_double(std::get<double>(value));
    case 1: // "camera.direction"
    case 6: // "camera.view_up"
    case 13: // "model.emissive.factor"
    case 49: // "scene.up_direction"
      return options_ns::format_Vector3(std::get<std::array<double, 3>>(value));
    case 3: // "camera.focal_point"
    case 4: // "camera.position"
      return options_ns::format_Point3(std::get<std::array<double, 3>>(value));
    case 8: // "interactor.axis"
    case 9: // "interactor.trackball"
    case 21: // "model.point_sprites.enable"
    case 22: // "model.scivis.cells"
    case 25: // "model.volume.enable"
    case 26: // "model.volume.inverse"
    case 28: // "render.background.blur.enable"
    case 31: // "render.effect.ambient_occlusion"
    case 32: // "render.effect.anti_aliasing"
    case 33: // "render.effect.tone_mapping"
    case 34: // "render.effect.translucency_support"
    case 35: // "render.grid.absolute"
    case 36: // "render.grid.enable"
    case 41: // "render.raytracing.denoise"
    case 42: // "render.raytracing.enable"
    case 44: // "render.show_edges"
    case 50: // "ui.bar"
    case 51: // "ui.filename"
    case 53: // "ui.fps"
    case 54: // "ui.loader_progress"
    case 55: // "ui.metadata"
      return options_ns::format_bool(std::get<bool>(value));
    case 11: // "model.color.rgb"
    case 29: // "render.background.color"
      return options_ns::format_Color(std::get<std::array<double, 3>>(value));
    case 12: // "model.color.texture"
    case 14: // "model.emissive.texture"
    case 15: // "model.matcap.texture"
    case 18: // "model.material.texture"
    case 20: // "model.normal.texture"
    case 30: // "render.background.hdri"
    case 52: // "ui.font_file"
      return options_ns::format_std_string(std::get<std::basic_string<char>>(value));
    case 23: // "model.scivis.colormap"
      return options_ns::format_Colormap(std::get<Colormap_t>(value));
    case 24: // "model.scivis.component"
    case 37: // "render.grid.subdivisions"
    case 43: // "render.raytracing.samples"
    case 46: // "scene.animation.index"
    case 48: // "scene.camera.index"
      return options_ns::format_int(std::get<int>(value));
    default: throw std::invalid_argument(key); // unreachable
  }
}

} // namespace options_ns::f3d_options_io
