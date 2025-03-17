#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map> // For hash map
#include <curl/curl.h>
#include "include/json.hpp"
#include <vector>
#include <windows.h> // For ShellExecute on Windows
#include <cstdlib>
#include <sstream>
#include <ctime> // For timestamp

using namespace std;
using json = nlohmann::json;

// Callback function to handle the response from cURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* outBuffer) {
    size_t totalSize = size * nmemb;
    outBuffer->append((char*)contents, totalSize);
    return totalSize;
}

// Function to open a URL in the default browser
void openURL(const string& url) {
#ifdef _WIN32
    ShellExecute(0, "open", url.c_str(), 0, 0, SW_SHOWNORMAL);
#endif
}

// Custom Queue for search history
class Queue {
private:
    struct Node {
        string data;
        Node* next;
        Node(const string& d) : data(d), next(nullptr) {}
    };
    Node* front;
    Node* rear;

public:
    Queue() : front(nullptr), rear(nullptr) {}

    void enqueue(const string& data) {
        Node* newNode = new Node(data);
        if (rear == nullptr) {
            front = rear = newNode;
        } else {
            rear->next = newNode;
            rear = newNode;
        }
    }

    string dequeue() {
        if (front == nullptr) {
            return ""; // Queue is empty
        }
        Node* temp = front;
        string data = temp->data;
        front = front->next;
        if (front == nullptr) {
            rear = nullptr;
        }
        delete temp;
        return data;
    }

    bool isEmpty() {
        return front == nullptr;
    }

    void display() {
        Node* temp = front;
        cout << "Search History:\n";
        while (temp != nullptr) {
            cout << "- " << temp->data << "\n";
            temp = temp->next;
        }
    }
};

// Function to URL encode a string
string urlEncode(const string& value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << uppercase;
            escaped << '%' << setw(2) << int((unsigned char)c);
            escaped << nouppercase;
        }
    }

    return escaped.str();
}

// Function to fetch quick answers using DuckDuckGo API
string getQuickAnswer(const string& query) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    // URL encode the query
    string encodedQuery = urlEncode(query);

    // DuckDuckGo API endpoint
    string apiUrl = "https://api.duckduckgo.com/?q=" + encodedQuery + "&format=json";

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "cURL error: " << curl_easy_strerror(res) << endl;
            return "";
        }

        curl_easy_cleanup(curl);
    } else {
        cerr << "cURL initialization failed!" << endl;
        return "";
    }

    // Parse the JSON response
    try {
        json responseJson = json::parse(readBuffer);


        if (responseJson.contains("AbstractText") && !responseJson["AbstractText"].empty()) {
            return responseJson["AbstractText"];
        } else {
            cerr << "No abstract text found in the API response." << endl;
        }
    } catch (json::parse_error& e) {
        cerr << "JSON parse error: " << e.what() << endl;
    }

    return "";
}

