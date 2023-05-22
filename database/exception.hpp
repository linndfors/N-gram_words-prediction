#ifndef NGRAM_EXCEPTION_HPP
#define NGRAM_EXCEPTION_HPP

#include <stdexcept>
#include <utility>

class database_error : public std::runtime_error
{
public:
    explicit database_error(std::string message) : m_msg(std::move(message)), std::runtime_error(message) {}

    [[nodiscard]] const char * what() const noexcept override
    {
        return m_msg.c_str();
    }

    void exit() const noexcept
    {
        ::exit(EXIT_CODE);
    }

private:
    std::string m_msg;
    int EXIT_CODE{6};
};


#endif //NGRAM_EXCEPTION_HPP
