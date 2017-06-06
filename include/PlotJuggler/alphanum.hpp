#ifndef ALPHANUM__HPP
#define ALPHANUM__HPP

/*
The Alphanum Algorithm is an improved sorting algorithm for strings
containing numbers.  Instead of sorting numbers in ASCII order like a
standard sort, this algorithm sorts numbers in numeric order.

The Alphanum Algorithm is discussed at http://www.DaveKoelle.com

This implementation is Copyright (c) 2008 Dirk Jagdmann <doj@cubic.org>.
It is a cleanroom implementation of the algorithm and not derived by
other's works. In contrast to the versions written by Dave Koelle this
source code is distributed with the libpng/zlib license.

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you
       must not claim that you wrote the original software. If you use
       this software in a product, an acknowledgment in the product
       documentation would be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and
       must not be misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
       distribution. */

/* $Header: /code/doj/alphanum.hpp,v 1.3 2008/01/28 23:06:47 doj Exp $ */

#include <cassert>
#include <functional>
#include <string>
#include <sstream>

#ifdef ALPHANUM_LOCALE
#include <cctype>
#endif

#ifdef DOJDEBUG
#include <iostream>
#include <typeinfo>
#endif

// TODO: make comparison with hexadecimal numbers. Extend the alphanum_comp() function by traits to choose between decimal and hexadecimal.

namespace doj
{

  // anonymous namespace for functions we use internally. But if you
  // are coding in C, you can use alphanum_impl() directly, since it
  // uses not C++ features.
  namespace {

    // if you want to honour the locale settings for detecting digit
    // characters, you should define ALPHANUM_LOCALE
#ifdef ALPHANUM_LOCALE
    /** wrapper function for ::isdigit() */
    bool alphanum_isdigit(int c)
    {
      return isdigit(c);
    }
#else
    /** this function does not consider the current locale and only
	works with ASCII digits.
	@return true if c is a digit character
    */
    bool alphanum_isdigit(const char c)
    {
      return c>='0' && c<='9';
    }
#endif

    /**
       compare l and r with strcmp() semantics, but using
       the "Alphanum Algorithm". This function is designed to read
       through the l and r strings only one time, for
       maximum performance. It does not allocate memory for
       substrings. It can either use the C-library functions isdigit()
       and atoi() to honour your locale settings, when recognizing
       digit characters when you "#define ALPHANUM_LOCALE=1" or use
       it's own digit character handling which only works with ASCII
       digit characters, but provides better performance.

       @param l NULL-terminated C-style string
       @param r NULL-terminated C-style string
       @return negative if l<r, 0 if l equals r, positive if l>r
     */
    int alphanum_impl(const char *l, const char *r)
    {
      enum mode_t { STRING, NUMBER } mode=STRING;

      while(*l && *r)
	{
	  if(mode == STRING)
	    {
	      char l_char, r_char;
	      while((l_char=*l) && (r_char=*r))
		{
		  // check if this are digit characters
		  const bool l_digit=alphanum_isdigit(l_char), r_digit=alphanum_isdigit(r_char);
		  // if both characters are digits, we continue in NUMBER mode
		  if(l_digit && r_digit)
		    {
		      mode=NUMBER;
		      break;
		    }
		  // if only the left character is a digit, we have a result
		  if(l_digit) return -1;
		  // if only the right character is a digit, we have a result
		  if(r_digit) return +1;
		  // compute the difference of both characters
		  const int diff=l_char - r_char;
		  // if they differ we have a result
		  if(diff != 0) return diff;
		  // otherwise process the next characters
		  ++l;
		  ++r;
		}
	    }
	  else // mode==NUMBER
	    {
#ifdef ALPHANUM_LOCALE
	      // get the left number
	      char *end;
	      unsigned long l_int=strtoul(l, &end, 0);
	      l=end;

	      // get the right number
	      unsigned long r_int=strtoul(r, &end, 0);
	      r=end;
#else
	      // get the left number
	      unsigned long l_int=0;
	      while(*l && alphanum_isdigit(*l))
		{
		  // TODO: this can overflow
		  l_int=l_int*10 + *l-'0';
		  ++l;
		}

	      // get the right number
	      unsigned long r_int=0;
	      while(*r && alphanum_isdigit(*r))
		{
		  // TODO: this can overflow
		  r_int=r_int*10 + *r-'0';
		  ++r;
		}
#endif

	      // if the difference is not equal to zero, we have a comparison result
	      const long diff=l_int-r_int;
	      if(diff != 0)
		return diff;

	      // otherwise we process the next substring in STRING mode
	      mode=STRING;
	    }
	}

      if(*r) return -1;
      if(*l) return +1;
      return 0;
    }

  }

