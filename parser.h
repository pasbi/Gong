#pragma once

#include <Arduino.h>
#include <map>

class Parser
{
public:
  explicit Parser(const String& line)
  {
    String* buffer = nullptr;
    for (int i = 0; i < line.length(); ++i) {
      const auto c = line[i];
      switch (c) {
      case '&':
        new_kv_pair();
        buffer = &m_current_key;
        break;
      case '=':
        buffer = &m_current_value;
        break;
      default:
        if (buffer != nullptr) {
          *buffer += c;
        }
      }
      if (c == ' ' || i == line.length() - 1) {
        new_kv_pair();
      }
    }
  }

  using ParamsMap = std::map<String, String>;

  const ParamsMap& params() const noexcept
  {
    return m_params;
  }
  
private:
  String m_current_key;
  String m_current_value;
  ParamsMap m_params;

  void new_kv_pair()
  {
    if (!m_current_key.isEmpty()) {
      m_params.emplace(std::move(m_current_key), std::move(m_current_value));
    }
    m_current_key = "";
    m_current_value = "";
  }
};
