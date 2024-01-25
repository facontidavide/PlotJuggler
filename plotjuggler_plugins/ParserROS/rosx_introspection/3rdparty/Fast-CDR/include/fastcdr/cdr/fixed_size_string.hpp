// Copyright 2023 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*!
 * @file fixed_size_string.hpp
 *
 */

#ifndef FASTCDR_UTILS_FIXED_SIZE_STRING_HPP_
#define FASTCDR_UTILS_FIXED_SIZE_STRING_HPP_

#include <string>
#include <cstring>

#ifdef _WIN32
#define MEMCCPY _memccpy
#else
#define MEMCCPY memccpy
#endif // ifdef _WIN32

namespace eprosima {
namespace fastcdr {

/**
 * @brief Template class for non-alloc strings.
 *
 * Will be truncated when assigned from a longer string.
 *
 * @tparam MAX_CHARS Maximum number of characters is specified as the template parameter.
 *                   Space for an additional null terminator will be reserved.
 */
template <size_t MAX_CHARS>
struct fixed_string
{
public:

    //! @brief Maximum number of characters.
    static constexpr size_t max_size = MAX_CHARS;

    //! @brief Default constructor.
    fixed_string() noexcept
    {
        memset(string_data, 0, sizeof(string_data));
        string_len = 0;
    }

    // We don't need to define copy/move constructors/assignment operators as the default ones would be enough

    /*!
     * @brief Constructs from a char array
     * @param[in] c_array Char array to be constructed from.
     * @param[in] n_chars Number of characters of the Char array
     */
    fixed_string(
            const char* c_array,
            size_t n_chars) noexcept
    {
        assign(c_array, n_chars);
    }

    /*!
     * @brief Assigns from a char array
     * @param[in] c_array Char array to be assigned from.
     * @param[in] n_chars Number of characters of the Char array.
     * @return Reference of this instance.
     */
    fixed_string& assign(
            const char* c_array,
            size_t n_chars) noexcept
    {
        string_len = (nullptr == c_array) ? 0 :
                (MAX_CHARS < n_chars) ? MAX_CHARS : n_chars;
        if (0 < string_len)
        {
            memcpy(string_data, c_array, string_len);
        }
        return *this;
    }

    /*!
     * @brief Constructs from a C string.
     * @param[in] c_string Pointer to the C string.
     */
    fixed_string (
            const char* c_string) noexcept
        : fixed_string()
    {
        set(c_string != nullptr ? c_string : "");
    }

    /*!
     * @brief Assigns from a C string.
     * @param[in] c_string Pointer to the C string.
     * @return Reference of this instance.
     */
    fixed_string& operator = (
            const char* c_string) noexcept
    {
        set(c_string != nullptr ? c_string : "");
        return *this;
    }

    /*!
     * @brief Constructs from a std::string.
     * @param[in] str Reference to the std::string.
     */
    fixed_string (
            const std::string& str) noexcept
        : fixed_string()
    {
        set(str.c_str());
    }

    /*!
     * @brief Assigns from a std::string.
     * @param[in] str Reference to the std::string.
     * return Reference of this instance.
     */
    fixed_string& operator = (
            const std::string& str) noexcept
    {
        set(str.c_str());
        return *this;
    }

    /*!
     * @brief Assigns from a fixed_string of any size.
     * @param[in] rhs Reference to the fixed_string.
     * return Reference of this instance.
     */
    template<size_t N> fixed_string& operator = (
            const fixed_string<N>& rhs) noexcept
    {
        set(rhs.c_str());
        return *this;
    }

    /*!
     * @brief Converts to C string.
     * @return Pointer to the C string.
     */
    const char* c_str() const noexcept
    {
        return string_data;
    }

    /*!
     * @brief Converts to std::string.
     * @return Reference to the std::string.
     */
    std::string to_string() const
    {
        return std::string(string_data);
    }

    /*!
     * @brief Compares equality with a C string.
     * @param[in] rhs C string to be compared with.
     * @return `true` if strings are equal. `false` otherwise.
     */
    bool operator == (
            const char* rhs) const noexcept
    {
        return strncmp(string_data, rhs, MAX_CHARS) == 0;
    }

    /*!
     * @brief Compares equality with a std::string.
     * @param[in] rhs std::string to be compared with.
     * @return `true` if strings are equal. `false` otherwise.
     */
    bool operator == (
            const std::string& rhs) const noexcept
    {
        return strncmp(string_data, rhs.c_str(), MAX_CHARS) == 0;
    }

