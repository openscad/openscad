// generate_cpp.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_GENERATE_CPP_HPP
#define LEXERTL_GENERATE_CPP_HPP

#include "enums.hpp"
#include <sstream>
#include "state_machine.hpp"

namespace lexertl
{
    class table_based_cpp
    {
    public:
        template<typename char_type, typename id_type>
        static void generate_cpp
        (const std::string& name_,
            const basic_state_machine<char_type, id_type>& sm_,
            const bool pointers_, std::ostream& os_)
        {
            using sm = basic_state_machine<char_type, id_type>;
            using internals = typename sm::internals;
            const internals& internals_ = sm_.data();
            std::size_t additional_tabs_ = 0;

            os_ << "template<typename iter_type, typename id_type>\n";
            os_ << "void " << name_ << " (lexertl::";

            if (internals_._features & recursive_bit)
            {
                os_ << "recursive_match_results";
            }
            else
            {
                os_ << "match_results";
            }

            os_ << "<iter_type, id_type> &results_)\n";
            os_ << "{\n";
            os_ << "    using results = lexertl::";

            if (internals_._features & recursive_bit)
            {
                os_ << "recursive_match_results";
            }
            else
            {
                os_ << "match_results";
            }

            os_ << "<iter_type, id_type>;\n";
            os_ << "    using char_type = typename results::char_type;\n";
            os_ << "    typename results::iter_type end_token_ = "
                "results_.second;\n";

            if (internals_._features & skip_bit)
            {
                os_ << "skip:\n";
            }

            os_ << "    typename results::iter_type curr_ = "
                "results_.second;\n\n";
            os_ << "    results_.first = curr_;\n\n";

            if (internals_._features & again_bit)
            {
                os_ << "again:\n";
            }

            os_ << "    if (curr_ == results_.eoi)\n";
            os_ << "    {\n";
            // We want a number regardless of id_type.
            os_ << "        results_.id = " << static_cast<std::size_t>
                (internals_._eoi) << ";\n";
            os_ << "        results_.user_id = results::npos();\n";
            os_ << "        return;\n";
            os_ << "    }\n\n";

            if (internals_._features & bol_bit)
            {
                os_ << "    bool bol_ = results_.bol;\n";
            }

            dump_tables(sm_, 1, pointers_, os_);

            if (internals_._dfa.size() > 1)
            {
                os_ << "    const id_type *lookup_ = "
                    "lookups_[results_.state];\n";
                os_ << "    const id_type dfa_alphabet_ = dfa_alphabets_"
                    "[results_.state];\n";
                os_ << "    const ";

                if (pointers_)
                {
                    os_ << "void * const";
                }
                else
                {
                    os_ << "id_type";
                }

                os_ << " *dfa_ = dfas_[results_.state];\n";
            }

            os_ << "    const ";

            if (pointers_)
            {
                os_ << "void * const";
            }
            else
            {
                os_ << "id_type";
            }

            os_ << " *ptr_ = dfa_ + dfa_alphabet_;\n";
            os_ << "    bool end_state_ = *ptr_ != 0;\n";

            if (internals_._features & recursive_bit)
            {
                os_ << "    bool pop_ = (";

                if (pointers_)
                {
                    // Done this way for GCC:
                    os_ << "static_cast<id_type>(reinterpret_cast<ptrdiff_t>(";
                }

                os_ << "*ptr_";

                if (pointers_)
                {
                    os_ << ')';
                }

                os_ << " & " << pop_dfa_bit;

                if (pointers_)
                {
                    os_ << ')';
                }

                os_ << ") != 0;\n";
            }

            os_ << "    id_type id_ = ";

            if (pointers_)
            {
                // Done this way for GCC:
                os_ << "static_cast<id_type>(reinterpret_cast<ptrdiff_t>(";
            }

            os_ << "*(ptr_ + " << id_index << ")";

            if (pointers_)
            {
                os_ << "))";
            }

            os_ << ";\n";
            os_ << "    id_type uid_ = ";

            if (pointers_)
            {
                // Done this way for GCC:
                os_ << "static_cast<id_type>(reinterpret_cast<ptrdiff_t>(";
            }

            os_ << "*(ptr_ + " << user_id_index << ")";

            if (pointers_)
            {
                os_ << "))";
            }

            os_ << ";\n";

            if (internals_._features & recursive_bit)
            {
                os_ << "    id_type push_dfa_ = ";

                if (pointers_)
                {
                    // Done this way for GCC:
                    os_ << "static_cast<id_type>(reinterpret_cast<ptrdiff_t>(";
                }

                os_ << "*(ptr_ + " << push_dfa_index << ")";

                if (pointers_)
                {
                    os_ << "))";
                }

                os_ << ";\n";
            }

            if (internals_._dfa.size() > 1)
            {
                os_ << "    id_type start_state_ = results_.state;\n";
            }

            if (internals_._features & bol_bit)
            {
                os_ << "    bool end_bol_ = bol_;\n";
            }

            if (internals_._features & eol_bit)
            {
                os_ << "    ";

                if (pointers_)
                {
                    os_ << "const void * const *";
                }
                else
                {
                    os_ << "id_type ";
                }

                os_ << "EOL_state_ = 0;\n";
            }

            os_ << '\n';

            if (internals_._features & bol_bit)
            {
                os_ << "    if (bol_)\n";
                os_ << "    {\n";
                os_ << "        const ";

                if (pointers_)
                {
                    os_ << "void *";
                }
                else
                {
                    os_ << "id_type ";
                }

                os_ << "state_ = *dfa_;\n\n";
                os_ << "        if (state_)\n";
                os_ << "        {\n";
                os_ << "            ptr_ = ";

                if (pointers_)
                {
                    os_ << "reinterpret_cast<void * const *>(state_);\n";
                }
                else
                {
                    os_ << "&dfa_[state_ * dfa_alphabet_];\n";
                }

                os_ << "        }\n";
                os_ << "    }\n\n";
            }

            os_ << "    while (curr_ != results_.eoi)\n";
            os_ << "    {\n";

            if (internals_._features & eol_bit)
            {
                os_ << "        EOL_state_ = ";

                if (pointers_)
                {
                    os_ << "reinterpret_cast<const void * const *>(";
                }

                os_ << "ptr_[" << eol_index << ']';

                if (pointers_)
                {
                    os_ << ')';
                }

                os_ << ";\n\n";
                os_ << "        if (EOL_state_ && *curr_ == '\\n')\n";
                os_ << "        {\n";
                os_ << "            ptr_ = ";

                if (pointers_)
                {
                    os_ << "EOL_state_";
                }
                else
                {
                    os_ << "&dfa_[EOL_state_ * dfa_alphabet_]";
                }

                os_ << ";\n";
                os_ << "        }\n";
                os_ << "        else\n";
                os_ << "        {\n";
                ++additional_tabs_;
            }

            output_char_loop(internals_._features, additional_tabs_, pointers_,
                os_, std::integral_constant<bool,
                (sizeof(typename sm::traits::input_char_type) > 1)>());

            if (internals_._features & eol_bit)
            {
                output_tabs(additional_tabs_, os_);
                os_ << "    }\n";
                --additional_tabs_;
            }

            os_ << '\n';
            os_ << "        if (*ptr_)\n";
            os_ << "        {\n";
            os_ << "            end_state_ = true;\n";


            if (internals_._features & recursive_bit)
            {
                os_ << "            pop_ = (";

                if (pointers_)
                {
                    // Done this way for GCC:
                    os_ << "static_cast<id_type>(reinterpret_cast<ptrdiff_t>(";
                }

                os_ << "*ptr_";

                if (pointers_)
                {
                    os_ << ')';
                }

                os_ << " & " << pop_dfa_bit;

                if (pointers_)
                {
                    os_ << ')';
                }

                os_ << ") != 0;\n";
            }

            os_ << "            id_ = ";

            if (pointers_)
            {
                // Done this way for GCC:
                os_ << "static_cast<id_type>(reinterpret_cast<ptrdiff_t>(";
            }

            os_ << "*(ptr_ + " << id_index << ")";

            if (pointers_)
            {
                os_ << "))";
            }

            os_ << ";\n";
            os_ << "            uid_ = ";

            if (pointers_)
            {
                // Done this way for GCC:
                os_ << "static_cast<id_type>(reinterpret_cast<ptrdiff_t>(";
            }

            os_ << "*(ptr_ + " << user_id_index << ")";

            if (pointers_)
            {
                os_ << "))";
            }

            os_ << ";\n";

            if (internals_._features & recursive_bit)
            {
                os_ << "            push_dfa_ = ";

                if (pointers_)
                {
                    // Done this way for GCC:
                    os_ << "static_cast<id_type>(reinterpret_cast<ptrdiff_t>(";
                }

                os_ << "*(ptr_ + " << push_dfa_index << ')';

                if (pointers_)
                {
                    os_ << "))";
                }

                os_ << ";\n";
            }

            if (internals_._dfa.size() > 1)
            {
                os_ << "            start_state_ = ";

                if (pointers_)
                {
                    // Done this way for GCC:
                    os_ << "static_cast<id_type>(reinterpret_cast<ptrdiff_t>(";
                }

                os_ << "*(ptr_ + " << next_dfa_index << ')';

                if (pointers_)
                {
                    os_ << "))";
                }

                os_ << ";\n";
            }

            if (internals_._features & bol_bit)
            {
                os_ << "            end_bol_ = bol_;\n";
            }

            os_ << "            end_token_ = curr_;\n";
            os_ << "        }\n";
            os_ << "    }\n\n";
            output_quit(os_, std::integral_constant<bool,
                (sizeof(typename sm::traits::input_char_type) > 1)>());

            if (internals_._features & eol_bit)
            {
                os_ << "    if (curr_ == results_.eoi)\n";
                os_ << "    {\n";
                os_ << "        EOL_state_ = ";

                if (pointers_)
                {
                    os_ << "reinterpret_cast<const void * const *>(";
                }

                os_ << "ptr_[" << eol_index << ']';

                if (pointers_)
                {
                    os_ << ')';
                }

                os_ << ";\n";
                os_ << "\n";
                os_ << "        if (EOL_state_)\n";
                os_ << "        {\n";
                os_ << "            ptr_ = ";

                if (pointers_)
                {
                    os_ << "EOL_state_";
                }
                else
                {
                    os_ << "&dfa_[EOL_state_ * dfa_alphabet_]";
                }

                os_ << ";\n\n";
                os_ << "            if (*ptr_)\n";
                os_ << "            {\n";
                os_ << "                end_state_ = true;\n";


                if (internals_._features & recursive_bit)
                {
                    os_ << "                pop_ = (";

                    if (pointers_)
                    {
                        // Done this way for GCC:
                        os_ << "static_cast<id_type>"
                            "(reinterpret_cast<ptrdiff_t>(";
                    }

                    os_ << "*ptr_";

                    if (pointers_)
                    {
                        os_ << ')';
                    }

                    os_ << " & " << pop_dfa_bit;

                    if (pointers_)
                    {
                        os_ << ')';
                    }

                    os_ << ") != 0;\n";
                }

                os_ << "                id_ = ";

                if (pointers_)
                {
                    // Done this way for GCC:
                    os_ << "static_cast<id_type>(reinterpret_cast<ptrdiff_t>(";
                }

                os_ << "*(ptr_ + " << id_index << ")";

                if (pointers_)
                {
                    os_ << "))";
                }

                os_ << ";\n";
                os_ << "                uid_ = ";

                if (pointers_)
                {
                    // Done this way for GCC:
                    os_ << "static_cast<id_type>(reinterpret_cast<ptrdiff_t>(";
                }

                os_ << "*(ptr_ + " << user_id_index << ")";

                if (pointers_)
                {
                    os_ << "))";
                }

                os_ << ";\n";

                if (internals_._features & recursive_bit)
                {
                    os_ << "                push_dfa_ = ";

                    if (pointers_)
                    {
                        // Done this way for GCC:
                        os_ << "static_cast<id_type>"
                            "(reinterpret_cast<ptrdiff_t>(";
                    }

                    os_ << "*(ptr_ + " << push_dfa_index << ')';

                    if (pointers_)
                    {
                        os_ << "))";
                    }

                    os_ << ";\n";
                }

                if (internals_._dfa.size() > 1)
                {
                    os_ << "                start_state_ = ";

                    if (pointers_)
                    {
                        // Done this way for GCC:
                        os_ << "static_cast<id_type>"
                            "(reinterpret_cast<ptrdiff_t>(";
                    }

                    os_ << "*(ptr_ + " << next_dfa_index << ')';

                    if (pointers_)
                    {
                        os_ << "))";
                    }

                    os_ << ";\n";
                }

                if (internals_._features & bol_bit)
                {
                    os_ << "                end_bol_ = bol_;\n";
                }

                os_ << "                end_token_ = curr_;\n";
                os_ << "            }\n";
                os_ << "        }\n";
                os_ << "    }\n\n";
            }

            os_ << "    if (end_state_)\n";
            os_ << "    {\n";
            os_ << "        // Return longest match\n";

            if (internals_._features & recursive_bit)
            {
                os_ << "        if (pop_)\n";
                os_ << "        {\n";
                os_ << "            start_state_ =  results_."
                    "stack.top().first;\n";
                os_ << "            results_.stack.pop();\n";
                os_ << "        }\n";
                os_ << "        else if (push_dfa_ != results_.npos())\n";
                os_ << "        {\n";
                os_ << "            results_.stack.push(typename results::"
                    "id_type_pair\n";
                os_ << "                (push_dfa_, id_));\n";
                os_ << "        }\n\n";
            }

            if (internals_._dfa.size() > 1)
            {
                os_ << "        results_.state = start_state_;\n";
            }

            if (internals_._features & bol_bit)
            {
                os_ << "        results_.bol = end_bol_;\n";
            }

            os_ << "        results_.second = end_token_;\n";

            if (internals_._features & skip_bit)
            {
                // We want a number regardless of id_type.
                os_ << "\n        if (id_ == results_.skip()) goto skip;\n";
            }

            if (internals_._features & again_bit)
            {
                // We want a number regardless of id_type.
                os_ << "\n        if (id_ == "
                    << static_cast<std::size_t>(internals_._eoi);

                if (internals_._features & recursive_bit)
                {
                    os_ << " || (pop_ && !results_.stack.empty() &&\n";
                    // We want a number regardless of id_type.
                    os_ << "            results_.stack.top().second == "
                        << static_cast<std::size_t>(internals_._eoi) << ')';
                }

                os_ << ")\n";
                os_ << "        {\n";
                os_ << "            curr_ = end_token_;\n";
                os_ << "            goto again;\n";
                os_ << "        }\n";
            }

            os_ << "    }\n";
            os_ << "    else\n";
            os_ << "    {\n";
            os_ << "        // No match causes char to be skipped\n";
            os_ << "        results_.second = end_token_;\n";

            if (internals_._features & bol_bit)
            {
                os_ << "        results_.bol = *results_.second == '\\n';\n";
            }

            os_ << "        results_.first = results_.second;\n";
            os_ << "        ++results_.second;\n";
            os_ << "        id_ = results::npos();\n";
            os_ << "        uid_ = results::npos();\n";
            os_ << "    }\n\n";
            os_ << "    results_.id = id_;\n";
            os_ << "    results_.user_id = uid_;\n";
            os_ << "}\n";
        }

