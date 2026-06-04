#include <string>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include <cctype>
#include <format>

static bool isIntegerSuffixChar(char c)
{
    switch (c)
    {
        case 'u':
        case 'U':
        case 'l':
        case 'L':
        case 'w':
        case 'W':
            return true;
        default:
            return false;
    }
}

static int digitValueForBase(char c, int base)
{
    if (c >= '0' && c <= '9')
    {
        int val = c - '0';
        if (val < base) return val;
    }
    if (base > 10)
    {
        char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        int span = base - 10;
        if (lower >= 'a' && lower < 'a' + span)
        {
            return 10 + (lower - 'a');
        }
    }
    return -1;
}

std::string parseIntegerLiteralToDecimal(std::string_view literal)
{
    if (literal.empty())
    {
        throw std::runtime_error("Empty number literal");
    }

    std::size_t pos = 0;
    bool negative = false;
    if (literal[pos] == '+' || literal[pos] == '-')
    {
        negative = literal[pos] == '-';
        pos++;
        if (pos >= literal.size())
        {
            throw std::runtime_error("Sign without digits in number literal");
        }
    }

    int base = 10;
    if (pos + 1 < literal.size() && literal[pos] == '0')
    {
        char prefix = literal[pos + 1];
        if (prefix == 'x' || prefix == 'X')
        {
            base = 16;
            pos += 2;
        }
        else if (prefix == 'b' || prefix == 'B')
        {
            base = 2;
            pos += 2;
        }
    }

    std::string cleanedDigits;
    cleanedDigits.reserve(literal.size() - pos);
    for (; pos < literal.size(); ++pos)
    {
        char c = literal[pos];
        if (c == '\'')
        {
            continue;
        }
        
        int digit = digitValueForBase(c, base);
        if (digit >= 0)
        {
            cleanedDigits.push_back(c);
        }
        else
        {
            // Check if this is a floating-point indicator
            if (c == '.' || c == 'e' || c == 'E' || c == 'p' || c == 'P')
            {
                throw std::runtime_error(std::string("Floating-point literal not supported: ") + std::string(literal));
            }
            // If it's not a valid digit and not a floating-point indicator, 
            // it might be an integer suffix, so we break here
            break;
        }
    }

    if (cleanedDigits.empty())
    {
        throw std::runtime_error(std::string("Failed to parse number literal: ") + std::string(literal));
    }

    std::string_view suffix = literal.substr(pos);
    for (char c : suffix)
    {
        if (!isIntegerSuffixChar(c))
        {
            throw std::runtime_error(std::string("Unexpected suffix in number literal: ") + std::string(literal));
        }
    }

    const std::uint64_t positiveLimit = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    const std::uint64_t negativeLimit = std::uint64_t{1} << 63; // abs(INT64_MIN)
    const std::uint64_t limit = negative ? negativeLimit : positiveLimit;

    std::uint64_t value = 0;
    for (char c : cleanedDigits)
    {
        int digit = digitValueForBase(c, base);
        // const std::uint64_t maxBeforeMul = (limit - static_cast<std::uint64_t>(digit)) / static_cast<std::uint64_t>(base);
        // if (value > maxBeforeMul)
        // {
        //     throw std::runtime_error(std::string("Integer literal exceeds 64-bit range: ") + std::string(literal));
        // }
        value = value * static_cast<std::uint64_t>(base) + static_cast<std::uint64_t>(digit);
    }

    if (value == 0)
    {
        return "0";
    }

    if (negative)
    {
        if (value == negativeLimit)
        {
            return std::format("{}", std::numeric_limits<std::int64_t>::min());
        }
        return std::format("-{}", value);
    }

    return std::format("{}", value);
}