int main() {
    // Initialize cURL
    CURL* curl;
    CURLcode res;
    string readBuffer;

    unordered_map<string, vector<pair<string, string>>> queryResults;

    // Load existing data from "search engine history.json" if it exists
    ifstream inFile("search engine history.json");
    if (inFile) {
        try {
            json jsonData;
            inFile >> jsonData;
            queryResults = jsonData.get<unordered_map<string, vector<pair<string, string>>>>();
        } catch (const exception& e) {
            cerr << "Error loading search engine history.json: " << e.what() << endl;
        }
    }
    inFile.close();



 while (true) {
        readBuffer.clear();

        // User input
        string query;
        cout << "Enter search query (or type 'exit' to quit): ";
        getline(cin, query);

        if (query == "exit") {
            break;
        }

    // Save the query to the search history file
    ofstream historyFile("search_history.txt", ios::app);
    if (historyFile.is_open()) {
        time_t now = time(0);
        string timestamp = ctime(&now);
        timestamp.pop_back(); // Remove the newline character from the timestamp
        historyFile << "[" << timestamp << "] " << query << "\n";
        historyFile.close();
    } else {
        cerr << "Unable to save search history.\n";
    }

    // Fetch quick answer
    string quickAnswer = getQuickAnswer(query);
    if (!quickAnswer.empty()) {
        cout << "\n\nQuick Answer:\n\n";
        cout << "> " << quickAnswer << "\n\n";
    } else {
        cout << "No quick answer found.\n";
    }

//first check data if it the in the cache (search engine history)

 if (queryResults.find(query) != queryResults.end()) {
    int index = 1; // Start numbering results
    cout << "\nCached Results for: " << query << "\n";
    for (const auto& result : queryResults[query]) {
    cout <<endl<< index << ". Title: " << result.first << "\n";
    cout << "   Link: " << result.second << "\n\n";
        index++;
    }
}
   
   else{

    // Google Custom Search API Key and Search Engine ID
   const string apiKey = "AIzaSyCJvwgP-_u3ODQ9Gxn787ehBqbhQIHBd8M"; 
             const string searchEngineId = "70d6240c838cb4907"; 

    // Construct the API URL
    string apiUrl = "https://www.googleapis.com/customsearch/v1?q=" + urlEncode(query) +
                        "&key=" + apiKey + "&cx=" + searchEngineId;

    curl = curl_easy_init();
    if (curl) {
        // Set up cURL options
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the HTTP request
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            cerr << "cURL error: " << curl_easy_strerror(res) << endl;
        } 
        else {
            try {
                // Parse the JSON response
                json responseJson = json::parse(readBuffer);

                if (responseJson.contains("items")) {
                    vector<pair<string, string>> Result; // Vector to store search results
                    Queue searchHistoryQueue; // Queue for search history

                    // Add query to queue
                    searchHistoryQueue.enqueue(query);

                    int index = 1;

                    // Display search results with numbers
                    cout << "\nSearch Results:\n";
                    for (const auto& item : responseJson["items"]) {
                        string title = item["title"];
                        string link = item["link"];

                        cout <<endl<< index << ". Title: " << title << "\n";
                        cout << "   Link: " << link << "\n\n";

                        Result.push_back({title, link}); // Add title and link to vector
                        index++;
                    }

                    
                    // Ask user to select a link by number
                    cout << "Enter the number of the link you want to open (or 0 to skip): ";
                    int choice;
                    cin >> choice;
                    cin.ignore(); // Ignore the newline character after the number
                    if (cin.fail() || choice < 0 || choice > Result.size()) {
                          cin.clear(); // Clear error state
                          cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore invalid input
                          cout << "Invalid choice!\n";
                          continue;
                }
                    if (choice > 0 && choice <= Result.size()) {
                        string selectedLink = Result[choice - 1].second;
                        openURL(selectedLink); // Open the selected link
                    } else if (choice != 0) {
                     cout << "Invalid choice!\n";
                    }

                     //storing memory in hash tabke

                     queryResults[query] = Result;
                      // Save the updated hash map to "search engine history.json"
                           ofstream outFile("search engine history.json");
                           if (outFile) {
                           try {
                               json jsonData(queryResults); // Convert to JSON
                               outFile << jsonData.dump(4); // Save with indentation
                           } catch (const exception& e) {
                               cerr << "Error writing to search engine history.json: " << e.what() << endl;
                           }
                           }
                           outFile.close();




                    // Ask the user if they want to see the search history
                    cout << "Do you want to see the search history? (y/n): ";
                    char showHistory;
                    cin >> showHistory;
                    cin.ignore(); // Ignore the newline character after the input

                    if (showHistory == 'y' || showHistory == 'Y') {
                        // Display search history from file using queue
                        ifstream historyFile("search_history.txt");
                        if (historyFile.is_open()) {
                            string line;
                            Queue historyQueue; // Queue to store history from file
                            while (getline(historyFile, line)) {
                                historyQueue.enqueue(line); // Add each line to the queue
                            }
                            historyQueue.display(); // Display the history
                            historyFile.close();
                        } else {
                            cerr << "No search history found.\n";
                        }
                    }

                } else {
                    cout << "No results found.\n";
                }
            } catch (json::parse_error& e) {
                cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }

        // Clean up cURL
        curl_easy_cleanup(curl);
    } else {
        cerr << "cURL initialization failed!" << std::endl;
    }
   }
   }
    return 0;
}
