#include <cmath>
#include <iomanip>
#include <iostream>
#include <regex>
#include <stdio.h>
#include <stdlib.h>

#include "options-io.h"
#include "options.h"

namespace options_ns {

std::string parse_std_string(const std::string &s) { return s; }
std::string format_std_string(const std::string &v) { return v; }

int parse_int(const std::string &s) { return std::stoi(s); }
std::string format_int(const int &v) {
  std::stringstream stream;
  stream << v;
  return stream.str();
}

double parse_double(const std::string &s) { return std::stod(s); }
std::string format_double(const double &v) {
  std::stringstream stream;
  stream << v;
  return stream.str();
}

bool parse_bool(const std::string &s) {
  if (s == "true" || s == "yes" || s == "y" || s == "on" || s == "1")
    return true;
  else if (s == "false" || s == "no" || s == "n" || s == "off" || s == "0")
    return false;
  else
    throw std::invalid_argument("cannot parse boolean: " + s);
}
std::string format_bool(const bool &boolean) {
  return boolean ? "true" : "false";
}

const std::string re_lbrackets = "[\\[\\{\\(]";
const std::string re_rbrackets = "[\\]\\}\\)]";
const std::string re_number =
    "[+-]?(?:\\d+(?:[.]\\d*)?(?:[eE][+-]?\\d+)?|[.]\\d+(?:[eE][+-]?\\d+)?)";
const std::string re_array3double =
    "(" + re_lbrackets + ")?\\s*(" + re_number + ")\\s*,?\\s*(" + re_number +
    ")\\s*,?\\s*(" + re_number + ")\\s*(" + re_rbrackets + ")?";

std::array<double, 3> _parse_array_double3(const std::string &s) {
  const std::regex pattern(re_array3double, std::regex_constants::icase);

  std::smatch match;
  if (std::regex_match(s, match, pattern)) {
    if ((match[1] == "(" and match[5] != ")") ||
        (match[1] == "[" and match[5] != "]") ||
        (match[1] == "{" and match[5] != "}") ||
        (match[1] == "" and match[5] != ""))
      throw std::invalid_argument("mismatched brackets");
    return {std::stod(match[2]), std::stod(match[3]), std::stod(match[4])};
  }
  throw std::invalid_argument("not a number triple");
}

Vector3 parse_Vector3(const std::string &s) {
  const std::regex pattern("(([+-]?)(x))?(([+-]?)(y))?(([+-]?)(z))?",
                           std::regex_constants::icase);
  std::smatch match;
  if (std::regex_match(s, match, pattern)) {
    int sign = 1;
    if (match[2] == "-")
      sign = -1;
    else if (match[2] == "+")
      sign = +1;

    double x = match[3] == "" ? 0 : sign;

    if (match[5] == "-")
      sign = -1;
    else if (match[5] == "+")
      sign = +1;

    double y = match[6] == "" ? 0 : sign;

    if (match[8] == "-")
      sign = -1;
    else if (match[8] == "+")
      sign = +1;

    double z = match[9] == "" ? 0 : sign;

    return {x, y, z};
  }
  try {
    return _parse_array_double3(s);
  } catch (std::invalid_argument &e) {
    throw std::invalid_argument("cannot parse Vector3 (" +
                                std::string(e.what()) + ")");
  }
}
std::string format_Vector3(const Vector3 &v) {
  std::stringstream stream;
  stream << "(" << v[0] << "," << v[1] << "," << v[2] << ")";
  return stream.str();
}

Point3 parse_Point3(const std::string &s) {
  try {
    return _parse_array_double3(s);
  } catch (std::invalid_argument &e) {
    throw std::invalid_argument("cannot parse Point3 (" +
                                std::string(e.what()) + ")");
  }
}
std::string format_Point3(const Point3 &p) {
  std::stringstream stream;
  stream << "(" << p[0] << "," << p[1] << "," << p[2] << ")";
  return stream.str();
}

Color parse_Color(const std::string &s) {
  const std::regex hexPattern("#([0-9a-f]{6})", std::regex_constants::icase);
  std::smatch match;
  if (std::regex_match(s, match, hexPattern)) {
    int r = 0, g = 0, b = 0;
    sscanf(match[1].str().c_str(), "%02x%02x%02x", &r, &g, &b);
    return {r / 255., g / 255., b / 255.};
  }

  try {
    return _parse_array_double3(s);
  } catch (std::invalid_argument &e) {
    throw std::invalid_argument("cannot parse Color (" + std::string(e.what()) +
                                ")");
  }
}

std::string format_Color(const Color &color) {
  std::stringstream stream;
  stream << "#" << std::hex //
         << std::setfill('0') << std::setw(2)
         << (int)std::round(color[0] * 255) //
         << std::setfill('0') << std::setw(2)
         << (int)std::round(color[1] * 255) //
         << std::setfill('0') << std::setw(2)
         << (int)std::round(color[2] * 255);
  return stream.str();
}

Colormap parse_Colormap(const std::string &v) {
  if (v == "redblue") {
    return Colormap({{Color({1., 0., 0.}), Color({0., 0., 1.})}, v});
  } else if (v == "frenchflag") {
    return Colormap(
        {{Color({0., 0., 1.}), Color({1., 1., 1.}), Color({1., 0., 0.})}, v});
    // } else if (v == "...") {
    // TODO lookup more names
  } else {
    Colormap cm;
    std::stringstream stream(v);
    while (stream.good()) {
      std::string substr;
      getline(stream, substr, ',');
      cm.colors.push_back(parse_Color(substr));
    }
    return cm;
  }
}

std::string format_Colormap(const Colormap &cm) {
  if (cm.name.empty()) {
    std::stringstream stream;
    if (!cm.colors.empty()) {
      bool first = true;
      for (auto color : cm.colors) {
        if (!first)
          stream << ",";
        stream << format_Color(color);
        first = false;
      }
    }
    return stream.str();
  } else {
    return cm.name;
  }
}

std::string json_to_std_string(const json &s) {
  if (s.is_string())
    return s.get<std::string>();
  if (s.is_structured())
    throw std::invalid_argument("complex object");
  std::stringstream ss;
  ss << s;
  return ss.str();
}

int json_to_int(const json &s) {
  if (s.is_number_integer())
    return s.get<int>();
  else
    return parse_int(json_to_std_string(s));
}

double json_to_double(const json &s) {
  if (s.is_number())
    return s.get<double>();
  else
    return parse_double(json_to_std_string(s));
}

bool json_to_bool(const json &s) {
  if (s.is_boolean())
    return s.get<bool>();
  else
    return parse_bool(json_to_std_string(s));
}

std::array<double, 3> _json_to_array_double3(const json &o) {
  if (o.is_array() && o.size() == 3 && o[0].is_number() && o[1].is_number() &&
      o[2].is_number()) {
    return {o[0].get<double>(), o[1].get<double>(), o[2].get<double>()};
  } else
    throw std::invalid_argument("not a number triple");
}

Vector3 json_to_Vector3(const json &v) {
  try {
    return _json_to_array_double3(v);
  } catch (std::invalid_argument &e) {
    try {
      return parse_Vector3(json_to_std_string(v));
    } catch (std::invalid_argument &e) {
      throw std::invalid_argument("cannot convert json to Vector3 (" +
                                  std::string(e.what()) + ")");
    }
  }
}

Point3 json_to_Point3(const json &v) {
  try {
    return _json_to_array_double3(v);
  } catch (std::invalid_argument &e) {
    try {
      return parse_Point3(json_to_std_string(v));
    } catch (std::invalid_argument &e) {
      throw std::invalid_argument("cannot convert json to Point3 (" +
                                  std::string(e.what()) + ")");
    }
  }
}

Color json_to_Color(const json &v) {
  try {
    return _json_to_array_double3(v);
  } catch (std::invalid_argument &e) {
    try {
      return parse_Color(json_to_std_string(v));
    } catch (std::invalid_argument &e) {
      throw std::invalid_argument("cannot convert json to Color (" +
                                  std::string(e.what()) + ")");
    }
  }
}

Colormap json_to_Colormap(const json &v) {
  // TODO
  try {
    return parse_Colormap(json_to_std_string(v));
  } catch (std::invalid_argument &e) {
    throw std::invalid_argument("cannot convert json to Colormap (" +
                                std::string(e.what()) + ")");
  }
}

} // namespace options_ns