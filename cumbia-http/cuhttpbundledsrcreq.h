#ifndef CUHTTPBUNDLEDSRCREQ_H
#define CUHTTPBUNDLEDSRCREQ_H

#include <QObject>
#include <QSslError>
#include <QNetworkReply>
#include "cuhttpsrcman.h"

class CuHttpBundledSrcReqPrivate;

class CuHttpBundledSrcReqListener {
public:
    virtual void onSrcBundleReplyReady(const QByteArray& json) = 0;
    virtual void onSrcBundleReplyError(const CuData& errd) = 0;
};

class CuHttpBundledSrcReq : public QObject
{
    Q_OBJECT
public:
    explicit CuHttpBundledSrcReq(const QList<SrcItem>& srcs,
                                 CuHttpBundledSrcReqListener *l,
                                 unsigned long long client_id = 0,
                                 QObject *parent = nullptr);
    explicit CuHttpBundledSrcReq(const QMap<QString, SrcData>& targetmap,
                                 CuHttpBundledSrcReqListener *l,
                                 const QByteArray& cookie = QByteArray(),
                                 QObject *parent = nullptr);
    virtual ~CuHttpBundledSrcReq();
    void start(const QUrl &url, QNetworkAccessManager *nam);
    void setBlocking(bool b);

signals:

protected slots:
    virtual void onNewData();
    virtual void onReplyFinished();
    virtual void onReplyDestroyed(QObject *);
    virtual void onSslErrors(const QList<QSslError> &errors);
    virtual void onError(QNetworkReply::NetworkError code);

    void m_test_check_reply();

private:
    CuHttpBundledSrcReqPrivate* d;

    void m_on_buf_complete();
    bool m_likely_valid(const QByteArray &ba) const;
};

#endif // CUHTTPBUNDLEDSRCREQ_H