        template<typename char_type, typename id_type>
        static void dump_tables
        (const basic_state_machine<char_type, id_type>& sm_,
            const std::size_t tabs_, const bool pointers_, std::ostream& os_)
        {
            const typename detail::basic_internals<id_type>& internals_ =
                sm_.data();
            const std::size_t lookup_divisor_ = 8;
            // Lookup is always 256 entries long now
            const std::size_t lookup_quotient_ = 256 / lookup_divisor_;
            const std::size_t dfas_ = internals_._lookup.size();

            output_tabs(tabs_, os_);
            os_ << "static const id_type lookup";

            if (dfas_ > 1)
            {
                os_ << "s_[][" << 256;
            }
            else
            {
                os_ << "_[";
            }

            os_ << "] = \n";
            output_tabs(tabs_ + 1, os_);

            if (dfas_ > 1)
            {
                os_ << '{';
            }

            for (std::size_t l_ = 0; l_ < dfas_; ++l_)
            {
                const id_type* ptr_ = &internals_._lookup[l_].front();

                // We want numbers regardless of id_type.
                os_ << "{0x" << std::hex << static_cast<std::size_t>(*ptr_++);

                for (std::size_t col_ = 1; col_ < lookup_divisor_; ++col_)
                {
                    // We want numbers regardless of id_type.
                    os_ << ", 0x" << std::hex <<
                        static_cast<std::size_t>(*ptr_++);
                }

                for (std::size_t row_ = 1; row_ < lookup_quotient_; ++row_)
                {
                    os_ << ",\n";
                    output_tabs(tabs_ + 1, os_);
                    // We want numbers regardless of id_type.
                    os_ << "0x" << std::hex <<
                        static_cast<std::size_t>(*ptr_++);

                    for (std::size_t col_ = 1; col_ < lookup_divisor_; ++col_)
                    {
                        // We want numbers regardless of id_type.
                        os_ << ", 0x" << std::hex <<
                            static_cast<std::size_t>(*ptr_++);
                    }
                }

                os_ << '}';

                if (l_ + 1 < dfas_)
                {
                    os_ << ",\n";
                    output_tabs(tabs_ + 1, os_);
                }
            }

            if (dfas_ > 1)
            {
                os_ << '}';
            }

            os_ << ";\n";
            output_tabs(tabs_, os_);
            os_ << "static const id_type dfa_alphabet";

            if (dfas_ > 1)
            {
                os_ << "s_[" << std::dec << dfas_ << "] = {";
            }
            else
            {
                os_ << "_ = ";
            }

            // We want numbers regardless of id_type.
            os_ << "0x" << std::hex << static_cast<std::size_t>
                (internals_._dfa_alphabet[0]);

            for (std::size_t col_ = 1; col_ < dfas_; ++col_)
            {
                // We want numbers regardless of id_type.
                os_ << ", 0x" << std::hex <<
                    static_cast<std::size_t>(internals_._dfa_alphabet[col_]);
            }

            if (dfas_ > 1)
            {
                os_ << '}';
            }

            os_ << ";\n";

            // DFAs are usually different sizes, so dump separately
            for (std::size_t dfa_ = 0; dfa_ < dfas_; ++dfa_)
            {
                const id_type dfa_alphabet_ = internals_._dfa_alphabet[dfa_];
                const std::size_t rows_ = internals_._dfa[dfa_].size() /
                    dfa_alphabet_;
                const id_type* ptr_ = &internals_._dfa[dfa_].front();
                std::string dfa_name_ = "dfa";

                output_tabs(tabs_, os_);
                os_ << "static const ";

                if (pointers_)
                {
                    os_ << "void *";
                }
                else
                {
                    os_ << "id_type ";
                }

                os_ << dfa_name_;

                if (dfas_ > 1)
                {
                    std::ostringstream ss_;

                    ss_ << dfa_;
                    dfa_name_ += ss_.str();
                    os_ << dfa_;
                }

                dfa_name_ += '_';
                os_ << "_[] = {";

                for (std::size_t row_ = 0; row_ < rows_; ++row_)
                {
                    dump_row(row_ == 0, ptr_, dfa_name_, dfa_alphabet_,
                        pointers_, os_);

                    if (row_ + 1 < rows_)
                    {
                        os_ << ",\n";
                        output_tabs(tabs_ + 1, os_);
                    }
                }

                os_ << "};\n";
            }

            if (dfas_ > 1)
            {
                output_tabs(tabs_, os_);
                os_ << "static const ";

                if (pointers_)
                {
                    os_ << "void * const";
                }
                else
                {
                    os_ << "id_type";
                }

                os_ << " *dfas_[] = {dfa0_";

                for (std::size_t col_ = 1; col_ < dfas_; ++col_)
                {
                    os_ << ", dfa" << col_ << '_';
                }

                os_ << "};\n";
            }

            os_ << std::dec;
        }

