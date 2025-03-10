#pragma once
#include <HTTPClient.h>
#include <WiFi.h>

namespace HTTPUtils {

/// @brief Sends a POST request to the specified URL.
/// @param url URL to send the request to
/// @param data data to be sent
void post(String url, String data);

/// @brief Sends a GET request to the specified URL.
/// @param url URL to send the request to
String get(String url);

}  // namespace HTTPUtils