    /*!
     * @brief Compares equality with a fixed_string of any size.
     * @param[in] rhs fixed_string to be compared with.
     * @return `true` if strings are equal. `false` otherwise.
     */
    template<size_t N>  bool operator == (
            const fixed_string<N>& rhs) const noexcept
    {
        return strncmp(string_data, rhs.c_str(), MAX_CHARS) == 0;
    }

    /*!
     * @brief Compares inequality with a C string.
     * @param[in] rhs C string to be compared with.
     * @return `true` if strings are not equal. `false` otherwise.
     */
    bool operator != (
            const char* rhs) const noexcept
    {
        return !(*this == rhs);
    }

    /*!
     * @brief Compares inequality with a std::string.
     * @param[in] rhs std::string to be compared with.
     * @return `true` if strings are not equal. `false` otherwise.
     */
    bool operator != (
            const std::string& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    /*!
     * @brief Compares inequality with a fixed_string of any size.
     * @param[in] rhs fixed_string to be compared with.
     * @return `true` if strings are not equal. `false` otherwise.
     */
    template<size_t N>  bool operator != (
            const fixed_string<N>& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    /*!
     * @brief Compares relational less than with a fixed_string of any size.
     * @param[in] rhs fixed_string to be compared with.
     * @return `true` if this string is less than the provided one. `false` otherwise.
     */
    template<size_t N>  bool operator < (
            const fixed_string<N>& rhs) const noexcept
    {
        return 0 > compare(rhs);
    }

    /*!
     * @brief Compares relational greater than with a fixed_string of any size.
     * @param[in] rhs fixed_string to be compared with.
     * @return `true` if this string is greater than the provided one. `false` otherwise.
     */
    template<size_t N>  bool operator > (
            const fixed_string<N>& rhs) const noexcept
    {
        return 0 < compare(rhs);
    }

    /*!
     * @brief Compares relational less than with a std::string of any size.
     * @param[in] rhs std::string to be compared with.
     * @return `true` if this string is less than the provided one. `false` otherwise.
     */
    bool operator < (
            const std::string& rhs) const noexcept
    {
        return 0 > compare(rhs);
    }

    /*!
     * @brief Compares relational greater than with a std::string of any size.
     * @param[in] rhs std::string to be compared with.
     * @return `true` if this string is greater than the provided one. `false` otherwise.
     */
    bool operator > (
            const std::string& rhs) const noexcept
    {
        return 0 < compare(rhs);
    }

    /*!
     * @brief Casts to a C string.
     */
    operator const char* () const noexcept {
        return c_str();
    }

    /*!
     * @brief Returns the size of the string.
     * @return Length of the string.
     */
    size_t size() const noexcept
    {
        return string_len;
    }

    /*!
     * @brief Compares with a C string.
     * @param[in] str C string to be compared with.
     * @return Integer value with the result of the comparison as described in `std::string::compare()`.
     */
    int compare(
            const char* str) const noexcept
    {
        return strncmp(string_data, str, MAX_CHARS);
    }

    /*!
     * @brief Compares with a std::string.
     * @param[in] str std::string to be compared with.
     * @return Integer value with the result of the comparison as described in `std::string::compare()`.
     */
    int compare(
            const std::string& str) const noexcept
    {
        return strncmp(string_data, str.c_str(), MAX_CHARS);
    }

    /*!
     * @brief Compares with a fixed_string
     * @param[in] str fixed_string to be compared with.
     * @return Integer value with the result of the comparison as described in `std::string::compare()`.
     */
    template<size_t N>  int compare(
            const fixed_string<N>& str) const noexcept
    {
        return strncmp(string_data, str.c_str(), MAX_CHARS);
    }

private:

    void set(
            const char* c_string) noexcept
    {
        char* result = static_cast<char*>(MEMCCPY(string_data, c_string, '\0', MAX_CHARS));
        string_len = (result == nullptr) ? MAX_CHARS : static_cast<size_t>(result - string_data) - 1u;
    }

    char string_data[MAX_CHARS + 1];      ///< Holds string data, including ending null character.
    size_t string_len;                    ///< Holds current string length.
};

using string_255 = fixed_string<255>;

} /* namespace fastcdr */
} /* namespace eprosima */

#endif /* FASTCDR_UTILS_FIXED_SIZE_STRING_HPP_ */
