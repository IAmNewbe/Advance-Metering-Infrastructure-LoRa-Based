#include "AntaresESPHTTP.h"

AntaresESPHTTP::AntaresESPHTTP(String accessKey)
{
    _accessKey = accessKey;
}

void AntaresESPHTTP::add(String key, String value)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonString);
    jsonDoc[key] = value;
    String newInsert;
    serializeJson(jsonDoc, newInsert);
    jsonString = newInsert;
}

void AntaresESPHTTP::add(String key, int value)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonString);
    jsonDoc[key] = value;
    String newInsert;
    serializeJson(jsonDoc, newInsert);
    jsonString = newInsert;
}

void AntaresESPHTTP::add(String key, float value)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonString);
    jsonDoc[key] = value;
    String newInsert;
    serializeJson(jsonDoc, newInsert);
    jsonString = newInsert;
}

void AntaresESPHTTP::add(String key, double value)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonString);
    jsonDoc[key] = value;
    String newInsert;
    serializeJson(jsonDoc, newInsert);
    jsonString = newInsert;
}

void AntaresESPHTTP::add(String key, String key2, String value)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonString);
    if (_currentKey != key)
    {
        JsonObject nested = jsonDoc.createNestedObject(key);
        nested[key2] = value;
    }
    else
    {
        jsonDoc[key][key2] = value;
    }

    String newInsert;
    serializeJson(jsonDoc, newInsert);
    jsonString = newInsert;
    _currentKey = key;
}

void AntaresESPHTTP::add(String key, String key2, int value)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonString);
    if (_currentKey != key)
    {
        JsonObject nested = jsonDoc.createNestedObject(key);
        nested[key2] = value;
    }
    else
    {
        jsonDoc[key][key2] = value;
    }

    String newInsert;
    serializeJson(jsonDoc, newInsert);
    jsonString = newInsert;
    _currentKey = key;
}

void AntaresESPHTTP::add(String key, String key2, float value)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonString);
    if (_currentKey != key)
    {
        JsonObject nested = jsonDoc.createNestedObject(key);
        nested[key2] = value;
    }
    else
    {
        jsonDoc[key][key2] = value;
    }

    String newInsert;
    serializeJson(jsonDoc, newInsert);
    jsonString = newInsert;
    _currentKey = key;
}

void AntaresESPHTTP::add(String key, String key2, double value)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonString);
    if (_currentKey != key)
    {
        JsonObject nested = jsonDoc.createNestedObject(key);
        nested[key2] = value;
    }
    else
    {
        jsonDoc[key][key2] = value;
    }

    String newInsert;
    serializeJson(jsonDoc, newInsert);
    jsonString = newInsert;
    _currentKey = key;
}

void AntaresESPHTTP::send(String projectName, String deviceName)
{
    printDebug("\n[ANTARES] Connecting to " + _serverNoHttp + "\n");

    // Use WiFiClient class to create TCP connections
    // WiFiClientSecure client;
    HTTPClient https;
    /*
    #if defined(ESP32)
        const char* ca_cert = cert_AlphaSSL_CA___SHA256___G4;
        client.setCACert(ca_cert);
    #elif defined(ESP8266)
        client.setFingerprint(fingerprint___antares_id);
    #endif

        const int httpPort = 8443;
        String httpUri = "/~/antares-cse/antares-id/"+ projectName + "/" + deviceName;
        if (!https.begin(client, _serverNoHttp, httpPort, httpUri, true)){
            printDebug("[ANTARES] Connection failed!\n");
            return;
        }*/
    String httpUri;
    httpUri = "https://platform.antares.id:8443/~/antares-cse/antares-id/" + projectName + "/" + deviceName;

    if (!https.begin(httpUri))
    {
        printDebug("[ANTARES] Connection failed!\n");
        return;
    };

    printDebug("[ANTARES] Connection success!\n");

    jsonString.replace("\"", "\\\"");

    String body;
    body += "{";
    body += "\"m2m:cin\": {";
    body += "\"con\": \"" + jsonString + "\"";
    body += "}";
    body += "}";

    printDebug("Requesting URI: " + httpUri + "\n");

    https.addHeader("X-M2M-Origin", _accessKey);
    https.addHeader("Content-Type", "application/json;ty=4");
    https.addHeader("Accept", "application/json");
    https.addHeader("Connection", "close");

    // This will send the request to the server
    int httpCode = https.POST(body);
    if (httpCode > 0)
    {
        printDebug("[ANTARES] POST... code: " + (String)httpCode + "\n");

        if (httpCode == 201)
        {
            String payload = https.getString();
            printDebug("[ANTARES] Creating data success\n");
            printDebug("[ANTARES] " + payload + "\n");
        }
        else
        {
            printDebug("[ANTARES] Creating data failed\n");
        }
    }
    else
    {
        printDebug("[ANTARES] POST... failed, error: " + (String)https.errorToString(httpCode) + "\n");
    }

    printDebug("[ANTARES] Closing connection...\n");
    https.end();

    jsonString = "{}";
    _currentKey = "";
}

