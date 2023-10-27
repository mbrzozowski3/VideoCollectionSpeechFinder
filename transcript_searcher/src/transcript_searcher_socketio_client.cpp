#include "sio_client.h"
#include "transcript_searcher.h"

#include "argparse/argparse.hpp"

#include <unistd.h>
#include <iostream>
#include <string>
#include <algorithm>

class TranscriptSearcherSocketIoClient {
    public:
        TranscriptSearcherSocketIoClient(std::string database_path = "C:\\Users\\mikeb\\Workspace\\ProgrammingProjects\\Dialocate\\database\\application.db") : running(true) {
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
            running = false;
        }
    
        void on_fail() {
            std::cout << "Connection Failed" << std::endl;
            running = false;
        }

        void bind_events() {
            client.socket()->on("Message", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp) {
                if (data->get_flag() == sio::message::flag::flag_string) {
                    const sio::message::ptr response = sio::string_message::create("Message Response");
                    std::cout << data->get_string() << std::endl;
                    ack_resp = sio::message::list(response);
                }
            }));
            client.socket()->on("perform_search", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp) {
                std::vector<std::string> search_terms = {"donate", "lads", "feeling"};
                unsigned int num_best_results = 3;
                std::vector<scored_transcript> best_transcripts;
                perform_search(search_terms, num_best_results, best_transcripts);
                std::string json_response = "{\n\t\"files\": [\n";
                std::string files = "";
                std::string scores = "";
                for (unsigned int i = 0; i < best_transcripts.size(); i++) {
                    auto& element = best_transcripts[i];
                    std::string file = element.first;
                    std::replace(element.first.begin(), element.first.end(), '\\', '/');
                    files += "\t\t\"" + element.first + "\"";
                    scores += "\t\t" + std::to_string(element.second);
                    if (i != best_transcripts.size() - 1) {
                        files += ",";
                        scores += ",";
                    }
                    files += "\n";
                    scores += "\n";
                }
                json_response += files + "\t],\n\t";
                json_response += "\"scores\": [\n" + scores + "\t]\n}";
                std::cout << json_response << std::endl;
                client.socket()->emit("results", sio::string_message::create(json_response));
            }));
        }

        void close() {
            client.sync_close();
            client.clear_con_listeners();
        }

        bool isRunning() {
            return running;
        }

        ~TranscriptSearcherSocketIoClient() {
            delete transcript_search_algorithm;
        }

    private:
        sio::client client;
        bool running;
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
    // while (client.isRunning());
    while (true);
    client.close();
    return 0;
}