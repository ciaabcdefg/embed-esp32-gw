#pragma once
#include <HTTPClient.h>
#include <WiFi.h>

namespace HTTPUtils {

/// @brief Sends a POST request to the specified URL.
/// @param url URL to send the request to
/// @param data data to be sent
void post(String url, String data);

} // namespace HTTPUtils