  /**
     Compare left and right with the same semantics as strcmp(), but with the
     "Alphanum Algorithm" which produces more human-friendly
     results. The classes lT and rT must implement "std::ostream
     operator<< (std::ostream&, const Ty&)".

     @return negative if left<right, 0 if left==right, positive if left>right.
  */
  template <typename lT, typename rT>
  inline int alphanum_comp(const lT& left, const rT& right)
  {
#ifdef DOJDEBUG
    std::clog << "alphanum_comp<" << typeid(left).name() << "," << typeid(right).name() << "> " << left << "," << right << std::endl;
#endif
    std::ostringstream l; l << left;
    std::ostringstream r; r << right;
    return alphanum_impl(l.str().c_str(), r.str().c_str());
  }

  /**
     Compare l and r with the same semantics as strcmp(), but with
     the "Alphanum Algorithm" which produces more human-friendly
     results.

     @return negative if l<r, 0 if l==r, positive if l>r.
  */
  template <>
  inline int alphanum_comp<std::string>(const std::string& l, const std::string& r)
  {
#ifdef DOJDEBUG
    std::clog << "alphanum_comp<std::string,std::string> " << l << "," << r << std::endl;
#endif
    return alphanum_impl(l.c_str(), r.c_str());
  }

  ////////////////////////////////////////////////////////////////////////////

  // now follow a lot of overloaded alphanum_comp() functions to get a
  // direct call to alphanum_impl() upon the various combinations of c
  // and c++ strings.

  /**
     Compare l and r with the same semantics as strcmp(), but with
     the "Alphanum Algorithm" which produces more human-friendly
     results.

     @return negative if l<r, 0 if l==r, positive if l>r.
  */
  inline int alphanum_comp(char* l, char* r)
  {
    assert(l);
    assert(r);
#ifdef DOJDEBUG
    std::clog << "alphanum_comp<char*,char*> " << l << "," << r << std::endl;
#endif
    return alphanum_impl(l, r);
  }

  inline int alphanum_comp(const char* l, const char* r)
  {
    assert(l);
    assert(r);
#ifdef DOJDEBUG
    std::clog << "alphanum_comp<const char*,const char*> " << l << "," << r << std::endl;
#endif
    return alphanum_impl(l, r);
  }

  inline int alphanum_comp(char* l, const char* r)
  {
    assert(l);
    assert(r);
#ifdef DOJDEBUG
    std::clog << "alphanum_comp<char*,const char*> " << l << "," << r << std::endl;
#endif
    return alphanum_impl(l, r);
  }

  inline int alphanum_comp(const char* l, char* r)
  {
    assert(l);
    assert(r);
#ifdef DOJDEBUG
    std::clog << "alphanum_comp<const char*,char*> " << l << "," << r << std::endl;
#endif
    return alphanum_impl(l, r);
  }

  inline int alphanum_comp(const std::string& l, char* r)
  {
    assert(r);
#ifdef DOJDEBUG
    std::clog << "alphanum_comp<std::string,char*> " << l << "," << r << std::endl;
#endif
    return alphanum_impl(l.c_str(), r);
  }

