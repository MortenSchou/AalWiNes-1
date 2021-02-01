/* 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  Copyright Morten K. Schou
 */

/* 
 * File:   GenerateQueries.cpp.cc
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 01-02-2021.
 */

#include "QueryTemplate.h"

#include <aalwines/model/builders/NetworkParsing.h>

#include <boost/program_options.hpp>

#include <iostream>
#include <fstream>
#include <cctype>
#include <filesystem>

namespace po = boost::program_options;
using namespace aalwines;
using namespace pdaaal;

int main(int argc, const char** argv) {
    po::options_description opts;
    opts.add_options()
            ("help,h", "produce help message");

    NetworkParsing parser("Network Input Options");
    po::options_description generation("Generation Options");
    po::options_description output("Output Options");

    std::string template_file;
    bool all_permutations = false;
    size_t permutations = 1;
    generation.add_options()
            ("template,t", po::value<std::string>(&template_file), "File with templates for the queries to be generated.")
            ("all,a", po::bool_switch(&all_permutations), "If set, generates all permutations of each of the given templates.")
            ("limit,n", po::value<size_t>(&permutations), "Specify the number of random permutations to generate of each template query. Default=1. Ignored if -a is set.")
            ;

    std::string output_file, output_directory;
    output.add_options()
            ("output-file,o", po::value<std::string>(&output_file), "Output file for writing the queries.")
            ("output-directory,d", po::value<std::string>(&output_directory), "Output directory for writing a file per query.")
            ; // TODO: Add option for generating a file per query in a specifies directory.

    opts.add(parser.options());
    opts.add(generation);
    opts.add(output);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << opts << "\n";
        return 1;
    }

    if (template_file.empty()) {
        std::cerr << "Please specify template file for generating queries: --template <file-name>" << std::endl;
        exit(-1);
    }
    if (output_file.empty() && output_directory.empty()) {
        std::cerr << "Please specify either an output file: --output-file <file-name>, or an output directory: --output-directory <directory>" << std::endl;
        exit(-1);
    }
    if (!output_file.empty() && !output_directory.empty()) {
        std::cerr << "Please only specify one of output file and output directory. Both were specified: --output-file " << output_file << " --output-directory " << output_directory << std::endl;
        exit(-1);
    }

    auto network = parser.parse();

    std::vector<std::string> router_names;
    for (const auto& router : network.routers()) {
        if (router->is_null()) continue;
        router_names.emplace_back(router->name());
    }


    // Parse template file:
    std::ifstream template_stream(template_file);
    if (!template_stream.is_open()) {
        std::stringstream es;
        es << "error: Could not open file : " << template_file << std::endl;
        throw base_error(es.str());
    }
    std::vector<QueryTemplate> query_templates;
    std::string line;
    while (std::getline(template_stream, line)) {
        if (line.empty() || std::count_if(line.begin(), line.end(), [](unsigned char ch){ return !std::isspace(ch); }) == 0 || line.rfind("//", 0) == 0) continue;
        query_templates.emplace_back(all_permutations, permutations);
        query_templates.back().parse(line);
    }
    template_stream.close();

    if (!output_file.empty()) {
        std::ofstream output_stream(output_file);
        if (!output_stream.is_open()) {
            std::stringstream es;
            es << "error: Could not open --output-file " << output_file << " for writing" << std::endl;
            throw base_error(es.str());
        }
        for (const auto& query_template : query_templates) {
            query_template.generate(output_stream, router_names);
        }
    } else if (!output_directory.empty()) {
        std::filesystem::create_directory(output_directory);
        size_t i = 0;
        for (const auto& query_template : query_templates) {
            for (const auto& permutation : query_template.get_permutations(router_names)) {
                std::stringstream file_name;
                file_name << output_directory << "/" << i << ".q";
                std::ofstream output_stream(file_name.str());
                if (!output_stream.is_open()) {
                    std::stringstream es;
                    es << "error: Could not open file " << file_name.str() << " for writing" << std::endl;
                    throw base_error(es.str());
                }
                query_template.output_permutation(output_stream, router_names, permutation);
                ++i;
            }
        }
    }




}
