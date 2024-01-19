#pragma once

#include <Arduino.h>
#include <map>

int get_int_default(const std::map<String, String>& map, const String& key, const int default_value)
{
  const auto it = map.find(key);
  if (it == map.end()) {
    return default_value;
  }
  return it->second.toInt();
}

struct GongParam
{
  static GongParam from_map(const std::map<String, String>& map)
  {
    return {
      .loudness = get_int_default(map, "loudness", 100),
      .index = get_int_default(map, "index", 1),
    };
  }

  int loudness;
  int index;
};
