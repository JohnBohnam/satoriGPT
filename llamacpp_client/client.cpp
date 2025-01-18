#include <iostream>
#include <string>
#include <sstream>
#include <curl/curl.h>
#include "json.hpp"

using namespace std;

class LLamaClient {
    string hostAddress;
    static size_t writeCallback(char *data, size_t size, size_t nmemb, string *buffer) {
        string str = string(data);
        // cout << "Received data: " << str << "END" << endl;
        int start = str.find_first_of('{');
        if (start == string::npos) {
            cout << "No JSON data found" << endl;
            return 0;
        }
        str = str.substr(start);
        int cnt = 0;
        bool inQuotes = false;
        bool specialChar = false;
        for (int i=0; i<str.size(); i++) {
            if (specialChar) {
                specialChar = false;
                continue;
            }

            if (str[i] == '"') {
                inQuotes = !inQuotes;
            }

            if (str[i] == '\\') {
                if (i > 0 && str[i-1] == '\\') {
                    specialChar = false;
                } else {
                    specialChar = true;
                }
            }
            if (inQuotes) continue;

            if (str[i] == '{') cnt++;
            else if (str[i] == '}') cnt--;
            if (cnt == 0) {
                str = str.substr(0, i+1);
                break;
            }
        }
        // cout << "JSON data: " << str << "END" << endl;
        nlohmann::json res = nlohmann::json::parse(str);
        // cout << "Parsed JSON: " << endl;
        // cout << res.dump(2) << endl;
        // res.
        // cout << "Content " << res["content"] << " - " << string(res["content"]) << endl;
        string response = string(res["content"]);
        // .substr(0, string(res["content"]).size()-1);
        cout << response;
        fflush(stdout);
        if (buffer) {
            buffer->append(data, size * nmemb);
            return size * nmemb;
        }
        return 0;
    }

public:
    LLamaClient(string hostAddress="127.0.0.1:8080") : hostAddress(hostAddress) {}

    string prompt(string promptText) {
        CURL *curl;
        CURLcode res;
        curl = curl_easy_init();

        if (!curl) {
            cerr << "CURL initialization failed" << endl;
            return "";
        }

        string url = "http://" + hostAddress + "/completion";
        string jsonPayload = "{"
            "\"prompt\": \"" + promptText + "\""
            // " ,\"temperature\": 0.8, "
            // "\"top_k\": 40, "
            // "\"top_p\": 0.95"
            // ",\"return_tokens\": true"
            ",\"stream\": true"
        "}";

        string responseBuffer;


        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        cout << "performing CURL request" << endl;

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "CURL request failed: " << curl_easy_strerror(res) << endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        return responseBuffer;
    }
};

int main() {
    LLamaClient client;
    string promptText = "Write a c program that prints 'Hello, World!' to the console.";
    string result = client.prompt(promptText);

    cout << "Response: " << result << endl;
    return 0;
}