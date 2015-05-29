// tokeniser_helper.hpp
// Copyright (c) 2005-2015 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_RE_TOKENISER_HELPER_H
#define LEXERTL_RE_TOKENISER_HELPER_H

#include "../../bool.hpp"
#include "../../char_traits.hpp"
// strlen()
#include <cstring>
#include "../../size_t.hpp"
#include "re_tokeniser_state.hpp"
#include "../../runtime_error.hpp"
#include <sstream>
#include "../../string_token.hpp"

namespace lexertl
{
namespace detail
{
template<typename rules_char_type, typename input_char_type, typename id_type,
    typename char_traits = basic_char_traits<input_char_type> >
class basic_re_tokeniser_helper
{
public:
    typedef basic_re_tokeniser_state<char, id_type> char_state;
    typedef basic_re_tokeniser_state<rules_char_type, id_type> state;
    typedef basic_string_token<input_char_type> string_token;

    template<char ch>
    struct size
    {
    };

    typedef size<1> one;
    typedef size<2> two;
    typedef size<4> four;

    template<typename state_type, typename char_type>
    static const char *escape_sequence(state_type &state_,
        char_type &ch_, std::size_t &str_len_)
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

        const char *str_ = charset_shortcut(state_, str_len_);

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
    static void charset(state_type &state_, string_token &token_)
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
                const char *str_ = escape_sequence(state_, prev_,
                    str_len_);

                chset_ = str_ != 0;

                if (chset_)
                {
                    char_state temp_state_(str_ + 1, str_ + str_len_,
                        state_._id, state_._flags, state_._locale, 0);
                    string_token temp_token_;

                    charset(temp_state_, temp_token_);
                    token_.insert(temp_token_);
                }
            }
            else if (ch_ == '[' && !state_.eos() && *state_._curr == ':')
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
                typename string_token::range range_(prev_, prev_);

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

    static void fold(const typename string_token::range &range_,
        const std::locale &locale_, string_token &out_, const one &)
    {
        // If string_token::char_type is 16 bit may overflow,
        // so use std::size_t.
        std::size_t start_ = range_.first;
        std::size_t end_ = range_.second;

        // In 8 bit char mode, use locale and therefore consider every char
        // individually.
        for (; start_ <= end_; ++start_)
        {
            const input_char_type upper_ = std::toupper
                (static_cast<input_char_type>(start_), locale_);
            const input_char_type lower_ = std::tolower
                (static_cast<input_char_type>(start_), locale_);

            if (upper_ != static_cast<input_char_type>(start_))
            {
                out_.insert(typename string_token::range(upper_, upper_));
            }

            if (lower_ != static_cast<input_char_type>(start_))
            {
                out_.insert(typename string_token::range(lower_, lower_));
            }
        }
    }

    static void fold(const typename string_token::range &range_,
        const std::locale &, string_token &out_, const two &)
    {
        static const fold_pair mapping_[] =
            {{{0x0041, 0x005a}, {0x0061, 0x007a}},
            {{0x0061, 0x007a}, {0x0041, 0x005a}},
            {{0x00b5, 0x00b5}, {0x039c, 0x039c}},
            {{0x00c0, 0x00d6}, {0x00e0, 0x00f6}},
            {{0x00d8, 0x00de}, {0x00f8, 0x00fe}},
            {{0x00e0, 0x00f6}, {0x00c0, 0x00d6}},
            {{0x00f8, 0x0137}, {0x00d8, 0x0117}},
            {{0x0139, 0x0148}, {0x013a, 0x0149}},
            {{0x014a, 0x018c}, {0x014b, 0x018d}},
            {{0x018e, 0x019a}, {0x01dd, 0x01e9}},
            {{0x019c, 0x01a9}, {0x026f, 0x027c}},
            {{0x01ac, 0x01b9}, {0x01ad, 0x01ba}},
            {{0x01bc, 0x01bd}, {0x01bd, 0x01be}},
            {{0x01bf, 0x01bf}, {0x01f7, 0x01f7}},
            {{0x01c4, 0x01c4}, {0x01c6, 0x01c6}},
            {{0x01c6, 0x01c7}, {0x01c4, 0x01c5}},
            {{0x01c9, 0x01ca}, {0x01c7, 0x01c8}},
            {{0x01cc, 0x01ef}, {0x01ca, 0x01ed}},
            {{0x01f1, 0x01f1}, {0x01f3, 0x01f3}},
            {{0x01f3, 0x0220}, {0x01f1, 0x021e}},
            {{0x0222, 0x0233}, {0x0223, 0x0234}},
            {{0x023a, 0x0254}, {0x2c65, 0x2c7f}},
            {{0x0256, 0x0257}, {0x0189, 0x018a}},
            {{0x0259, 0x0259}, {0x018f, 0x018f}},
            {{0x025b, 0x025b}, {0x0190, 0x0190}},
            {{0x0260, 0x0260}, {0x0193, 0x0193}},
            {{0x0263, 0x0263}, {0x0194, 0x0194}},
            {{0x0265, 0x0265}, {0xa78d, 0xa78d}},
            {{0x0268, 0x0269}, {0x0197, 0x0198}},
            {{0x026b, 0x026b}, {0x2c62, 0x2c62}},
            {{0x026f, 0x026f}, {0x019c, 0x019c}},
            {{0x0271, 0x0272}, {0x2c6e, 0x2c6f}},
            {{0x0275, 0x0275}, {0x019f, 0x019f}},
            {{0x027d, 0x027d}, {0x2c64, 0x2c64}},
            {{0x0280, 0x0280}, {0x01a6, 0x01a6}},
            {{0x0283, 0x0283}, {0x01a9, 0x01a9}},
            {{0x0288, 0x028c}, {0x01ae, 0x01b2}},
            {{0x0292, 0x0292}, {0x01b7, 0x01b7}},
            {{0x0370, 0x0373}, {0x0371, 0x0374}},
            {{0x0376, 0x0377}, {0x0377, 0x0378}},
            {{0x037b, 0x037d}, {0x03fd, 0x03ff}},
            {{0x0386, 0x0386}, {0x03ac, 0x03ac}},
            {{0x0388, 0x038a}, {0x03ad, 0x03af}},
            {{0x038c, 0x038c}, {0x03cc, 0x03cc}},
            {{0x038e, 0x038f}, {0x03cd, 0x03ce}},
            {{0x0391, 0x03a1}, {0x03b1, 0x03c1}},
            {{0x03a3, 0x03af}, {0x03c3, 0x03cf}},
            {{0x03b1, 0x03d1}, {0x0391, 0x03b1}},
            {{0x03d5, 0x03f2}, {0x03a6, 0x03c3}},
            {{0x03f4, 0x03f5}, {0x03b8, 0x03b9}},
            {{0x03f7, 0x03fb}, {0x03f8, 0x03fc}},
            {{0x03fd, 0x0481}, {0x037b, 0x03ff}},
            {{0x048a, 0x0527}, {0x048b, 0x0528}},
            {{0x0531, 0x0556}, {0x0561, 0x0586}},
            {{0x0561, 0x0586}, {0x0531, 0x0556}},
            {{0x10a0, 0x10c5}, {0x2d00, 0x2d25}},
            {{0x1d79, 0x1d79}, {0xa77d, 0xa77d}},
            {{0x1d7d, 0x1d7d}, {0x2c63, 0x2c63}},
            {{0x1e00, 0x1e95}, {0x1e01, 0x1e96}},
            {{0x1e9b, 0x1e9b}, {0x1e60, 0x1e60}},
            {{0x1e9e, 0x1e9e}, {0x00df, 0x00df}},
            {{0x1ea0, 0x1f15}, {0x1ea1, 0x1f16}},
            {{0x1f18, 0x1f1d}, {0x1f10, 0x1f15}},
            {{0x1f20, 0x1f45}, {0x1f28, 0x1f4d}},
            {{0x1f48, 0x1f4d}, {0x1f40, 0x1f45}},
            {{0x1f51, 0x1f51}, {0x1f59, 0x1f59}},
            {{0x1f53, 0x1f53}, {0x1f5b, 0x1f5b}},
            {{0x1f55, 0x1f55}, {0x1f5d, 0x1f5d}},
            {{0x1f57, 0x1f57}, {0x1f5f, 0x1f5f}},
            {{0x1f59, 0x1f59}, {0x1f51, 0x1f51}},
            {{0x1f5b, 0x1f5b}, {0x1f53, 0x1f53}},
            {{0x1f5d, 0x1f5d}, {0x1f55, 0x1f55}},
            {{0x1f5f, 0x1f7d}, {0x1f57, 0x1f75}},
            {{0x1f80, 0x1f87}, {0x1f88, 0x1f8f}},
            {{0x1f90, 0x1f97}, {0x1f98, 0x1f9f}},
            {{0x1fa0, 0x1fa7}, {0x1fa8, 0x1faf}},
            {{0x1fb0, 0x1fb1}, {0x1fb8, 0x1fb9}},
            {{0x1fb3, 0x1fb3}, {0x1fbc, 0x1fbc}},
            {{0x1fb8, 0x1fbb}, {0x1fb0, 0x1fb3}},
            {{0x1fbe, 0x1fbe}, {0x0399, 0x0399}},
            {{0x1fc3, 0x1fc3}, {0x1fcc, 0x1fcc}},
            {{0x1fc8, 0x1fcb}, {0x1f72, 0x1f75}},
            {{0x1fd0, 0x1fd1}, {0x1fd8, 0x1fd9}},
            {{0x1fd8, 0x1fdb}, {0x1fd0, 0x1fd3}},
            {{0x1fe0, 0x1fe1}, {0x1fe8, 0x1fe9}},
            {{0x1fe5, 0x1fe5}, {0x1fec, 0x1fec}},
            {{0x1fe8, 0x1fec}, {0x1fe0, 0x1fe4}},
            {{0x1ff3, 0x1ff3}, {0x1ffc, 0x1ffc}},
            {{0x1ff8, 0x1ffb}, {0x1f78, 0x1f7b}},
            {{0x2126, 0x2126}, {0x03c9, 0x03c9}},
            {{0x212a, 0x212b}, {0x006b, 0x006c}},
            {{0x2132, 0x2132}, {0x214e, 0x214e}},
            {{0x214e, 0x214e}, {0x2132, 0x2132}},
            {{0x2183, 0x2184}, {0x2184, 0x2185}},
            {{0x2c00, 0x2c2e}, {0x2c30, 0x2c5e}},
            {{0x2c30, 0x2c5e}, {0x2c00, 0x2c2e}},
            {{0x2c60, 0x2c70}, {0x2c61, 0x2c71}},
            {{0x2c72, 0x2c73}, {0x2c73, 0x2c74}},
            {{0x2c75, 0x2c76}, {0x2c76, 0x2c77}},
            {{0x2c7e, 0x2ce3}, {0x023f, 0x02a4}},
            {{0x2ceb, 0x2cee}, {0x2cec, 0x2cef}},
            {{0x2d00, 0x2d25}, {0x10a0, 0x10c5}},
            {{0xa640, 0xa66d}, {0xa641, 0xa66e}},
            {{0xa680, 0xa697}, {0xa681, 0xa698}},
            {{0xa722, 0xa72f}, {0xa723, 0xa730}},
            {{0xa732, 0xa76f}, {0xa733, 0xa770}},
            {{0xa779, 0xa787}, {0xa77a, 0xa788}},
            {{0xa78b, 0xa78d}, {0xa78c, 0xa78e}},
            {{0xa790, 0xa791}, {0xa791, 0xa792}},
            {{0xa7a0, 0xa7a9}, {0xa7a1, 0xa7aa}},
            {{0xff21, 0xff3a}, {0xff41, 0xff5a}},
            {{0xff41, 0xff5a}, {0xff21, 0xff3a}},
            {{0, 0}, {0, 0}}};
        const fold_pair *ptr_ = mapping_;

        for (; ptr_->from.first != 0; ++ptr_)
        {
            if (range_.second < ptr_->from.first) break;

            if (range_.first >= ptr_->from.first &&
                range_.first <= ptr_->from.second)
            {
                out_.insert(typename string_token::range
                    (ptr_->to.first + (range_.first - ptr_->from.first),
                    range_.second > ptr_->from.second ? ptr_->to.second :
                    ptr_->to.first + (range_.second - ptr_->from.first)));
            }
            else if (range_.second >= ptr_->from.first &&
                range_.second <= ptr_->from.second)
            {
                out_.insert(typename string_token::range(ptr_->to.first,
                    ptr_->to.first + (range_.second - ptr_->from.first)));
            }
            // Either range fully encompasses from range or not at all.
            else if (ptr_->from.first >= range_.first &&
                ptr_->from.first <= range_.second)
            {
                out_.insert(typename string_token::range(ptr_->to.first,
                    ptr_->to.second));
            }
        }
    }

