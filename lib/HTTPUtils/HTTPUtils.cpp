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

void postTask(void* parameter) {
    PostTaskParams* params = (PostTaskParams*)parameter;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HTTP] Error: not connected to WiFi");
    } else {
        HTTPClient http;
        http.begin(params->url);

        Serial.println("[HTTP] POST " + params->url);
        int httpCode = http.POST(params->data);

        if (httpCode > 0) {
            String payload = http.getString();
            Serial.println("[HTTP] Response code: " + String(httpCode));
            Serial.println("[HTTP] Response payload: " + payload);
        } else {
            Serial.println("[HTTP] Error in sending POST request");
        }

        http.end();
    }

    delete params;      // Free allocated memory
    vTaskDelete(NULL);  // Delete task after execution
}

void postAsync(String url, String data) {
    PostTaskParams* params = new PostTaskParams{url, data};  // Allocate memory for parameters

    xTaskCreatePinnedToCore(
        postTask,          // Function to execute
        "HTTP_Post_Task",  // Task name
        10000,             // Stack size
        params,            // Parameter to pass
        1,                 // Priority
        NULL,              // Task handle (not needed)
        0                  // Core ID (0 or 1)
    );
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