// tokeniser_helper.hpp
// Copyright (c) 2005-2020 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_RE_TOKENISER_HELPER_HPP
#define LEXERTL_RE_TOKENISER_HELPER_HPP

#include "../../char_traits.hpp"
// strlen()
#include <cstring>
#include "re_tokeniser_state.hpp"
#include "../../runtime_error.hpp"
#include <sstream>
#include "../../string_token.hpp"

namespace lexertl
{
    namespace detail
    {
        struct block
        {
            const char* _name;
            const char* (*_func)();
        };

        template<typename rules_char_type, typename input_char_type,
            typename id_type,
            typename char_traits = basic_char_traits<input_char_type> >
            class basic_re_tokeniser_helper
        {
        public:
            using char_state = basic_re_tokeniser_state<char, id_type>;
            using state = basic_re_tokeniser_state<rules_char_type, id_type>;
            using string_token = basic_string_token<input_char_type>;
            using index_type = typename string_token::index_type;
            using range = typename string_token::range;

            template<char ch>
            struct size
            {
            };

            using one = size<1>;
            using two = size<2>;
            using four = size<4>;

            template<typename state_type, typename char_type>
            static const char* escape_sequence(state_type& state_,
                char_type& ch_, std::size_t& str_len_)
            {
                bool eos_ = state_.eos();

                if (eos_)
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following '\\'";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                const char* str_ = charset_shortcut(state_, str_len_);

                if (str_)
                {
                    state_.increment();
                }
                else
                {
                    ch_ = chr(state_);
                }

                return str_;
            }