    static void fold(const typename string_token::range &range_,
        const std::locale &locale_, string_token &out_, const four &)
    {
        if (range_.first < 0x10000)
        {
            fold(range_, locale_, out_, two());
        }

        static const fold_pair mapping_[] =
            {{{0x10400, 0x1044f}, {0x10428, 0x10477}},
            {{0, 0}, {0, 0}}};
        const fold_pair *ptr_ = mapping_;

        for (; ptr_->from.first != 0; ++ptr_)
        {
            if (range_.second < ptr_->from.first) break;

            if (range_.first >= ptr_->from.first &&
                range_.first <= ptr_->from.second)
            {
                out_.insert(typename string_token::range
                    (ptr_->to.first + (range_.first - ptr_->from.first),
                    range_.second > ptr_->from.second ? ptr_->to.second :
                    ptr_->to.first + (range_.second - ptr_->from.first)));
            }
            else if (range_.second >= ptr_->from.first &&
                range_.second <= ptr_->from.second)
            {
                out_.insert(typename string_token::range(ptr_->to.first,
                    ptr_->to.first + (range_.second - ptr_->from.first)));
            }
            // Either range fully encompasses from range or not at all.
            else if (ptr_->from.first >= range_.first &&
                ptr_->from.first <= range_.second)
            {
                out_.insert(typename string_token::range(ptr_->to.first,
                    ptr_->to.second));
            }
        }
    }

    template<typename state_type>
    static input_char_type chr(state_type &state_)
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
    static void posix(state_type &state_, string_token &token_)
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
    static void alnum_alpha(state_type &state_, string_token &token_,
        const bool negate_)
    {
        enum {unknown, alnum, alpha};
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
                    std::string("[\\p{Ll}\\p{Lu}\\p{Nd}]");
            }
            else
            {
                // alpha
                str_ = sizeof(input_char_type) == 1 ?
                    make_alpha(state_._locale) :
                    std::string("[\\p{Ll}\\p{Lu}]");
            }