void AntaresESPHTTP::sendRaw(String text, String projectName, String deviceName)
{
    printDebug("\n[ANTARES] Connecting to " + _serverNoHttp + "\n");
    // Use WiFiClient class to create TCP connections
    // WiFiClientSecure client;
    HTTPClient https;
    /*
    #if defined(ESP32)
        const char* ca_cert = cert_AlphaSSL_CA___SHA256___G4;
        client.setCACert(ca_cert);
    #elif defined(ESP8266)
        client.setFingerprint(fingerprint___antares_id);
    #endif

        const int httpPort = 8443;
        String httpUri = "/~/antares-cse/antares-id/"+ projectName + "/" + deviceName;
        if (!https.begin(client, _serverNoHttp, httpPort, httpUri, true)){
            printDebug("[ANTARES] Connection failed!\n");
            return;
        }*/
    String httpUri;
    httpUri = "https://platform.antares.id:8443/~/antares-cse/antares-id/" + projectName + "/" + deviceName;

    if (!https.begin(httpUri))
    {
        printDebug("[ANTARES] Connection failed!\n");
        return;
    };
    printDebug("[ANTARES] Connection success!\n");

    String body;
    body += "{";
    body += "\"m2m:cin\": {";
    body += "\"con\": \"" + text + "\"";
    body += "}";
    body += "}";

    printDebug("Requesting URI: " + httpUri + "\n");

    https.addHeader("X-M2M-Origin", _accessKey);
    https.addHeader("Content-Type", "application/json;ty=4");
    https.addHeader("Accept", "application/json");
    https.addHeader("Connection", "close");

    // This will send the request to the server
    int httpCode = https.POST(body);
    if (httpCode > 0)
    {
        printDebug("[ANTARES] POST... code: " + (String)httpCode + "\n");

        if (httpCode == 201)
        {
            String payload = https.getString();
            printDebug("[ANTARES] Creating data success\n");
            printDebug("[ANTARES] " + payload + "\n");
        }
        else
        {
            printDebug("[ANTARES] Creating data failed\n");
        }
    }
    else
    {
        printDebug("[ANTARES] POST... failed, error: " + (String)https.errorToString(httpCode) + "\n");
    }

    printDebug("[ANTARES] Closing connection...\n");
    https.end();

    jsonString = "{}";
    _currentKey = "";
}

void AntaresESPHTTP::sendRawNonSecure(String text, String projectName, String deviceName)
{
    printDebug("\n[ANTARES] Connecting to " + _serverNoHttp + "\n");

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    HTTPClient http;

    String httpUri = "/~/antares-cse/antares-id/" + projectName + "/" + deviceName;
    if (!http.begin(client, _serverNoHttp, _portNum, httpUri, false))
    {
        printDebug("[ANTARES] Connection failed!\n");
        return;
    }
    printDebug("[ANTARES] Connection success!\n");

    String body;
    body += "{";
    body += "\"m2m:cin\": {";
    body += "\"con\": \"" + text + "\"";
    body += "}";
    body += "}";

    printDebug("Requesting URI: " + httpUri + "\n");

    http.addHeader("X-M2M-Origin", _accessKey);
    http.addHeader("Content-Type", "application/json;ty=4");
    http.addHeader("Accept", "application/json");
    http.addHeader("Connection", "close");

    // This will send the request to the server
    int httpCode = http.POST(body);
    if (httpCode > 0)
    {
        printDebug("[ANTARES] POST... code: " + (String)httpCode + "\n");

        if (httpCode == 201)
        {
            String payload = http.getString();
            printDebug("[ANTARES] Creating data success\n");
            printDebug("[ANTARES] " + payload + "\n");
        }
        else
        {
            printDebug("[ANTARES] Creating data failed\n");
        }
    }
    else
    {
        printDebug("[ANTARES] POST... failed, error: " + (String)http.errorToString(httpCode) + "\n");
    }

    printDebug("[ANTARES] Closing connection...\n");
    http.end();

    jsonString = "{}";
    _currentKey = "";
}

