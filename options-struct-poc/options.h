#pragma once

#include <array>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

typedef std::array<double, 3> Color;
typedef std::array<double, 3> Vector3;
typedef std::array<double, 3> Point3;

struct Colormap_t {
  std::vector<Color> colors;
  std::string name;

  bool operator==(const struct Colormap_t &other) const {
    return this->colors == other.colors;
  }
  bool operator!=(const struct Colormap_t &other) const {
    return this->colors != other.colors;
  }
};
typedef struct Colormap_t Colormap;

namespace options_ns {

struct app_options {
  bool watch = false;
};

struct f3d_options {
  /** options related to how the scene is being displayed
   * blah blah blah
   */
  struct scene_t {
    struct animation {
      /** Select the animation to load.
       * Any negative value means all animations (glTF only).
       * The default scene always has at most one animation.
       * @load
       */
      int index = 0;

      /** Set the animation speed factor to slow, speed up or even invert
       * animation.
       * @render
       */
      double speed_factor = 1;

      /** Set the animation frame rate used to play the animation interactively.
       *
       * @render
       */
      double frame_rate = 60;

    } animation;

    struct camera {
      /** Select the scene camera to use when available in the file.
       * Any negative value means automatic camera.
       * The default scene always uses automatic camera.
       * @load
       */
      int index = -1;

    } camera;

    /** Define the Up direction
     * @load
     */
    Vector3 up_direction = {0., +1., 0.};

  } scene;

  /** Initial camera parameters*/
  struct camera {
    std::optional<Point3> focal_point;

    std::optional<Point3> position;

    Vector3 direction = {-1., -1., -1.};

    std::optional<Vector3> view_up;

    /** camera view angle in degrees.
     * a strictly positive value.
     */
    double view_angle = 60;

    /** zoom factor relative to the autozoom on data.
     *  a strictly positive value.
     */
    double zoom_factor = 0.9;

    /** additional azimuth angle.
     * Apply an azimuth transformation to the camera, in degrees, added after
     * other camera options.
     */
    std::optional<double> azimuth_angle;

    /** additional elevation angle.
     * Apply an elevation transformation to the camera, in degrees, added after
     * other camera options.
     */
    std::optional<double> elevation_angle;
  } camera;

  /** requires an interactor to be present to have any effect. */
  struct interactor {
    /** Show *axes* as a trihedron in the scene.
     * @render
     */
    bool axis = false;

    /** Enable trackball interaction.
     * @render
     */
    bool trackball = false;

  } interactor;

  /** options related to modifications on the model.
   * Only meaningful when using the default scene.
   */
  struct {
    struct matcap {
      /** Path to a texture file containing a material capture. All other model
       * options for surfaces are ignored if this is set.
       * @render
       */
      std::string texture;

    } matcap;

    struct color {
      /** Set *opacity* on the geometry. Usually used with Depth Peeling option.
       * Multiplied with the `model.color.texture` when present.
       * @render
       */
      double opacity = 1.0;

      /** Set a *color* on the geometry. Multiplied with the
       * `model.color.texture` when present.
       * @render
       */
      Color rgb = {1.0, 1.0, 1.0};

      /** Path to a texture file that sets the color of the object. Will be
       * multiplied with rgb and opacity.
       * @render
       */
      std::string texture;

    } color;

    struct emissive {
      /**  Multiply the emissive color when an emissive texture is present.
       * @render
       */
      Vector3 factor = {1.0, 1.0, 1.0};

      /** Path to a texture file that sets the emitted light of the object.
       * Multiplied with the `model.emissive.factor`.
       * @render
       */
      std::string texture;

    } emissive;

    struct material {
      /** Set the *metallic coefficient* on the geometry.
       * Value should be between `0.0` and `1.0`.
       * Multiplied with the `model.material.texture` when present.
       * @render
       */
      double metallic = 0.0;

      /** Set the *roughness coefficient* on the geometry (0.0-1.0). Multiplied
       * with the `model.material.texture` when present.
       * @render
       */
      double roughness = 0.3;

      /** Path to a texture file that sets the Occlusion, Roughness and Metallic
       * values of the object. Multiplied with the `model.material.roughness`
       * and `model.material.metallic`, set both of them to 1.0 to get a true
       * result.
       * @render
       */
      std::string texture;

    } material;

    struct normal {
      /** Normal scale affects the strength of the normal deviation from the
       * normal texture.
       * @render
       */
      double scale = 1.0;

      /** Path to a texture file that sets the normal map of the object.
       * @render
       */
      std::string texture;

    } normal;

    struct scivis {
      /** Color the data with value found *on the cells* instead of points
       * @render
       */
      bool cells = false;

