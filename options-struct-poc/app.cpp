
#include <iostream>

#include <cxxopts.hpp>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using namespace nlohmann::literals;

#include "options.h"
#include "options-structio.h" // generated

typedef struct options_ns::f3d_options Options;
typedef options_ns::generated::Diff OptionsDiff;
namespace OptionsIO = options_ns::generated;
auto OptionsKeys = options_ns::generated::keys;

const auto RESET = "\033[0m";
const auto BOLD = "\033[1m";
const auto DIM = "\033[2m";

void print_diff(const OptionsDiff &diff) {
  for (const auto [key, val] : diff) {
    if (val.has_value())
      std::cout << "+ " << key << " = "
                << OptionsIO::to_string(key, val.value()) << std::endl;
    else
      std::cout << "- " << key << std::endl;
  }
  std::cout << std::endl;
}

void print_diff(const OptionsDiff &diff, const Options &previous) {
  for (const auto [key, newVal] : diff) {
    const auto oldVal = OptionsIO::get(previous, key);

    if (oldVal == newVal)
      continue;

    if (!newVal.has_value())
      std::cout << "- " << BOLD << key << RESET << std::endl;
    else {
      if (oldVal.has_value())
        std::cout << DIM << "- " << key << " = " << BOLD
                  << OptionsIO::to_string(key, oldVal.value()) << RESET
                  << std::endl;
      std::cout << "+ " << key << " = " << BOLD
                << OptionsIO::to_string(key, newVal.value()) << RESET
                << std::endl;
    }
  }
  std::cout << std::endl;
}

class CliOptionsHelper {
private:
  const Options &defaults;
  std::unordered_map<std::string, std::string> arg_to_key;

public:
  CliOptionsHelper(const Options &defaults) : defaults(defaults) {}

  void add(cxxopts::OptionAdder &group, const std::string &key,
           const std::pair<std::string, std::string> &names, std::string help) {
    const auto type = OptionsIO::type(key);
    const auto defautVal = OptionsIO::get(defaults, key);

    /* we don't let cxxopts do any type parsing.
      we're only using it to handle the argumets as strings
      and we'll do our own parsing after retrieiving cxxopts's result */
    auto arg = cxxopts::value<std::string>();

    if (defautVal.has_value())
      /* we don't use cxxopts default values because we don't care about
        retrieving values that have not been explicitly passed.
        but be want to display the defaults to the user,
        so we append to the help text instead */
      help +=
          " (default: " + OptionsIO::to_string(key, defautVal.value()) + ")";

    if (type == "bool")
      arg->implicit_value("true");

    group(names.first.empty() ? names.second : names.first + "," + names.second,
          help, arg, type);

    /* keep track of which key the arg is for so we can parse the type later */
    arg_to_key[names.second] = key;
  }

  OptionsDiff retrieve_diff(const cxxopts::ParseResult &result,
                            const std::string &unset_str,
                            const std::string &default_str) {
    OptionsDiff diff;
    /* cxxopts result may contain other arguments,
     we loop through ours to map arg names to original keys and parse values
    */
    for (const auto [arg, key] : arg_to_key) {
      if (result.count(arg)) {
        const std::string str = result[arg].as<std::string>();
        if (str == unset_str)
          diff[key] = std::nullopt;
        else if (str == default_str)
          diff[key] = OptionsIO::get(defaults, key);
        else
          diff[key] = OptionsIO::from_string(key, str);
      }
    }
    return diff;
  }
};

void collect_json_by_key(const json &o, const std::string key_sep,
                         std::map<std::string, json> &result,
                         const std::string &key = "") {
  if (o.is_object())
    for (auto [k, v] : o.items())
      collect_json_by_key(v, key_sep, result,
                          key.empty() ? k : (key + key_sep + k));
  else
    result[key] = o;
}

std::map<std::string, json>
collect_json_by_key(const json &o, const std::string key_sep = ".") {
  std::map<std::string, json> result;
  collect_json_by_key(o, key_sep, result);
  return result;
}

/* with the helper we just need to maps options keys to CLI flags and help
  text. Defaults, metavars, implicit values will be infered */
// clang-format off
const std::map<std::string, std::vector<std::tuple<std::string, std::pair<std::string,std::string>, std::string>>> options_cli = {
  {"Render", {
    {OptionsKeys.render.background.color, {"b", "background-color"}, "background color"},
    {OptionsKeys.render.grid.enable, {"g", "grid"}, "enable grid"},
    {OptionsKeys.render.grid.unit, {"", "grid-unit"}, "grid unit square"},
    // ...
  }},
  {"Scene", {
    {OptionsKeys.scene.up_direction, {"", "up"}, "environment up directions"},
    // ...
  }},
  {"Camera", {
    {OptionsKeys.camera.focal_point, {"", "camera-focus"}, "camera focal point"},
    {OptionsKeys.camera.position, {"", "camera-position"}, "camera position"},
    {OptionsKeys.camera.direction, {"", "camera-direction"}, "camera direction"},
    {OptionsKeys.camera.zoom_factor, {"", "camera-zoom-factor"}, "camera zoom factor"},
    // ...
  }},
  {"Effects", {
    {OptionsKeys.render.effect.anti_aliasing, {"", "anti-aliasing"}, "anti aliasing"},
    {OptionsKeys.render.effect.ambient_occlusion, {"", "ambient-occlusion"}, "ambient occlusion"},
    {OptionsKeys.render.effect.tone_mapping, {"", "tone-mapping"}, "tone mapping"},
    {OptionsKeys.render.effect.translucency_support, {"", "translucency"}, "translucency support"},
    // ...
  }},
  {"Scivis", {
    {OptionsKeys.model.scivis.colormap, {"", "colormap"}, "colormap"},
    // ...
  }},
  {"UI", {
    {OptionsKeys.ui.font_file, {"", "font"}, "interface font file"},
    // ...
  }},
  // ...
};
// clang-format on

