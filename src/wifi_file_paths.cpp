#include "wifi_file_paths.h"

String htmlEscape(const String &value) {
  String escaped;
  escaped.reserve(value.length());
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    switch (c) {
      case '&':
        escaped += "&amp;";
        break;
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      default:
        escaped += c;
        break;
    }
  }
  return escaped;
}

static String urlDecode(const String &value) {
  String decoded;
  decoded.reserve(value.length());
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '+') {
      decoded += ' ';
      continue;
    }
    if (c == '%' && i + 2 < value.length()) {
      const char hex[3] = {value[i + 1], value[i + 2], 0};
      char *end = nullptr;
      const long parsed = strtol(hex, &end, 16);
      if (end != hex + 2) {
        return String();
      }
      decoded += static_cast<char>(parsed);
      i += 2;
      continue;
    }
    decoded += c;
  }
  return decoded;
}

bool normalizeSdPath(const String &rawPath, String &path) {
  String decoded = urlDecode(rawPath.length() == 0 ? "/" : rawPath);
  if (decoded.length() == 0) {
    return false;
  }
  decoded.replace('\\', '/');
  if (!decoded.startsWith("/")) {
    decoded = "/" + decoded;
  }
  if (decoded.indexOf("/../") >= 0 || decoded.endsWith("/..") ||
      decoded.indexOf("/./") >= 0 || decoded.endsWith("/.") ||
      decoded.indexOf("//") >= 0) {
    return false;
  }
  path = decoded;
  return true;
}

String parentSdPath(const String &path) {
  if (path == "/") {
    return "/";
  }
  const int slash = path.lastIndexOf('/');
  return slash <= 0 ? "/" : path.substring(0, slash);
}

String leafSdName(const String &path) {
  if (path == "/") {
    return "/";
  }
  const int slash = path.lastIndexOf('/');
  return slash >= 0 ? path.substring(slash + 1) : path;
}
