//Author: luotuo44@gmail.com
//Use of this source code is governed by a BSD-style license

#ifndef HELPER_H
#define HELPER_H


#include<string>
#include<iterator>
#include<vector>
#include<utility>
#include<numeric>
#include<limits>
#include<algorithm>


using StrVec = std::vector<std::string>;
using StrIntPair = std::pair<std::string, int>;
using uchar = unsigned char;

namespace Helper
{
template<typename T, size_t N>
struct __GetNBits
{
    static size_t value(T t, size_t pos)
    {
        constexpr size_t mask = (1<<N) - 1;
        return (t>>pos) & mask;
    }
};


template<typename T>
struct __GetNBits<T, 0>
{
    static size_t value(T , size_t )
    {
        return 0;
    }
};


template<size_t N, typename T>
inline size_t getNBits(T t, size_t pos)
{
    return __GetNBits<T, N>::value(t, pos);
}


template<typename T, size_t N>
struct __SetNBits
{
    static void setValue(T &t, size_t pos, size_t val)
    {
        constexpr size_t mask = (1<<N) - 1;
        val &= mask;//清空多余的
        t &= ~(mask << pos);//先将那N比特设置为0。非那N比特的数据&1， 不受影响
        t ^= val<<pos;//然后再针对那N比特异或，非那N比特的数据异或0，不受影响
    }
};

template<typename T>
struct __SetNBits<T, 0>
{
    static void setValue(T&, size_t, size_t)
    {
        //do nothing
    }
};


template<size_t N, typename T>
inline void setNBits(T &t, size_t pos, size_t val)
{
    __SetNBits<T, N>::setValue(t, pos, val);
}



inline size_t getSizeValue(const unsigned char *begin, const unsigned char *end)
{
    size_t init = 0;
    return std::accumulate(begin, end, init, [](size_t val, unsigned char cur){
        return (val<<8) + cur;
    });
}


inline size_t getSizeValue(const char *begin, const char *end)
{
    const unsigned char *ubegin = reinterpret_cast<const unsigned char *>(begin);
    const unsigned char *uend = reinterpret_cast<const unsigned char *>(end);
    return getSizeValue(ubegin, uend);
}




template<size_t N, typename T>
std::string parseSizeValue(T t)
{
    static_assert(std::is_integral<T>::value, "should be integral");

    std::string str(N, '\0');

    for(size_t i = 0; i < N && t ; ++i)
    {
        str[N-i-1] = t & 7;
        t >>= 8;
    }

    return str;
}

//==========================================================================



class StringSplit : public std::iterator<std::input_iterator_tag, std::string>
{
public:
    StringSplit()=default;
    StringSplit(const std::string &str, const std::string &delim)
        : m_pos(0), m_last_pos(0), m_str(str), m_delim(delim)
    {
        m_pos = m_str.find(m_delim);
    }

    std::string operator * () { return m_str.substr(m_last_pos, m_pos-m_last_pos); }
    void operator ++ () { next(); }
    void operator ++ (int) { next(); }
    bool operator == (const StringSplit &ss)const;
    bool operator != (const StringSplit &ss)const { return !(*this == ss); }

private:
    void next();

private:
    std::string::size_type m_pos = std::string::npos;
    std::string::size_type m_last_pos = std::string::npos;
    const std::string &m_str = "";
    const std::string &m_delim = "";
};


std::string& trim(std::string &str);
std::string& trim(std::string &str, char ch);
std::pair<StrVec, std::vector<StrIntPair>> parseConfig(const std::string &filename);


std::vector<uchar> subVec(const std::vector<uchar> &vec, size_t pos, size_t n=std::numeric_limits<size_t>::max());



template<typename T>
std::vector<uchar> toUVec(T t)
{
    static_assert(std::is_arithmetic<T>::value, "must be arithmetic type");

    std::vector<uchar> vec;
    for(size_t i = 0; i < sizeof(T); ++i)
    {
        vec.push_back(t&0xFF);
        t >>= 8;
    }

    std::reverse(vec.begin(), vec.end());
    return vec;
}


std::vector<uchar> toUVec(const std::string &str);

}

inline std::vector<uchar>& operator += (std::vector<uchar> &lhs, const std::vector<uchar> &rhs)
{
    lhs.insert(lhs.end(), rhs.begin(), rhs.end());
    return lhs;
}

#endif // HELPER_H