  inline int alphanum_comp(char* l, const std::string& r)
  {
    assert(l);
#ifdef DOJDEBUG
    std::clog << "alphanum_comp<char*,std::string> " << l << "," << r << std::endl;
#endif
    return alphanum_impl(l, r.c_str());
  }

  inline int alphanum_comp(const std::string& l, const char* r)
  {
    assert(r);
#ifdef DOJDEBUG
    std::clog << "alphanum_comp<std::string,const char*> " << l << "," << r << std::endl;
#endif
    return alphanum_impl(l.c_str(), r);
  }

  inline int alphanum_comp(const char* l, const std::string& r)
  {
    assert(l);
#ifdef DOJDEBUG
    std::clog << "alphanum_comp<const char*,std::string> " << l << "," << r << std::endl;
#endif
    return alphanum_impl(l, r.c_str());
  }

  ////////////////////////////////////////////////////////////////////////////

  /**
     Functor class to compare two objects with the "Alphanum
     Algorithm". If the objects are no std::string, they must
     implement "std::ostream operator<< (std::ostream&, const Ty&)".
  */
  template<class Ty>
  struct alphanum_less : public std::binary_function<Ty, Ty, bool>
  {
    bool operator()(const Ty& left, const Ty& right) const
    {
      return alphanum_comp(left, right) < 0;
    }
  };

}

