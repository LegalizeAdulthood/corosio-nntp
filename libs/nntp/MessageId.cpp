#include <nntp/MessageId.h>

#include <stdexcept>

namespace nntp
{

MessageId::MessageId(const std::string_view &value) :
    m_value(value)
{
    if (m_value.empty())
    {
        throw std::invalid_argument("MessageId cannot be empty");
    }

    // Check minimum length: at least "<x>" = 3 characters
    if (m_value.size() < 3)
    {
        throw std::invalid_argument("MessageId must have at least 3 characters");
    }

    // Check for opening and closing angle brackets
    if (m_value.front() != '<' || m_value.back() != '>')
    {
        throw std::invalid_argument("MessageId must be enclosed in angle brackets");
    }

    // Extract content between brackets
    std::string_view content = std::string_view{m_value}.substr(1, m_value.size() - 2);

    // Check content length: 1 to 248 characters
    if (content.size() > 248)
    {
        throw std::invalid_argument("MessageId content exceeds maximum length of 248 characters");
    }

    // Validate each character in content
    // A-NOTGT: %x21-3D / %x3F-7E (A-CHAR excluding '>' which is %x3E)
    // A-CHAR is %x21-7E (printable ASCII excluding space and controls)
    for (char c : content)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        
        // Must be in range 0x21-0x7E (printable ASCII excluding space)
        // This excludes: controls (0x00-0x1F), space (0x20), DEL (0x7F), and high bytes (0x80-0xFF)
        if (uc < 0x21 || uc > 0x7E)
        {
            throw std::invalid_argument("MessageId contains invalid character");
        }
        
        // Cannot be '>' (0x3E)
        if (uc == 0x3E)
        {
            throw std::invalid_argument("MessageId content cannot contain '>'");
        }
    }
}

} // namespace nntp