    protected:
        template<typename id_type>
        static void dump_row(const bool first_, const id_type*& ptr_,
            const std::string& dfa_name_, const id_type dfa_alphabet_,
            const bool pointers_, std::ostream& os_)
        {
            if (pointers_)
            {
                bool zero_ = *ptr_ == 0;

                if (first_)
                {
                    // We want numbers regardless of id_type.
                    os_ << dfa_name_ << " + 0x" << std::hex <<
                        static_cast<std::size_t>(*ptr_++) * dfa_alphabet_;
                }
                else if (!zero_)
                {
                    os_ << "reinterpret_cast<const void *>(0x"
                        // We want numbers regardless of id_type.
                        << std::hex << static_cast<std::size_t>(*ptr_++) << ')';
                }
                else
                {
                    // We want numbers regardless of id_type.
                    os_ << "0x" << std::hex <<
                        static_cast<std::size_t>(*ptr_++);
                }

                for (id_type id_index_ = id_index;
                    id_index_ < transitions_index; ++id_index_, ++ptr_)
                {
                    os_ << ", ";
                    zero_ = *ptr_ == 0;

                    if (!zero_)
                    {
                        os_ << "reinterpret_cast<const void *>(";
                    }

                    // We want numbers regardless of id_type.
                    os_ << "0x" << std::hex << static_cast<std::size_t>(*ptr_);

                    if (!zero_)
                    {
                        os_ << ')';
                    }
                }

                for (id_type alphabet_ = transitions_index;
                    alphabet_ < dfa_alphabet_; ++alphabet_, ++ptr_)
                {
                    // We want numbers regardless of id_type.
                    os_ << ", ";

                    if (*ptr_ == 0)
                    {
                        os_ << 0;
                    }
                    else
                    {
                        // We want numbers regardless of id_type.
                        os_ << dfa_name_ + " + 0x" << std::hex <<
                            static_cast<std::size_t>(*ptr_) * dfa_alphabet_;
                    }
                }
            }
            else
            {
                // We want numbers regardless of id_type.
                os_ << "0x" << std::hex << static_cast<std::size_t>(*ptr_++);

                for (id_type alphabet_ = 1; alphabet_ < dfa_alphabet_;
                    ++alphabet_, ++ptr_)
                {
                    // We want numbers regardless of id_type.
                    os_ << ", 0x" << std::hex <<
                        static_cast<std::size_t>(*ptr_);
                }
            }
        }