            // This function can call itself.
            template<typename state_type>
            static void charset(state_type& state_, string_token& token_)
            {
                bool negated_ = false;
                typename state_type::char_type ch_ = 0;
                bool eos_ = state_.next(ch_);

                if (eos_)
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following '['";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                negated_ = ch_ == '^';

                if (negated_)
                {
                    eos_ = state_.next(ch_);

                    if (eos_)
                    {
                        std::ostringstream ss_;

                        // Pointless returning index if at end of string
                        state_.unexpected_end(ss_);
                        ss_ << " following '^'";
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                }

                bool chset_ = false;
                typename string_token::char_type prev_ = 0;

                do
                {
                    if (ch_ == '\\')
                    {
                        std::size_t str_len_ = 0;
                        const char* str_ = escape_sequence(state_, prev_,
                            str_len_);

                        chset_ = str_ != nullptr;

                        if (chset_)
                        {
                            char_state temp_state_(str_ + 1, str_ + str_len_,
                                state_._id, state_._flags, state_._locale,
                                nullptr);
                            string_token temp_token_;

                            charset(temp_state_, temp_token_);
                            token_.insert(temp_token_);
                        }
                    }
                    else if (ch_ == '[' && !state_.eos() &&
                        *state_._curr == ':')
                    {
                        state_.increment();
                        posix(state_, token_);
                        chset_ = true;
                    }
                    else
                    {
                        chset_ = false;
                        prev_ = ch_;
                    }

                    eos_ = state_.next(ch_);

                    // Covers preceding if, else if and else
                    if (eos_)
                    {
                        std::ostringstream ss_;

                        // Pointless returning index if at end of string
                        state_.unexpected_end(ss_);
                        ss_ << " (missing ']')";
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }

                    if (ch_ == '-' && *state_._curr != ']')
                    {
                        charset_range(chset_, state_, eos_, ch_, prev_,
                            token_);
                    }
                    else if (!chset_)
                    {
                        range range_(prev_, prev_);

                        token_.insert(range_);

                        if (state_._flags & icase)
                        {
                            string_token folded_;

                            fold(range_, state_._locale, folded_,
                                size<sizeof(input_char_type)>());

                            if (!folded_.empty())
                            {
                                token_.insert(folded_);
                            }
                        }
                    }
                } while (ch_ != ']');

                if (negated_)
                {
                    token_.negate();
                }

                if (token_.empty())
                {
                    std::ostringstream ss_;

                    ss_ << "Empty charset not allowed preceding index " <<
                        state_.index();
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }
            }

            static void fold(const range& range_, const std::locale& locale_,
                string_token& out_, const one&)
            {
                // If string_token::char_type is 16 bit may overflow,
                // so use std::size_t.
                std::size_t start_ = range_.first;
                std::size_t end_ = range_.second;

                // In 8 bit char mode, use locale and
                // therefore consider every char individually.
                for (; start_ <= end_; ++start_)
                {
                    const input_char_type upper_ = std::toupper
                    (static_cast<input_char_type>(start_), locale_);
                    const input_char_type lower_ = std::tolower
                    (static_cast<input_char_type>(start_), locale_);

                    if (upper_ != static_cast<input_char_type>(start_))
                    {
                        out_.insert(range(upper_, upper_));
                    }

                    if (lower_ != static_cast<input_char_type>(start_))
                    {
                        out_.insert(range(lower_, lower_));
                    }
                }
            }

            static void fold(const range& range_, const std::locale&,
                string_token& out_, const two&)
            {
                static const fold_pair mapping_[] =
                {
#include "fold2.inc"
                    {{0, 0}, {0, 0}}
                };
                const fold_pair* ptr_ = mapping_;

                for (; ptr_->from.first != 0; ++ptr_)
                {
                    if (range_.second < ptr_->from.first) break;

                    if (range_.first >= ptr_->from.first &&
                        range_.first <= ptr_->from.second)
                    {
                        if (ptr_->to.first <= ptr_->to.second)
                        {
                            const index_type first_ = ptr_->to.first +
                                (range_.first - ptr_->from.first);

                            out_.insert(range(first_,
                                range_.second > ptr_->from.second ?
                                ptr_->to.second :
                                static_cast<index_type>(ptr_->to.first +
                                    (range_.second - ptr_->from.first))));
                        }
                        else
                        {
                            const index_type first_ = ptr_->to.second +
                                (range_.first - ptr_->from.first);

                            out_.insert(range(first_,
                                range_.second > ptr_->from.second ?
                                ptr_->to.first :
                                static_cast<index_type>(ptr_->to.second +
                                    (range_.second - ptr_->from.first))));
                        }
                    }
                    else if (range_.second >= ptr_->from.first &&
                        range_.second <= ptr_->from.second)
                    {
                        if (ptr_->to.first <= ptr_->to.second)
                        {
                            const index_type second_ = ptr_->to.first +
                                (range_.second - ptr_->from.first);

                            out_.insert(range(ptr_->to.first, second_));
                        }
                        else
                        {
                            const index_type second_ = ptr_->to.second +
                                (range_.second - ptr_->from.first);

                            out_.insert(range(ptr_->to.second, second_));
                        }
                    }
                    // Either range fully encompasses from range or not at all.
                    else if (ptr_->from.first >= range_.first &&
                        ptr_->from.first <= range_.second)
                    {
                        if (ptr_->to.first <= ptr_->to.second)
                        {
                            out_.insert(range(ptr_->to.first, ptr_->to.second));
                        }
                        else
                        {
                            out_.insert(range(ptr_->to.second, ptr_->to.first));
                        }
                    }
                }
            }

            static void fold(const range& range_, const std::locale& locale_,
                string_token& out_, const four&)
            {
                if (range_.first < 0x10000)
                {
                    fold(range_, locale_, out_, two());
                }

                static const fold_pair mapping_[] =
                {
#include "fold4.inc"
                    {{0, 0}, {0, 0}}
                };
                const fold_pair* ptr_ = mapping_;

                for (; ptr_->from.first != 0; ++ptr_)
                {
                    if (range_.second < ptr_->from.first) break;

                    if (range_.first >= ptr_->from.first &&
                        range_.first <= ptr_->from.second)
                    {
                        out_.insert(range(ptr_->to.first +
                            (range_.first - ptr_->from.first),
                            range_.second > ptr_->from.second ?
                            ptr_->to.second :
                            ptr_->to.first + (range_.second -
                                ptr_->from.first)));
                    }
                    else if (range_.second >= ptr_->from.first &&
                        range_.second <= ptr_->from.second)
                    {
                        out_.insert(range(ptr_->to.first,
                            ptr_->to.first + (range_.second -
                                ptr_->from.first)));
                    }
                    // Either range fully encompasses from range or not at all.
                    else if (ptr_->from.first >= range_.first &&
                        ptr_->from.first <= range_.second)
                    {
                        out_.insert(range(ptr_->to.first, ptr_->to.second));
                    }
                }
            }

            template<typename state_type>
            static input_char_type chr(state_type& state_)
            {
                input_char_type ch_ = 0;

                // eos_ has already been checked for.
                switch (*state_._curr)
                {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    ch_ = decode_octal(state_);
                    break;
                case 'a':
                    ch_ = '\a';
                    state_.increment();
                    break;
                case 'b':
                    ch_ = '\b';
                    state_.increment();
                    break;
                case 'c':
                    ch_ = decode_control_char(state_);
                    break;
                case 'e':
                    ch_ = 27; // '\e' not recognised by compiler
                    state_.increment();
                    break;
                case 'f':
                    ch_ = '\f';
                    state_.increment();
                    break;
                case 'n':
                    ch_ = '\n';
                    state_.increment();
                    break;
                case 'r':
                    ch_ = '\r';
                    state_.increment();
                    break;
                case 't':
                    ch_ = '\t';
                    state_.increment();
                    break;
                case 'v':
                    ch_ = '\v';
                    state_.increment();
                    break;
                case 'x':
                    ch_ = decode_hex(state_);
                    break;
                default:
                    ch_ = *state_._curr;
                    state_.increment();
                    break;
                }

                return ch_;
            }

        private:
            struct char_pair
            {
                input_char_type first;
                input_char_type second;
            };

            struct fold_pair
            {
                char_pair from;
                char_pair to;
            };

            template<typename state_type>
            static void posix(state_type& state_, string_token& token_)
            {
                bool negate_ = false;

                if (!state_.eos() && *state_._curr == '^')
                {
                    negate_ = true;
                    state_.increment();
                }

                if (state_.eos())
                {
                    unterminated_posix(state_);
                }
                else
                {
                    switch (*state_._curr)
                    {
                    case 'a':
                        // alnum
                        // alpha
                        alnum_alpha(state_, token_, negate_);
                        break;
                    case 'b':
                        // blank
                        blank(state_, token_, negate_);
                        break;
                    case 'c':
                        // cntrl
                        cntrl(state_, token_, negate_);
                        break;
                    case 'd':
                        // digit
                        digit(state_, token_, negate_);
                        break;
                    case 'g':
                        // graph
                        graph(state_, token_, negate_);
                        break;
                    case 'l':
                        // lower
                        lower(state_, token_, negate_);
                        break;
                    case 'p':
                        // print
                        // punct
                        print_punct(state_, token_, negate_);
                        break;
                    case 's':
                        // space
                        space(state_, token_, negate_);
                        break;
                    case 'u':
                        // upper
                        upper(state_, token_, negate_);
                        break;
                    case 'x':
                        // xdigit
                        xdigit(state_, token_, negate_);
                        break;
                    default:
                        unknown_posix(state_);
                        break;
                    }
                }
            }

            template<typename state_type>
            static void alnum_alpha(state_type& state_, string_token& token_,
                const bool negate_)
            {
                enum { unknown, alnum, alpha };
                std::size_t type_ = unknown;

                state_.increment();

                if (!state_.eos() && *state_._curr == 'l')
                {
                    state_.increment();

                    if (!state_.eos())
                    {
                        if (*state_._curr == 'n')
                        {
                            state_.increment();

                            if (!state_.eos() && *state_._curr == 'u')
                            {
                                state_.increment();

                                if (!state_.eos() && *state_._curr == 'm')
                                {
                                    state_.increment();
                                    type_ = alnum;
                                }
                            }
                        }
                        else if (*state_._curr == 'p')
                        {
                            state_.increment();

                            if (!state_.eos() && *state_._curr == 'h')
                            {
                                state_.increment();

                                if (!state_.eos() && *state_._curr == 'a')
                                {
                                    state_.increment();
                                    type_ = alpha;
                                }
                            }
                        }
                    }
                }

                if (type_ == unknown)
                {
                    unknown_posix(state_);
                }
                else
                {
                    std::string str_;

                    check_posix_termination(state_);

                    if (type_ == alnum)
                    {
                        // alnum
                        str_ = sizeof(input_char_type) == 1 ?
                            make_alnum(state_._locale) :
                            std::string(R"([\p{Ll}\p{Lu}\p{Nd}])");
                    }
                    else
                    {
                        // alpha
                        str_ = sizeof(input_char_type) == 1 ?
                            make_alpha(state_._locale) :
                            std::string(R"([\p{Ll}\p{Lu}])");
                    }

                    insert_charset(str_.c_str(), state_, token_, negate_);
                }
            }

            static std::string make_alnum(std::locale& locale_)
            {
                std::string str_(1, '[');

                for (std::size_t i_ = 0; i_ < 256; ++i_)
                {
                    if (std::use_facet<std::ctype<char> >(locale_).
                        is(std::ctype_base::alnum, static_cast<char>(i_)))
                    {
                        str_ += static_cast<char>(i_);
                    }
                }

                str_ += ']';
                return str_;
            }

            static std::string make_alpha(std::locale& locale_)
            {
                std::string str_(1, '[');

                for (std::size_t i_ = 0; i_ < 256; ++i_)
                {
                    if (std::use_facet<std::ctype<char> >(locale_).
                        is(std::ctype_base::alpha, static_cast<char>(i_)))
                    {
                        str_ += static_cast<char>(i_);
                    }
                }

                str_ += ']';
                return str_;
            }

            template<typename state_type>
            static void blank(state_type& state_, string_token& token_,
                const bool negate_)
            {
                const char* blank_ = "lank";

                state_.increment();

                // Casts to prevent warnings (VC++ 2012)
                while (!state_.eos() && *blank_ &&
                    static_cast<rules_char_type>(*state_._curr) ==
                    static_cast<rules_char_type>(*blank_))
                {
                    state_.increment();
                    ++blank_;
                }

                if (*blank_)
                {
                    unknown_posix(state_);
                }
                else
                {
                    const char* str_ = sizeof(input_char_type) == 1 ?
                        "[ \t]" : "[\\p{Zs}\t]";

                    check_posix_termination(state_);
                    insert_charset(str_, state_, token_, negate_);
                }
            }

            template<typename state_type>
            static void cntrl(state_type& state_, string_token& token_,
                const bool negate_)
            {
                const char* cntrl_ = "ntrl";

                state_.increment();

                // Casts to prevent warnings (VC++ 2012)
                while (!state_.eos() && *cntrl_ &&
                    static_cast<rules_char_type>(*state_._curr) ==
                    static_cast<rules_char_type>(*cntrl_))
                {
                    state_.increment();
                    ++cntrl_;
                }

                if (*cntrl_)
                {
                    unknown_posix(state_);
                }
                else
                {
                    const char* str_ = sizeof(input_char_type) == 1 ?
                        "[\\x00-\x1f\x7f]" : "[\\p{Cc}]";

                    check_posix_termination(state_);
                    insert_charset(str_, state_, token_, negate_);
                }
            }

            template<typename state_type>
            static void digit(state_type& state_, string_token& token_,
                const bool negate_)
            {
                const char* digit_ = "igit";

                state_.increment();

                // Casts to prevent warnings (VC++ 2012)
                while (!state_.eos() && *digit_ &&
                    static_cast<rules_char_type>(*state_._curr) ==
                    static_cast<rules_char_type>(*digit_))
                {
                    state_.increment();
                    ++digit_;
                }

                if (*digit_)
                {
                    unknown_posix(state_);
                }
                else
                {
                    const char* str_ = sizeof(input_char_type) == 1 ?
                        "[0-9]" : "[\\p{Nd}]";

                    check_posix_termination(state_);
                    insert_charset(str_, state_, token_, negate_);
                }
            }

            template<typename state_type>
            static void graph(state_type& state_, string_token& token_,
                const bool negate_)
            {
                const char* graph_ = "raph";

                state_.increment();

                // Casts to prevent warnings (VC++ 2012)
                while (!state_.eos() && *graph_ &&
                    static_cast<rules_char_type>(*state_._curr) ==
                    static_cast<rules_char_type>(*graph_))
                {
                    state_.increment();
                    ++graph_;
                }

                if (*graph_)
                {
                    unknown_posix(state_);
                }
                else
                {
                    const char* str_ = sizeof(input_char_type) == 1 ?
                        "[\x21-\x7e]" : R"([^\p{Z}\p{C}])";

                    check_posix_termination(state_);
                    insert_charset(str_, state_, token_, negate_);
                }
            }

            template<typename state_type>
            static void lower(state_type& state_, string_token& token_,
                const bool negate_)
            {
                const char* lower_ = "ower";

                state_.increment();

                // Casts to prevent warnings (VC++ 2012)
                while (!state_.eos() && *lower_ &&
                    static_cast<rules_char_type>(*state_._curr) ==
                    static_cast<rules_char_type>(*lower_))
                {
                    state_.increment();
                    ++lower_;
                }

                if (*lower_)
                {
                    unknown_posix(state_);
                }
                else
                {
                    std::string str_ = sizeof(input_char_type) == 1 ?
                        create_lower(state_._locale) :
                        std::string("[\\p{Ll}]");

                    check_posix_termination(state_);
                    insert_charset(str_.c_str(), state_, token_, negate_);
                }
            }

            static std::string create_lower(std::locale& locale_)
            {
                std::string str_(1, '[');

                for (std::size_t i_ = 0; i_ < 256; ++i_)
                {
                    if (std::use_facet<std::ctype<char> >(locale_).
                        is(std::ctype_base::lower, static_cast<char>(i_)))
                    {
                        str_ += static_cast<char>(i_);
                    }
                }

                str_ += ']';
                return str_;
            }

            template<typename state_type>
            static void print_punct(state_type& state_, string_token& token_,
                const bool negate_)
            {
                enum { unknown, print, punct };
                std::size_t type_ = unknown;

                state_.increment();

                if (!state_.eos())
                {
                    if (*state_._curr == 'r')
                    {
                        state_.increment();

                        if (!state_.eos() && *state_._curr == 'i')
                        {
                            state_.increment();

                            if (!state_.eos() && *state_._curr == 'n')
                            {
                                state_.increment();

                                if (!state_.eos() && *state_._curr == 't')
                                {
                                    state_.increment();
                                    type_ = print;
                                }
                            }
                        }
                    }
                    else if (*state_._curr == 'u')
                    {
                        state_.increment();

                        if (!state_.eos() && *state_._curr == 'n')
                        {
                            state_.increment();

                            if (!state_.eos() && *state_._curr == 'c')
                            {
                                state_.increment();

                                if (!state_.eos() && *state_._curr == 't')
                                {
                                    state_.increment();
                                    type_ = punct;
                                }
                            }
                        }
                    }
                }

                if (type_ == unknown)
                {
                    unknown_posix(state_);
                }
                else
                {
                    const char* str_ = nullptr;

                    check_posix_termination(state_);

                    if (type_ == print)
                    {
                        // print
                        str_ = sizeof(input_char_type) == 1 ?
                            "[\x20-\x7e]" : R"([\p{C}])";
                    }
                    else
                    {
                        // punct
                        str_ = sizeof(input_char_type) == 1 ?
                            R"([!"#$%&'()*+,\-./:;<=>?@[\\\]^_`{|}~])" :
                            R"([\p{P}\p{S}])";
                    }

                    insert_charset(str_, state_, token_, negate_);
                }
            }

            template<typename state_type>
            static void space(state_type& state_, string_token& token_,
                const bool negate_)
            {
                const char* space_ = "pace";

                state_.increment();

                // Casts to prevent warnings (VC++ 2012)
                while (!state_.eos() && *space_ &&
                    static_cast<rules_char_type>(*state_._curr) ==
                    static_cast<rules_char_type>(*space_))
                {
                    state_.increment();
                    ++space_;
                }

                if (*space_)
                {
                    unknown_posix(state_);
                }
                else
                {
                    const char* str_ = sizeof(input_char_type) == 1 ?
                        "[ \t\r\n\v\f]" : "[\\p{Z}\t\r\n\v\f]";

                    check_posix_termination(state_);
                    insert_charset(str_, state_, token_, negate_);
                }
            }

            template<typename state_type>
            static void upper(state_type& state_, string_token& token_,
                const bool negate_)
            {
                const char* upper_ = "pper";

                state_.increment();

                // Casts to prevent warnings (VC++ 2012)
                while (!state_.eos() && *upper_ &&
                    static_cast<rules_char_type>(*state_._curr) ==
                    static_cast<rules_char_type>(*upper_))
                {
                    state_.increment();
                    ++upper_;
                }

                if (*upper_)
                {
                    unknown_posix(state_);
                }
                else
                {
                    std::string str_ = sizeof(input_char_type) == 1 ?
                        create_upper(state_._locale) :
                        std::string("[\\p{Lu}]");

                    check_posix_termination(state_);
                    insert_charset(str_.c_str(), state_, token_, negate_);
                }
            }

            static std::string create_upper(std::locale& locale_)
            {
                std::string str_(1, '[');

                for (std::size_t i_ = 0; i_ < 256; ++i_)
                {
                    if (std::use_facet<std::ctype<char> >(locale_).
                        is(std::ctype_base::upper, static_cast<char>(i_)))
                    {
                        str_ += static_cast<char>(i_);
                    }
                }

                str_ += ']';
                return str_;
            }

            template<typename state_type>
            static void xdigit(state_type& state_, string_token& token_,
                const bool negate_)
            {
                const char* xdigit_ = "digit";

                state_.increment();

                // Casts to prevent warnings (VC++ 2012)
                while (!state_.eos() && *xdigit_ &&
                    static_cast<rules_char_type>(*state_._curr) ==
                    static_cast<rules_char_type>(*xdigit_))
                {
                    state_.increment();
                    ++xdigit_;
                }

                if (*xdigit_)
                {
                    unknown_posix(state_);
                }
                else
                {
                    const char* str_ = "[0-9A-Fa-f]";

                    check_posix_termination(state_);
                    insert_charset(str_, state_, token_, negate_);
                }
            }

            template<typename state_type>
            static void check_posix_termination(state_type& state_)
            {
                if (state_.eos())
                {
                    unterminated_posix(state_);
                }

                if (*state_._curr != ':')
                {
                    std::ostringstream ss_;

                    ss_ << "Missing ':' at index " << state_.index();
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                state_.increment();

                if (state_.eos())
                {
                    unterminated_posix(state_);
                }

                if (*state_._curr != ']')
                {
                    std::ostringstream ss_;

                    ss_ << "Missing ']' at index " << state_.index();
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                state_.increment();
            }

            template<typename state_type>
            static void unterminated_posix(state_type& state_)
            {
                std::ostringstream ss_;

                // Pointless returning index if at end of string
                state_.unexpected_end(ss_);
                ss_ << " (unterminated POSIX charset)";
                state_.error(ss_);
                throw runtime_error(ss_.str());
            }

            template<typename state_type>
            static void unknown_posix(state_type& state_)
            {
                std::ostringstream ss_;

                ss_ << "Unknown POSIX charset at index " <<
                    state_.index();
                state_.error(ss_);
                throw runtime_error(ss_.str());
            }

            template<typename state_type>
            static void insert_charset(const char* str_, state_type& state_,
                string_token& token_, const bool negate_)
            {
                // Some systems have strlen in namespace std.
                using namespace std;

                char_state temp_state_(str_ + 1, str_ + strlen(str_),
                    state_._id, state_._flags, state_._locale, nullptr);
                string_token temp_token_;

                charset(temp_state_, temp_token_);

                if (negate_) temp_token_.negate();

                token_.insert(temp_token_);
            }

            template<typename state_type>
            static const char* charset_shortcut
            (state_type& state_, std::size_t& str_len_)
            {
                const char* str_ = nullptr;

                switch (*state_._curr)
                {
                case 'd':
                    str_ = "[0-9]";
                    break;
                case 'D':
                    str_ = "[^0-9]";
                    break;
                case 'p':
                    str_ = unicode_escape(state_);
                    break;
                case 's':
                    str_ = "[ \t\n\r\f\v]";
                    break;
                case 'S':
                    str_ = "[^ \t\n\r\f\v]";
                    break;
                case 'w':
                    str_ = "[_0-9A-Za-z]";
                    break;
                case 'W':
                    str_ = "[^_0-9A-Za-z]";
                    break;
                }

                if (str_)
                {
                    // Some systems have strlen in namespace std.
                    using namespace std;

                    str_len_ = strlen(str_);
                }
                else
                {
                    str_len_ = 0;
                }

                return str_;
            }

            template<typename state_type>
            static const char* unicode_escape(state_type& state_)
            {
                const char* str_ = nullptr;

                state_.increment();

                if (state_.eos())
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\p";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                if (*state_._curr != '{')
                {
                    std::ostringstream ss_;

                    ss_ << "Missing '{' following \\p at index " <<
                        state_.index();
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                state_.increment();

                if (state_.eos())
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\p{";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                const typename state_type::char_type* start_ = state_._curr;
                const std::size_t idx = state_.index();

                do
                {
                    state_.increment();
                } while (!state_.eos() && *state_._curr != '}');

                if (state_.eos())
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\p{";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                static const block lookup_[] =
                {
#include "table.inc"
                };

                for (const block* entry_ = lookup_; entry_->_name; ++entry_)
                {
                    const typename state_type::char_type* source_ = start_;
                    const char* name_ = entry_->_name;

                    for (; source_ != state_._curr && *name_; ++source_, ++name_)
                    {
                        if (*source_ !=
                            static_cast<typename state_type::char_type>(*name_))
                            break;
                    }

                    if (source_ == state_._curr && !*name_)
                        return str_ = entry_->_func();
                }

                if (str_ == nullptr)
                {
                    std::ostringstream ss_;

                    ss_ << "Syntax error following \\p{ at index " << idx;
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                return str_;
            }

#include "blocks.hpp"
#include "scripts.hpp"
#include "unicode.hpp"

            template<typename state_type>
            static input_char_type decode_octal(state_type& state_)
            {
                std::size_t oct_ = 0;
                auto ch_ = *state_._curr;
                unsigned short count_ = 3;
                bool eos_ = false;

                for (;;)
                {
                    oct_ *= 8;
                    oct_ += static_cast<std::size_t>(ch_) - '0';
                    --count_;
                    state_.increment();
                    eos_ = state_.eos();

                    if (!count_ || eos_) break;

                    ch_ = *state_._curr;

                    // Don't consume invalid chars!
                    if (ch_ < '0' || ch_ > '7')
                    {
                        break;
                    }
                }

                if (oct_ > static_cast<std::size_t>(char_traits::max_val()))
                {
                    std::ostringstream ss_;

                    ss_ << "Escape \\" << std::oct << oct_ <<
                        " is too big for the state machine char type "
                        "preceding index " << std::dec << state_.index();
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                return static_cast<input_char_type>(oct_);
            }

            template<typename state_type>
            static input_char_type decode_control_char(state_type& state_)
            {
                // Skip over 'c'
                state_.increment();

                typename state_type::char_type ch_ = 0;
                bool eos_ = state_.next(ch_);

                if (eos_)
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\c";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }
                else
                {
                    if (ch_ >= 'a' && ch_ <= 'z')
                    {
                        ch_ -= 'a' - 1;
                    }
                    else if (ch_ >= 'A' && ch_ <= 'Z')
                    {
                        ch_ -= 'A' - 1;
                    }
                    else if (ch_ == '@')
                    {
                        // Apparently...
                        ch_ = 0;
                    }
                    else
                    {
                        std::ostringstream ss_;

                        ss_ << "Invalid control char at index " <<
                            state_.index() - 1;
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                }

                return ch_;
            }

            template<typename state_type>
            static input_char_type decode_hex(state_type& state_)
            {
                // Skip over 'x'
                state_.increment();

                typename state_type::char_type ch_ = 0;
                bool eos_ = state_.next(ch_);

                if (eos_)
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\x";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                if (!((ch_ >= '0' && ch_ <= '9') ||
                    (ch_ >= 'a' && ch_ <= 'f') ||
                    (ch_ >= 'A' && ch_ <= 'F')))
                {
                    std::ostringstream ss_;

                    ss_ << "Illegal char following \\x at index " <<
                        state_.index() - 1;
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                std::size_t hex_ = 0;

                do
                {
                    hex_ *= 16;

                    if (ch_ >= '0' && ch_ <= '9')
                    {
                        hex_ += static_cast<std::size_t>(ch_) - '0';
                    }
                    else if (ch_ >= 'a' && ch_ <= 'f')
                    {
                        hex_ += 10 + (static_cast<std::size_t>(ch_) - 'a');
                    }
                    else
                    {
                        hex_ += 10 + (static_cast<std::size_t>(ch_) - 'A');
                    }

                    eos_ = state_.eos();

                    if (!eos_)
                    {
                        ch_ = *state_._curr;

                        // Don't consume invalid chars!
                        if (((ch_ >= '0' && ch_ <= '9') ||
                            (ch_ >= 'a' && ch_ <= 'f') ||
                            (ch_ >= 'A' && ch_ <= 'F')))
                        {
                            state_.increment();
                        }
                        else
                        {
                            eos_ = true;
                        }
                    }
                } while (!eos_);

                if (hex_ > static_cast<std::size_t>(char_traits::max_val()))
                {
                    std::ostringstream ss_;

                    ss_ << "Escape \\x" << std::hex << hex_ <<
                        " is too big for the state machine char type " <<
                        "preceding index " <<
                        std::dec << state_.index();
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                return static_cast<input_char_type>(hex_);
            }

            template<typename state_type>
            static void charset_range(const bool chset_, state_type& state_,
                bool& eos_, typename state_type::char_type& ch_,
                const input_char_type prev_, string_token& chars_)
            {
                if (chset_)
                {
                    std::ostringstream ss_;

                    ss_ << "Charset cannot form start of range preceding "
                        "index " << state_.index() - 1;
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                eos_ = state_.next(ch_);

                if (eos_)
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following '-'";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                input_char_type curr_ = 0;

                if (ch_ == '\\')
                {
                    std::size_t str_len_ = 0;

                    if (escape_sequence(state_, curr_, str_len_))
                    {
                        std::ostringstream ss_;

                        ss_ << "Charset cannot form end of "
                            "range preceding index " << state_.index();
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                }
                else if (ch_ == '[' && !state_.eos() && *state_._curr == ':')
                {
                    std::ostringstream ss_;

                    ss_ << "POSIX char class cannot form end of range at "
                        "index " << state_.index() - 1;
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }
                else
                {
                    curr_ = ch_;
                }

                eos_ = state_.next(ch_);

                // Covers preceding if and else
                if (eos_)
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " (missing ']')";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                // Use index_type as char is generally signed
                // and we want to ignore signedness.
                auto start_ =
                    static_cast<typename char_traits::index_type>(prev_);
                auto end_ =
                    static_cast<typename char_traits::index_type>(curr_);

                // Semanic check
                if (end_ < start_)
                {
                    std::ostringstream ss_;

                    ss_ << "Max less than Min in charset range "
                        "preceding index " <<
                        state_.index() - 1;
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                // Even though ranges are used now, we still need to consider
                // each character if icase is set.
                if (state_._flags & icase)
                {
                    range range_(start_, end_);
                    string_token folded_;

                    chars_.insert(range_);
                    fold(range_, state_._locale, folded_,
                        size<sizeof(input_char_type)>());

                    if (!folded_.empty())
                    {
                        chars_.insert(folded_);
                    }
                }
                else
                {
                    chars_.insert(range(prev_, curr_));
                }
            }
        };
    }
}

#endif