void AntaresESPHTTP::sendNonSecure(String projectName, String deviceName)
{
    printDebug("\n[ANTARES] Connecting to " + _serverNoHttp + "\n");

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    HTTPClient http;

    String httpUri = "/~/antares-cse/antares-id/" + projectName + "/" + deviceName;
    if (!http.begin(client, _serverNoHttp, _portNum, httpUri, false))
    {
        printDebug("[ANTARES] Connection failed!\n");
        return;
    }
    printDebug("[ANTARES] Connection success!\n");

    jsonString.replace("\"", "\\\"");

    String body;
    body += "{";
    body += "\"m2m:cin\": {";
    body += "\"con\": \"" + jsonString + "\"";
    body += "}";
    body += "}";

    printDebug("Requesting URI: " + httpUri + "\n");

    http.addHeader("X-M2M-Origin", _accessKey);
    http.addHeader("Content-Type", "application/json;ty=4");
    http.addHeader("Accept", "application/json");
    http.addHeader("Connection", "close");

    // This will send the request to the server
    int httpCode = http.POST(body);
    if (httpCode > 0)
    {
        printDebug("[ANTARES] POST... code: " + (String)httpCode + "\n");

        if (httpCode == 201)
        {
            String payload = http.getString();
            printDebug("[ANTARES] Creating data success\n");
            printDebug("[ANTARES] " + payload + "\n");
        }
        else
        {
            printDebug("[ANTARES] Creating data failed\n");
        }
    }
    else
    {
        printDebug("[ANTARES] POST... failed, error: " + (String)http.errorToString(httpCode) + "\n");
    }

    printDebug("[ANTARES] Closing connection...\n");
    http.end();

    jsonString = "{}";
    _currentKey = "";
}

void AntaresESPHTTP::printData()
{
    DynamicJsonDocument jsonDocSend(1024);
    DynamicJsonDocument jsonDocGet(1024);
    deserializeJson(jsonDocSend, jsonString);
    deserializeJson(jsonDocGet, jsonGetString);
    printDebug("\n\n[ANTARES] Data to send: \n\n");
    serializeJsonPretty(jsonDocSend, Serial);
    printDebug("\n\n[ANTARES] Data available to get: \n\n");
    serializeJsonPretty(jsonDocGet, Serial);
}

void AntaresESPHTTP::get(String projectName, String deviceName)
{

    jsonGetString = "";
    _getSuccess = false;
    printDebug("\n[ANTARES] Connecting to " + _serverNoHttp + "\n");

    // Use WiFiClient class to create TCP connections
    // WiFiClientSecure client;
    HTTPClient https;

    /*
    #if defined(ESP32)
        const char *ca_cert = cert_AlphaSSL_CA___SHA256___G4;
        client.setCACert(ca_cert);
    #elif defined(ESP8266)
        client.setFingerprint(fingerprint___antares_id);
    #endif*/

    // const int httpPort = 8443;

    String httpUri;
    httpUri = "https://platform.antares.id:8443/~/antares-cse/antares-id/" + projectName + "/" + deviceName + "/la";
    // String httpUri = "/~/antares-cse/antares-id/" + projectName + "/" + deviceName + "/la";

    if (!https.begin(httpUri))
    {
        printDebug("[ANTARES] Connection failed!\n");
        return;
    }
    printDebug("[ANTARES] Connection success!\n");
    printDebug("Requesting URI: " + httpUri + "\n");

    https.addHeader("X-M2M-Origin", _accessKey);
    https.addHeader("Content-Type", "application/json;ty=4");
    https.addHeader("Accept", "application/json");
    https.addHeader("Connection", "close");

    // This will send the request to the server
    int httpCode = https.GET();
    if (httpCode > 0)
    {
        printDebug("[ANTARES] GET... code: " + (String)httpCode + "\n");

        if (httpCode == 200)
        {
            printDebug("[ANTARES] Getting data success\n");
            String payload = https.getString();
            DynamicJsonDocument jsonDoc(1024);
            deserializeJson(jsonDoc, payload);
            String receivedData = jsonDoc["m2m:cin"]["con"];
            DynamicJsonDocument jsonDocCon(1024);
            deserializeJson(jsonDocCon, receivedData);
            if (_debug)
            {
                serializeJson(jsonDocCon, jsonGetString);
                serializeJsonPretty(jsonDocCon, Serial);
            }
            _getSuccess = true;
        }
        else
        {
            printDebug("[ANTARES] Getting data failed\n");
        }
    }
    else
    {
        printDebug("[ANTARES] GET... failed, error: " + (String)https.errorToString(httpCode) + "\n");
    }

    printDebug("\n[ANTARES] Closing connection...\n");
    https.end();
}

