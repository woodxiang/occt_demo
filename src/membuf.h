#pragma once
#include <streambuf>

template <class _Elem, class _Traits = std::char_traits<_Elem>>
class basic_membuf : public std::basic_streambuf<_Elem, _Traits> {
public:
	basic_membuf(_Elem* buf, std::streamsize size) {
		std::basic_streambuf<_Elem, _Traits>::setp(buf, buf + size);
		std::basic_streambuf<_Elem, _Traits>::setg(buf, buf, buf + size);
	}

protected:
	std::basic_streambuf<_Elem, _Traits>* setbuf(_Elem* buf, std::streamsize size)
	{
		this->setp(buf, buf + size);
		this->setg(buf, buf, buf + size);
		return std::basic_streambuf<_Elem, _Traits>::setbuf(buf, size);
	}

	typename std::basic_streambuf<_Elem, _Traits>::pos_type seekoff(
		typename std::basic_streambuf<_Elem, _Traits>::off_type offset,
		std::ios_base::seekdir seekDir,
		std::ios_base::openmode openMode = std::ios_base::in | std::ios_base::out) {
		if (openMode | std::ios_base::in)
		{
			switch (seekDir)
			{
			case std::ios_base::beg:
				this->setg(this->eback(), this->eback() + offset, this->egptr());
				break;
			case std::ios_base::end:
				this->setg(this->eback(), this->egptr() + offset, this->egptr());
				break;
			default:
				this->setg(this->eback(), this->gptr() + offset, this->egptr());
				break;
			}

			return this->gptr() - this->eback();
		}
		else
		{
			switch (seekDir)
			{
			case std::ios_base::beg:
				this->setp(this->pbase(), this->pbase() + offset);
				break;
			case std::ios_base::end:
				this->setp(this->pbase(), this->epptr() + offset);
				break;
			default:
				this->setp(this->pbase(), this->pptr() + offset);
				break;
			}

			return this->pptr() - this->pbase();
		}
	}
};

typedef basic_membuf<char> membuf;