        static void output_tabs(const std::size_t tabs_, std::ostream& os_)
        {
            for (std::size_t i_ = 0; i_ < tabs_; ++i_)
            {
                os_ << "    ";
            }
        }

        template<typename id_type>
        static void output_char_loop(const id_type features_,
            const std::size_t additional_tabs_, const bool pointers_,
            std::ostream& os_, const std::false_type&)
        {
            output_tabs(additional_tabs_, os_);
            os_ << "        const typename results::char_type prev_char_ = "
                "*curr_++;\n";
            output_tabs(additional_tabs_, os_);
            os_ << "        const ";

            if (pointers_)
            {
                os_ << "void * const *";
            }
            else
            {
                os_ << "id_type ";
            }

            os_ << "state_ = ";

            if (pointers_)
            {
                os_ << "reinterpret_cast<void * const *>\n            ";
                output_tabs(additional_tabs_, os_);
                os_ << '(';
            }

            os_ << "ptr_[lookup_";

            if (!pointers_)
            {
                os_ << "\n            ";
                output_tabs(additional_tabs_, os_);
            }

            os_ << "[static_cast<typename results::index_type>";

            if (pointers_)
            {
                os_ << "\n            ";
                output_tabs(additional_tabs_, os_);
            }

            os_ << "(prev_char_)]]";

            if (pointers_)
            {
                os_ << ')';
            }

            os_ << ";\n\n";

            if (features_ & bol_bit)
            {
                output_tabs(additional_tabs_, os_);
                os_ << "        bol_ = prev_char_ == '\\n';\n\n";
            }

            output_tabs(additional_tabs_, os_);
            os_ << "        if (state_ == 0)\n";
            output_tabs(additional_tabs_, os_);
            os_ << "        {\n";

            if (features_ & eol_bit)
            {
                output_tabs(additional_tabs_, os_);
                os_ << "            EOL_state_ = 0;\n";
            }

            output_tabs(additional_tabs_, os_);
            os_ << "            break;\n";
            output_tabs(additional_tabs_, os_);
            os_ << "        }\n\n";
            output_tabs(additional_tabs_, os_);
            os_ << "        ptr_ = ";

            if (pointers_)
            {
                os_ << "state_";
            }
            else
            {
                os_ << "&dfa_[state_ * dfa_alphabet_]";
            }

            os_ << ";\n";
        }

