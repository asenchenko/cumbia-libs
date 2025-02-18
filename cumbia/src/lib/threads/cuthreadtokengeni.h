#ifndef CUTHREADTOKENGENI_H
#define CUTHREADTOKENGENI_H

#include <string>

/*!
 * \brief The CuThreadTokenGenI is an interface for custom thread token generators
 *
 * Subclasses must implement the *generate* method that returns a *token* in the form
 * of CuData using the information available in the input parameter of type CuData.
 *
 * @see Cumbia::setThreadTokenGenerator
 * @see Cumbia::threadToken
 */
class CuThreadTokenGenI
{
public:
    virtual ~CuThreadTokenGenI() {}

    virtual std::string generate(const std::string& in) = 0;
};

#endif // CUTHREADTOKENGENI_H
