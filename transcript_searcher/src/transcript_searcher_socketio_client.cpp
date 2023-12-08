#include "sio_client.h"
#include "transcript_searcher.h"

#include "argparse/argparse.hpp"

#include <unistd.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>

class TranscriptSearcherSocketIoClient {
    public:
        TranscriptSearcherSocketIoClient(std::string database_path) {
            client.set_open_listener(std::bind(&TranscriptSearcherSocketIoClient::on_connected, this));
            client.set_close_listener(std::bind(&TranscriptSearcherSocketIoClient::on_close, this, std::placeholders::_1));
            client.set_fail_listener(std::bind(&TranscriptSearcherSocketIoClient::on_fail, this));
            client.connect("http://127.0.0.1:8081");
            bind_events();
            transcript_search_algorithm = new TfIdfTranscriptSearch(database_path);
        }

        void perform_search(
            const std::vector<std::string>& search_terms,
            const unsigned int num_best_results,
            std::vector<scored_transcript>& best_transcripts) {
            transcript_search_algorithm->getBestTranscriptMatches(search_terms, num_best_results, best_transcripts);
        }
    
        void on_connected() {
            std::cout << "on_connected" << std::endl;
        }

        void on_close(sio::client::close_reason const& reason) {
            std::cout << "Connection Closed" << std::endl;
        }
    
        void on_fail() {
            std::cout << "Connection Failed" << std::endl;
        }

        std::string jsonify_results(std::vector<scored_transcript>& results, const std::chrono::nanoseconds duration_ns) {
            auto duration_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration_ns);
            std::string results_json = "{\n\t\"videos\": [\n";
            for (unsigned int i = 0; i < results.size(); i++) {
                auto& element = results[i];
                std::string file = element.first;
                std::replace(element.first.begin(), element.first.end(), '\\', '/');
                results_json += "\t\t{\n";
                results_json += "\t\t\t\"file\": \"" + element.first + "\",\n";
                results_json += "\t\t\t\"score\": " + std::to_string(element.second) + "\n";
                results_json += "\t\t}\n";
                if (i != results.size() - 1) {
                    results_json += ",";
                }
            }
            results_json += "\t],\n";
            results_json += "\t\"duration\": {\n\t\t\"count\": " + std::to_string(duration_milliseconds.count()) + ",\n";
            results_json += "\t\t\"unit\": \"ms\"\n";
            results_json += "\t}\n";
            results_json += "}";
            return results_json;
        }

        void perform_search_handler(std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp) {
            if (data->get_flag() == sio::message::flag::flag_array) {
                std::vector<std::string> search_terms;
                for (auto& message : data->get_vector()) {
                    search_terms.push_back(message->get_string());
                }
                unsigned int num_best_results = 3;
                std::vector<scored_transcript> best_transcripts;

                // Time the search function
                auto start_time = std::chrono::high_resolution_clock::now();

                perform_search(search_terms, num_best_results, best_transcripts);

                // Finish timing search function
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = end_time - start_time;

                std::string json_response = jsonify_results(best_transcripts, duration);
                std::cout << json_response << std::endl;
                client.socket()->emit("results", sio::string_message::create(json_response));
            }
        }

        void bind_events() {
            client.socket()->on("perform_search", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp) {
                perform_search_handler(name, data, isAck, ack_resp);
            }));
        }

        void close() {
            client.sync_close();
            client.clear_con_listeners();
        }

        ~TranscriptSearcherSocketIoClient() {
            delete transcript_search_algorithm;
        }

    private:
        sio::client client;
        TranscriptSearchAlgorithm* transcript_search_algorithm = nullptr;
};

int main (int argc, char** argv) {
    
    // Configure the CLI
    argparse::ArgumentParser program("transcript_searcher_socketio_client");
    program.add_argument("database_path").default_value(std::string{"application.db"});
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }

    std::string database_path = program.get<std::string>("database_path");

    TranscriptSearcherSocketIoClient client(database_path);
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    client.close();
    return 0;
}