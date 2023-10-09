#include <iostream>
#include "transcript_searcher.h"
#include "argparse/argparse.hpp"

#ifndef PROJECT_BASE_DIR
    #define PROJECT_BASE_DIR "../../"
#endif

int main(int argc, char** argv) {

    argparse::ArgumentParser program("transcript_searcher");
    program.add_argument("-c", "--config_file")
        .default_value(std::string{"config.json"});

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }
    std::string config_path = program.get<std::string>("--config_file");
    std::string full_config_path = PROJECT_BASE_DIR + config_path;
    TranscriptSearcher transcriptSearcher(full_config_path);
    transcriptSearcher.runSearch();
    return 0;
}
