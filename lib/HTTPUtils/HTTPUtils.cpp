#include <HTTPUtils.h>

namespace HTTPUtils {

void post(String url, String data) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HTTP] Error: not connected to WiFi");
        return;
    }

    HTTPClient http;

    http.begin(url);  // Specify the URL to send the POST request to

    Serial.println("[HTTP] POST " + String(url));
    int httpCode =
        http.POST(data);  // Send the POST request and get the HTTP response code

    // Check the response
    if (httpCode > 0) {
        String payload = http.getString();  // Print the response
        Serial.println("[HTTP] Response code: " + String(httpCode));
        Serial.println("[HTTP] Response payload: " + payload);
    } else {
        Serial.println("[HTTP] Error in sending POST request");
    }

    http.end();
}

String get(String url) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HTTP] Error: not connected to WiFi");
        return String();
    }

    HTTPClient http;

    http.begin(url);  // Specify the URL to send the GET request to

    Serial.println("[HTTP] GET " + String(url));
    int httpCode =
        http.GET();  // Send the GET request and get the HTTP response code

    // Check the response
    if (httpCode > 0) {
        String payload = http.getString();  // Print the response
        Serial.println("[HTTP] Response code: " + String(httpCode));
        Serial.println("[HTTP] Response payload: " + payload);
        http.end();
        return payload;
    } else {
        Serial.println("[HTTP] Error in sending GET request");
        http.end();
        return String();
    }
}

}  // namespace HTTPUtils