        template<typename id_type>
        static void output_char_loop(const id_type features_,
            const std::size_t additional_tabs_, const bool pointers_,
            std::ostream& os_, const std::true_type&)
        {
            output_tabs(additional_tabs_, os_);
            os_ << "        const std::size_t bytes_ =\n";
            output_tabs(additional_tabs_, os_);
            os_ << "            sizeof(typename results::char_type) < 3 ?\n";
            output_tabs(additional_tabs_, os_);
            os_ << "            sizeof(typename results::char_type) : 3;\n";
            output_tabs(additional_tabs_, os_);
            os_ << "        const std::size_t shift_[] = {0, 8, 16};\n";
            output_tabs(additional_tabs_, os_);
            os_ << "        typename results::char_type prev_char_ = "
                "*curr_++;\n\n";

            if (features_ & bol_bit)
            {
                output_tabs(additional_tabs_, os_);
                os_ << "        bol_ = prev_char_ == '\\n';\n\n";
            }

            output_tabs(additional_tabs_, os_);
            os_ << "        for (std::size_t i_ = 0; i_ < bytes_; ++i_)\n";
            output_tabs(additional_tabs_, os_);
            os_ << "        {\n";
            output_tabs(additional_tabs_, os_);
            os_ << "            const ";

            if (pointers_)
            {
                os_ << "void * const *";
            }
            else
            {
                os_ << "id_type ";
            }

            os_ << "state_ = ";

            if (pointers_)
            {
                os_ << "reinterpret_cast<void * const *>\n                ";
                output_tabs(additional_tabs_, os_);
                os_ << '(';
            }

            os_ << "ptr_[lookup_[static_cast\n";
            output_tabs(additional_tabs_, os_);
            os_ << "                <unsigned char>((prev_char_ >>\n"
                "                shift_[bytes_ - 1 - i_]) & 0xff)]]";

            if (pointers_)
            {
                os_ << ')';
            }

            os_ << ";\n\n";
            output_tabs(additional_tabs_, os_);
            os_ << "            if (state_ == 0)\n";
            output_tabs(additional_tabs_, os_);
            os_ << "            {\n";

            if (features_ & eol_bit)
            {
                output_tabs(additional_tabs_, os_);
                os_ << "                EOL_state_ = 0;\n";
            }

            output_tabs(additional_tabs_, os_);
            os_ << "                goto quit;\n";
            output_tabs(additional_tabs_, os_);
            os_ << "            }\n\n";
            output_tabs(additional_tabs_, os_);
            os_ << "            ptr_ = ";

            if (pointers_)
            {
                os_ << "state_";
            }
            else
            {
                os_ << "&dfa_[state_ * dfa_alphabet_]";
            }

            os_ << ";\n";
            output_tabs(additional_tabs_, os_);
            os_ << "        }\n";
        }

        static void output_quit(std::ostream&, const std::false_type&)
        {
            // Nothing to do
        }

        static void output_quit(std::ostream& os_, const std::true_type&)
        {
            os_ << "quit:\n";
        }
    };
}

#endif
