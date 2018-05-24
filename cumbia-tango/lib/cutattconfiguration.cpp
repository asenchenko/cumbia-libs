#include "cutattconfiguration.h"
#include <culog.h>
#include <cumacros.h>
#include <cuserviceprovider.h>
#include <cuactivity.h>
#include <cudatalistener.h>
#include "cumbiatango.h"
#include "tsource.h"
#include "cudevicefactoryservice.h"
#include "cutattconfigactivity.h"
#include "cuactionfactoryservice.h"
#include "cudatalistener.h"

class CuTAttConfigurationPrivate
{
public:
    std::list<CuDataListener *> listeners;
    TSource tsrc;
    CumbiaTango *cumbia_t;
    CuActivity *activity;
    bool exiting; // set to true by stop()
    CuConLogImpl li;
    CuLog log;
    CuVariant write_val;
    std::vector<std::string> desired_props;
    CuData conf_data; // save locally
};

CuTAttConfiguration::CuTAttConfiguration(const TSource& src,
                                         CumbiaTango *ct)
{
    d = new CuTAttConfigurationPrivate;
    d->cumbia_t = ct;
    d->tsrc = src;
    d->exiting = false;
}

CuTAttConfiguration::~CuTAttConfiguration()
{
    pdelete("~CuTAttConfiguration: %p", this);
    delete d;
}

void CuTAttConfiguration::setDesiredAttributeProperties(const std::vector<string> props)
{
    d->desired_props = props;
}

void CuTAttConfiguration::onProgress(int step, int total, const CuData &data)
{
    (void) step; // unused
    (void) total;
    (void) data;
}

void CuTAttConfiguration::onResult(const CuData &data)
{
    d->conf_data = data;
    if(data["exit"].toBool()) // ! important: evaluate data["exit"] before deleting this
    {
        d->exiting = true; // for action factory to unregisterAction, exiting must return true
        CuActionFactoryService * af = static_cast<CuActionFactoryService *>(d->cumbia_t->getServiceProvider()
                                                                            ->get(static_cast<CuServices::Type>(CuActionFactoryService::CuActionFactoryServiceType)));
        af->unregisterAction(d->tsrc.getName(), getType());
        d->listeners.clear();
        delete this;
    }
    else { // do not update configuration data if exit
        std::list <CuDataListener *> listeners = d->listeners;
        std::list<CuDataListener *>::iterator it;
        for(it = listeners.begin(); it != listeners.end(); ++it) {
            (*it)->onUpdate(data);
        }
    }
}

CuData CuTAttConfiguration::getToken() const
{
    CuData da("source", d->tsrc.getName());
    da["type"] = std::string("attconfig");
    return da;
}

TSource CuTAttConfiguration::getSource() const
{
    return d->tsrc;
}

CuTangoActionI::Type CuTAttConfiguration::getType() const
{
    return CuTangoActionI::AttConfig;
}

void CuTAttConfiguration::addDataListener(CuDataListener *l)
{
    std::list<CuDataListener *>::iterator it = d->listeners.begin();
    d->listeners.insert(it, l);
    l->setValid();
    /* if a new listener is added after onResult, call onUpdate.
     * This happens when multiple items connect to the same source
     */
    if(!d->conf_data.isEmpty()) {
        l->onUpdate(d->conf_data);
    }
}

void CuTAttConfiguration::removeDataListener(CuDataListener *l)
{
    if(l->invalid()) {
        d->listeners.remove(l);
        if(!d->listeners.size())
            stop();
    }
    else if(d->listeners.size() == 1)
        stop();
    else
        d->listeners.remove(l);
}

void CuTAttConfiguration::sendData(const CuData &) {
}

void CuTAttConfiguration::getData(CuData &d_inout) const {

}

size_t CuTAttConfiguration::dataListenersCount() {
    return d->listeners.size();
}

void CuTAttConfiguration::start()
{
    CuDeviceFactoryService *df =
            static_cast<CuDeviceFactoryService *>(d->cumbia_t->getServiceProvider()->
                                                  get(static_cast<CuServices::Type> (CuDeviceFactoryService::CuDeviceFactoryServiceType)));
    CuData at("src", d->tsrc.getName()); /* activity token */
    at["device"] = d->tsrc.getDeviceName();
    at["point"] = d->tsrc.getPoint();
    at["activity"] = "attconfig";
    at["is_command"] = d->tsrc.getType() == TSource::Cmd;

    CuData tt("device", d->tsrc.getDeviceName()); /* thread token */
    d->activity = new CuTAttConfigActivity(at, df);
    static_cast<CuTAttConfigActivity *>(d->activity)->setDesiredAttributeProperties(d->desired_props);
    const CuThreadsEventBridgeFactory_I &bf = *(d->cumbia_t->getThreadEventsBridgeFactory());
    const CuThreadFactoryImplI &fi = *(d->cumbia_t->getThreadFactoryImpl());
    d->cumbia_t->registerActivity(d->activity, this, tt, fi, bf);
    cuprintf("> CuTAttConfiguration.start attconfig %p thread 0x%lx ACTIVITY %p\n", this, pthread_self(), d->activity);
}

/*! \brief calls unregisterActivity on Cumbia in order to enter the exit state
 *
 * - sets the exiting flag to true
 * - calls Cumbia::unregisterActivity
 */
void CuTAttConfiguration::stop()
{
    cuprintf("\e[1;35mCuTattConfigureation.stop called this %p activity %p d->exiting %d\e[0m\n", this, d->activity, d->exiting);
    if(!d->exiting) {
        d->exiting = true;
        d->cumbia_t->unregisterActivity(d->activity);
    }
}

/*! \brief CuActionFactory relies on this returning true to unregister the action
 */
bool CuTAttConfiguration::exiting() const
{
    return d->exiting;
}
