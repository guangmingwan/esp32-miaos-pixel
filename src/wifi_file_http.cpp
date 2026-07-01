#include "wifi_file_http.h"

#include <SD.h>
#include <WebServer.h>

#include "wifi_file_paths.h"

static WebServer g_server(80);
static File g_uploadFile;
static bool g_routesConfigured = false;
static bool g_serverStarted = false;

static bool requestPath(String &path) {
  const String rawPath = g_server.hasArg("path") ? g_server.arg("path") : "/";
  return normalizeSdPath(rawPath, path);
}

static void sendPlain(int code, const char *message) {
  g_server.send(code, "text/plain", message);
}

static void sendRedirect(const String &path) {
  g_server.sendHeader("Location", "/?path=" + path);
  g_server.send(303, "text/plain", "See Other");
}

static void appendHeader(String &html, const String &path) {
  html += F("<!doctype html><html><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>MiaOS SD</title><style>");
  html += F("body{font-family:sans-serif;margin:16px;max-width:760px}");
  html += F("a{color:#0645ad}table{border-collapse:collapse;width:100%}");
  html += F("td,th{border-bottom:1px solid #ddd;padding:6px;text-align:left}");
  html += F("input,button{font-size:16px;margin:4px 0}code{background:#eee;padding:2px 4px}");
  html += F("</style></head><body><h1>MiaOS SD</h1><p>Path: <code>");
  html += htmlEscape(path);
  html += F("</code></p>");
}

static void appendUploadForm(String &html, const String &path) {
  html += F("<form method='post' action='/upload?path=");
  html += path;
  html += F("' enctype='multipart/form-data'>");
  html += F("<input type='file' name='file'><button type='submit'>Upload</button></form>");
  html += F("<form method='post' action='/mkdir?path=");
  html += path;
  html += F("'><input name='name' placeholder='Folder name'><button type='submit'>New folder</button></form>");
}

static void appendEntryRow(String &html, File &entry, const String &path) {
  const String name = leafSdName(entry.name());
  String childPath = path;
  if (!childPath.endsWith("/")) {
    childPath += "/";
  }
  childPath += name;
  html += F("<tr><td>");
  if (entry.isDirectory()) {
    html += F("<a href='/?path=");
    html += childPath;
    html += F("'>");
    html += htmlEscape(name);
    html += F("/</a>");
  } else {
    html += htmlEscape(name);
  }
  html += F("</td><td>");
  if (!entry.isDirectory()) {
    html += String(entry.size());
  }
  html += F("</td><td>");
  if (!entry.isDirectory()) {
    html += F("<a href='/download?path=");
    html += childPath;
    html += F("'>Download</a> ");
  }
  html += F("<form method='post' action='/delete?path=");
  html += childPath;
  html += F("' style='display:inline'><button type='submit'>Delete</button></form>");
  html += F("</td></tr>");
}

static void handleList() {
  String path;
  if (!requestPath(path)) {
    sendPlain(400, "Invalid path");
    return;
  }
  File dir = SD.open(path);
  if (!dir) {
    sendPlain(404, "Not found");
    return;
  }
  if (!dir.isDirectory()) {
    dir.close();
    sendRedirect("/download?path=" + path);
    return;
  }
  String html;
  html.reserve(4096);
  appendHeader(html, path);
  appendUploadForm(html, path);
  html += F("<table><tr><th>Name</th><th>Size</th><th>Actions</th></tr>");
  if (path != "/") {
    html += F("<tr><td><a href='/?path=");
    html += parentSdPath(path);
    html += F("'>..</a></td><td></td><td></td></tr>");
  }
  for (;;) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    appendEntryRow(html, entry, path);
    entry.close();
  }
  dir.close();
  html += F("</table></body></html>");
  g_server.send(200, "text/html", html);
}

static void handleDownload() {
  String path;
  if (!requestPath(path)) {
    sendPlain(400, "Invalid path");
    return;
  }
  File file = SD.open(path, FILE_READ);
  if (!file) {
    sendPlain(404, "Not found");
    return;
  }
  if (file.isDirectory()) {
    file.close();
    sendPlain(400, "Cannot download directory");
    return;
  }
  g_server.sendHeader("Content-Disposition",
                      "attachment; filename=\"" + leafSdName(path) + "\"");
  g_server.streamFile(file, "application/octet-stream");
  file.close();
}

static void handleUploadDone() {
  String path;
  if (!requestPath(path)) {
    sendPlain(400, "Invalid path");
    return;
  }
  sendRedirect(path);
}

static void handleUploadData() {
  HTTPUpload &upload = g_server.upload();
  String dirPath;
  if (!requestPath(dirPath)) {
    return;
  }
  if (upload.status == UPLOAD_FILE_START) {
    String targetPath = dirPath;
    if (!targetPath.endsWith("/")) {
      targetPath += "/";
    }
    targetPath += upload.filename;
    if (!normalizeSdPath(targetPath, targetPath)) {
      return;
    }
    g_uploadFile = SD.open(targetPath, FILE_WRITE);
    return;
  }
  if (upload.status == UPLOAD_FILE_WRITE && g_uploadFile) {
    g_uploadFile.write(upload.buf, upload.currentSize);
    return;
  }
  if (upload.status == UPLOAD_FILE_END && g_uploadFile) {
    g_uploadFile.close();
  }
}

static void handleMkdir() {
  String path;
  if (!requestPath(path)) {
    sendPlain(400, "Invalid path");
    return;
  }
  if (!g_server.hasArg("name")) {
    sendPlain(400, "Missing name");
    return;
  }
  String folderPath = path;
  if (!folderPath.endsWith("/")) {
    folderPath += "/";
  }
  folderPath += g_server.arg("name");
  if (!normalizeSdPath(folderPath, folderPath)) {
    sendPlain(400, "Invalid folder name");
    return;
  }
  if (!SD.mkdir(folderPath)) {
    sendPlain(500, "mkdir failed");
    return;
  }
  sendRedirect(path);
}

static void handleDelete() {
  String path;
  if (!requestPath(path) || path == "/") {
    sendPlain(400, "Invalid path");
    return;
  }
  File target = SD.open(path);
  if (!target) {
    sendPlain(404, "Not found");
    return;
  }
  const bool directory = target.isDirectory();
  target.close();
  const bool removed = directory ? SD.rmdir(path) : SD.remove(path);
  if (!removed) {
    sendPlain(500, "delete failed");
    return;
  }
  sendRedirect(parentSdPath(path));
}

static void configureRoutes() {
  if (g_routesConfigured) {
    return;
  }
  g_server.on("/", HTTP_GET, handleList);
  g_server.on("/download", HTTP_GET, handleDownload);
  g_server.on("/upload", HTTP_POST, handleUploadDone, handleUploadData);
  g_server.on("/mkdir", HTTP_POST, handleMkdir);
  g_server.on("/delete", HTTP_POST, handleDelete);
  g_server.onNotFound([]() { sendPlain(404, "Not found"); });
  g_routesConfigured = true;
}

void startWifiFileHttpServer() {
  configureRoutes();
  g_server.begin();
  g_serverStarted = true;
}

void handleWifiFileHttpClient() {
  if (g_serverStarted) {
    g_server.handleClient();
  }
}

void stopWifiFileHttpServer() {
  if (g_uploadFile) {
    g_uploadFile.close();
  }
  if (g_serverStarted) {
    g_server.stop();
  }
  g_serverStarted = false;
}