            insert_charset(str_.c_str(), state_, token_, negate_);
        }
    }

    static std::string make_alnum(std::locale &locale_)
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

    static std::string make_alpha(std::locale &locale_)
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
    static void blank(state_type &state_, string_token &token_,
        const bool negate_)
    {
        const char *blank_ = "lank";

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
            const char *str_ = sizeof(input_char_type) == 1 ?
                "[ \t]" : "[\\p{Zs}\t]";

            check_posix_termination(state_);
            insert_charset(str_, state_, token_, negate_);
        }
    }

    template<typename state_type>
    static void cntrl(state_type &state_, string_token &token_,
        const bool negate_)
    {
        const char *cntrl_ = "ntrl";

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
            const char *str_ = sizeof(input_char_type) == 1 ?
                "[\\x00-\x1f\x7f]" : "[\\p{Cc}]";

            check_posix_termination(state_);
            insert_charset(str_, state_, token_, negate_);
        }
    }

    template<typename state_type>
    static void digit(state_type &state_, string_token &token_,
        const bool negate_)
    {
        const char *digit_ = "igit";

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
            const char *str_ = sizeof(input_char_type) == 1 ?
                "[0-9]" : "[\\p{Nd}]";

            check_posix_termination(state_);
            insert_charset(str_, state_, token_, negate_);
        }
    }

    template<typename state_type>
    static void graph(state_type &state_, string_token &token_,
        const bool negate_)
    {
        const char *graph_ = "raph";

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
            const char *str_ = sizeof(input_char_type) == 1 ?
                "[\x21-\x7e]" : "[^\\p{Z}\\p{C}]";

            check_posix_termination(state_);
            insert_charset(str_, state_, token_, negate_);
        }
    }

    template<typename state_type>
    static void lower(state_type &state_, string_token &token_,
        const bool negate_)
    {
        const char *lower_ = "ower";

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

    static std::string create_lower(std::locale &locale_)
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
    static void print_punct(state_type &state_, string_token &token_,
        const bool negate_)
    {
        enum {unknown, print, punct};
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
            const char *str_ = 0;

            check_posix_termination(state_);

            if (type_ == print)
            {
                // print
                str_ = sizeof(input_char_type) == 1 ?
                    "[\x20-\x7e]" : "[\\p{C}]";
            }
            else
            {
                // punct
                str_ = sizeof(input_char_type) == 1 ?
                    "[!\"#$%&'()*+,\\-./:;<=>?@[\\\\\\]^_`{|}~]" :
                    "[\\p{P}\\p{S}]";
            }

            insert_charset(str_, state_, token_, negate_);
        }
    }

    template<typename state_type>
    static void space(state_type &state_, string_token &token_,
        const bool negate_)
    {
        const char *space_ = "pace";

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
            const char *str_ = sizeof(input_char_type) == 1 ?
                "[ \t\r\n\v\f]" : "[\\p{Z}\t\r\n\v\f]";

            check_posix_termination(state_);
            insert_charset(str_, state_, token_, negate_);
        }
    }

    template<typename state_type>
    static void upper(state_type &state_, string_token &token_,
        const bool negate_)
    {
        const char *upper_ = "pper";

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

    static std::string create_upper(std::locale &locale_)
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
    static void xdigit(state_type &state_, string_token &token_,
        const bool negate_)
    {
        const char *xdigit_ = "digit";

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
            const char *str_ = "[0-9A-Fa-f]";

            check_posix_termination(state_);
            insert_charset(str_, state_, token_, negate_);
        }
    }

    template<typename state_type>
    static void check_posix_termination(state_type &state_)
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
    static void unterminated_posix(state_type &state_)
    {
        std::ostringstream ss_;

        // Pointless returning index if at end of string
        state_.unexpected_end(ss_);
        ss_ << " (unterminated POSIX charset)";
        state_.error(ss_);
        throw runtime_error(ss_.str());
    }

    template<typename state_type>
    static void unknown_posix(state_type &state_)
    {
        std::ostringstream ss_;

        ss_ << "Unknown POSIX charset at index " <<
            state_.index();
        state_.error(ss_);
        throw runtime_error(ss_.str());
    }

    template<typename state_type>
    static void insert_charset(const char *str_, state_type &state_,
        string_token &token_, const bool negate_)
    {
        // Some systems have strlen in namespace std.
        using namespace std;

        char_state temp_state_(str_ + 1, str_ + strlen(str_),
            state_._id, state_._flags, state_._locale, 0);
        string_token temp_token_;

        charset(temp_state_, temp_token_);

        if (negate_) temp_token_.negate();

        token_.insert(temp_token_);
    }

    template<typename state_type>
    static const char *charset_shortcut
        (state_type &state_, std::size_t &str_len_)
    {
        const char *str_ = 0;

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
    static const char *unicode_escape(state_type &state_)
    {
        const char *str_ = 0;

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

        switch (*state_._curr)
        {
            case 'C':
                state_.increment();

                if (state_.eos())
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\p{C";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                switch (*state_._curr)
                {
                    case '}':
                        str_ = "[\\p{Cc}\\p{Cf}\\p{Co}\\p{Cs}]";
                        break;
                    case 'c':
                        str_ = other_control();
                        state_.increment();
                        break;
                    case 'f':
                        str_ = other_format();
                        state_.increment();
                        break;
//                    case 'n':
//                        break;
                    case 'o':
                        str_ = other_private();
                        state_.increment();
                        break;
                    case 's':
                        str_ = other_surrogate();
                        state_.increment();
                        break;
                    default:
                    {
                        std::ostringstream ss_;

                        ss_ << "Syntax error following \\p{C at index " <<
                            state_.index();
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                }

                break;
            case 'L':
                state_.increment();

                if (state_.eos())
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\p{L";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                switch (*state_._curr)
                {
                    case '}':
                        str_ = "[\\p{Ll}\\p{Lm}\\p{Lo}\\p{Lt}\\p{Lu}]";
                        break;
                    case 'C':
                        str_ = "[\\p{Ll}\\p{Lt}\\p{Lu}]";
                        state_.increment();
                        break;
                    case 'l':
                        str_ = letter_lowercase();
                        state_.increment();
                        break;
                    case 'm':
                        str_ = letter_modifier();
                        state_.increment();
                        break;
                    case 'o':
                        str_ = letter_other();
                        state_.increment();
                        break;
                    case 't':
                        str_ = letter_titlecase();
                        state_.increment();
                        break;
                    case 'u':
                        str_ = letter_uppercase();
                        state_.increment();
                        break;
                    default:
                    {
                        std::ostringstream ss_;

                        ss_ << "Syntax error following \\p{L at index " <<
                            state_.index();
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                }

                break;
            case 'M':
                state_.increment();

                if (state_.eos())
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\p{M";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                switch (*state_._curr)
                {
                    case '}':
                        str_ = "[\\p{Mc}\\p{Me}\\p{Mn}]";
                        break;
                    case 'c':
                        str_ = mark_combining();
                        state_.increment();
                        break;
                    case 'e':
                        str_ = mark_enclosing();
                        state_.increment();
                        break;
                    case 'n':
                        str_ = mark_nonspacing();
                        state_.increment();
                        break;
                    default:
                    {
                        std::ostringstream ss_;

                        ss_ << "Syntax error following \\p{M at index " <<
                            state_.index();
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                }

                break;
            case 'N':
                state_.increment();

                if (state_.eos())
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\p{N";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                switch (*state_._curr)
                {
                    case '}':
                        str_ = "[\\p{Nd}\\p{Nl}\\p{No}]";
                        break;
                    case 'd':
                        str_ = number_decimal();
                        state_.increment();
                        break;
                    case 'l':
                        str_ = number_letter();
                        state_.increment();
                        break;
                    case 'o':
                        str_ = number_other();
                        state_.increment();
                        break;
                    default:
                    {
                        std::ostringstream ss_;

                        ss_ << "Syntax error following \\p{N at index " <<
                            state_.index();
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                }

                break;
            case 'P':
                state_.increment();

                if (state_.eos())
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\p{P";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                switch (*state_._curr)
                {
                    case '}':
                        str_ = "[\\p{Pc}\\p{Pd}\\p{Pe}\\p{Pf}\\p{Pi}\\p{Po}"
                            "\\p{Ps}]";
                        break;
                    case 'c':
                        str_ = punctuation_connector();
                        state_.increment();
                        break;
                    case 'd':
                        str_ = punctuation_dash();
                        state_.increment();
                        break;
                    case 'e':
                        str_ = punctuation_close();
                        state_.increment();
                        break;
                    case 'f':
                        str_ = punctuation_final();
                        state_.increment();
                        break;
                    case 'i':
                        str_ = punctuation_initial();
                        state_.increment();
                        break;
                    case 'o':
                        str_ = punctuation_other();
                        state_.increment();
                        break;
                    case 's':
                        str_ = punctuation_open();
                        state_.increment();
                        break;
                    default:
                    {
                        std::ostringstream ss_;

                        ss_ << "Syntax error following \\p{P at index " <<
                            state_.index();
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                }

                break;
            case 'S':
                state_.increment();

                if (state_.eos())
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\p{S";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                switch (*state_._curr)
                {
                    case '}':
                        str_ = "[\\p{Sc}\\p{Sk}\\p{Sm}\\p{So}]";
                        break;
                    case 'c':
                        str_ = symbol_currency();
                        state_.increment();
                        break;
                    case 'k':
                        str_ = symbol_modifier();
                        state_.increment();
                        break;
                    case 'm':
                        str_ = symbol_math();
                        state_.increment();
                        break;
                    case 'o':
                        str_ = symbol_other();
                        state_.increment();
                        break;
                    default:
                    {
                        std::ostringstream ss_;

                        ss_ << "Syntax error following \\p{S at index " <<
                            state_.index();
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                }

                break;
            case 'Z':
                state_.increment();

                if (state_.eos())
                {
                    std::ostringstream ss_;

                    // Pointless returning index if at end of string
                    state_.unexpected_end(ss_);
                    ss_ << " following \\p{Z";
                    state_.error(ss_);
                    throw runtime_error(ss_.str());
                }

                switch (*state_._curr)
                {
                    case '}':
                        str_ = "[\\p{Zl}\\p{Zp}\\p{Zs}]";
                        break;
                    case 'l':
                        str_ = separator_line();
                        state_.increment();
                        break;
                    case 'p':
                        str_ = separator_paragraph();
                        state_.increment();
                        break;
                    case 's':
                        str_ = separator_space();
                        state_.increment();
                        break;
                    default:
                    {
                        std::ostringstream ss_;

                        ss_ << "Syntax error following \\p{Z at index " <<
                            state_.index();
                        state_.error(ss_);
                        throw runtime_error(ss_.str());
                    }
                }

                break;
            default:
            {
                std::ostringstream ss_;

                ss_ << "Syntax error following \\p{ at index " <<
                    state_.index();
                state_.error(ss_);
                throw runtime_error(ss_.str());
            }
        }

        if (*state_._curr != '}')
        {
            std::ostringstream ss_;

            ss_ << "Missing '}' at index " << state_.index();
            state_.error(ss_);
            throw runtime_error(ss_.str());
        }

        return str_;
    }

    static const char *letter_uppercase()
    {
        return "[\\x41-\\x5a\\xc0-\\xd6\\xd8-\\xde\\x100\\x102\\x104\\x106"
            "\\x108\\x10a\\x10c\\x10e\\x110\\x112\\x114\\x116\\x118\\x11a"
            "\\x11c\\x11e\\x120\\x122\\x124\\x126\\x128\\x12a\\x12c\\x12e"
            "\\x130\\x132\\x134\\x136\\x139\\x13b\\x13d\\x13f\\x141\\x143"
            "\\x145\\x147\\x14a\\x14c\\x14e\\x150\\x152\\x154\\x156\\x158"
            "\\x15a\\x15c\\x15e\\x160\\x162\\x164\\x166\\x168\\x16a\\x16c"
            "\\x16e\\x170\\x172\\x174\\x176\\x178\\x179\\x17b\\x17d\\x181"
            "\\x182\\x184\\x186\\x187\\x189-\\x18b\\x18e-\\x191\\x193\\x194"
            "\\x196-\\x198\\x19c\\x19d\\x19f\\x1a0\\x1a2\\x1a4\\x1a6\\x1a7"
            "\\x1a9\\x1ac\\x1ae\\x1af\\x1b1-\\x1b3\\x1b5\\x1b7\\x1b8\\x1bc"
            "\\x1c4\\x1c7\\x1ca\\x1cd\\x1cf\\x1d1\\x1d3\\x1d5\\x1d7\\x1d9"
            "\\x1db\\x1de\\x1e0\\x1e2\\x1e4\\x1e6\\x1e8\\x1ea\\x1ec\\x1ee"
            "\\x1f1\\x1f4\\x1f6-\\x1f8\\x1fa\\x1fc\\x1fe\\x200\\x202\\x204"
            "\\x206\\x208\\x20a\\x20c\\x20e\\x210\\x212\\x214\\x216\\x218"
            "\\x21a\\x21c\\x21e\\x220\\x222\\x224\\x226\\x228\\x22a\\x22c"
            "\\x22e\\x230\\x232\\x23a\\x23b\\x23d\\x23e\\x241\\x243-\\x246"
            "\\x248\\x24a\\x24c\\x24e\\x370\\x372\\x376\\x386\\x388-\\x38a"
            "\\x38c\\x38e\\x38f\\x391-\\x3a1\\x3a3-\\x3ab\\x3cf\\x3d2-\\x3d4"
            "\\x3d8\\x3da\\x3dc\\x3de\\x3e0\\x3e2\\x3e4\\x3e6\\x3e8\\x3ea"
            "\\x3ec\\x3ee\\x3f4\\x3f7\\x3f9\\x3fa\\x3fd-\\x42f\\x460\\x462"
            "\\x464\\x466\\x468\\x46a\\x46c\\x46e\\x470\\x472\\x474\\x476"
            "\\x478\\x47a\\x47c\\x47e\\x480\\x48a\\x48c\\x48e\\x490\\x492"
            "\\x494\\x496\\x498\\x49a\\x49c\\x49e\\x4a0\\x4a2\\x4a4\\x4a6"
            "\\x4a8\\x4aa\\x4ac\\x4ae\\x4b0\\x4b2\\x4b4\\x4b6\\x4b8\\x4ba"
            "\\x4bc\\x4be\\x4c0\\x4c1\\x4c3\\x4c5\\x4c7\\x4c9\\x4cb\\x4cd"
            "\\x4d0\\x4d2\\x4d4\\x4d6\\x4d8\\x4da\\x4dc\\x4de\\x4e0\\x4e2"
            "\\x4e4\\x4e6\\x4e8\\x4ea\\x4ec\\x4ee\\x4f0\\x4f2\\x4f4\\x4f6"
            "\\x4f8\\x4fa\\x4fc\\x4fe\\x500\\x502\\x504\\x506\\x508\\x50a"
            "\\x50c\\x50e\\x510\\x512\\x514\\x516\\x518\\x51a\\x51c\\x51e"
            "\\x520\\x522\\x524\\x526\\x531-\\x556\\x10a0-\\x10c5\\x1e00"
            "\\x1e02\\x1e04\\x1e06\\x1e08\\x1e0a\\x1e0c\\x1e0e\\x1e10\\x1e12"
            "\\x1e14\\x1e16\\x1e18\\x1e1a\\x1e1c\\x1e1e\\x1e20\\x1e22\\x1e24"
            "\\x1e26\\x1e28\\x1e2a\\x1e2c\\x1e2e\\x1e30\\x1e32\\x1e34\\x1e36"
            "\\x1e38\\x1e3a\\x1e3c\\x1e3e\\x1e40\\x1e42\\x1e44\\x1e46\\x1e48"
            "\\x1e4a\\x1e4c\\x1e4e\\x1e50\\x1e52\\x1e54\\x1e56\\x1e58\\x1e5a"
            "\\x1e5c\\x1e5e\\x1e60\\x1e62\\x1e64\\x1e66\\x1e68\\x1e6a\\x1e6c"
            "\\x1e6e\\x1e70\\x1e72\\x1e74\\x1e76\\x1e78\\x1e7a\\x1e7c\\x1e7e"
            "\\x1e80\\x1e82\\x1e84\\x1e86\\x1e88\\x1e8a\\x1e8c\\x1e8e\\x1e90"
            "\\x1e92\\x1e94\\x1e9e\\x1ea0\\x1ea2\\x1ea4\\x1ea6\\x1ea8\\x1eaa"
            "\\x1eac\\x1eae\\x1eb0\\x1eb2\\x1eb4\\x1eb6\\x1eb8\\x1eba\\x1ebc"
            "\\x1ebe\\x1ec0\\x1ec2\\x1ec4\\x1ec6\\x1ec8\\x1eca\\x1ecc\\x1ece"
            "\\x1ed0\\x1ed2\\x1ed4\\x1ed6\\x1ed8\\x1eda\\x1edc\\x1ede\\x1ee0"
            "\\x1ee2\\x1ee4\\x1ee6\\x1ee8\\x1eea\\x1eec\\x1eee\\x1ef0\\x1ef2"
            "\\x1ef4\\x1ef6\\x1ef8\\x1efa\\x1efc\\x1efe\\x1f08-\\x1f0f"
            "\\x1f18-\\x1f1d\\x1f28-\\x1f2f\\x1f38-\\x1f3f\\x1f48-\\x1f4d"
            "\\x1f59\\x1f5b\\x1f5d\\x1f5f\\x1f68-\\x1f6f\\x1fb8-\\x1fbb"
            "\\x1fc8-\\x1fcb\\x1fd8-\\x1fdb\\x1fe8-\\x1fec\\x1ff8-\\x1ffb"
            "\\x2102\\x2107\\x210b-\\x210d\\x2110-\\x2112\\x2115"
            "\\x2119-\\x211d\\x2124\\x2126\\x2128\\x212a-\\x212d"
            "\\x2130-\\x2133\\x213e\\x213f\\x2145\\x2183\\x2c00-\\x2c2e"
            "\\x2c60\\x2c62-\\x2c64\\x2c67\\x2c69\\x2c6b\\x2c6d-\\x2c70"
            "\\x2c72\\x2c75\\x2c7e-\\x2c80\\x2c82\\x2c84\\x2c86\\x2c88\\x2c8a"
            "\\x2c8c\\x2c8e\\x2c90\\x2c92\\x2c94\\x2c96\\x2c98\\x2c9a\\x2c9c"
            "\\x2c9e\\x2ca0\\x2ca2\\x2ca4\\x2ca6\\x2ca8\\x2caa\\x2cac\\x2cae"
            "\\x2cb0\\x2cb2\\x2cb4\\x2cb6\\x2cb8\\x2cba\\x2cbc\\x2cbe\\x2cc0"
            "\\x2cc2\\x2cc4\\x2cc6\\x2cc8\\x2cca\\x2ccc\\x2cce\\x2cd0\\x2cd2"
            "\\x2cd4\\x2cd6\\x2cd8\\x2cda\\x2cdc\\x2cde\\x2ce0\\x2ce2\\x2ceb"
            "\\x2ced\\xa640\\xa642\\xa644\\xa646\\xa648\\xa64a\\xa64c\\xa64e"
            "\\xa650\\xa652\\xa654\\xa656\\xa658\\xa65a\\xa65c\\xa65e\\xa660"
            "\\xa662\\xa664\\xa666\\xa668\\xa66a\\xa66c\\xa680\\xa682\\xa684"
            "\\xa686\\xa688\\xa68a\\xa68c\\xa68e\\xa690\\xa692\\xa694\\xa696"
            "\\xa722\\xa724\\xa726\\xa728\\xa72a\\xa72c\\xa72e\\xa732\\xa734"
            "\\xa736\\xa738\\xa73a\\xa73c\\xa73e\\xa740\\xa742\\xa744\\xa746"
            "\\xa748\\xa74a\\xa74c\\xa74e\\xa750\\xa752\\xa754\\xa756\\xa758"
            "\\xa75a\\xa75c\\xa75e\\xa760\\xa762\\xa764\\xa766\\xa768\\xa76a"
            "\\xa76c\\xa76e\\xa779\\xa77b\\xa77d\\xa77e\\xa780\\xa782\\xa784"
            "\\xa786\\xa78b\\xa78d\\xa790\\xa7a0\\xa7a2\\xa7a4\\xa7a6\\xa7a8"
            "\\xff21-\\xff3a\\x10400-\\x10427\\x1d400-\\x1d419"
            "\\x1d434-\\x1d44d\\x1d468-\\x1d481\\x1d49c\\x1d49e\\x1d49f"
            "\\x1d4a2\\x1d4a5\\x1d4a6\\x1d4a9-\\x1d4ac\\x1d4ae-\\x1d4b5"
            "\\x1d4d0-\\x1d4e9\\x1d504\\x1d505\\x1d507-\\x1d50a"
            "\\x1d50d-\\x1d514\\x1d516-\\x1d51c\\x1d538\\x1d539"
            "\\x1d53b-\\x1d53e\\x1d540-\\x1d544\\x1d546\\x1d54a-\\x1d550"
            "\\x1d56c-\\x1d585\\x1d5a0-\\x1d5b9\\x1d5d4-\\x1d5ed"
            "\\x1d608-\\x1d621\\x1d63c-\\x1d655\\x1d670-\\x1d689"
            "\\x1d6a8-\\x1d6c0\\x1d6e2-\\x1d6fa\\x1d71c-\\x1d734"
            "\\x1d756-\\x1d76e\\x1d790-\\x1d7a8\\x1d7ca]";
    }

    static const char *letter_lowercase()
    {
        return "[\\x61-\\x7a\\xaa\\xb5\\xba\\xdf-\\xf6\\xf8-\\xff\\x101"
            "\\x103\\x105\\x107\\x109\\x10b\\x10d\\x10f\\x111\\x113\\x115"
            "\\x117\\x119\\x11b\\x11d\\x11f\\x121\\x123\\x125\\x127\\x129"
            "\\x12b\\x12d\\x12f\\x131\\x133\\x135\\x137\\x138\\x13a\\x13c"
            "\\x13e\\x140\\x142\\x144\\x146\\x148\\x149\\x14b\\x14d\\x14f"
            "\\x151\\x153\\x155\\x157\\x159\\x15b\\x15d\\x15f\\x161\\x163"
            "\\x165\\x167\\x169\\x16b\\x16d\\x16f\\x171\\x173\\x175\\x177"
            "\\x17a\\x17c\\x17e-\\x180\\x183\\x185\\x188\\x18c\\x18d\\x192"
            "\\x195\\x199-\\x19b\\x19e\\x1a1\\x1a3\\x1a5\\x1a8\\x1aa\\x1ab"
            "\\x1ad\\x1b0\\x1b4\\x1b6\\x1b9\\x1ba\\x1bd-\\x1bf\\x1c6\\x1c9"
            "\\x1cc\\x1ce\\x1d0\\x1d2\\x1d4\\x1d6\\x1d8\\x1da\\x1dc\\x1dd"
            "\\x1df\\x1e1\\x1e3\\x1e5\\x1e7\\x1e9\\x1eb\\x1ed\\x1ef\\x1f0"
            "\\x1f3\\x1f5\\x1f9\\x1fb\\x1fd\\x1ff\\x201\\x203\\x205\\x207"
            "\\x209\\x20b\\x20d\\x20f\\x211\\x213\\x215\\x217\\x219\\x21b"
            "\\x21d\\x21f\\x221\\x223\\x225\\x227\\x229\\x22b\\x22d\\x22f"
            "\\x231\\x233-\\x239\\x23c\\x23f\\x240\\x242\\x247\\x249\\x24b"
            "\\x24d\\x24f-\\x293\\x295-\\x2af\\x371\\x373\\x377\\x37b-\\x37d"
            "\\x390\\x3ac-\\x3ce\\x3d0\\x3d1\\x3d5-\\x3d7\\x3d9\\x3db\\x3dd"
            "\\x3df\\x3e1\\x3e3\\x3e5\\x3e7\\x3e9\\x3eb\\x3ed\\x3ef-\\x3f3"
            "\\x3f5\\x3f8\\x3fb\\x3fc\\x430-\\x45f\\x461\\x463\\x465\\x467"
            "\\x469\\x46b\\x46d\\x46f\\x471\\x473\\x475\\x477\\x479\\x47b"
            "\\x47d\\x47f\\x481\\x48b\\x48d\\x48f\\x491\\x493\\x495\\x497"
            "\\x499\\x49b\\x49d\\x49f\\x4a1\\x4a3\\x4a5\\x4a7\\x4a9\\x4ab"
            "\\x4ad\\x4af\\x4b1\\x4b3\\x4b5\\x4b7\\x4b9\\x4bb\\x4bd\\x4bf"
            "\\x4c2\\x4c4\\x4c6\\x4c8\\x4ca\\x4cc\\x4ce\\x4cf\\x4d1\\x4d3"
            "\\x4d5\\x4d7\\x4d9\\x4db\\x4dd\\x4df\\x4e1\\x4e3\\x4e5\\x4e7"
            "\\x4e9\\x4eb\\x4ed\\x4ef\\x4f1\\x4f3\\x4f5\\x4f7\\x4f9\\x4fb"
            "\\x4fd\\x4ff\\x501\\x503\\x505\\x507\\x509\\x50b\\x50d\\x50f"
            "\\x511\\x513\\x515\\x517\\x519\\x51b\\x51d\\x51f\\x521\\x523"
            "\\x525\\x527\\x561-\\x587\\x1d00-\\x1d2b\\x1d62-\\x1d77"
            "\\x1d79-\\x1d9a\\x1e01\\x1e03\\x1e05\\x1e07\\x1e09\\x1e0b"
            "\\x1e0d\\x1e0f\\x1e11\\x1e13\\x1e15\\x1e17\\x1e19\\x1e1b"
            "\\x1e1d\\x1e1f\\x1e21\\x1e23\\x1e25\\x1e27\\x1e29\\x1e2b\\x1e2d"
            "\\x1e2f\\x1e31\\x1e33\\x1e35\\x1e37\\x1e39\\x1e3b\\x1e3d\\x1e3f"
            "\\x1e41\\x1e43\\x1e45\\x1e47\\x1e49\\x1e4b\\x1e4d\\x1e4f\\x1e51"
            "\\x1e53\\x1e55\\x1e57\\x1e59\\x1e5b\\x1e5d\\x1e5f\\x1e61\\x1e63"
            "\\x1e65\\x1e67\\x1e69\\x1e6b\\x1e6d\\x1e6f\\x1e71\\x1e73\\x1e75"
            "\\x1e77\\x1e79\\x1e7b\\x1e7d\\x1e7f\\x1e81\\x1e83\\x1e85\\x1e87"
            "\\x1e89\\x1e8b\\x1e8d\\x1e8f\\x1e91\\x1e93\\x1e95-\\x1e9d\\x1e9f"
            "\\x1ea1\\x1ea3\\x1ea5\\x1ea7\\x1ea9\\x1eab\\x1ead\\x1eaf\\x1eb1"
            "\\x1eb3\\x1eb5\\x1eb7\\x1eb9\\x1ebb\\x1ebd\\x1ebf\\x1ec1\\x1ec3"
            "\\x1ec5\\x1ec7\\x1ec9\\x1ecb\\x1ecd\\x1ecf\\x1ed1\\x1ed3\\x1ed5"
            "\\x1ed7\\x1ed9\\x1edb\\x1edd\\x1edf\\x1ee1\\x1ee3\\x1ee5\\x1ee7"
            "\\x1ee9\\x1eeb\\x1eed\\x1eef\\x1ef1\\x1ef3\\x1ef5\\x1ef7\\x1ef9"
            "\\x1efb\\x1efd\\x1eff-\\x1f07\\x1f10-\\x1f15\\x1f20-\\x1f27"
            "\\x1f30-\\x1f37\\x1f40-\\x1f45\\x1f50-\\x1f57\\x1f60-\\x1f67"
            "\\x1f70-\\x1f7d\\x1f80-\\x1f87\\x1f90-\\x1f97\\x1fa0-\\x1fa7"
            "\\x1fb0-\\x1fb4\\x1fb6\\x1fb7\\x1fbe\\x1fc2-\\x1fc4\\x1fc6"
            "\\x1fc7\\x1fd0-\\x1fd3\\x1fd6\\x1fd7\\x1fe0-\\x1fe7"
            "\\x1ff2-\\x1ff4\\x1ff6\\x1ff7\\x210a\\x210e\\x210f\\x2113"
            "\\x212f\\x2134\\x2139\\x213c\\x213d\\x2146-\\x2149\\x214e"
            "\\x2184\\x2c30-\\x2c5e\\x2c61\\x2c65\\x2c66\\x2c68\\x2c6a"
            "\\x2c6c\\x2c71\\x2c73\\x2c74\\x2c76-\\x2c7c\\x2c81\\x2c83"
            "\\x2c85\\x2c87\\x2c89\\x2c8b\\x2c8d\\x2c8f\\x2c91\\x2c93\\x2c95"
            "\\x2c97\\x2c99\\x2c9b\\x2c9d\\x2c9f\\x2ca1\\x2ca3\\x2ca5\\x2ca7"
            "\\x2ca9\\x2cab\\x2cad\\x2caf\\x2cb1\\x2cb3\\x2cb5\\x2cb7\\x2cb9"
            "\\x2cbb\\x2cbd\\x2cbf\\x2cc1\\x2cc3\\x2cc5\\x2cc7\\x2cc9\\x2ccb"
            "\\x2ccd\\x2ccf\\x2cd1\\x2cd3\\x2cd5\\x2cd7\\x2cd9\\x2cdb\\x2cdd"
            "\\x2cdf\\x2ce1\\x2ce3\\x2ce4\\x2cec\\x2cee\\x2d00-\\x2d25\\xa641"
            "\\xa643\\xa645\\xa647\\xa649\\xa64b\\xa64d\\xa64f\\xa651\\xa653"
            "\\xa655\\xa657\\xa659\\xa65b\\xa65d\\xa65f\\xa661\\xa663\\xa665"
            "\\xa667\\xa669\\xa66b\\xa66d\\xa681\\xa683\\xa685\\xa687\\xa689"
            "\\xa68b\\xa68d\\xa68f\\xa691\\xa693\\xa695\\xa697\\xa723\\xa725"
            "\\xa727\\xa729\\xa72b\\xa72d\\xa72f-\\xa731\\xa733\\xa735\\xa737"
            "\\xa739\\xa73b\\xa73d\\xa73f\\xa741\\xa743\\xa745\\xa747\\xa749"
            "\\xa74b\\xa74d\\xa74f\\xa751\\xa753\\xa755\\xa757\\xa759\\xa75b"
            "\\xa75d\\xa75f\\xa761\\xa763\\xa765\\xa767\\xa769\\xa76b\\xa76d"
            "\\xa76f\\xa771-\\xa778\\xa77a\\xa77c\\xa77f\\xa781\\xa783"
            "\\xa785\\xa787\\xa78c\\xa78e\\xa791\\xa7a1\\xa7a3\\xa7a5\\xa7a7"
            "\\xa7a9\\xa7fa\\xfb00-\\xfb06\\xfb13-\\xfb17\\xff41-\\xff5a"
            "\\x10428-\\x1044f\\x1d41a-\\x1d433\\x1d44e-\\x1d454"
            "\\x1d456-\\x1d467\\x1d482-\\x1d49b\\x1d4b6-\\x1d4b9\\x1d4bb"
            "\\x1d4bd-\\x1d4c3\\x1d4c5-\\x1d4cf\\x1d4ea-\\x1d503"
            "\\x1d51e-\\x1d537\\x1d552-\\x1d56b\\x1d586-\\x1d59f"
            "\\x1d5ba-\\x1d5d3\\x1d5ee-\\x1d607\\x1d622-\\x1d63b"
            "\\x1d656-\\x1d66f\\x1d68a-\\x1d6a5\\x1d6c2-\\x1d6da"
            "\\x1d6dc-\\x1d6e1\\x1d6fc-\\x1d714\\x1d716-\\x1d71b"
            "\\x1d736-\\x1d74e\\x1d750-\\x1d755\\x1d770-\\x1d788"
            "\\x1d78a-\\x1d78f\\x1d7aa-\\x1d7c2\\x1d7c4-\\x1d7c9\\x1d7cb]";
    }

    static const char *letter_titlecase()
    {
        return "[\\x1c5\\x1c8\\x1cb\\x1f2\\x1f88-\\x1f8f\\x1f98-\\x1f9f"
            "\\x1fa8-\\x1faf\\x1fbc\\x1fcc\\x1ffc]";
    }

    static const char *letter_modifier()
    {
        return "[\\x2b0-\\x2c1\\x2c6-\\x2d1\\x2e0-\\x2e4\\x2ec\\x2ee\\x374"
            "\\x37a\\x559\\x640\\x6e5\\x6e6\\x7f4\\x7f5\\x7fa\\x81a\\x824"
            "\\x828\\x971\\xe46\\xec6\\x10fc\\x17d7\\x1843\\x1aa7"
            "\\x1c78-\\x1c7d\\x1d2c-\\x1d61\\x1d78\\x1d9b-\\x1dbf\\x2071"
            "\\x207f\\x2090-\\x209c\\x2c7d\\x2d6f\\x2e2f\\x3005"
            "\\x3031-\\x3035\\x303b\\x309d\\x309e\\x30fc-\\x30fe\\xa015"
            "\\xa4f8-\\xa4fd\\xa60c\\xa67f\\xa717-\\xa71f\\xa770\\xa788"
            "\\xa9cf\\xaa70\\xaadd\\xff70\\xff9e\\xff9f]";
    }

    static const char *letter_other()
    {
        return "[\\x1bb\\x1c0-\\x1c3\\x294\\x5d0-\\x5ea\\x5f0-\\x5f2"
            "\\x620-\\x63f\\x641-\\x64a\\x66e\\x66f\\x671-\\x6d3\\x6d5\\x6ee"
            "\\x6ef\\x6fa-\\x6fc\\x6ff\\x710\\x712-\\x72f\\x74d-\\x7a5\\x7b1"
            "\\x7ca-\\x7ea\\x800-\\x815\\x840-\\x858\\x904-\\x939\\x93d"
            "\\x950\\x958-\\x961\\x972-\\x977\\x979-\\x97f\\x985-\\x98c\\x98f"
            "\\x990\\x993-\\x9a8\\x9aa-\\x9b0\\x9b2\\x9b6-\\x9b9\\x9bd\\x9ce"
            "\\x9dc\\x9dd\\x9df-\\x9e1\\x9f0\\x9f1\\xa05-\\xa0a\\xa0f\\xa10"
            "\\xa13-\\xa28\\xa2a-\\xa30\\xa32\\xa33\\xa35\\xa36\\xa38\\xa39"
            "\\xa59-\\xa5c\\xa5e\\xa72-\\xa74\\xa85-\\xa8d\\xa8f-\\xa91"
            "\\xa93-\\xaa8\\xaaa-\\xab0\\xab2\\xab3\\xab5-\\xab9\\xabd\\xad0"
            "\\xae0\\xae1\\xb05-\\xb0c\\xb0f\\xb10\\xb13-\\xb28\\xb2a-\\xb30"
            "\\xb32\\xb33\\xb35-\\xb39\\xb3d\\xb5c\\xb5d\\xb5f-\\xb61\\xb71"
            "\\xb83\\xb85-\\xb8a\\xb8e-\\xb90\\xb92-\\xb95\\xb99\\xb9a\\xb9c"
            "\\xb9e\\xb9f\\xba3\\xba4\\xba8-\\xbaa\\xbae-\\xbb9\\xbd0"
            "\\xc05-\\xc0c\\xc0e-\\xc10\\xc12-\\xc28\\xc2a-\\xc33"
            "\\xc35-\\xc39\\xc3d\\xc58\\xc59\\xc60\\xc61\\xc85-\\xc8c"
            "\\xc8e-\\xc90\\xc92-\\xca8\\xcaa-\\xcb3\\xcb5-\\xcb9\\xcbd"
            "\\xcde\\xce0\\xce1\\xcf1\\xcf2\\xd05-\\xd0c\\xd0e-\\xd10"
            "\\xd12-\\xd3a\\xd3d\\xd4e\\xd60\\xd61\\xd7a-\\xd7f\\xd85-\\xd96"
            "\\xd9a-\\xdb1\\xdb3-\\xdbb\\xdbd\\xdc0-\\xdc6\\xe01-\\xe30\\xe32"
            "\\xe33\\xe40-\\xe45\\xe81\\xe82\\xe84\\xe87\\xe88\\xe8a\\xe8d"
            "\\xe94-\\xe97\\xe99-\\xe9f\\xea1-\\xea3\\xea5\\xea7\\xeaa\\xeab"
            "\\xead-\\xeb0\\xeb2\\xeb3\\xebd\\xec0-\\xec4\\xedc\\xedd\\xf00"
            "\\xf40-\\xf47\\xf49-\\xf6c\\xf88-\\xf8c\\x1000-\\x102a\\x103f"
            "\\x1050-\\x1055\\x105a-\\x105d\\x1061\\x1065\\x1066"
            "\\x106e-\\x1070\\x1075-\\x1081\\x108e\\x10d0-\\x10fa"
            "\\x1100-\\x1248\\x124a-\\x124d\\x1250-\\x1256\\x1258"
            "\\x125a-\\x125d\\x1260-\\x1288\\x128a-\\x128d\\x1290-\\x12b0"
            "\\x12b2-\\x12b5\\x12b8-\\x12be\\x12c0\\x12c2-\\x12c5"
            "\\x12c8-\\x12d6\\x12d8-\\x1310\\x1312-\\x1315\\x1318-\\x135a"
            "\\x1380-\\x138f\\x13a0-\\x13f4\\x1401-\\x166c\\x166f-\\x167f"
            "\\x1681-\\x169a\\x16a0-\\x16ea\\x1700-\\x170c\\x170e-\\x1711"
            "\\x1720-\\x1731\\x1740-\\x1751\\x1760-\\x176c\\x176e-\\x1770"
            "\\x1780-\\x17b3\\x17dc\\x1820-\\x1842\\x1844-\\x1877"
            "\\x1880-\\x18a8\\x18aa\\x18b0-\\x18f5\\x1900-\\x191c"
            "\\x1950-\\x196d\\x1970-\\x1974\\x1980-\\x19ab\\x19c1-\\x19c7"
            "\\x1a00-\\x1a16\\x1a20-\\x1a54\\x1b05-\\x1b33\\x1b45-\\x1b4b"
            "\\x1b83-\\x1ba0\\x1bae\\x1baf\\x1bc0-\\x1be5\\x1c00-\\x1c23"
            "\\x1c4d-\\x1c4f\\x1c5a-\\x1c77\\x1ce9-\\x1cec\\x1cee-\\x1cf1"
            "\\x2135-\\x2138\\x2d30-\\x2d65\\x2d80-\\x2d96\\x2da0-\\x2da6"
            "\\x2da8-\\x2dae\\x2db0-\\x2db6\\x2db8-\\x2dbe\\x2dc0-\\x2dc6"
            "\\x2dc8-\\x2dce\\x2dd0-\\x2dd6\\x2dd8-\\x2dde\\x3006\\x303c"
            "\\x3041-\\x3096\\x309f\\x30a1-\\x30fa\\x30ff\\x3105-\\x312d"
            "\\x3131-\\x318e\\x31a0-\\x31ba\\x31f0-\\x31ff\\x3400\\x4db5"
            "\\x4e00\\x9fcb\\xa000-\\xa014\\xa016-\\xa48c\\xa4d0-\\xa4f7"
            "\\xa500-\\xa60b\\xa610-\\xa61f\\xa62a\\xa62b\\xa66e"
            "\\xa6a0-\\xa6e5\\xa7fb-\\xa801\\xa803-\\xa805\\xa807-\\xa80a"
            "\\xa80c-\\xa822\\xa840-\\xa873\\xa882-\\xa8b3\\xa8f2-\\xa8f7"
            "\\xa8fb\\xa90a-\\xa925\\xa930-\\xa946\\xa960-\\xa97c"
            "\\xa984-\\xa9b2\\xaa00-\\xaa28\\xaa40-\\xaa42\\xaa44-\\xaa4b"
            "\\xaa60-\\xaa6f\\xaa71-\\xaa76\\xaa7a\\xaa80-\\xaaaf\\xaab1"
            "\\xaab5\\xaab6\\xaab9-\\xaabd\\xaac0\\xaac2\\xaadb\\xaadc"
            "\\xab01-\\xab06\\xab09-\\xab0e\\xab11-\\xab16\\xab20-\\xab26"
            "\\xab28-\\xab2e\\xabc0-\\xabe2\\xac00\\xd7a3\\xd7b0-\\xd7c6"
            "\\xd7cb-\\xd7fb\\xf900-\\xfa2d\\xfa30-\\xfa6d\\xfa70-\\xfad9"
            "\\xfb1d\\xfb1f-\\xfb28\\xfb2a-\\xfb36\\xfb38-\\xfb3c\\xfb3e"
            "\\xfb40\\xfb41\\xfb43\\xfb44\\xfb46-\\xfbb1\\xfbd3-\\xfd3d"
            "\\xfd50-\\xfd8f\\xfd92-\\xfdc7\\xfdf0-\\xfdfb\\xfe70-\\xfe74"
            "\\xfe76-\\xfefc\\xff66-\\xff6f\\xff71-\\xff9d\\xffa0-\\xffbe"
            "\\xffc2-\\xffc7\\xffca-\\xffcf\\xffd2-\\xffd7\\xffda-\\xffdc"
            "\\x10000-\\x1000b\\x1000d-\\x10026\\x10028-\\x1003a\\x1003c"
            "\\x1003d\\x1003f-\\x1004d\\x10050-\\x1005d\\x10080-\\x100fa"
            "\\x10280-\\x1029c\\x102a0-\\x102d0\\x10300-\\x1031e"
            "\\x10330-\\x10340\\x10342-\\x10349\\x10380-\\x1039d"
            "\\x103a0-\\x103c3\\x103c8-\\x103cf\\x10450-\\x1049d"
            "\\x10800-\\x10805\\x10808\\x1080a-\\x10835\\x10837\\x10838"
            "\\x1083c\\x1083f-\\x10855\\x10900-\\x10915\\x10920-\\x10939"
            "\\x10a00\\x10a10-\\x10a13\\x10a15-\\x10a17\\x10a19-\\x10a33"
            "\\x10a60-\\x10a7c\\x10b00-\\x10b35\\x10b40-\\x10b55"
            "\\x10b60-\\x10b72\\x10c00-\\x10c48\\x11003-\\x11037"
            "\\x11083-\\x110af\\x12000-\\x1236e\\x13000-\\x1342e"
            "\\x16800-\\x16a38\\x1b000\\x1b001\\x20000\\x2a6d6\\x2a700"
            "\\x2b734\\x2b740\\x2b81d\\x2f800-\\x2fa1d]";
    }

    static const char *mark_nonspacing()
    {
        return "[\\x300-\\x36f\\x483-\\x487\\x591-\\x5bd\\x5bf\\x5c1\\x5c2"
            "\\x5c4\\x5c5\\x5c7\\x610-\\x61a\\x64b-\\x65f\\x670\\x6d6-\\x6dc"
            "\\x6df-\\x6e4\\x6e7\\x6e8\\x6ea-\\x6ed\\x711\\x730-\\x74a"
            "\\x7a6-\\x7b0\\x7eb-\\x7f3\\x816-\\x819\\x81b-\\x823"
            "\\x825-\\x827\\x829-\\x82d\\x859-\\x85b\\x900-\\x902\\x93a\\x93c"
            "\\x941-\\x948\\x94d\\x951-\\x957\\x962\\x963\\x981\\x9bc"
            "\\x9c1-\\x9c4\\x9cd\\x9e2\\x9e3\\xa01\\xa02\\xa3c\\xa41\\xa42"
            "\\xa47\\xa48\\xa4b-\\xa4d\\xa51\\xa70\\xa71\\xa75\\xa81\\xa82"
            "\\xabc\\xac1-\\xac5\\xac7\\xac8\\xacd\\xae2\\xae3\\xb01\\xb3c"
            "\\xb3f\\xb41-\\xb44\\xb4d\\xb56\\xb62\\xb63\\xb82\\xbc0\\xbcd"
            "\\xc3e-\\xc40\\xc46-\\xc48\\xc4a-\\xc4d\\xc55\\xc56\\xc62\\xc63"
            "\\xcbc\\xcbf\\xcc6\\xccc\\xccd\\xce2\\xce3\\xd41-\\xd44\\xd4d"
            "\\xd62\\xd63\\xdca\\xdd2-\\xdd4\\xdd6\\xe31\\xe34-\\xe3a"
            "\\xe47-\\xe4e\\xeb1\\xeb4-\\xeb9\\xebb\\xebc\\xec8-\\xecd\\xf18"
            "\\xf19\\xf35\\xf37\\xf39\\xf71-\\xf7e\\xf80-\\xf84\\xf86\\xf87"
            "\\xf8d-\\xf97\\xf99-\\xfbc\\xfc6\\x102d-\\x1030\\x1032-\\x1037"
            "\\x1039\\x103a\\x103d\\x103e\\x1058\\x1059\\x105e-\\x1060"
            "\\x1071-\\x1074\\x1082\\x1085\\x1086\\x108d\\x109d"
            "\\x135d-\\x135f\\x1712-\\x1714\\x1732-\\x1734\\x1752\\x1753"
            "\\x1772\\x1773\\x17b7-\\x17bd\\x17c6\\x17c9-\\x17d3\\x17dd"
            "\\x180b-\\x180d\\x18a9\\x1920-\\x1922\\x1927\\x1928\\x1932"
            "\\x1939-\\x193b\\x1a17\\x1a18\\x1a56\\x1a58-\\x1a5e\\x1a60"
            "\\x1a62\\x1a65-\\x1a6c\\x1a73-\\x1a7c\\x1a7f\\x1b00-\\x1b03"
            "\\x1b34\\x1b36-\\x1b3a\\x1b3c\\x1b42\\x1b6b-\\x1b73\\x1b80"
            "\\x1b81\\x1ba2-\\x1ba5\\x1ba8\\x1ba9\\x1be6\\x1be8\\x1be9\\x1bed"
            "\\x1bef-\\x1bf1\\x1c2c-\\x1c33\\x1c36\\x1c37\\x1cd0-\\x1cd2"
            "\\x1cd4-\\x1ce0\\x1ce2-\\x1ce8\\x1ced\\x1dc0-\\x1de6"
            "\\x1dfc-\\x1dff\\x20d0-\\x20dc\\x20e1\\x20e5-\\x20f0"
            "\\x2cef-\\x2cf1\\x2d7f\\x2de0-\\x2dff\\x302a-\\x302f\\x3099"
            "\\x309a\\xa66f\\xa67c\\xa67d\\xa6f0\\xa6f1\\xa802\\xa806\\xa80b"
            "\\xa825\\xa826\\xa8c4\\xa8e0-\\xa8f1\\xa926-\\xa92d"
            "\\xa947-\\xa951\\xa980-\\xa982\\xa9b3\\xa9b6-\\xa9b9\\xa9bc"
            "\\xaa29-\\xaa2e\\xaa31\\xaa32\\xaa35\\xaa36\\xaa43\\xaa4c\\xaab0"
            "\\xaab2-\\xaab4\\xaab7\\xaab8\\xaabe\\xaabf\\xaac1\\xabe5\\xabe8"
            "\\xabed\\xfb1e\\xfe00-\\xfe0f\\xfe20-\\xfe26\\x101fd"
            "\\x10a01-\\x10a03\\x10a05\\x10a06\\x10a0c-\\x10a0f"
            "\\x10a38-\\x10a3a\\x10a3f\\x11001\\x11038-\\x11046\\x11080"
            "\\x11081\\x110b3-\\x110b6\\x110b9\\x110ba\\x1d167-\\x1d169"
            "\\x1d17b-\\x1d182\\x1d185-\\x1d18b\\x1d1aa-\\x1d1ad"
            "\\x1d242-\\x1d244\\xe0100-\\xe01ef]";
    }

    static const char *mark_combining()
    {
        return "[\\x903\\x93b\\x93e-\\x940\\x949-\\x94c\\x94e\\x94f\\x982"
            "\\x983\\x9be-\\x9c0\\x9c7\\x9c8\\x9cb\\x9cc\\x9d7\\xa03"
            "\\xa3e-\\xa40\\xa83\\xabe-\\xac0\\xac9\\xacb\\xacc\\xb02\\xb03"
            "\\xb3e\\xb40\\xb47\\xb48\\xb4b\\xb4c\\xb57\\xbbe\\xbbf\\xbc1"
            "\\xbc2\\xbc6-\\xbc8\\xbca-\\xbcc\\xbd7\\xc01-\\xc03\\xc41-\\xc44"
            "\\xc82\\xc83\\xcbe\\xcc0-\\xcc4\\xcc7\\xcc8\\xcca\\xccb\\xcd5"
            "\\xcd6\\xd02\\xd03\\xd3e-\\xd40\\xd46-\\xd48\\xd4a-\\xd4c\\xd57"
            "\\xd82\\xd83\\xdcf-\\xdd1\\xdd8-\\xddf\\xdf2\\xdf3\\xf3e\\xf3f"
            "\\xf7f\\x102b\\x102c\\x1031\\x1038\\x103b\\x103c\\x1056\\x1057"
            "\\x1062-\\x1064\\x1067-\\x106d\\x1083\\x1084\\x1087-\\x108c"
            "\\x108f\\x109a-\\x109c\\x17b6\\x17be-\\x17c5\\x17c7\\x17c8"
            "\\x1923-\\x1926\\x1929-\\x192b\\x1930\\x1931\\x1933-\\x1938"
            "\\x19b0-\\x19c0\\x19c8\\x19c9\\x1a19-\\x1a1b\\x1a55\\x1a57"
            "\\x1a61\\x1a63\\x1a64\\x1a6d-\\x1a72\\x1b04\\x1b35\\x1b3b"
            "\\x1b3d-\\x1b41\\x1b43\\x1b44\\x1b82\\x1ba1\\x1ba6\\x1ba7\\x1baa"
            "\\x1be7\\x1bea-\\x1bec\\x1bee\\x1bf2\\x1bf3\\x1c24-\\x1c2b"
            "\\x1c34\\x1c35\\x1ce1\\x1cf2\\xa823\\xa824\\xa827\\xa880\\xa881"
            "\\xa8b4-\\xa8c3\\xa952\\xa953\\xa983\\xa9b4\\xa9b5\\xa9ba"
            "\\xa9bb\\xa9bd-\\xa9c0\\xaa2f\\xaa30\\xaa33\\xaa34\\xaa4d\\xaa7b"
            "\\xabe3\\xabe4\\xabe6\\xabe7\\xabe9\\xabea\\xabec\\x11000"
            "\\x11002\\x11082\\x110b0-\\x110b2\\x110b7\\x110b8\\x1d165"
            "\\x1d166\\x1d16d-\\x1d172]";
    }

    static const char *mark_enclosing()
    {
        return "[\\x488\\x489\\x20dd-\\x20e0\\x20e2-\\x20e4\\xa670-\\xa672]";
    }

    static const char *number_decimal()
    {
        return "[\\x30-\\x39\\x660-\\x669\\x6f0-\\x6f9\\x7c0-\\x7c9"
            "\\x966-\\x96f\\x9e6-\\x9ef\\xa66-\\xa6f\\xae6-\\xaef"
            "\\xb66-\\xb6f\\xbe6-\\xbef\\xc66-\\xc6f\\xce6-\\xcef"
            "\\xd66-\\xd6f\\xe50-\\xe59\\xed0-\\xed9\\xf20-\\xf29"
            "\\x1040-\\x1049\\x1090-\\x1099\\x17e0-\\x17e9\\x1810-\\x1819"
            "\\x1946-\\x194f\\x19d0-\\x19d9\\x1a80-\\x1a89\\x1a90-\\x1a99"
            "\\x1b50-\\x1b59\\x1bb0-\\x1bb9\\x1c40-\\x1c49\\x1c50-\\x1c59"
            "\\xa620-\\xa629\\xa8d0-\\xa8d9\\xa900-\\xa909\\xa9d0-\\xa9d9"
            "\\xaa50-\\xaa59\\xabf0-\\xabf9\\xff10-\\xff19\\x104a0-\\x104a9"
            "\\x11066-\\x1106f\\x1d7ce-\\x1d7ff]";
    }

    static const char *number_letter()
    {
        return "[\\x16ee-\\x16f0\\x2160-\\x2182\\x2185-\\x2188\\x3007"
            "\\x3021-\\x3029\\x3038-\\x303a\\xa6e6-\\xa6ef\\x10140-\\x10174"
            "\\x10341\\x1034a\\x103d1-\\x103d5\\x12400-\\x12462]";
    }

    static const char *number_other()
    {
        return "[\\xb2\\xb3\\xb9\\xbc-\\xbe\\x9f4-\\x9f9\\xb72-\\xb77"
            "\\xbf0-\\xbf2\\xc78-\\xc7e\\xd70-\\xd75\\xf2a-\\xf33"
            "\\x1369-\\x137c\\x17f0-\\x17f9\\x19da\\x2070\\x2074-\\x2079"
            "\\x2080-\\x2089\\x2150-\\x215f\\x2189\\x2460-\\x249b"
            "\\x24ea-\\x24ff\\x2776-\\x2793\\x2cfd\\x3192-\\x3195"
            "\\x3220-\\x3229\\x3251-\\x325f\\x3280-\\x3289\\x32b1-\\x32bf"
            "\\xa830-\\xa835\\x10107-\\x10133\\x10175-\\x10178\\x1018a"
            "\\x10320-\\x10323\\x10858-\\x1085f\\x10916-\\x1091b"
            "\\x10a40-\\x10a47\\x10a7d\\x10a7e\\x10b58-\\x10b5f"
            "\\x10b78-\\x10b7f\\x10e60-\\x10e7e\\x11052-\\x11065"
            "\\x1d360-\\x1d371\\x1f100-\\x1f10a]";
    }

    static const char *punctuation_connector()
    {
        return "[\\x5f\\x203f\\x2040\\x2054\\xfe33\\xfe34\\xfe4d-\\xfe4f"
            "\\xff3f]";
    }

    static const char *punctuation_dash()
    {
        return "[\\x2d\\x58a\\x5be\\x1400\\x1806\\x2010-\\x2015\\x2e17\\x2e1a"
            "\\x301c\\x3030\\x30a0\\xfe31\\xfe32\\xfe58\\xfe63\\xff0d]";
    }

    static const char *punctuation_open()
    {
        return "[\\x28\\x5b\\x7b\\xf3a\\xf3c\\x169b\\x201a\\x201e\\x2045"
            "\\x207d\\x208d\\x2329\\x2768\\x276a\\x276c\\x276e\\x2770\\x2772"
            "\\x2774\\x27c5\\x27e6\\x27e8\\x27ea\\x27ec\\x27ee\\x2983\\x2985"
            "\\x2987\\x2989\\x298b\\x298d\\x298f\\x2991\\x2993\\x2995\\x2997"
            "\\x29d8\\x29da\\x29fc\\x2e22\\x2e24\\x2e26\\x2e28\\x3008\\x300a"
            "\\x300c\\x300e\\x3010\\x3014\\x3016\\x3018\\x301a\\x301d\\xfd3e"
            "\\xfe17\\xfe35\\xfe37\\xfe39\\xfe3b\\xfe3d\\xfe3f\\xfe41\\xfe43"
            "\\xfe47\\xfe59\\xfe5b\\xfe5d\\xff08\\xff3b\\xff5b\\xff5f\\xff62]";
    }

    static const char *punctuation_close()
    {
        return "[\\x29\\x5d\\x7d\\xf3b\\xf3d\\x169c\\x2046\\x207e\\x208e"
            "\\x232a\\x2769\\x276b\\x276d\\x276f\\x2771\\x2773\\x2775\\x27c6"
            "\\x27e7\\x27e9\\x27eb\\x27ed\\x27ef\\x2984\\x2986\\x2988\\x298a"
            "\\x298c\\x298e\\x2990\\x2992\\x2994\\x2996\\x2998\\x29d9\\x29db"
            "\\x29fd\\x2e23\\x2e25\\x2e27\\x2e29\\x3009\\x300b\\x300d\\x300f"
            "\\x3011\\x3015\\x3017\\x3019\\x301b\\x301e\\x301f\\xfd3f\\xfe18"
            "\\xfe36\\xfe38\\xfe3a\\xfe3c\\xfe3e\\xfe40\\xfe42\\xfe44\\xfe48"
            "\\xfe5a\\xfe5c\\xfe5e\\xff09\\xff3d\\xff5d\\xff60\\xff63]";
    }

    static const char *punctuation_initial()
    {
        return "[\\x00AB\\x2018\\x201B\\x201C\\x201F\\x2039\\x2E02\\x2E04"
            "\\x2E09\\x2E0C\\x2E1C\\x2E20]";
    }

    static const char *punctuation_final()
    {
        return "[\\x00BB\\x2019\\x201D\\x203A\\x2E03\\x2E05\\x2E0A\\x2E0D"
            "\\x2E1D\\x2E21]";
    }

    static const char *punctuation_other()
    {
        return "[\\x21-\\x23\\x25-\\x27\\x2a\\x2c\\x2e\\x2f\\x3a\\x3b\\x3f"
            "\\x40\\x5c\\xa1\\xb7\\xbf\\x37e\\x387\\x55a-\\x55f\\x589\\x5c0"
            "\\x5c3\\x5c6\\x5f3\\x5f4\\x609\\x60a\\x60c\\x60d\\x61b\\x61e"
            "\\x61f\\x66a-\\x66d\\x6d4\\x700-\\x70d\\x7f7-\\x7f9\\x830-\\x83e"
            "\\x85e\\x964\\x965\\x970\\xdf4\\xe4f\\xe5a\\xe5b\\xf04-\\xf12"
            "\\xf85\\xfd0-\\xfd4\\xfd9\\xfda\\x104a-\\x104f\\x10fb"
            "\\x1361-\\x1368\\x166d\\x166e\\x16eb-\\x16ed\\x1735\\x1736"
            "\\x17d4-\\x17d6\\x17d8-\\x17da\\x1800-\\x1805\\x1807-\\x180a"
            "\\x1944\\x1945\\x1a1e\\x1a1f\\x1aa0-\\x1aa6\\x1aa8-\\x1aad"
            "\\x1b5a-\\x1b60\\x1bfc-\\x1bff\\x1c3b-\\x1c3f\\x1c7e\\x1c7f"
            "\\x1cd3\\x2016\\x2017\\x2020-\\x2027\\x2030-\\x2038"
            "\\x203b-\\x203e\\x2041-\\x2043\\x2047-\\x2051\\x2053"
            "\\x2055-\\x205e\\x2cf9-\\x2cfc\\x2cfe\\x2cff\\x2d70\\x2e00"
            "\\x2e01\\x2e06-\\x2e08\\x2e0b\\x2e0e-\\x2e16\\x2e18\\x2e19"
            "\\x2e1b\\x2e1e\\x2e1f\\x2e2a-\\x2e2e\\x2e30\\x2e31"
            "\\x3001-\\x3003\\x303d\\x30fb\\xa4fe\\xa4ff\\xa60d-\\xa60f"
            "\\xa673\\xa67e\\xa6f2-\\xa6f7\\xa874-\\xa877\\xa8ce\\xa8cf"
            "\\xa8f8-\\xa8fa\\xa92e\\xa92f\\xa95f\\xa9c1-\\xa9cd\\xa9de"
            "\\xa9df\\xaa5c-\\xaa5f\\xaade\\xaadf\\xabeb\\xfe10-\\xfe16"
            "\\xfe19\\xfe30\\xfe45\\xfe46\\xfe49-\\xfe4c\\xfe50-\\xfe52"
            "\\xfe54-\\xfe57\\xfe5f-\\xfe61\\xfe68\\xfe6a\\xfe6b"
            "\\xff01-\\xff03\\xff05-\\xff07\\xff0a\\xff0c\\xff0e\\xff0f"
            "\\xff1a\\xff1b\\xff1f\\xff20\\xff3c\\xff61\\xff64\\xff65"
            "\\x10100\\x10101\\x1039f\\x103d0\\x10857\\x1091f\\x1093f"
            "\\x10a50-\\x10a58\\x10a7f\\x10b39-\\x10b3f\\x11047-\\x1104d"
            "\\x110bb\\x110bc\\x110be-\\x110c1\\x12470-\\x12473]";
    }

    static const char *symbol_math()
    {
        return "[\\x2b\\x3c-\\x3e\\x7c\\x7e\\xac\\xb1\\xd7\\xf7\\x3f6"
            "\\x606-\\x608\\x2044\\x2052\\x207a-\\x207c\\x208a-\\x208c"
            "\\x2118\\x2140-\\x2144\\x214b\\x2190-\\x2194\\x219a\\x219b"
            "\\x21a0\\x21a3\\x21a6\\x21ae\\x21ce\\x21cf\\x21d2\\x21d4"
            "\\x21f4-\\x22ff\\x2308-\\x230b\\x2320\\x2321\\x237c"
            "\\x239b-\\x23b3\\x23dc-\\x23e1\\x25b7\\x25c1\\x25f8-\\x25ff"
            "\\x266f\\x27c0-\\x27c4\\x27c7-\\x27ca\\x27cc\\x27ce-\\x27e5"
            "\\x27f0-\\x27ff\\x2900-\\x2982\\x2999-\\x29d7\\x29dc-\\x29fb"
            "\\x29fe-\\x2aff\\x2b30-\\x2b44\\x2b47-\\x2b4c\\xfb29\\xfe62"
            "\\xfe64-\\xfe66\\xff0b\\xff1c-\\xff1e\\xff5c\\xff5e\\xffe2"
            "\\xffe9-\\xffec\\x1d6c1\\x1d6db\\x1d6fb\\x1d715\\x1d735\\x1d74f"
            "\\x1d76f\\x1d789\\x1d7a9\\x1d7c3]";
    }

    static const char *symbol_currency()
    {
        return "[\\x24\\xa2-\\xa5\\x60b\\x9f2\\x9f3\\x9fb\\xaf1\\xbf9\\xe3f"
            "\\x17db\\x20a0-\\x20b9\\xa838\\xfdfc\\xfe69\\xff04\\xffe0\\xffe1"
            "\\xffe5\\xffe6]";
    }

    static const char *symbol_modifier()
    {
        return "[\\x5e\\x60\\xa8\\xaf\\xb4\\xb8\\x2c2-\\x2c5\\x2d2-\\x2df"
            "\\x2e5-\\x2eb\\x2ed\\x2ef-\\x2ff\\x375\\x384\\x385\\x1fbd"
            "\\x1fbf-\\x1fc1\\x1fcd-\\x1fcf\\x1fdd-\\x1fdf\\x1fed-\\x1fef"
            "\\x1ffd\\x1ffe\\x309b\\x309c\\xa700-\\xa716\\xa720\\xa721"
            "\\xa789\\xa78a\\xfbb2-\\xfbc1\\xff3e\\xff40\\xffe3]";
    }

    static const char *symbol_other()
    {
        return "[\\xa6\\xa7\\xa9\\xae\\xb0\\xb6\\x482\\x60e\\x60f\\x6de"
            "\\x6e9\\x6fd\\x6fe\\x7f6\\x9fa\\xb70\\xbf3-\\xbf8\\xbfa\\xc7f"
            "\\xd79\\xf01-\\xf03\\xf13-\\xf17\\xf1a-\\xf1f\\xf34\\xf36\\xf38"
            "\\xfbe-\\xfc5\\xfc7-\\xfcc\\xfce\\xfcf\\xfd5-\\xfd8"
            "\\x109e\\x109f\\x1360\\x1390-\\x1399\\x1940\\x19de-\\x19ff"
            "\\x1b61-\\x1b6a\\x1b74-\\x1b7c\\x2100\\x2101\\x2103-\\x2106"
            "\\x2108\\x2109\\x2114\\x2116\\x2117\\x211e-\\x2123\\x2125"
            "\\x2127\\x2129\\x212e\\x213a\\x213b\\x214a\\x214c\\x214d\\x214f"
            "\\x2195-\\x2199\\x219c-\\x219f\\x21a1\\x21a2\\x21a4\\x21a5"
            "\\x21a7-\\x21ad\\x21af-\\x21cd\\x21d0\\x21d1\\x21d3"
            "\\x21d5-\\x21f3\\x2300-\\x2307\\x230c-\\x231f\\x2322-\\x2328"
            "\\x232b-\\x237b\\x237d-\\x239a\\x23b4-\\x23db\\x23e2-\\x23f3"
            "\\x2400-\\x2426\\x2440-\\x244a\\x249c-\\x24e9\\x2500-\\x25b6"
            "\\x25b8-\\x25c0\\x25c2-\\x25f7\\x2600-\\x266e\\x2670-\\x26ff"
            "\\x2701-\\x2767\\x2794-\\x27bf\\x2800-\\x28ff\\x2b00-\\x2b2f"
            "\\x2b45\\x2b46\\x2b50-\\x2b59\\x2ce5-\\x2cea\\x2e80-\\x2e99"
            "\\x2e9b-\\x2ef3\\x2f00-\\x2fd5\\x2ff0-\\x2ffb\\x3004\\x3012"
            "\\x3013\\x3020\\x3036\\x3037\\x303e\\x303f\\x3190\\x3191"
            "\\x3196-\\x319f\\x31c0-\\x31e3\\x3200-\\x321e\\x322a-\\x3250"
            "\\x3260-\\x327f\\x328a-\\x32b0\\x32c0-\\x32fe\\x3300-\\x33ff"
            "\\x4dc0-\\x4dff\\xa490-\\xa4c6\\xa828-\\xa82b\\xa836\\xa837"
            "\\xa839\\xaa77-\\xaa79\\xfdfd\\xffe4\\xffe8\\xffed\\xffee"
            "\\xfffc\\xfffd\\x10102\\x10137-\\x1013f\\x10179-\\x10189"
            "\\x10190-\\x1019b\\x101d0-\\x101fc\\x1d000-\\x1d0f5"
            "\\x1d100-\\x1d126\\x1d129-\\x1d164\\x1d16a-\\x1d16c\\x1d183"
            "\\x1d184\\x1d18c-\\x1d1a9\\x1d1ae-\\x1d1dd\\x1d200-\\x1d241"
            "\\x1d245\\x1d300-\\x1d356\\x1f000-\\x1f02b\\x1f030-\\x1f093"
            "\\x1f0a0-\\x1f0ae\\x1f0b1-\\x1f0be\\x1f0c1-\\x1f0cf"
            "\\x1f0d1-\\x1f0df\\x1f110-\\x1f12e\\x1f130-\\x1f169"
            "\\x1f170-\\x1f19a\\x1f1e6-\\x1f202\\x1f210-\\x1f23a"
            "\\x1f240-\\x1f248\\x1f250\\x1f251\\x1f300-\\x1f320"
            "\\x1f330-\\x1f335\\x1f337-\\x1f37c\\x1f380-\\x1f393"
            "\\x1f3a0-\\x1f3c4\\x1f3c6-\\x1f3ca\\x1f3e0-\\x1f3f0"
            "\\x1f400-\\x1f43e\\x1f440\\x1f442-\\x1f4f7\\x1f4f9-\\x1f4fc"
            "\\x1f500-\\x1f53d\\x1f550-\\x1f567\\x1f5fb-\\x1f5ff"
            "\\x1f601-\\x1f610\\x1f612-\\x1f614\\x1f616\\x1f618\\x1f61a"
            "\\x1f61c-\\x1f61e\\x1f620-\\x1f625\\x1f628-\\x1f62b\\x1f62d"
            "\\x1f630-\\x1f633\\x1f635-\\x1f640\\x1f645-\\x1f64f"
            "\\x1f680-\\x1f6c5\\x1f700-\\x1f773]";
    }

    static const char *separator_space()
    {
        return "[\\x20\\xa0\\x1680\\x180e\\x2000-\\x200a\\x202f\\x205f"
            "\\x3000]";
    }

    static const char *separator_line()
    {
        return "[\\x2028]";
    }

    static const char *separator_paragraph()
    {
        return "[\\x2029]";
    }

    static const char *other_control()
    {
        return "[\\x0-\\x1f\\x7f-\\x9f]";
    }

    static const char *other_format()
    {
        return "[\\xad\\x600-\\x603\\x6dd\\x70f\\x17b4\\x17b5\\x200b-\\x200f"
            "\\x202a-\\x202e\\x2060-\\x2064\\x206a-\\x206f\\xfeff"
            "\\xfff9-\\xfffb\\x110bd\\x1d173-\\x1d17a\\xe0001"
            "\\xe0020-\\xe007f]";
    }

    static const char *other_surrogate()
    {
        return "[\\xD800\\xDB7F\\xDB80\\xDBFF\\xDC00\\xDFFF]";
    }

    static const char *other_private()
    {
        return "[\\xE000\\xF8FF\\xF0000\\xFFFFD\\x100000\\x10FFFD]";
    }

    template<typename state_type>
    static input_char_type decode_octal(state_type &state_)
    {
        std::size_t oct_ = 0;
        typename state_type::char_type ch_ = *state_._curr;
        unsigned short count_ = 3;
        bool eos_ = false;

        for (;;)
        {
            oct_ *= 8;
            oct_ += ch_ - '0';
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
    static input_char_type decode_control_char(state_type &state_)
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
    static input_char_type decode_hex(state_type &state_)
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

        if (!((ch_ >= '0' && ch_ <= '9') || (ch_ >= 'a' && ch_ <= 'f') ||
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
                hex_ += ch_ - '0';
            }
            else if (ch_ >= 'a' && ch_ <= 'f')
            {
                hex_ += 10 + (ch_ - 'a');
            }
            else
            {
                hex_ += 10 + (ch_ - 'A');
            }

            eos_ = state_.eos();

            if (!eos_)
            {
                ch_ = *state_._curr;

                // Don't consume invalid chars!
                if (((ch_ >= '0' && ch_ <= '9') ||
                    (ch_ >= 'a' && ch_ <= 'f') || (ch_ >= 'A' && ch_ <= 'F')))
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
    static void charset_range(const bool chset_, state_type &state_,
        bool &eos_, typename state_type::char_type &ch_,
        const input_char_type prev_, string_token &chars_)
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

                ss_ << "Charset cannot form end of range preceding index "
                    << state_.index();
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
        typename char_traits::index_type start_ =
            static_cast<typename char_traits::index_type>(prev_);
        typename char_traits::index_type end_ =
            static_cast<typename char_traits::index_type>(curr_);

        // Semanic check
        if (end_ < start_)
        {
            std::ostringstream ss_;

            ss_ << "Max less than Min in charset range preceding index " <<
                state_.index() - 1;
            state_.error(ss_);
            throw runtime_error(ss_.str());
        }

        // Even though ranges are used now, we still need to consider
        // each character if icase is set.
        if (state_._flags & icase)
        {
            typename string_token::range range_(start_, end_);
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
            chars_.insert(typename string_token::range(prev_, curr_));
        }
    }
};
}
}

#endif