std::string quote_for_cli(const std::string &str) {
  // TODO add quotes if has spaces, escape quotes and whatnot
  return str;
}
std::string diff_to_cli(const OptionsDiff &diff) {
  std::stringstream ss;
  for (const auto [_group_name, options] : options_cli) {
    for (const auto [key, names, _help] : options) {
      if (diff.count(key)) {
        if (!names.first.empty())
          ss << "-" << names.first;
        else
          ss << "--" << names.second;

        const auto v = diff.at(key);
        if (v.has_value()) {
          const auto v_str = OptionsIO::to_string(key, v.value());
          if (!(OptionsIO::type(key) == "bool" && v_str == "true"))
            ss << "=" << quote_for_cli(v_str);
        } else {
          ss << "=<unset>";
        }

        ss << " ";
      }
    }
  }
  return ss.str();
}

int main(int argc, char **argv) {
  Options options;

  cxxopts::Options cxxOptions("App", "Test app");
  cxxOptions.positional_help("file1 file2 ...");
  cxxOptions.show_positional_help();
  cxxOptions.allow_unrecognised_options();

  cxxOptions.add_options()("h,help", "Print usage");

  /* add options arguments */
  CliOptionsHelper helper(options);
  for (const auto [grp, args] : options_cli) {
    auto g = cxxOptions.add_options(grp);
    for (const auto arg : args)
      helper.add(g, std::get<0>(arg), std::get<1>(arg), std::get<2>(arg));
  }

  /* run cxxopts and bail if help requested */
  auto result = cxxOptions.parse(argc, argv);
  if (result.count("help")) {
    std::cout << cxxOptions.help() << std::endl;
    return 0;
  }

  /* let's pretend this has been loaded from a config file */
  json ex = R"(
  {
    "render": {
      "background":{
        "color": [0.5, 0.5, 0.5]
      }
    },
    "ui": {
      "font-file": "~/myfont.ttf"
    },
    "render.effect.ambient-occlusion": true,
    "render.effect.anti-aliasing": "yes"
  }
  )"_json;

  OptionsDiff cfg_diff;
  for (auto [k, v] : collect_json_by_key(ex))
    if (v.is_null())
      cfg_diff[k] = std::nullopt;
    else
      cfg_diff[k] = OptionsIO::from_json(k, v);

  std::cout << "changes from config:" << std::endl;
  print_diff(cfg_diff);

  /* the helper will apply the correct parsing functions to give us a
    map<key, variant> of the changes from the CLI */
  auto cli_diff = helper.retrieve_diff(result, "<unset>", "<default>");
  std::cout << "changes from command line:" << std::endl;
  print_diff(cli_diff);

  /* apply the changes */
  OptionsIO::apply(options, cfg_diff);
  OptionsIO::apply(options, cli_diff);
  // TODO combine diffs and apply instead of applying twice

  const Options default_options;
  const auto effective_diff = OptionsIO::diff(options, default_options);

  std::cout << "final diff from default options:" << std::endl;
  print_diff(effective_diff, default_options);

  std::cout << "cli args to match:" << std::endl;
  std::cout << diff_to_cli(effective_diff) << std::endl << std::endl;

  /* now we can use the options struct directly */
  const Vector3 view_up =
      options.camera.view_up.value_or(options.scene.up_direction);

  const auto compute_model_centroid = []() { return (Point3){1., 2., 3.}; };
  const Point3 focus = options.camera.focal_point.has_value()
                           ? options.camera.focal_point.value()
                           : compute_model_centroid(); //

  /* */
  const auto colormap_lookup = [](const Colormap &cm, double v) {
    const double t = v * (cm.colors.size() - 1);
    const int i = (int)(t);
    const auto c0 = cm.colors[i];
    const auto c1 = cm.colors[std::min(i + 1, (int)cm.colors.size() - 1)];
    return Color({
        c0[0] + (c1[0] - c0[0]) * (t - i),
        c0[1] + (c1[1] - c0[1]) * (t - i),
        c0[2] + (c1[2] - c0[2]) * (t - i),
    });
  };

  std::cout << "colormap: ";
  for (int i = 0; i < 80; ++i) {
    const auto c = colormap_lookup(options.model.scivis.colormap, i / 80.);
    std::cout << "\033[48;2;" << (int)(c[0] * 255) << ";" << (int)(c[1] * 255)
              << ";" << (int)(c[2] * 255) << "m"
              << " ";
  }
  std::cout << RESET << std::endl;

  return 0;
}