#ifdef TESTMAIN
#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <vector>
int main()
{
  // testcases for the algorithm
  assert(doj::alphanum_comp("","") == 0);
  assert(doj::alphanum_comp("","a") < 0);
  assert(doj::alphanum_comp("a","") > 0);
  assert(doj::alphanum_comp("a","a") == 0);
  assert(doj::alphanum_comp("","9") < 0);
  assert(doj::alphanum_comp("9","") > 0);
  assert(doj::alphanum_comp("1","1") == 0);
  assert(doj::alphanum_comp("1","2") < 0);
  assert(doj::alphanum_comp("3","2") > 0);
  assert(doj::alphanum_comp("a1","a1") == 0);
  assert(doj::alphanum_comp("a1","a2") < 0);
  assert(doj::alphanum_comp("a2","a1") > 0);
  assert(doj::alphanum_comp("a1a2","a1a3") < 0);
  assert(doj::alphanum_comp("a1a2","a1a0") > 0);
  assert(doj::alphanum_comp("134","122") > 0);
  assert(doj::alphanum_comp("12a3","12a3") == 0);
  assert(doj::alphanum_comp("12a1","12a0") > 0);
  assert(doj::alphanum_comp("12a1","12a2") < 0);
  assert(doj::alphanum_comp("a","aa") < 0);
  assert(doj::alphanum_comp("aaa","aa") > 0);
  assert(doj::alphanum_comp("Alpha 2","Alpha 2") == 0);
  assert(doj::alphanum_comp("Alpha 2","Alpha 2A") < 0);
  assert(doj::alphanum_comp("Alpha 2 B","Alpha 2") > 0);

  assert(doj::alphanum_comp(1,1) == 0);
  assert(doj::alphanum_comp(1,2) < 0);
  assert(doj::alphanum_comp(2,1) > 0);
  assert(doj::alphanum_comp(1.2,3.14) < 0);
  assert(doj::alphanum_comp(3.14,2.71) > 0);
  assert(doj::alphanum_comp(true,true) == 0);
  assert(doj::alphanum_comp(true,false) > 0);
  assert(doj::alphanum_comp(false,true) < 0);

  std::string str("Alpha 2");
  assert(doj::alphanum_comp(str,"Alpha 2") == 0);
  assert(doj::alphanum_comp(str,"Alpha 2A") < 0);
  assert(doj::alphanum_comp("Alpha 2 B",str) > 0);

  assert(doj::alphanum_comp(str,strdup("Alpha 2")) == 0);
  assert(doj::alphanum_comp(str,strdup("Alpha 2A")) < 0);
  assert(doj::alphanum_comp(strdup("Alpha 2 B"),str) > 0);

#if 1
  // show usage of the comparison functor with a set
  std::set<std::string, doj::alphanum_less<std::string> > s;
  s.insert("Xiph Xlater 58");
  s.insert("Xiph Xlater 5000");
  s.insert("Xiph Xlater 500");
  s.insert("Xiph Xlater 50");
  s.insert("Xiph Xlater 5");
  s.insert("Xiph Xlater 40");
  s.insert("Xiph Xlater 300");
  s.insert("Xiph Xlater 2000");
  s.insert("Xiph Xlater 10000");
  s.insert("QRS-62F Intrinsia Machine");
  s.insert("QRS-62 Intrinsia Machine");
  s.insert("QRS-60F Intrinsia Machine");
  s.insert("QRS-60 Intrinsia Machine");
  s.insert("Callisto Morphamax 7000 SE2");
  s.insert("Callisto Morphamax 7000 SE");
  s.insert("Callisto Morphamax 7000");
  s.insert("Callisto Morphamax 700");
  s.insert("Callisto Morphamax 600");
  s.insert("Callisto Morphamax 5000");
  s.insert("Callisto Morphamax 500");
  s.insert("Callisto Morphamax");
  s.insert("Alpha 2A-900");
  s.insert("Alpha 2A-8000");
  s.insert("Alpha 2A");
  s.insert("Alpha 200");
  s.insert("Alpha 2");
  s.insert("Alpha 100");
  s.insert("Allegia 60 Clasteron");
  s.insert("Allegia 52 Clasteron");
  s.insert("Allegia 51B Clasteron");
  s.insert("Allegia 51 Clasteron");
  s.insert("Allegia 500 Clasteron");
  s.insert("Allegia 50 Clasteron");
  s.insert("40X Radonius");
  s.insert("30X Radonius");
  s.insert("20X Radonius Prime");
  s.insert("20X Radonius");
  s.insert("200X Radonius");
  s.insert("10X Radonius");
  s.insert("1000X Radonius Maximus");
  // print sorted set to cout
  std::copy(s.begin(), s.end(), std::ostream_iterator<std::string>(std::cout, "\n"));

  // show usage of comparision functor with a map
  typedef std::map<std::string, int, doj::alphanum_less<std::string> > m_t;
  m_t m;
  m["z1.doc"]=1;
  m["z10.doc"]=2;
  m["z100.doc"]=3;
  m["z101.doc"]=4;
  m["z102.doc"]=5;
  m["z11.doc"]=6;
  m["z12.doc"]=7;
  m["z13.doc"]=8;
  m["z14.doc"]=9;
  m["z15.doc"]=10;
  m["z16.doc"]=11;
  m["z17.doc"]=12;
  m["z18.doc"]=13;
  m["z19.doc"]=14;
  m["z2.doc"]=15;
  m["z20.doc"]=16;
  m["z3.doc"]=17;
  m["z4.doc"]=18;
  m["z5.doc"]=19;
  m["z6.doc"]=20;
  m["z7.doc"]=21;
  m["z8.doc"]=22;
  m["z9.doc"]=23;
  // print sorted map to cout
  for(m_t::iterator i=m.begin(); i!=m.end(); ++i)
    std::cout << i->first << '\t' << i->second << std::endl;

  // show usage of comparison functor with an STL algorithm on a vector
  std::vector<std::string> v;
  // vector contents are reversed sorted contents of the old set
  std::copy(s.rbegin(), s.rend(), std::back_inserter(v));
  // now sort the vector with the algorithm
  std::sort(v.begin(), v.end(), doj::alphanum_less<std::string>());
  // and print the vector to cout
  std::copy(v.begin(), v.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
#endif

  return 0;
}
#endif

#endif
