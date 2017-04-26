#pragma once

namespace details {
	template<typename InputIteratorT, typename SeparatorT, typename PrefixT = void, typename SuffixT = void>
	struct Joiner
	{
		InputIteratorT first, last;
		const SeparatorT sep;
		const PrefixT prefix;
		const SuffixT suffix;

		Joiner(InputIteratorT _first, InputIteratorT _last, const SeparatorT& _sep,
				const PrefixT& _prefix, const SuffixT& _suffix)
                	: first(_first), last(_last),  sep(_sep), prefix(_prefix), suffix(_suffix)
		{
		}

		void join_on(std::ostream& stream) const
		{
			stream << prefix;

			InputIteratorT it = first;

			if (it != last)
			{
				stream << *it;
				++it;
			}

			for (; it != last; ++it)
			{
				stream << sep << *it;
			}

			stream << suffix;
		}
	};

	template<typename InputIteratorT, typename SeparatorT>
	struct Joiner<InputIteratorT, SeparatorT, void, void>
	{
		InputIteratorT first, last;
		const SeparatorT sep;

		Joiner(InputIteratorT _first, InputIteratorT _last, const SeparatorT &_sep)
			: first(_first), last(_last),  sep(_sep)
		{
		}

		void join_on(std::ostream& stream) const
		{
			InputIteratorT it = first;

			if (it != last)
			{
				stream << *it;
				++it;
			}

			for (; it != last; ++it)
			{
				stream << sep << *it;
			}
		}
	};
};

template <typename InputIteratorT, typename SeparatorT, typename PrefixT, typename SuffixT>
std::ostream& operator << (std::ostream& stream, const details::Joiner<InputIteratorT, SeparatorT, PrefixT, SuffixT> &joiner)
{
	joiner.join_on(stream);
	return stream;
}

template <typename InputIteratorT, typename SeparatorT>
details::Joiner<InputIteratorT, SeparatorT> joiner(InputIteratorT first, InputIteratorT last, const SeparatorT &sep)
{
	return details::Joiner<InputIteratorT, SeparatorT>(first, last, sep);
}

template <typename InputIteratorT, typename SeparatorT, typename PrefixT, typename SuffixT>
details::Joiner<InputIteratorT, SeparatorT, PrefixT, SuffixT>
joiner(InputIteratorT first, InputIteratorT last, const SeparatorT &sep, const PrefixT& prefix, const SuffixT& suffix)
{
	return details::Joiner<InputIteratorT, SeparatorT, PrefixT, SuffixT>(first, last, sep, prefix, suffix);
}

template <typename SequenceT, typename SeparatorT>
details::Joiner<typename SequenceT::const_iterator, SeparatorT>
joiner(const SequenceT &seq, const SeparatorT &sep)
{
	return details::Joiner<typename SequenceT::const_iterator, SeparatorT>(seq.begin(), seq.end(), sep);
}

template <typename SequenceT, typename SeparatorT, typename PrefixT, typename SuffixT>
details::Joiner<typename SequenceT::const_iterator, SeparatorT, PrefixT, SuffixT>
joiner(const SequenceT &seq, const SeparatorT &sep, const PrefixT& prefix, const SuffixT& suffix)
{
	return details::Joiner<typename SequenceT::const_iterator, SeparatorT, PrefixT, SuffixT>(seq.begin(), seq.end(), sep, prefix, suffix);
}

template <typename InputIteratorT>
details::Joiner<InputIteratorT, const char*, const char*, const char*> vectorizer(InputIteratorT first, InputIteratorT last)
{
	return joiner(first, last, (const char *)", ", (const char *)"[", (const char *)"]");
}

template <typename SequenceT>
details::Joiner<typename SequenceT::const_iterator, const char*, const char*, const char*> vectorizer(const SequenceT &seq)
{
	return vectorizer(seq.begin(), seq.end());
}
