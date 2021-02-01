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
 * File:   QueryTemplate.cpp.cc
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 01-02-2021.
 */

#include "QueryTemplate.h"

#include <cctype>
#include <algorithm>
#include <limits>
#include <cassert>
#include <random>
#include <regex>

void QueryTemplate::parse(std::string line) {
    auto pos = line.find('\\');
    while (pos != std::string::npos) {
        add_literal(line.substr(0, pos));
        auto i = pos + 1;
        if (i >= line.size()) {
            std::cerr << "error: In template file. Incomplete template parameter." << std::endl;
            exit(-1);
        }
        template_type type = template_type::normal;
        if (line[i] == '=') {
            type = template_type::equal;
            ++i;
        } else if (line[i] == '!') {
            type = template_type::not_equal;
            ++i;
        }
        auto j = i;
        while (j < line.size() && std::isdigit(static_cast<unsigned char>(line[j]))) ++j;
        if (i == j) {
            std::cerr << "error: In template file. Template parameter is missing an index number" << std::endl;
            exit(-1);
        }
        auto index_number = std::stoul(line.substr(i, j - i));
        add_parameter(type, index_number);
        line = line.substr(j);
        pos = line.find('\\');
    }
    add_literal(line);
}

void QueryTemplate::generate(std::ostream& s, const std::vector<std::string>& names, size_t permutation_limit) const {
    auto num = std::count_if(_parameters.begin(), _parameters.end(), [](const auto& parameter){ return parameter._type != template_type::equal; });
    std::vector<size_t> current_permutation(num, 0);
    if (num == 0) {
        output_permutation(s, names, current_permutation); // If no template parameters, still print one copy of the query.
        return;
    }
    if (permutation_limit == 0) { // no limit.
        do {
            if (valid_permutation(current_permutation)) {
                output_permutation(s, names, current_permutation);
            }
        } while (next_permutation(current_permutation, names.size()));
    } else {
        std::vector<std::vector<size_t>> all_permutations; // TODO: Can we somehow do this more efficiently? Without computing all permutations. Yeah probably, but do we bother...
        do {
            if (valid_permutation(current_permutation)) {
                all_permutations.push_back(current_permutation);
            }
        } while (next_permutation(current_permutation, names.size()));

        std::vector<std::vector<size_t>> selected_permutations;
        selected_permutations.reserve(permutation_limit);
        std::sample(all_permutations.begin(), all_permutations.end(), std::back_inserter(selected_permutations), permutation_limit, std::mt19937{std::random_device{}()});
        for (const auto& selected : selected_permutations) {
            output_permutation(s, names, selected);
        }
    }
}

bool QueryTemplate::valid_permutation(const std::vector<size_t>& permutation) const {
    size_t perm_i = 0;
    for (auto parameter : _parameters) {
        switch (parameter._type) {
            case template_type::not_equal: {
                auto matches = find_permutation_indexes(parameter._index);
                if (std::count_if(matches.begin(), matches.end(), [&permutation, perm_i](const auto& i) {
                    return permutation[i] == permutation[perm_i];
                }) > 1) { // matches includes current element, so check > 1.
                    return false;
                }
                ++perm_i;
                break;
            }
            case template_type::normal:
                ++perm_i;
                break;
            case template_type::equal:
                break;
        }
    }
    return true;
}
bool QueryTemplate::next_permutation(std::vector<size_t>& permutation, size_t max_value) {
    for (auto it = permutation.rbegin(); it != permutation.rend(); ++it) {
        *it = *it + 1;
        if (*it == max_value) {
            *it = 0;
        } else {
            return true;
        }
    }
    return false;
}

inline std::string escape(const std::string& name){
    return std::regex_replace(name, std::regex(R"(')"), R"(\')");
}

void QueryTemplate::output_permutation(std::ostream& s, const std::vector<std::string>& names, const std::vector<size_t>& permutation) const {
    assert(_literal_parts.size() == _parameters.size() + 1);
    s << _literal_parts[0];
    size_t perm_i = 0;
    for (size_t param_i = 0; param_i < _parameters.size(); ++param_i) {
        size_t name_index;
        switch (_parameters[param_i]._type) {
            case template_type::normal:
            case template_type::not_equal:
                name_index = permutation[perm_i];
                ++perm_i;
                break;
            case template_type::equal:
                auto normal_index = find_normal_permutation_index(_parameters[param_i]._index);
                name_index = permutation[normal_index];
                break;
        }
        assert(name_index < names.size());
        s << "'" << escape(names[name_index]) << "'";
        s << _literal_parts[param_i+1];
    }
    s << std::endl;
}

[[nodiscard]] size_t QueryTemplate::find_normal_permutation_index(unsigned long index) const {
    size_t perm_i = 0;
    for (auto parameter : _parameters) {
        if (parameter._index == index && parameter._type == template_type::normal) {
            return perm_i;
        }
        if (parameter._type != template_type::equal) {
            ++perm_i;
        }
    }
    return std::numeric_limits<size_t>::max();
}

std::vector<size_t> QueryTemplate::find_permutation_indexes(unsigned long index) const {
    std::vector<size_t> result;
    size_t perm_i = 0;
    for (auto parameter : _parameters) {
        if (parameter._index == index && parameter._type != template_type::equal) {
            result.push_back(perm_i);
        }
        if (parameter._type != template_type::equal) {
            ++perm_i;
        }
    }
    return result;
}

void QueryTemplate::add_parameter(template_type type, size_t index) {
    switch (type) {
        case template_type::normal:
            if (std::find_if(_parameters.begin(), _parameters.end(),
                             [index](const auto& parameter){ return parameter._index == index; }) != _parameters.end()) {
                std::cerr << "error: In template file. Normal parameter with index " << index << " declared twice." << std::endl;
                exit(-1);
            }
            break;
        case template_type::equal:
            if (std::find_if(_parameters.begin(), _parameters.end(),
                             [index](const auto& parameter){ return parameter._index == index && parameter._type == template_type::normal; }) == _parameters.end()) {
                std::cerr << "error: In template file. Equal parameter with index " << index << " needs a previous matching Normal parameter with the same index." << std::endl;
                exit(-1);
            }
            break;
        case template_type::not_equal:
            if (std::find_if(_parameters.begin(), _parameters.end(),
                             [index](const auto& parameter){ return parameter._index == index && parameter._type == template_type::normal; }) == _parameters.end()) {
                std::cerr << "error: In template file. Not-Equal parameter with index " << index << " needs a previous matching Normal parameter with the same index." << std::endl;
                exit(-1);
            }
            break;
    }
    _parameters.emplace_back(type, index);
}

void QueryTemplate::add_literal(const std::string& literal_part) {
    _literal_parts.push_back(literal_part);
}