#pragma once

#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "options.h"

namespace options_ns {

std::string parse_std_string(const std::string &);
std::string format_std_string(const std::string &);

int parse_int(const std::string &);
std::string format_int(const int &);

double parse_double(const std::string &);
std::string format_double(const double &);

bool parse_bool(const std::string &);
std::string format_bool(const bool &);

Color parse_Color(const std::string &);
std::string format_Color(const Color &);

Vector3 parse_Vector3(const std::string &);
std::string format_Vector3(const Vector3 &);

Point3 parse_Point3(const std::string &);
std::string format_Point3(const Point3 &);

Colormap parse_Colormap(const std::string &);
std::string format_Colormap(const Colormap &);

std::string json_to_std_string(const json &);
int json_to_int(const json &);
double json_to_double(const json &);
bool json_to_bool(const json &);
Vector3 json_to_Vector3(const json &);
Point3 json_to_Point3(const json &);
Color json_to_Color(const json &);
Colormap json_to_Colormap(const json &);

} // namespace options_ns