String AntaresESPHTTP::getRaw(String projectName, String deviceName)
{

    String payload = "";
    _getSuccess = false;
    printDebug("\n[ANTARES] Connecting to " + _serverNoHttp + "\n");

    // Use WiFiClient class to create TCP connections
    // WiFiClientSecure client;
    HTTPClient https;

    /*
    #if defined(ESP32)
        const char *ca_cert = cert_AlphaSSL_CA___SHA256___G4;
        client.setCACert(ca_cert);
    #elif defined(ESP8266)
        client.setFingerprint(fingerprint___antares_id);
    #endif*/

    // const int httpPort = 8443;
    // String httpUri = "/~/antares-cse/antares-id/" + projectName + "/" + deviceName + "/la";

    String httpUri;
    httpUri = "https://platform.antares.id:8443/~/antares-cse/antares-id/" + projectName + "/" + deviceName + "/la";

    if (!https.begin(httpUri))
    {
        printDebug("[ANTARES] Connection failed!\n");
        return payload;
    }
    printDebug("[ANTARES] Connection success!\n");
    printDebug("Requesting URI: " + httpUri + "\n");

    https.addHeader("X-M2M-Origin", _accessKey);
    https.addHeader("Content-Type", "application/json;ty=4");
    https.addHeader("Accept", "application/json");
    https.addHeader("Connection", "close");

    // This will send the request to the server
    int httpCode = https.GET();
    if (httpCode > 0)
    {
        printDebug("[ANTARES] GET... code: " + (String)httpCode + "\n");

        if (httpCode == 200)
        {
            printDebug("[ANTARES] Getting data success\n");
            payload = https.getString();
            DynamicJsonDocument jsonDoc(1024);
            deserializeJson(jsonDoc, payload);
            String receivedData = jsonDoc["m2m:cin"]["con"];
            payload = receivedData;
            _getSuccess = true;
        }
        else
        {
            printDebug("[ANTARES] Getting data failed\n");
        }
    }
    else
    {
        printDebug("[ANTARES] GET... failed, error: " + (String)https.errorToString(httpCode) + "\n");
    }

    printDebug("[ANTARES] Closing connection...\n");
    https.end();

    return payload;
}

String AntaresESPHTTP::getRawNonSecure(String projectName, String deviceName)
{

    String payload = "";
    _getSuccess = false;
    printDebug("\n[ANTARES] Connecting to " + _serverNoHttp + "\n");

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    HTTPClient http;

    String httpUri = "/~/antares-cse/antares-id/" + projectName + "/" + deviceName + "/la";
    if (!http.begin(client, _serverNoHttp, _portNum, httpUri, false))
    {
        printDebug("[ANTARES] Connection failed!\n");
        return payload;
    }
    printDebug("[ANTARES] Connection success!\n");
    printDebug("Requesting URI: " + httpUri + "\n");

    http.addHeader("X-M2M-Origin", _accessKey);
    http.addHeader("Content-Type", "application/json;ty=4");
    http.addHeader("Accept", "application/json");
    http.addHeader("Connection", "close");

    // This will send the request to the server
    int httpCode = http.GET();
    if (httpCode > 0)
    {
        printDebug("[ANTARES] GET... code: " + (String)httpCode + "\n");

        if (httpCode == 200)
        {
            printDebug("[ANTARES] Getting data success\n");
            payload = http.getString();
            DynamicJsonDocument jsonDoc(1024);
            deserializeJson(jsonDoc, payload);
            String receivedData = jsonDoc["m2m:cin"]["con"];
            payload = receivedData;
            _getSuccess = true;
        }
        else
        {
            printDebug("[ANTARES] Getting data failed\n");
        }
    }
    else
    {
        printDebug("[ANTARES] GET... failed, error: " + (String)http.errorToString(httpCode) + "\n");
    }

    printDebug("[ANTARES] Closing connection...\n");
    http.end();

    return payload;
}