      /** Set a *custom colormap for the coloring*.
       * This is a list of colors in the format
       * `val1,red1,green1,blue1,...,valN,redN,greenN,blueN` where all
       values
       * are in the range (0,1).
       * @render
       */
      Colormap colormap = {
          .colors = {Color({0., 0., 0.}), Color({1., 1., 1.})},
          .name = "default",
      };

      /** Specify the component to color with. -1 means *magnitude*. -2 means
       * *direct values*.
       * @render
       */
      int component = -1;

      // TODO optional<std::string>?
      //  /** *Color by a specific data array* present in on the data. Set to
      //   * <empty> to let libf3d find the first available array.
      //   * @render
      //      //   */
      //  std::string array_name = "<reserved>";

      // TODO pair<double, double>?
      //  /** Set a *custom range for the coloring*.
      //   * @render
      //      //   */
      //  vector<double> range;

    } scivis;

    struct point_sprites {
      /** Show sphere *points sprites* instead of the geometry.
       * @render
       */
      bool enable = false;

    } point_sprites;

    struct volume {
      /** Enable *volume rendering*. It is only available for 3D image data
       * (vti, dcm, nrrd, mhd files) and will display nothing with other default
       * scene formats.
       * @render
       */
      bool enable = false;

      /** Inverse the linear opacity function.
       * @render
       */
      bool inverse = false;

    } volume;

  } model;

  /** options related to the way the render is done */
  struct render {

    /** options related to specific techniques used that modify the render */
    struct effect {
      /** Enable *translucency support*. This is a technique used to correctly
       * render translucent objects, implemented using depth peeling
       * @render
       */
      bool translucency_support = false;

      /** Enable *anti-aliasing*. This technique is used to reduce aliasing,
       * implemented using FXAA.
       * @render
       */
      bool anti_aliasing = false;

      /** Enable *ambient occlusion*. This is a technique providing approximate
       * shadows, used to improve the depth perception of the object.
       * Implemented using SSAO
       * @render
       */
      bool ambient_occlusion = false;

      /** Enable generic filmic *Tone Mapping Pass*.
       * This technique is used to map colors properly to the monitor colors.
       * @render
       */
      bool tone_mapping = false;

    } effect;

    /** Set the *width* of lines when showing edges.
     * @render
     */
    double line_width = 1.0;

    /** Show the *cell edges*
     * @render
     */
    bool show_edges = false;

    /** Set the *size* of points when showing vertices and point sprites.
     * @render
     */
    double point_size = 10.0;

    struct grid {
      /** Show *a grid* aligned with the horizontal (orthogonal to the Up
       * direction) plane.
       * @render
       */
      bool enable = false;

      /** Position the grid at the *absolute origin* of the model's coordinate
       * system instead of below the model.
       * @render
       */
      bool absolute = false;

      /** Set the size of the *unit square* for the grid. If set to non-positive
       * (the default) a suitable value will be automatically computed.
       * @render
       */
      std::optional<double> unit;

      /** Set the number of subdivisions for the grid.
       * @render
       */
      int subdivisions = 10;

    } grid;

    struct raytracing {
      /** Enable *raytracing*. Requires the raytracing module to be enabled.
       * @render
       */
      bool enable = false;

      /** The number of *samples per pixel*.
       * @render
       */
      int samples = 5;

      /** *Denoise* the raytracing rendering.
       * @render
       */
      bool denoise = false;

    } raytracing;

    struct background {
      /** Set the window *background color*.
       * Ignored if *hdri* is set.
       * @render
       */
      Color color = {0.2, 0.2, 0.2};

      /** Set the *HDRI* image used to create the environment.
       * The environment act as a light source and is reflected on the material.
       * Valid file format are hdr, exr, png, jpg, pnm, tiff, bmp. Override the
       * color.
       * @render
       */
      std::string hdri;

      struct blur {
        /** Blur background when using a HDRI.
         * @render
         */
        bool enable = false;

        /** Blur background circle of confusion radius.
         * @render
         */
        double coc = 20.0;

      } blur;

    } background;

  } render;

  /** options related to the screenspace UI element displayed */
  struct ui {
    /** Show *scalar bar* of the coloring by data array.
     * @render
     */
    bool bar = false;

    /** Display the *filename info content* on top of the window.
     * @render
     */
    bool filename = false;

    /** Use the provided FreeType compatible font file to display text.
     * Can be useful to display non-ASCII filenames.
     * @render
     */
    std::optional<std::string> font_file;

    /** Display a *frame per second counter*.
     * @render
     */
    bool fps = false;

    /** Show a *progress bar* when loading the file.
     * @load
     */
    bool loader_progress = false;

    /** Display the *metadata*.
     * @render
     */
    bool metadata = false;

  } ui;
};

} // namespace options_ns