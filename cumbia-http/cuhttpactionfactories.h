#ifndef CUHTTPACTIONFACTORIES_H
#define CUHTTPACTIONFACTORIES_H

#include <cuhttpactiona.h>
#include <cuhttpactionfactoryi.h>

class QNetworkAccessManager;

class CuHTTPActionReaderFactory : public CuHTTPActionFactoryI {
public:
    virtual ~CuHTTPActionReaderFactory();

    void setOptions(const CuData &o);
    // CuTangoActionFactoryI interface
    CuHTTPActionA *create(const std::string &s, QNetworkAccessManager *nam, const QString& http_addr) const;
    CuHTTPActionA::Type getType() const;
private:
    CuData options;
};

class CuHTTPActionWriterFactory : public CuHTTPActionFactoryI
{
public:
    virtual ~CuHTTPActionWriterFactory();

    void setConfiguration(const CuData &conf);
    void setWriteValue(const CuVariant &write_val);

    // CuTangoActionFactoryI interface
    CuHTTPActionA *create(const std::string &s, QNetworkAccessManager *qnam, const QString& http_addr) const;
    CuHTTPActionA::Type getType() const;

private:
    CuData configuration;
    CuVariant m_write_val;
};

class CuHTTPActionConfFactory : public CuHTTPActionFactoryI
{
public:
    void setOptions(const CuData &o);
    virtual ~CuHTTPActionConfFactory();

    // CuTangoActionFactoryI interface
    CuHTTPActionA *create(const std::string &s, QNetworkAccessManager *qnam, const QString& http_addr) const;
    CuHTTPActionA::Type getType() const;

private:
    CuData options;
};

#endif // CUHTTPACTIONFACTORIES_H
