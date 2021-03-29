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
 * File:   QueryTemplate.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 01-02-2021.
 */

#ifndef AALWINES_QUERYTEMPLATE_H
#define AALWINES_QUERYTEMPLATE_H

#include <iostream>
#include <string>
#include <vector>
#include <limits>

// Implements a simple template based query generator.
// In the string, template parameters can be specified using "\<number>" (e.g. "\1"). This is a 'normal' parameter,
// and specifies that the generator should iterate over the router names and insert each on this place.
// The parameter "\=<number>" (e.g. "\=1") can be used to repeat the value of a parameter previously define by "\1"
// The parameter "\!<number>" (e.g. "\!1") iterates over all values except the one currently hold by the parameter previously defined by "\1".
class QueryTemplate {
    enum class template_type {normal, equal, not_equal};
    struct template_parameter {
        template_type _type;
        unsigned long _index;
        size_t _current_value = 0;
        template_parameter(template_type type, size_t index) : _type(type), _index(index) {};
    };

public:
    QueryTemplate() = default;
    QueryTemplate(bool all_permutations, size_t permutations)
    : _permutation_limit(all_permutations ? std::numeric_limits<size_t>::max() : permutations) {};

    void parse(std::string line);

    // TODO: Add version for one file per query...
    void generate(std::ostream& s, const std::vector<std::string>& names) const;

    [[nodiscard]] std::vector<std::vector<size_t>> get_permutations(const std::vector<std::string>& names);
    void output_permutation(std::ostream& s, const std::vector<std::string>& names, const std::vector<size_t>& permutation) const;

private:
    [[nodiscard]] bool valid_permutation(const std::vector<size_t>& permutation) const;
    static bool next_permutation(std::vector<size_t>& permutation, size_t max_value);
    [[nodiscard]] size_t find_normal_permutation_index(unsigned long index) const;
    [[nodiscard]] std::vector<size_t> find_permutation_indexes(unsigned long index) const;
    void add_parameter(template_type type, size_t index);
    void add_literal(const std::string& literal_part);
    [[nodiscard]] bool all_permutations() const {
        return _permutation_limit == std::numeric_limits<size_t>::max();
    }

    size_t _permutation_limit = std::numeric_limits<size_t>::max();
    std::vector<std::string> _literal_parts;
    std::vector<template_parameter> _parameters;
};


#endif //AALWINES_QUERYTEMPLATE_H