void AntaresESPHTTP::getNonSecure(String projectName, String deviceName)
{

    jsonGetString = "";
    _getSuccess = false;
    printDebug("\n[ANTARES] Connecting to " + _serverNoHttp + "\n");

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    HTTPClient http;

    String httpUri = "/~/antares-cse/antares-id/" + projectName + "/" + deviceName + "/la";
    if (!http.begin(client, _serverNoHttp, _portNum, httpUri, false))
    {
        printDebug("[ANTARES] Connection failed!\n");
        return;
    }
    printDebug("[ANTARES] Connection success!\n");
    printDebug("Requesting URI: " + httpUri + "\n");

    http.addHeader("X-M2M-Origin", _accessKey);
    http.addHeader("Content-Type", "application/json;ty=4");
    http.addHeader("Accept", "application/json");
    http.addHeader("Connection", "close");

    // This will send the request to the server
    int httpCode = http.GET();
    if (httpCode > 0)
    {
        printDebug("[ANTARES] GET... code: " + (String)httpCode + "\n");

        if (httpCode == 200)
        {
            printDebug("[ANTARES] Getting data success\n");
            String payload = http.getString();
            DynamicJsonDocument jsonDoc(1024);
            deserializeJson(jsonDoc, payload);
            String receivedData = jsonDoc["m2m:cin"]["con"];
            DynamicJsonDocument jsonDocCon(1024);
            deserializeJson(jsonDocCon, receivedData);
            if (_debug)
            {
                serializeJson(jsonDocCon, jsonGetString);
                serializeJsonPretty(jsonDocCon, Serial);
            }
            _getSuccess = true;
        }
        else
        {
            printDebug("[ANTARES] Getting data failed\n");
        }
    }
    else
    {
        printDebug("[ANTARES] GET... failed, error: " + (String)http.errorToString(httpCode) + "\n");
    }

    printDebug("\n[ANTARES] Closing connection...\n");
    http.end();
}

String AntaresESPHTTP::getString(String key)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonGetString);
    return jsonDoc[key];
}

int AntaresESPHTTP::getInt(String key)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonGetString);
    return jsonDoc[key];
}

float AntaresESPHTTP::getFloat(String key)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonGetString);
    return jsonDoc[key];
}

double AntaresESPHTTP::getDouble(String key)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonGetString);
    return jsonDoc[key];
}

String AntaresESPHTTP::getString(String key, String key2)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonGetString);
    return jsonDoc[key][key2];
}

int AntaresESPHTTP::getInt(String key, String key2)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonGetString);
    int value = jsonDoc[key][key2];
    return value;
}

float AntaresESPHTTP::getFloat(String key, String key2)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonGetString);
    float value = jsonDoc[key][key2];
    return value;
}

double AntaresESPHTTP::getDouble(String key, String key2)
{
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, jsonGetString);
    double value = jsonDoc[key][key2];
    return value;
}

bool AntaresESPHTTP::getSuccess()
{
    return _getSuccess;
}

bool AntaresESPHTTP::wifiConnection(String SSID, String wifiPassword)
{
    _wifiSSID = SSID;
    _wifiPass = wifiPassword;

    WiFi.begin(SSID.c_str(), wifiPassword.c_str());
    printDebug("[ANTARES] Trying to connect to " + SSID + "...\n");

    int counter = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        printDebug(".");
        counter++;
        if (counter >= 10)
        {
            counter = 0;
            printDebug("[ANTARES] Could not connect to " + SSID + "! Retrying...\n");
        }
    }

    WiFi.setAutoReconnect(true);
    printDebug("\n[ANTARES] WiFi Connected!\n");
    printDebug("[ANTARES] IP Address: " + ipToString(WiFi.localIP()) + "\n");
    delay(1000);
    printDebug("[ANTARES] Setting time using SNTP\n");
    configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);

    while (now < 7 * 3600 * 2)
    {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    printDebug("\n");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    printDebug("Current time: ");
    printDebug(String(asctime(&timeinfo)));

    return true;
}

bool AntaresESPHTTP::checkWifiConnection()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        printDebug("[ANTARES] WIFI RECONNECT...");
        return wifiConnection(_wifiSSID, _wifiPass);
    }
    return true;
}

void AntaresESPHTTP::setDebug(bool trueFalse)
{
    _debug = trueFalse;
}

void AntaresESPHTTP::printDebug(String text)
{
    if (_debug)
        Serial.print(text);
}

String AntaresESPHTTP::ipToString(IPAddress ip)
{
    String s = "";
    for (int i = 0; i < 4; i++)
        s += i ? "." + String(ip[i]) : String(ip[i]);
    return s;
}